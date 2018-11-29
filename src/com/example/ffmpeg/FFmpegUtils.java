package com.example.ffmpeg;

import android.view.Surface;

public class FFmpegUtils {
	public native  static void decode(String input, String output);
	
	public native static void paly(String path, Surface surface);
	
	
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
