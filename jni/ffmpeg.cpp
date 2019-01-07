#include "com_example_ffmpeg_FFmpegUtils.h"

#define LOG_TAG "Test_Jni"
#include <android/log.h>
// for native window JNI
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <unistd.h>
#include <string.h>

#define MAX_AUDIO_FRME_SIZE (48000 * 4)



//用于打印debug级别的log信息
//__VA_ARGS__ 可变参数
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

//用于打印info级别的log信息
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

//用于打印error级别的log信息
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

	#include "ffmpeg/include/libavcodec/avcodec.h"


	#include "ffmpeg/include/libavformat/avformat.h"


	#include "ffmpeg/include/libswscale/swscale.h"
	#include "libyuv/include/libyuv.h"
	#include "libavutil/opt.h"
	#include "libswresample/swresample.h"
	#include "libavutil/channel_layout.h"
};

/*
 * Class:     com_example_ffmpeg_FFmpegUtils
 * Method:    decode
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffmpeg_FFmpegUtils_decode
  (JNIEnv * env, jclass jcls, jstring input_jstr, jstring output_jstr) {

	const char *inputFile = env->GetStringUTFChars(input_jstr, NULL);
	const char *outputFile = env->GetStringUTFChars(output_jstr, NULL);
	LOGE("outputFile: %s", outputFile);
	FILE *fp_yuv = fopen(outputFile, "wb+");
	if(fp_yuv == NULL) {
		LOGE("打开文件失败");
		return;
	}

	//头文件libavformat/avformat.h
	//注册所有组件
	/**
	 * 初始化libavformat和注册所有的 muxers, demuxers 和协议，如果你不想使用次函数，
	 * 则可以手动选择你想要的支持格式
	 * 详情你可以选择下面的函数查询
	 * @see av_register_input_format()
	 * @see av_register_output_format()
	 *
	 * muxer是合并将视频文件、音频文件和字幕文件合并为某一个视频格式。如，可将a.avi, a.mp3, a.srt用muxer合并为mkv格式的视频文件。
	 * demuxer是拆分这些文件的。
	 */
	av_register_all();

	// 封装格式上下文结构体，也是统领全局的结构体，保存了视频文件封装格式相关信息。
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	//AVFormatContext *pFormatCtx = NULL;

	/**
	 * 打开输入流并且读取头信息。但解码器没有打开
	 * 这个输入流必须使用avformat_close_input()关闭
	 * @param ps（第一个参数的形参名称） 指向 你由你提供AVFormatContext（AVFormatContext是由avformat_alloc_context函数分配的）。
	 * ps有可能指向空，在这种情况下，AVFormatContext由此函数分配并写入ps。
	 * 注意： 你提供的AVFormatContext在函数执行失败的时候将会被释放
	 * @param url 你要打开视频文件路径.
	 * @param fmt  如果不为空，那么这个参数将强制作为输入格式，否则自动检索
	 * @param options 一个关于AVFormatContext and demuxer-private 选项的字典.
	 * 返回时，此参数将被销毁，并替换为包含未找到的选项的dict。有可能是空的
	 *
	 * @return 返回0表示成功, 一个负数常量AVERROR是失败的.
	 *
	 * @note 如果你想自定义IO,你需要预分配格式内容并且设置pd属性
	 */
	if(avformat_open_input(&pFormatCtx, inputFile, NULL, NULL) != 0) {
		LOGE("视频文件找不到");
		return;
	}


	/**
	 *上面打开输入流后会将视频封装格式信息写入AVFormatContext中那么我们下一步就可以得到一些展示信息
	 * 读取媒体文件中的数据包以获取流信息，这个对于文件格式没有头信息的很有帮助，比如说mpeg
	 * 这个函数还可以计算在MPEG-2重复帧模式的真实帧速率。
	 * 逻辑文件位置不会被这个函数改变
	 * 检索过的数据包或许会缓存以供后续处理
	 * @param ic  第一个参数 封装格式上下文
	 * @param options
	 *              如果不为空， 一个长度为 ic.nb_streams （封装格式所有流，字幕 视频 音频等） 的字典。
	 *              字典中第i个成员  包含一个对应ic第i个流中对的编码器。
	 *              在返回时，每个字典将会填充没有找到的选项
	 * @return 如果返回>=0 代表成功, AVERROR_xxx 表示失败
	 *
	 * @note 这个函数 不保证能打开所有编码器，所以返回一个非空的选项是一个完全正常的行为
	 *
	 *
	 * @todo
	 *  下个版本目标无视即可
	 * Let the user decide somehow what information is needed so that
	 *       we do not waste time getting stuff the user does not need.
	 */
	if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
		LOGE("无法获取视频文件信息");
		return;
	}

	//获取视频流对应的AVSream的索引位置
	//遍历所有类型的流（音频流、视频流、字幕流），找到视频流
	int video_sream_index = -1;
	for(int i = 0; i < pFormatCtx->nb_streams; i++) {
		 //获取视频流pFormatCtx->streams[i]
		 //pFormatCtx->streams[i]->codec获取编码器
		 //codec_type获取编码器类型
		 //当前流等于视频 记录下标
		//根据AVMEDIA_TYPE属性判断是否是视频流
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_sream_index = i;
			break;
		}
	}

	if(video_sream_index == -1) {
		LOGE("未找到视频流");
		return;
	}

	//5.解码
	//编码器上下文结构体，保存了视频（音频）编解码相关信息
	//得到视频流编码器
	//只有知道视频的编码方式，才能够根据编码方式去找到解码器
	//获取视频流中的编解码上下文
	 AVCodecContext *pCodecCtx = pFormatCtx->streams[video_sream_index]->codec;

	 // 每种视频（音频）编解码器(例如H.264解码器)对应一个该结构体。
	 AVCodec *avcodec = avcodec_find_decoder(pCodecCtx->codec_id);

	 if(avcodec == NULL) {
		 LOGE("无法解码");
		 return;
	 }

	 //打开解码器
	/**
	 * 初始化 指定AVCodecContext去使用 给定的AVCodec
	 * 在使用之前函数必须使用avcodec_alloc_context3（）分配上下文。
	 *
	 * 以下函数 avcodec_find_decoder_by_name(), avcodec_find_encoder_by_name(),
	 * avcodec_find_decoder() and avcodec_find_encoder() 提供了一个简便的得到一个解码器的方法
	 *
	 * @warning 这个函数线程不是安全的
	 *
	 * @note 在使用解码程序之前，始终调用此函数 (如 @ref avcodec_decode_video2()).
	 * 下面是示例代码
	 * @code
	 * avcodec_register_all();
	 * av_dict_set(&opts, "b", "2.5M", 0);
	 * codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	 * if (!codec)
	 *     exit(1);
	 *
	 * context = avcodec_alloc_context3(codec);
	 *
	 * if (avcodec_open2(context, codec, opts) < 0)
	 *     exit(1);
	 * @endcode
	 *
	 *
	 * @param avctx 要初始化的编码器
	 * @param codec 用这个codec去打开给定的上下文编码器.如果 codec 不为空 那么必须
	 * 事先用avcodec_alloc_context3和avcodec_get_context_defaults3传递给这个context，那么这个codec
	 * 要么为NULL要么就是上面调用函数所使用的codec
	 *
	 * @param
	 *
	 * 选项填充AVCodecContext和编解码器私有选项的字典。返回时，此对象将填充未找到的选项。
	 *
	 * @return 返回0表示成功， 负数失败
	 * @see avcodec_alloc_context3(), avcodec_find_decoder(), avcodec_find_encoder(),
	 *      av_dict_set(), av_opt_find().
	 */
	 if(avcodec_open2(pCodecCtx, avcodec, NULL) != 0) {
		 LOGE("解码器无法打开");
		 return;
	 }

	 //输出视频信息
	LOGI("视频的文件格式：%s",pFormatCtx->iformat->name);
	LOGI("视频时长：%d", (pFormatCtx->duration)/1000000);
	LOGI("视频的宽高：%d,%d",pCodecCtx->width,pCodecCtx->height);
	LOGI("解码器的名称：%s",avcodec->name);

	 //得到视频播放时长
	if(pFormatCtx->duration != AV_NOPTS_VALUE){
		int hours, mins, secs, us;
		int64_t duration = pFormatCtx->duration + 5000;
		secs = duration / AV_TIME_BASE;
		us = duration % AV_TIME_BASE;
		mins = secs / 60;
		secs %= 60;
		hours = mins/ 60;
		mins %= 60;
		LOGE("%02d:%02d:%02d.%02d\n", hours, mins, secs, (100 * us) / AV_TIME_BASE);
	}

	 //存储一帧压缩编码数据。
	 AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	 //标志位
	 int got_picture, ret;

	 //AVFrame用于存储解码后的像素数据(YUV)
	 //内存分配
	 AVFrame *pFrame = av_frame_alloc();

	   //YUV420转码用
	    //  AVFrame *pFrameYUV = av_frame_alloc();

	    //  //avpicture_get_size()函数介绍:
	    //  //
	    //  /**
	    //   * 如果给定存储图片的格式,那么计算给定的宽高所占用的大小
	    //   *
	    //   * @param pix_fmt   图片像素格式
	    //   * @param width     图片宽
	    //   * @param height     图片高
	    //   * @return 返回计算的图片缓存大小或者错误情况下的负数错误代码
	    //   *
	    //   *
	    //   * 这里计算缓存区的大小,但是没有分配,这里是用来后面转码使用
	    //   */
	    //  uint8_t *out_buffer = av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P,pCodecCtx->width,pCodecCtx->height));
	    //
	    //  //初始化缓冲区
	    //  /**
	    //   * 基于指定的图片参数和提供的图片缓存区去设置图片字段
	    //   *
	    //   * 使用ptr所指向的图片数据缓存  填充图片属性
	    //   *
	    //   * 如果 ptr是空,这个函数仅填充图片行大小(linesize)的数组并且返回图片缓存请求的大小
	    //   *
	    //   * 要分配图片缓存并且再一次填充图片数据请使用 avpicture_alloc().
	    //   * @param picture       要填充的图片
	    //   * @param ptr           存储图片的数据的缓存区, or NULL
	    //   * @param pix_fmt       图片像素格式
	    //   * @param width         图片宽
	    //   * @param height        图片高
	    //   * @return 返回请求的字节大小,在错误的情况下返回负数
	    //   *
	    //   * @see av_image_fill_arrays()
	    //   */
	    //  avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	    //  //用于转码（缩放）的参数，转之前的宽高，转之后的宽高，格式等
	    //  /**
	    //   *分配和返回 SwsContext. You need it to perform
	    //   * scaling/conversion operations using sws_scale().
	    //   *
	    //   * @param srcW 原始图宽
	    //   * @param srcH 原始图高
	    //   * @param srcFormat 原始图格式
	    //   * @param dstW 目标图宽
	    //   * @param dstH 不解释
	    //   * @param dstFormat 不解释
	    //   * @param flags 指定一个标志用于重新调整算法和选项
	    //   *  具体参考:http://blog.csdn.net/leixiaohua1020/article/details/12029505
	    //   * @return 一个指定分配内容的指针, 错误情况返回空
	    //   * @note this function is to be removed after a saner alternative is
	    //   *       written
	    //   */
	    //      struct SwsContext *sws_ctx =sws_getContext(pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt,
	    //              pCodecCtx->width, pCodecCtx->height,AV_PIX_FMT_YUV420P,
	    //              SWS_BICUBIC, NULL, NULL, NULL);
	    //
	    //

	 //YUV420转码用
	 AVFrame *pFrameYUV = av_frame_alloc();

	 //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
	 //缓冲区分配内存缓冲区的帧
	 int numburBytes;

	  //avpicture_get_size()函数介绍:
	  /**
	   * 如果给定存储图片的格式,那么计算给定的宽高所占用的大小
	   *
	   * @param pix_fmt   图片像素格式
	   * @param width     图片宽
	   * @param height     图片高
	   * @return 返回计算的图片缓存大小或者错误情况下的负数错误代码
	   * 这里计算缓存区的大小,但是没有分配,这里是用来后面转码使用
	   */
	 numburBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

	 //初始化缓冲区
	 uint8_t *out_buffer = (uint8_t *)av_malloc(numburBytes);

	//关联缓冲区
	/**
	* 基于指定的图片参数和提供的图片缓存区去设置图片字段
	*
	* 使用ptr所指向的图片数据缓存  填充图片属性
	*
	* 如果 ptr是空,这个函数仅填充图片行大小(linesize)的数组并且返回图片缓存请求的大小
	*
	* 要分配图片缓存并且再一次填充图片数据请使用 avpicture_alloc().
	* @param picture       要填充的图片
	* @param ptr           存储图片的数据的缓存区, or NULL
	* @param pix_fmt       图片像素格式
	* @param width         图片宽
	* @param height        图片高
	* @return 返回请求的字节大小,在错误的情况下返回负数
	*
	* @see av_image_fill_arrays()
	*/
	 avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

	 //用来打印出了的帧数据
	 int frame_count = 0;
	 //一帧一帧的读取压缩数据
	 /**
	  *返回下一帧的流
	  * 此函数返回存储在文件中的内容，并且不会验证解码器有什么有效帧。
	  * 函数将存储在文件中的帧进行分割 并且返回给每一个调用者。
	  *
	  * 函数不会删除在有效帧之间的无效数据 以便在可能解码过程中提供解码器最大的信息帮助
	  * 如果 pkt->buf 是空的,那么这个对应数据包是有效的直到下一次调用av_read_frame()
	  * 或者直到使用avformat_close_input().否则包无期限有效
	  * 在这两种情况下 这个数据包当你不在需要的时候,你必须使用使用av_free_packet释放它
	  * 对于视屏,数据包刚好只包含一帧.对于音频,如果它每一帧是一个已知固定大小的,那么他包含整数帧(如. PCM or ADPCM data)
	  * 如果音频帧具有可变大小(如. MPEG audio),那么他只包含一帧
	  * pkt->pts, pkt->dts and pkt->duration 始终在AVStream.time_base 单位设置正确的数值
	  *(如果这个格式无法提供.那么进行猜测)
	  * 如果视频格式有B帧那么pkt->pts可以是 AV_NOPTS_VALUE.如果你没有解压他的有效部分那么最好依靠pkt->dts
	  *
	  * @return 0表示成功, < 0 错误或者文结束
	  */
	 while(av_read_frame(pFormatCtx,packet) >=0) {
		 //只要视频压缩数据（根据流的索引位置判断）
		 if(packet->stream_index == video_sream_index) {

			 /**
			 * 解码视频帧 从avpkt->data读取数据并且解码avpkt->size的大小后转化为图片.
			 * 一些解码器可以支持在一个ACpacket中存在多帧的情况,像这样的解码器将只解码第一帧
			 *
			 * @warning  输入缓存区必须 实际读取的字节流小于 FF_INPUT_BUFFER_PADDING_SIZE,
			 * 一些优化过的比特流 读取32位或者64字节 的时候可以一次性读取完
			 *
			 * @warning 在缓冲器的buf结尾设置0以确保被破坏的MPEG流不会发生超线程
			 *
			 * @note 有 CODEC_CAP_DELAY 才能设置一个在输入和输出之间的延迟,这些需要使用avpkt->data=NULL,
			 *  在结束返回剩余帧数的时候avpkt->size=0
			 *
			 * @note  这个AVCodecContext 在数据包传入解码器之前必须调用avcodec_open2
			 *
			 *
			 * @param avctx 解码器上下文
			 *
			 * @param[out] 解码的视频帧图片将会被存储在AVFrame.
		     *  		        使用av_frame_alloc 得到一个AVFrame,
		     * 			        编码器将会分配 使用  AVCodecContext.get_buffer2() 回调
		     *			        的实际图片的内存.
		     *			        当AVCodecContext.refcounted_frames 设置为1,这帧(frame)是引用计数,并且返回
		     *			         的引用计数是属于调用者的.
			 *             frame在长实际不使用的时候调用者必须调用av_frame_unref()就行释放
			 *             如果av_frame_is_writable()返回1那么调用者可以安全的写入到这个frame中。
			 *             当AVCodecContext.refcounted_frames设置为0，返回的引用属于解码器，
			 *             只有下次使用这个函数或者关闭或者刷新这个编码器之前有效。调用者不会写入它
			 *
			 *@param[in,out] got_picture_ptr 如果为0表示不能解压, 否者它不是0.
			 *
			 * @param[in] avpkt 这个输入的avpkt包含输入缓存区
			 *              你能使用av_init_packet（）创建像这样的packet然后设置数据和大小，
			 *              一些解码器还可以添加一些其他字段 比如  flags&AV_PKT_FLAG_KEY  flags&AV_PKT_FLAG_KEY
			 *          所有解码器都设计为尽可能少地使用
			 *
			 * @return 再错误时返回一个负数 , 否则返回使用字节数或者或者0(没有帧被解压返回0)otherwise the number of bytes
			 *
			 */
			 avcodec_decode_video2(pCodecCtx, pFrame, &ret, packet);

			 //如果为0表示不能解压, 否者它不是0
			 if(ret) {

				 //用于转码（缩放）的参数，转之前的宽高，转之后的宽高，格式等
				 SwsContext * img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
								 pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC,NULL, NULL,NULL);

				 //AVFrame转为像素格式YUV420，宽高
				 //2 6输入、输出数据
				 //3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
				 //4 输入数据第一列要转码的位置 从0开始
				 //5 输入画面的高度
				 sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
				 		pFrameYUV->data, pFrameYUV->linesize);

				//输出到YUV文件
				//AVFrame像素帧写入文件
				//data解码后的图像像素数据（音频采样数据）
				//Y 亮度 UV 色度（压缩了） 人对亮度更加敏感
				//U V 个数是Y的1/4
				int y_size = pCodecCtx->width * pCodecCtx->height;
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);

				frame_count++;
				LOGI("解码第%d帧",frame_count);

			 }
		 }

		 //释放资源
		 av_free_packet(packet);

	 }

	fclose(fp_yuv);

	env->ReleaseStringUTFChars(input_jstr, inputFile);
	env->ReleaseStringUTFChars(output_jstr, outputFile);

	av_free(out_buffer);

	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);
	avformat_free_context(pFormatCtx);

	avcodec_close(pCodecCtx);

}





/*
 * Class:     com_example_ffmpeg_FFmpegUtils
 * Method:    paly
 * Signature: (Ljava/lang/String;Landroid/view/Surface;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffmpeg_FFmpegUtils_paly
  (JNIEnv * env, jclass jcls, jstring path_jstr, jobject surface) {

	const char *file_path =env->GetStringUTFChars(path_jstr, NULL);

	av_register_all();

	AVFormatContext * pFormatCtx = avformat_alloc_context();

	if(avformat_open_input(&pFormatCtx, file_path, NULL, NULL) != 0) {
		LOGE("打开视频文件失败");
		LOGE("%s\n", strerror(errno));
		return;
	}

	if(avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGE("无法获取视频文件信息");
		return;
	}

	int stream_video_index = -1;

	for(int i = 0; i < pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			stream_video_index = i;
			break;
		}
	}

	if(stream_video_index == -1) {
		LOGE("未找到视频流");
		return;
	}

	//获取视频流的编解码器
	AVCodecContext *pCodecCtx   = pFormatCtx->streams[stream_video_index]->codec;
	AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL){
		LOGE("%s","找不到解码器\n");
		return;
	}else{
		LOGE("%s","找到解码器\n");
	}

	//打开解码器
	if(avcodec_open2(pCodecCtx, pCodec, NULL) != 0){
		LOGE("%s","打开编码器失败\n");
		 return;
	}
	AVPacket *pkt = (AVPacket *)av_malloc(sizeof(AVPacket));
	AVFrame *picture = av_frame_alloc();

	int got_picture_ptr;

	ANativeWindow_Buffer outBuffer;

	//设置缓存的几何信息
	 AVFrame *rgb_frame = av_frame_alloc();

	//uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA,pCodecCtx->width,pCodecCtx->height));



	ANativeWindow* mWindow = ANativeWindow_fromSurface(env, surface);

	//一帧一帧解码
	while(av_read_frame(pFormatCtx, pkt) >= 0) {
		if(pkt->stream_index == stream_video_index) {
			//解包成一帧一帧图像
			int ret = avcodec_decode_video2(pCodecCtx, picture, &got_picture_ptr,pkt);

             if(ret >= 0) {

            	 ANativeWindow_setBuffersGeometry(mWindow, pCodecCtx->width,pCodecCtx->height,WINDOW_FORMAT_RGBA_8888);
            	 ANativeWindow_lock(mWindow, &outBuffer, NULL);

            	 avpicture_fill((AVPicture *)rgb_frame,(uint8_t *)outBuffer.bits, AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height);

            	 libyuv::I420ToARGB(picture->data[0], picture->linesize[0],
            			 	 picture->data[2], picture->linesize[2],
            			 	 picture->data[1], picture->linesize[1],
            			 	 rgb_frame->data[0], rgb_frame->linesize[0],
            			 	 pCodecCtx->width, pCodecCtx->height);
            	 ANativeWindow_unlockAndPost(mWindow);
            	 usleep(1000 * 10);
             }
		}
		av_free_packet(pkt);
	}


	ANativeWindow_release(mWindow);
	av_frame_free(&picture);
	avcodec_close(pCodecCtx);
	avformat_free_context(pFormatCtx);


	env->ReleaseStringUTFChars(path_jstr, file_path);
}


/**
 * 音频重采样
 */
JNIEXPORT void JNICALL Java_com_example_ffmpeg_FFmpegUtils_decodeAudio
  (JNIEnv *env, jclass jcls, jstring input_jstr, jstring output_jstr) {

	const char *input_cstr = env->GetStringUTFChars(input_jstr, NULL);
	const char *output_cstr = env->GetStringUTFChars(output_jstr, NULL);

	FILE *out_fp = fopen(output_cstr,"wb+");

	av_register_all();
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
		LOGE("打开音频文件失败  %s\n", strerror(errno));
		return;
	}
	if(avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGE("找不到音频信息");
		return;
	}
	int stream_audio_index = -1;
	for(int i = 0; i < pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			stream_audio_index = i;
			break;
		}
	}
	if(stream_audio_index == -1) {
		LOGE("文件找不到音频信息");
		return;
	}

	 AVCodecContext *pCodecCtx = pFormatCtx->streams[stream_audio_index]->codec;
	 AVCodec *codec = avcodec_find_decoder(pCodecCtx->codec_id);

	if(codec == NULL){
		LOGE("%s","无法获取解码器");
		return;
	}
	//打开解码器
	if(avcodec_open2(pCodecCtx,codec,NULL) < 0){
		LOGE("%s","无法打开解码器");
		return;
	}

	int got_frame_ptr = 0;
	//压缩数据
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	//解压缩数据
	AVFrame *frame = av_frame_alloc();
	//---------------------------------设置重采样参数-----------------------------------------
	 SwrContext *swrCtx = swr_alloc();

	 int src_ch_layout = pCodecCtx->channel_layout;
	 int src_sample_rate = pCodecCtx->sample_rate;
	 enum AVSampleFormat src_sample_fmt = pCodecCtx->sample_fmt;

	int dst_ch_layout = AV_CH_LAYOUT_STEREO;
	int dst_sample_rate = 48000;
	enum AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;

	//两种设置采样参数的方法

	/*av_opt_set_int(swrCtx, "in_channel_layout", src_ch_layout, 0);
	av_opt_set_int(swrCtx, "in_sample_rate", src_sample_rate, 0);
	av_opt_set_int(swrCtx, "in_sample_fmt", src_sample_fmt, 0);

	av_opt_set_int(swrCtx, "out_channel_layout", dst_ch_layout, 0);
	av_opt_set_int(swrCtx, "out_sample_rate", dst_sample_rate, 0);
	av_opt_set_int(swrCtx, "out_sample_fmt", dst_sample_fmt, 0);*/

	swr_alloc_set_opts(swrCtx,
	                  dst_ch_layout, dst_sample_fmt, dst_sample_rate,
	                  src_ch_layout, src_sample_fmt, src_sample_rate,
	                                      0, NULL);

	if(swr_init(swrCtx) < 0) {
		LOGE("%s", "Failed to initialize the resampling context\n");
		return;
	}
	//--------------------------------------------------------------------------

	uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);
	//输出的声道个数
	int dst_channel_nb = av_get_channel_layout_nb_channels(dst_ch_layout);

	int count = 0;
	 while(av_read_frame(pFormatCtx,packet) >=0) {
			 //只要视频压缩数据（根据流的索引位置判断）
			 if(packet->stream_index == stream_audio_index) {
				 int ret = avcodec_decode_audio4(pCodecCtx, frame,&got_frame_ptr, packet);

				 if(ret >= 0) {
					 if(got_frame_ptr > 0) {

						 LOGE("解码音频: %d", count++);
						  swr_convert(swrCtx, &out_buffer, MAX_AUDIO_FRME_SIZE,
						                                 (const uint8_t **)frame->data , frame->nb_samples);
						  //计算输出音频占用内存
						  int out_size = av_samples_get_buffer_size(NULL, dst_channel_nb,
								  	  frame->nb_samples, dst_sample_fmt, 1);
						  fwrite(out_buffer, 1, out_size, out_fp);

					 }
				 }

			 }

			 av_free_packet(packet);
	}
	LOGE("解码完成");
	fclose(out_fp);
	av_frame_free(&frame);
	av_free(out_buffer);

	swr_free(&swrCtx);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	env->ReleaseStringUTFChars(input_jstr, input_cstr);
	env->ReleaseStringUTFChars(output_jstr, output_cstr);
}


JNIEXPORT void JNICALL Java_com_example_ffmpeg_FFmpegUtils_playAudio
  (JNIEnv *env, jclass jcls, jstring input_file_jstr) {

	//------------------------------jni获取AudioTrack对象
	jmethodID method_creat_audiotrack = env->GetStaticMethodID(jcls, "getAudioTrack", "()Landroid/media/AudioTrack;");
	jobject audiotrack =   env->CallStaticObjectMethod( jcls, method_creat_audiotrack);
	jclass audiotrack_cls = env->GetObjectClass(audiotrack);
	jmethodID play = env->GetMethodID(audiotrack_cls, "play", "()V");
	env->CallVoidMethod(audiotrack, play);

	jmethodID write = env->GetMethodID(audiotrack_cls, "write", "([BII)I");
	//-----------------------------------------------
	const char *input_file_cstr = env->GetStringUTFChars(input_file_jstr, NULL);
	int ret = 0;
	av_register_all();
	AVFormatContext * pFrameCtx = avformat_alloc_context();
	if(avformat_open_input(&pFrameCtx, input_file_cstr, NULL, NULL) != 0) {
		LOGE("打开文件失败：%s\n", strerror(errno));
		return;
	}
	if(avformat_find_stream_info(pFrameCtx, NULL) < 0) {
		LOGE("文件中未找到流信息：%s\n", strerror(errno));
		return;
	}

	int stream_audio_index = -1;

	for(int i=0; i < pFrameCtx->nb_streams; i++) {
		if(pFrameCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			stream_audio_index = i;
			break;
		}
	}

	if(stream_audio_index == -1) {
		LOGE("音频解码器未找到");
		return;
	}

	 AVCodecContext *pCodecCtx = pFrameCtx->streams[stream_audio_index]->codec;
	 AVCodec *avcodec = avcodec_find_decoder(pCodecCtx->codec_id);

	 SwrContext *swr_ctx = swr_alloc();
	 int out_ch_layout = AV_CH_LAYOUT_STEREO;
	 enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	 int out_sample_rate = 44100;
	 swr_alloc_set_opts(swr_ctx,
	                      out_ch_layout, out_sample_fmt, out_sample_rate,
	                      pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate,
	                      0, NULL);
	 if(swr_init(swr_ctx) < 0){
		 LOGE("初始化重采样参数异常: %s", strerror(errno));
		 return;
	 }


	if(avcodec_open2(pCodecCtx, avcodec, NULL) != 0) {
		LOGE("解码器打开异常：%s\n", strerror(errno));
		return;
	}


	AVPacket *pkt = (AVPacket *)av_malloc(sizeof(AVPacket));
	AVFrame *frame = av_frame_alloc();
	uint8_t * out_buff = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);
	int out_nb_channels = av_get_channel_layout_nb_channels(out_ch_layout);
	int got_frame_ptr = 0;

	while(av_read_frame(pFrameCtx, pkt) >= 0) {
		if(pkt->stream_index == stream_audio_index) {
			ret = avcodec_decode_audio4(pCodecCtx, frame,
			                          &got_frame_ptr, pkt);

			swr_convert(swr_ctx, &out_buff, MAX_AUDIO_FRME_SIZE,
			                                (const uint8_t **)frame->data , frame->nb_samples);
			 int out_size = av_samples_get_buffer_size(NULL, out_nb_channels, frame->nb_samples,
					 out_sample_fmt, 1);

			//out_buffer缓冲区数据，转成byte数组
			 jbyteArray byteArray = env->NewByteArray(out_size);
			 //给数组赋值
			 jbyte* elments = env->GetByteArrayElements(byteArray, NULL);
			 memcpy(elments, out_buff, out_size);
			 //同步数组
			 env->ReleaseByteArrayElements(byteArray, elments, 0);


			 env->CallVoidMethod(audiotrack, write, byteArray,0, out_size);
			 //释放局部引用
			 env->DeleteLocalRef(byteArray);
		}
	}
	av_frame_free(&frame);
	av_free(out_buff);

	swr_free(&swr_ctx);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFrameCtx);
	env->ReleaseStringUTFChars(input_file_jstr, input_file_cstr);
}


#include <pthread.h>
struct Player{
	AVFormatContext *avFormatCtx;
	int stream_audio_index;
	int stream_video_index;

	AVCodecContext *audio_codec_ctx;
	AVCodecContext *video_codec_ctx;

	ANativeWindow* mWindow;

	 SwrContext *swr_ctx;

	pthread_t video_thread_id;
	pthread_t audio_thread_id;

	jmethodID audioTrack_write;
	jobject audioTrack;


};


jclass global_jcls;
JavaVM *javaVm = NULL;

/**
 * 初始化AVFormatContext
 */
void init_av_ctx(const char *input_file, struct Player *myPlayer) {
	av_register_all();
	AVFormatContext *avFormatCtx = avformat_alloc_context();
	if(avformat_open_input(&avFormatCtx, input_file, NULL, NULL) != 0) {
		LOGE("打开文件失败: %s\n", strerror(errno));
		if(avFormatCtx != NULL) {
				LOGE("释放资源");
				avformat_close_input(&avFormatCtx);
			}
	}
	if(avformat_find_stream_info(avFormatCtx, NULL) < 0) {
		LOGE("未找到视频流信息: %s\n", strerror(errno));
		if(avFormatCtx != NULL) {
				LOGE("释放资源");
				avformat_close_input(&avFormatCtx);
			}
	}
	myPlayer->avFormatCtx = avFormatCtx;
}

/**
 * 获取音视频在文件的索引位置
 */
void find_stream_index(struct Player *myPlayer) {
	for(int i = 0; i < myPlayer->avFormatCtx->nb_streams; i++) {
		if(myPlayer->avFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			LOGE("音频流位置: %d", i);
			myPlayer->stream_audio_index = i;
		}else if(myPlayer->avFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			LOGE("视频频流位置: %d", i);
			myPlayer->stream_video_index = i;
		}
	}
}

/**
 * 打开解码器
 * return value
 * -1 打开解码器失败
 * 0  打开成功
 */
int open_video_decoder(struct Player *myPlayer) {
	AVCodecContext *codec_ctx = myPlayer->avFormatCtx->streams[myPlayer->stream_video_index]->codec;
	AVCodec *pCodec = avcodec_find_decoder(codec_ctx->codec_id);

	if(avcodec_open2(codec_ctx, pCodec, NULL) != 0) {
		LOGE("视频解码器打开错误: %s\n", strerror(errno));
		return -1;
	}
	myPlayer->video_codec_ctx = codec_ctx;
	return 0;
}


/**
 * 打开解码器
 * return value
 * -1 打开解码器失败
 * 0  打开成功
 */
int open_audio_decoder(struct Player *myPlayer) {
	LOGE("%d\n", myPlayer->stream_audio_index);
	AVCodecContext *codec_ctx = myPlayer->avFormatCtx->streams[myPlayer->stream_audio_index]->codec;
	AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
	if(codec == NULL) {
		LOGE("获取解码器错误: %s\n", strerror(errno));
		return -1;
	}
	if(avcodec_open2(codec_ctx, codec, NULL) != 0) {
		LOGE("音频解码器打开错误: %s\n", strerror(errno));
		return -1;
	}
	myPlayer->audio_codec_ctx = codec_ctx;
	return 0;
}


void init_window(JNIEnv *env, jobject surface, struct Player *myPlayer) {
	ANativeWindow* mWindow = ANativeWindow_fromSurface(env, surface);
	myPlayer->mWindow = mWindow;
}

/**
 * 解码视频
 */
void decode_v(struct Player *myPlayer, AVPacket *pkt) {
	AVFrame *picture = av_frame_alloc();

	int got_picture_ptr;

	ANativeWindow_Buffer outBuffer;

	//设置缓存的几何信息
	 AVFrame *rgb_frame = av_frame_alloc();

	//uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_RGBA,pCodecCtx->width,pCodecCtx->height));

	ANativeWindow* mWindow = myPlayer->mWindow;
	//解包成一帧一帧图像
	int ret = avcodec_decode_video2(myPlayer->video_codec_ctx, picture, &got_picture_ptr,pkt);

	 if(ret >= 0) {
		 ANativeWindow_setBuffersGeometry(mWindow, myPlayer->video_codec_ctx->width,myPlayer->video_codec_ctx->height,WINDOW_FORMAT_RGBA_8888);
		 ANativeWindow_lock(mWindow, &outBuffer, NULL);

		 avpicture_fill((AVPicture *)rgb_frame,(uint8_t *)outBuffer.bits, AV_PIX_FMT_RGBA, myPlayer->video_codec_ctx->width, myPlayer->video_codec_ctx->height);

		 libyuv::I420ToARGB(picture->data[0], picture->linesize[0],
					 picture->data[2], picture->linesize[2],
					 picture->data[1], picture->linesize[1],
					 rgb_frame->data[0], rgb_frame->linesize[0],
					 myPlayer->video_codec_ctx->width, myPlayer->video_codec_ctx->height);
		 ANativeWindow_unlockAndPost(mWindow);
		 usleep(1000 * 10);
	 }

	 av_frame_free(&picture);
	 av_frame_free(&rgb_frame);
}



/**
 * 解码视频
 */
void *decode_video(void *param) {
	struct Player *myPlayer = (struct Player *)param;
	//编码数据
	AVPacket *pkt = (AVPacket *)av_malloc(sizeof(AVPacket));
	//一帧一帧解码
	while(av_read_frame(myPlayer->avFormatCtx, pkt) >= 0) {
		if(pkt->stream_index == myPlayer->stream_video_index) {
			//解包成一帧一帧图像
			decode_v(myPlayer, pkt);
		}
		av_free_packet(pkt);
	}
}






void getAudioTrack(JNIEnv *env, Player *myPlayer) {
	jmethodID method_creat_audiotrack = env->GetStaticMethodID(global_jcls, "getAudioTrack", "()Landroid/media/AudioTrack;");
	jobject audiotrack =   env->CallStaticObjectMethod(global_jcls, method_creat_audiotrack);
	jclass audiotrack_cls = env->GetObjectClass(audiotrack);

	jmethodID play = env->GetMethodID(audiotrack_cls, "play", "()V");
	env->CallVoidMethod(audiotrack, play);
	jmethodID write = env->GetMethodID(audiotrack_cls, "write", "([BII)I");

	myPlayer->audioTrack_write = write;
	myPlayer->audioTrack = audiotrack;
}

/**
 * jni初始化调用方法
 */
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	javaVm = vm;
	return JNI_VERSION_1_4;
}


/**
 * 重采样
 */
void swr_audio(Player *myPlayer) {
	AVCodecContext *pCodecCtx = myPlayer->audio_codec_ctx;
	 SwrContext *swr_ctx = swr_alloc();
	 int out_ch_layout = AV_CH_LAYOUT_STEREO;
	 enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	 int out_sample_rate = 44100;
	 swr_alloc_set_opts(swr_ctx,
						  out_ch_layout, out_sample_fmt, out_sample_rate,
						  pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate,
						  0, NULL);
	 if(swr_init(swr_ctx) < 0){
		 LOGE("初始化重采样参数异常: %s", strerror(errno));
		 return;
	 }

	 myPlayer->swr_ctx = swr_ctx;
}


/**
 * 解码音频
 */
void*decode_audio(void *param) {
	struct Player *myPlayer = (struct Player *)param;


	AVFormatContext *pFrameCtx = myPlayer->avFormatCtx;
	AVCodecContext *pCodecCtx = myPlayer->audio_codec_ctx;
	JNIEnv *env;
	javaVm->AttachCurrentThread(&env,NULL);

	getAudioTrack(env, myPlayer);

	AVPacket *pkt = (AVPacket *)av_malloc(sizeof(AVPacket));
	AVFrame *frame = av_frame_alloc();
	uint8_t * out_buff = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);

	int out_ch_layout = AV_CH_LAYOUT_STEREO;
	int out_sample_rate = 44100;
	int out_nb_channels = av_get_channel_layout_nb_channels(out_ch_layout);
	int got_frame_ptr = 0;
	int ret = -1;
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;

	while(av_read_frame(pFrameCtx, pkt) >= 0) {
		if(pkt->stream_index == myPlayer->stream_audio_index) {
			ret = avcodec_decode_audio4(pCodecCtx, frame,
									  &got_frame_ptr, pkt);

			swr_convert(myPlayer->swr_ctx, &out_buff, MAX_AUDIO_FRME_SIZE,
											(const uint8_t **)frame->data , frame->nb_samples);
			 int out_size = av_samples_get_buffer_size(NULL, out_nb_channels, frame->nb_samples,
					 out_sample_fmt, 1);

			//out_buffer缓冲区数据，转成byte数组
			 jbyteArray byteArray = env->NewByteArray(out_size);
			 //给数组赋值
			 jbyte* elments = env->GetByteArrayElements(byteArray, NULL);
			 memcpy(elments, out_buff, out_size);
			 //同步数组
			 env->ReleaseByteArrayElements(byteArray, elments, 0);


			 env->CallVoidMethod(myPlayer->audioTrack, myPlayer->audioTrack_write, byteArray,0, out_size);
			 //释放局部引用
			 env->DeleteLocalRef(byteArray);
		}
	}

	javaVm->DetachCurrentThread();
}


/*
 * Class:     com_example_ffmpeg_FFmpegUtils
 * Method:    palyAudioAndVideo
 * Signature: (Ljava/lang/String;Landroid/view/Surface;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffmpeg_FFmpegUtils_palyAudioAndVideo
  (JNIEnv *env, jclass jcls, jstring input_file_jstr, jobject surface) {

	struct Player *myPlayer = (struct Player *)malloc(sizeof(struct Player));
	global_jcls = (jclass)env->NewGlobalRef(jcls);
	const char *inpuft_file_cst = env->GetStringUTFChars(input_file_jstr, NULL);
	myPlayer->stream_audio_index = AVMEDIA_TYPE_UNKNOWN;
	myPlayer->stream_video_index = AVMEDIA_TYPE_UNKNOWN;
	init_av_ctx(inpuft_file_cst, myPlayer);
	//javaVm = env->GetJavaVM(&javaVm);
	find_stream_index(myPlayer);

	if(myPlayer->stream_audio_index == AVMEDIA_TYPE_UNKNOWN || myPlayer->stream_video_index == AVMEDIA_TYPE_UNKNOWN) {
		LOGE("找不到音视频流信息");
		return;
	}
	//打开视频解码器
	open_video_decoder(myPlayer);

	//打开音频解码器
	open_audio_decoder(myPlayer);

	swr_audio(myPlayer);

	init_window(env, surface, myPlayer);
	//pthread_create(&(myPlayer->video_thread_id), NULL,decode_video, (void *)myPlayer);

	pthread_create(&myPlayer->audio_thread_id, NULL, decode_audio, (void *)myPlayer);
	env->ReleaseStringUTFChars(input_file_jstr, inpuft_file_cst);
}



