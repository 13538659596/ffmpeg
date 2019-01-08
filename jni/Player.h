/*
 * Player.h
 *
 *  Created on: 2019-1-7
 *      Author: lenovo
 */

#ifndef PLAYER_H_
#define PLAYER_H_
#include <jni.h>
extern "C" {

	#include "ffmpeg/include/libavcodec/avcodec.h"


	#include "ffmpeg/include/libavformat/avformat.h"


	#include "ffmpeg/include/libswscale/swscale.h"
	#include "libyuv/include/libyuv.h"
	#include "libavutil/opt.h"
	#include "libswresample/swresample.h"
	#include "libavutil/channel_layout.h"
};
// for native window JNI
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <pthread.h>

//nb_streams，视频文件中存在，音频流，视频流，字幕
#define MAX_STREAM 2

typedef struct Player{

	JavaVM *javaVm;
	AVFormatContext *pFormatContext;

	//文件中流的数量
	int captrue_streams_no;

	int video_stream_index;
	int audio_stream_index;

	ANativeWindow *nativeWindow;

	//解码器上下文数组
	AVCodecContext *input_codec_ctx[MAX_STREAM];


	//音频相关
	SwrContext *swr_ctx;
	//输入的采样格式
	enum AVSampleFormat in_sample_fmt;
	//输出采样格式16bit PCM
	enum AVSampleFormat out_sample_fmt;
	//输入采样率
	int in_sample_rate;
	//输出采样率
	int out_sample_rate;
	//输出的声道个数
	int out_channel_nb;

	pthread_t thread_read_from_stream;

	//音频，视频队列数组
	Queue *packets[MAX_STREAM];

	//解码线程ID
	pthread_t decode_threads[MAX_STREAM];


	//播放音频相关
	jobject audio_track;
	jmethodID audio_track_write_mid;

}MyPlayer;


typedef struct Decoder{
	MyPlayer *player;
	int stream_index;
}MyDecoder;

#endif /* PLAYER_H_ */
