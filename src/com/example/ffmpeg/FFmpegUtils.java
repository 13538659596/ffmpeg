package com.example.ffmpeg;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.view.Surface;

public class FFmpegUtils {
	/**
	 * ffmpeg解码视频封装格式
	 * @param input
	 * @param output
	 */
	public native  static void decode(String input, String output);
	
	/**
	 * ffmpeg解码视频，转换成RGB用AvNativeWindow绘制每一帧数据
	 * @param path
	 * @param surface
	 */
	public native static void paly(String path, Surface surface);
	
	
	public native static void decodeAudio(String input, String output); 
	
	public native static void playAudio(String input);
	
	
	public static AudioTrack getAudioTrack() {
		
		int channel = AudioFormat.CHANNEL_OUT_STEREO;
		int sample = 48000;
		int format =  AudioFormat.ENCODING_PCM_16BIT;
	    int bufsize = AudioTrack.getMinBufferSize(sample,
			   channel,
			   format);

		   AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
				   sample, channel, 
				   format, bufsize, AudioTrack.MODE_STREAM);
		   return audioTrack;
	}
	
	static {
		System.loadLibrary("avutil-54");
		System.loadLibrary("avformat-56");
		System.loadLibrary("avcodec-56");
		System.loadLibrary("avcodec-56");
		System.loadLibrary("avfilter-5");
		System.loadLibrary("avformat-56");
		System.loadLibrary("swresample-1");
		System.loadLibrary("postproc-53");
		System.loadLibrary("myffmpeg");
		System.loadLibrary("yuv");
	}
}
