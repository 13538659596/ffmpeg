package com.example.ffmpeg;

import java.io.File;

import android.os.Bundle;
import android.os.Environment;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.SurfaceView;
import android.view.View;

public class MainActivity extends Activity {

	SurfaceView surfaceView;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		surfaceView = (SurfaceView) findViewById(R.id.viedio_view);
	}
	
	public void decode(View view) {
		String input = new File(Environment.getExternalStorageDirectory(),"input.mp4").getAbsolutePath();
		String output = new File(Environment.getExternalStorageDirectory(),"output_1920x1080_yuv420p.yuv").getAbsolutePath();
		FFmpegUtils.decode(input, output);
	}
	
	public void play(View view) {
		String input = new File(Environment.getExternalStorageDirectory(),"input.mp4").getAbsolutePath();
		FFmpegUtils.paly(input, surfaceView.getHolder().getSurface());
	}
	
	public void decodeAudio(View view) {
		String input = new File(Environment.getExternalStorageDirectory(),"input.mp4").getAbsolutePath();
		String output = new File(Environment.getExternalStorageDirectory(),"output_pcm.pcm").getAbsolutePath();
		FFmpegUtils.decodeAudio(input, output);
	}
	
	public void decodePlayAudio(View view) {
		String input = new File(Environment.getExternalStorageDirectory(),"input.mp4").getAbsolutePath();
		FFmpegUtils.playAudio(input);
	}
}
