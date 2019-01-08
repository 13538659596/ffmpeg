#include "syn_audio_video.h"
#include <stdio.h>
#include "Queue.h"
#include "Player.h"
#include "log.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>


#define MAX_PACKAT_SIZE 500
/**
 * 初始化ffmpeg组件,打开文件获取音视频索引位置
 */
void init_input_format_ctx(MyPlayer *player, const char *input_file) {

	av_register_all();

	int ret = -1;
	AVFormatContext *pFormatContext = avformat_alloc_context();

	if(avformat_open_input(&pFormatContext, input_file, NULL, NULL) != 0) {
		LOGE("打开文件失败: %s\n", strerror(errno));
		if(pFormatContext != NULL) {
			LOGE("释放资源");
			avformat_close_input(&pFormatContext);
		}
		return;
	}
	if(avformat_find_stream_info(pFormatContext, NULL) < 0) {
		LOGE("未找到视频流信息: %s\n", strerror(errno));
		if(pFormatContext != NULL) {
			LOGE("释放资源");
			avformat_close_input(&pFormatContext);
		}
		return;
	}

	player->captrue_streams_no = pFormatContext->nb_streams;

	LOGI("captrue_streams_no:%d",player->captrue_streams_no);
	//视频解码，需要找到视频对应的AVStream所在format_ctx->streams的索引位置
	//获取音频和视频流的索引位置
	for(int i = 0; i < player->captrue_streams_no;i++){
		if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
			player->video_stream_index = i;
		}
		else if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
			player->audio_stream_index = i;
		}
	}
	player->pFormatContext = pFormatContext;
}

/**
 * 初始化解码器上下文
 */
void init_codec_context(MyPlayer *player,int stream_idx){
	AVFormatContext *format_ctx = player->pFormatContext;
	//获取解码器
	LOGI("init_codec_context begin");
	AVCodecContext *codec_ctx = format_ctx->streams[stream_idx]->codec;
	LOGI("init_codec_context end");
	AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
	if(codec == NULL){
		LOGE("%s","无法解码");
		return;
	}
	//打开解码器
	if(avcodec_open2(codec_ctx,codec,NULL) < 0){
		LOGE("%s","解码器无法打开");
		return;
	}
	player->input_codec_ctx[stream_idx] = codec_ctx;
}


void decode_video_prepare(JNIEnv *env,Player *player,jobject surface){
	player->nativeWindow = ANativeWindow_fromSurface(env,surface);
}


/**
 * 音频解码准备
 */
void decode_audio_prepare(Player *player){
	AVCodecContext *codec_ctx = player->input_codec_ctx[player->audio_stream_index];

	//重采样设置参数-------------start
	//输入的采样格式
	enum AVSampleFormat in_sample_fmt = codec_ctx->sample_fmt;
	//输出采样格式16bit PCM
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	//输入采样率
	int in_sample_rate = codec_ctx->sample_rate;
	//输出采样率
	int out_sample_rate = in_sample_rate;
	//获取输入的声道布局
	//根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
	//av_get_default_channel_layout(codecCtx->channels);
	uint64_t in_ch_layout = codec_ctx->channel_layout;
	//输出的声道布局（立体声）
	uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

	//frame->16bit 44100 PCM 统一音频采样格式与采样率
	SwrContext *swr_ctx = swr_alloc();
	swr_alloc_set_opts(swr_ctx,
		  out_ch_layout,out_sample_fmt,out_sample_rate,
		  in_ch_layout,in_sample_fmt,in_sample_rate,
		  0, NULL);
	swr_init(swr_ctx);

	//输出的声道个数
	int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

	//重采样设置参数-------------end

	player->in_sample_fmt = in_sample_fmt;
	player->out_sample_fmt = out_sample_fmt;
	player->in_sample_rate = in_sample_rate;
	player->out_sample_rate = out_sample_rate;
	player->out_channel_nb = out_channel_nb;
	player->swr_ctx = swr_ctx;

}


/**
 * 生产者线程：负责不断的读取视频文件中AVPacket，分别放入两个队列中
 */
void* player_read_from_stream(void* param){

	MyPlayer *player = (MyPlayer *)param;
	int ret;
	//栈内存上保存一个AVPacket
	AVPacket packet, *pkt = &packet;
	while(1) {
		ret = av_read_frame(player->pFormatContext ,pkt);
		//到文件结尾了
		if(ret < 0){
			break;
		}
		//根据AVpacket->stream_index获取对应的队列
		Queue *queue = player->packets[pkt->stream_index];

		//示范队列内存释放
		//queue_free(queue,packet_free_func);

		//将AVPacket压入队列

		AVPacket *packet_data = (AVPacket *)queue_push(queue);
		*packet_data = packet;

	}
}


void prepare_audioTrack(JNIEnv *env,jclass jcls,MyPlayer *player) {
	//------------------------------jni获取AudioTrack对象
		jmethodID method_creat_audiotrack = env->GetStaticMethodID(jcls, "getAudioTrack", "()Landroid/media/AudioTrack;");
		jobject audiotrack =   env->CallStaticObjectMethod( jcls, method_creat_audiotrack);
		jclass audiotrack_cls = env->GetObjectClass(audiotrack);
		jmethodID play = env->GetMethodID(audiotrack_cls, "play", "()V");
		env->CallVoidMethod(audiotrack, play);
		jmethodID write = env->GetMethodID(audiotrack_cls, "write", "([BII)I");


		player->audio_track = env->NewGlobalRef(audiotrack);

		player->audio_track_write_mid = write;
}


/**
 * 解码视频
 */
void decode_video(MyPlayer *player,AVPacket *packet){
	//像素数据（解码数据）
	AVFrame *yuv_frame = av_frame_alloc();
	AVFrame *rgb_frame = av_frame_alloc();
	//绘制时的缓冲区
	ANativeWindow_Buffer outBuffer;
	AVCodecContext *codec_ctx = player->input_codec_ctx[player->video_stream_index];
	int got_frame;
	//解码AVPacket->AVFrame
	avcodec_decode_video2(codec_ctx, yuv_frame, &got_frame, packet);
	//Zero if no frame could be decompressed
	//非零，正在解码
	if(got_frame){
		//lock
		//设置缓冲区的属性（宽、高、像素格式）
		ANativeWindow_setBuffersGeometry(player->nativeWindow, codec_ctx->width, codec_ctx->height,WINDOW_FORMAT_RGBA_8888);
		ANativeWindow_lock(player->nativeWindow,&outBuffer,NULL);

		//设置rgb_frame的属性（像素格式、宽高）和缓冲区
		//rgb_frame缓冲区与outBuffer.bits是同一块内存
		avpicture_fill((AVPicture *)rgb_frame, (uint8_t *)outBuffer.bits, AV_PIX_FMT_RGBA, codec_ctx->width, codec_ctx->height);

		//YUV->RGBA_8888
		libyuv::I420ToARGB(yuv_frame->data[0],yuv_frame->linesize[0],
				yuv_frame->data[2],yuv_frame->linesize[2],
				yuv_frame->data[1],yuv_frame->linesize[1],
				rgb_frame->data[0], rgb_frame->linesize[0],
				codec_ctx->width,codec_ctx->height);

		//unlock
		ANativeWindow_unlockAndPost(player->nativeWindow);

		usleep(1000 * 16);
	}

	av_frame_free(&yuv_frame);
	av_frame_free(&rgb_frame);
}


/**
 * 音频解码
 */
void decode_audio(MyPlayer *player,AVPacket *packet){
	AVCodecContext *codec_ctx = player->input_codec_ctx[player->audio_stream_index];
	LOGI("%s","decode_audio");
	//解压缩数据
	AVFrame *frame = av_frame_alloc();
	int got_frame;
	avcodec_decode_audio4(codec_ctx,frame,&got_frame,packet);

	//16bit 44100 PCM 数据（重采样缓冲区）
	uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);
	//解码一帧成功
	if(got_frame > 0){
		swr_convert(player->swr_ctx, &out_buffer, MAX_AUDIO_FRME_SIZE,(const uint8_t **)frame->data,frame->nb_samples);
		//获取sample的size
		int out_buffer_size = av_samples_get_buffer_size(NULL, player->out_channel_nb,
				frame->nb_samples, player->out_sample_fmt, 1);

		//关联当前线程的JNIEnv
		JavaVM *javaVM = player->javaVm;
		JNIEnv *env;
		javaVM->AttachCurrentThread(&env,NULL);

		//out_buffer缓冲区数据，转成byte数组
		jbyteArray audio_sample_array = env->NewByteArray(out_buffer_size);
		jbyte* sample_bytep = env->GetByteArrayElements(audio_sample_array,NULL);
		//out_buffer的数据复制到sampe_bytep
		memcpy(sample_bytep,out_buffer,out_buffer_size);
		//同步
		env->ReleaseByteArrayElements(audio_sample_array,sample_bytep,0);

		//AudioTrack.write PCM数据
		env->CallIntMethod(player->audio_track,player->audio_track_write_mid,
				audio_sample_array,0,out_buffer_size);
		//释放局部引用
		env->DeleteLocalRef(player->audio_track);

		javaVM->DetachCurrentThread();

		usleep(1000 * 16);
	}

	av_frame_free(&frame);
}


/**
 * 解码子线程函数
 */
void* decode_data(void* arg){
	MyDecoder *decoder = (MyDecoder*)arg;
	MyPlayer *player = decoder->player;
	int stream_index = decoder->stream_index;
	LOGE("============================================== %d", stream_index);
	//根据stream_index获取对应的AVPacket队列
	Queue *queue = player->packets[stream_index];

	AVFormatContext *format_ctx = player->pFormatContext;
	//编码数据

	//6.一帧一帧读取压缩的视频数据AVPacket
	int video_frame_count = 0, audio_frame_count = 0;
	while(1) {
		//消费AVPacket
		AVPacket *packet = (AVPacket*)queue_pop(queue);
		if(stream_index == player->video_stream_index){
			decode_video(player,packet);
			LOGI("video_frame_count:%d",video_frame_count++);
		}else if(stream_index == player->audio_stream_index){
			decode_audio(player,packet);
			LOGI("audio_frame_count:%d",audio_frame_count++);
		}

	}
}


void *queue_free_func() {
	AVPacket *pack = (AVPacket *)malloc(sizeof(AVPacket));
	return (void *)pack;
}
/**
 * 初始化音频，视频AVPacket队列，长度50
 */
void player_alloc_queues(MyPlayer *player){
	int i;
	//这里，正常是初始化两个队列
	for (i = 0; i < player->captrue_streams_no; ++i) {
		Queue *queue = queue_init(MAX_PACKAT_SIZE, queue_free_func);
		player->packets[i] = queue;
		//打印视频音频队列地址
		LOGI("stream index:%d,queue:%#x",i,queue);
	}
}



void syn_audeo_vedio(JNIEnv *env, jclass jcls, jstring srcFile, jobject surface) {

	const char *input_cstr = env->GetStringUTFChars(srcFile, NULL);

	MyPlayer *player = (MyPlayer *)malloc(sizeof(MyPlayer));

	player->audio_stream_index = AVMEDIA_TYPE_UNKNOWN;
	player->video_stream_index = AVMEDIA_TYPE_UNKNOWN;

	env->GetJavaVM(&player->javaVm);

	//初始化封装格式上下文
	init_input_format_ctx(player,input_cstr);

	if(player->audio_stream_index == AVMEDIA_TYPE_UNKNOWN || player->video_stream_index == AVMEDIA_TYPE_UNKNOWN) {
		LOGE("找不到音视频流信息");
		return;
	}

	//获取音视频解码器，并打开
	init_codec_context(player,player->video_stream_index);
	init_codec_context(player,player->audio_stream_index);


	decode_video_prepare(env,player,surface);
	decode_audio_prepare(player);
	prepare_audioTrack(env, jcls, player);

	//初始化队列
	player_alloc_queues(player);

	//生产者线程
	pthread_create(&player->thread_read_from_stream,NULL,player_read_from_stream,(void *)player);

	usleep(20);

	//音频解码
	MyDecoder decoder_audeo = {player, player->audio_stream_index};
	//消费者播放音频
	pthread_create(&player->decode_threads[player->audio_stream_index], NULL, decode_data, (void *)&decoder_audeo);

	//视频解码
	/*MyDecoder decoder_video = {player, player->video_stream_index};
	pthread_create(&player->decode_threads[player->video_stream_index], NULL, decode_data, (void *)&decoder_video);*/

	pthread_join(player->thread_read_from_stream, NULL);
	pthread_join(player->decode_threads[player->video_stream_index], NULL);
	pthread_join(player->decode_threads[player->audio_stream_index], NULL);

	env->ReleaseStringUTFChars(srcFile, input_cstr);

}



