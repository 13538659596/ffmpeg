package come.view;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class ViedioSurface extends SurfaceView{

	public ViedioSurface(Context context, AttributeSet attrs, int defStyleAttr) {
		super(context, attrs, defStyleAttr);
		// TODO Auto-generated constructor stub
		
		setPixelFormat();
	}

	public ViedioSurface(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		// TODO Auto-generated constructor stub
	}

	public ViedioSurface(Context context) {
		this(context, null);
		// TODO Auto-generated constructor stub
	}
	
	private void setPixelFormat() {
		//初始化，SufaceView绘制的像素格式
		SurfaceHolder holder = getHolder();
		holder.setFormat(PixelFormat.RGBA_8888);
	}

}
