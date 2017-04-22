package tuchsen.hellocube;

import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GLView extends GLSurfaceView {
	public GLView(Context context) {
		super(context);
		setEGLContextClientVersion(2);
		setRenderer(new Renderer(context));
	}

	private static class Renderer implements GLSurfaceView.Renderer {
		private Context context;

		Renderer(Context context) {
			this.context = context;
		}

		public void onDrawFrame(GL10 gl) {
			drawFrame();
		}

		public void onSurfaceChanged(GL10 gl, int width, int height) {
			init(context.getAssets(), width, height);
		}

		public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		}
	}

	public static native void drawFrame();

	private static native void init(AssetManager am, int w, int h);

	static {
		System.loadLibrary("helloCube");
	}
}
