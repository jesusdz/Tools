package com.tools.game;

import android.os.Bundle;
import android.util.Log;

public class MainActivity extends android.app.NativeActivity
{
	/*
	@Override
		public void onCreate(Bundle savedInstanceState) {
			super.onCreate(savedInstanceState);

			try
			{
				Log.w("com.tools.game", "Trying to load shared library!");
			}
			catch (java.lang.UnsatisfiedLinkError e)
			{
				Log.e("com.tools.game", e.getMessage());
				throw e;
			}


			//System.loadLibrary("c++");
			//System.loadLibrary("c++");
			//System.loadLibrary("log");
			//System.loadLibrary("android");
			//System.loadLibrary("EGL");
			//System.loadLibrary("GLESv1_CM");
			//System.loadLibrary("game");
		}
*/
	static
	{
		//AllLoadedNativeLibrariesInJVM.listAllLoadedNativeLibrariesFromJVM();

		Log.i("tools", "Trying to load shared library!");
		try
		{
			//initializeJni();
			System.loadLibrary("game");
		}
		catch(java.lang.UnsatisfiedLinkError e)
		{
			Log.e("tools", e.getMessage());
			throw e;
			//if(e.getMessage().contains("libOpenCL"))
			//{
			//	Log.e( TAG, "This device does not support OpenCL" );
			//	mWasError = true;
			//	mErrorMessage = "This device does not support OpenCL";
			//}
			//else
			//{
			//	throw e;
			//}
		}
	}

}

