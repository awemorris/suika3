/* -*- coding: utf-8; tab-width: 4; indent-tabs-mode: t; -*- */

/*
 * StratoHAL
 * Android port's Java part
 * (See also ndkmain.c and ndkfile.c for the C part.)
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

package io.noctvm.playfield.engineandroid;

import static android.opengl.GLSurfaceView.RENDERMODE_CONTINUOUSLY;
import static android.opengl.GLSurfaceView.RENDERMODE_WHEN_DIRTY;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.AssetFileDescriptor;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.opengl.GLES20;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.view.SurfaceHolder;
import android.view.WindowManager.LayoutParams;
import android.widget.LinearLayout;

import androidx.annotation.RequiresApi;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.File;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

//
// Main class
//
public class MainActivity extends Activity {
    //
    // Config values (Please change these constants for your game)
    //

    // Window title
    private static final String APP_NAME = "App";

    // Window width
    private static int VIEWPORT_WIDTH = 1280;

    // Window height
    private static int VIEWPORT_HEIGHT = 720;

    //
    // JNI (do not touch)
    //

    // Load an Android NDK library.
    static {
        System.loadLibrary("playfield");
    }

    // These are the native methods implemented in ndk*.c
    private native void nativeInitGame();
    private native int nativeGetScreenWidth();
    private native int nativeGetScreenHeight();
    private native void nativeReinitOpenGL();
    private native void nativeCleanup();
    private native boolean nativeRunFrame();
    private native boolean nativeOnPause();
    private native boolean nativeOnResume();
    private native void nativeOnTouchStart(int x, int y, int points);
    private native void nativeOnTouchMove(int x, int y);
    private native void nativeOnTouchEnd(int x, int y, int points);
	private native void nativeOnKeyDown(int key);
	private native void nativeOnKeyUp(int key);
	private native void nativeOnGamepadAnalog(float x1, float y1, float x2, float y2, float l, float r);

    //
    // Variables (do not touch)
    //

    // The main view.
    private MainView view;

    // The viewport scale factor.
    private float scale;

    // The viewport offset X.
    private int offsetX;

    // The viewport offset Y.
    private int offsetY;

    // The last touched Y coordinate.
    private int touchLastY;

    // Finger count of a last touch.
    private int touchCount;

    // A flag that indicates if the game is loaded.
    private boolean isLoaded;

    // A flag that indicates if the game is finished.
    private boolean isFinished;

    // MediaPlayer for a video playback.
    private MediaPlayer video;

    // The view for video playback.
    private VideoSurfaceView videoView;

    // The layout for videoView.
    private LinearLayout videoLayout;

    // A flag that indicates if we are right after back from video playback.
    private boolean resumeFromVideo;

    // The synchronization object for the mutual exclusion between the main thread and the rendering thread.
    private final Object syncObj = new Object();

	//
	// Key code
	//
	private static int KEY_CONTROL = 0;
	private static int KEY_SPACE = 1;
	private static int KEY_RETURN = 2;
	private static int KEY_UP = 3;
	private static int KEY_DOWN = 4;
	private static int KEY_LEFT = 5;
	private static int KEY_RIGHT = 6;
	private static int KEY_ESCAPE = 7;
	private static int KEY_C = 8;
	private static int KEY_S = 9;
	private static int KEY_L = 10;
	private static int KEY_H = 11;
	private static int KEY_GAMEPAD_UP = 12;
	private static int KEY_GAMEPAD_DOWN = 13;
	private static int KEY_GAMEPAD_LEFT = 14;
	private static int KEY_GAMEPAD_RIGHT = 15;
	private static int KEY_GAMEPAD_A = 16;
	private static int KEY_GAMEPAD_B = 17;
	private static int KEY_GAMEPAD_X = 18;
	private static int KEY_GAMEPAD_Y = 19;
	private static int KEY_GAMEPAD_L = 20;
	private static int KEY_GAMEPAD_R = 21;

    //
    // Called when the app instance is created.
    //
    @Override
    @SuppressWarnings("deprecation")
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        isFinished = false;

        // Do full screen settings. (step1)
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        // Create the main view.
        view = new MainView(this);
        setContentView(view);

        // Do full screen settings. (step2)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            final WindowInsetsController controller = getWindow().getInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.systemBars());
                controller.setSystemBarsBehavior(
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
                );
            }
        } else {
            getWindow().setFlags(
                    WindowManager.LayoutParams.FLAG_FULLSCREEN,
                    WindowManager.LayoutParams.FLAG_FULLSCREEN
            );
        }

        // Prepare the video view.
        videoView = new VideoSurfaceView(this);
        videoLayout = new LinearLayout(this);
        videoLayout.setGravity(Gravity.CENTER);
        videoLayout.setBackgroundColor(Color.BLACK);
        videoLayout.addView(videoView);
        Thread videoThread = new Thread(videoView);
        videoThread.start();
    }

    //
    // Called when the app instance is paused.
    //
    @Override
    public void onPause() {
        super.onPause();
        nativeOnPause();
    }

    //
    // Called when the app instance is resumed.
    //
    @Override
    public void onResume() {
        super.onResume();
        nativeOnResume();
    }

    //
    // Called when the app instance is destroyed.
    //
    @Override
    public void onDestroy() {
        super.onDestroy();

        if(!isFinished) {
            synchronized (syncObj) {
                nativeCleanup();
            }
            isFinished = true;
        }
    }

    //
    // View class
    //
    private class MainView extends GLSurfaceView implements View.OnTouchListener, Renderer {
        //
        // Constructor of the view class
        //
        public MainView(Context context) {
            super(context);

            setFocusable(true);
			setFocusableInTouchMode(true);
			requestFocus();
            setOnTouchListener(this);
            setEGLConfigChooser(8, 8, 8, 8, 0, 0);
            setEGLContextClientVersion(2);
            setRenderer(this);
        }

        //
        // Called when the view is created.
        //
        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            if(!resumeFromVideo) {
                synchronized (syncObj) {
                    nativeInitGame();
                    VIEWPORT_WIDTH = nativeGetScreenWidth();
                    VIEWPORT_HEIGHT = nativeGetScreenHeight();
                    isLoaded = true;
                }
            } else {
                resumeFromVideo = false;
                synchronized (syncObj) {
                    nativeReinitOpenGL();
                }
            }
        }

        //
        // Called when initialized or the screen size is changed.
        //
        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            // Get the aspect ratio.
            float aspect = (float)VIEWPORT_HEIGHT / (float)VIEWPORT_WIDTH;

            // Width-first.
            float w = width;
            float h = width * aspect;
            scale = w / (float)VIEWPORT_WIDTH;
            offsetX = 0;
            offsetY = (int)((float)(height - h) / 2.0f);

            // Height-first.
            if(h > height) {
                h = height;
                w = height / aspect;
                scale = h / (float)VIEWPORT_HEIGHT;
                offsetX = (int)((float)(width - w) / 2.0f);
                offsetY = 0;
            }

            GLES20.glViewport(offsetX, offsetY, (int)w, (int)h);
        }

        //
        // Called when drawing to screen.
        //
        @Override
        public void onDrawFrame(GL10 gl) {
            boolean ret = true;
            synchronized(syncObj) {
                if(!isLoaded)
                    return;
                if(isFinished)
                    return;
                if(video != null)
                    return;
                ret = nativeRunFrame();
                if (!ret)
                    nativeCleanup();
            }
            if (!ret) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
                    finishAndRemoveTask();
                isFinished = true;
            }
        }

        //
        // Called when touched.
        //
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            int x = (int)((event.getX() - offsetX) / scale);
            int y = (int)((event.getY() - offsetY) / scale);
            int pointed = event.getPointerCount();
            int delta = y - touchLastY;

            synchronized(syncObj) {
                switch (event.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        nativeOnTouchStart(x, y, pointed);
                        break;
                    case MotionEvent.ACTION_MOVE:
                        nativeOnTouchMove(x, y);
                        break;
                    case MotionEvent.ACTION_UP:
                        nativeOnTouchEnd(x, y, touchCount);
                        break;
                }
            }

            touchCount = pointed;
            return true;
        }

		@Override
		public boolean onKeyDown(int keyCode, KeyEvent event) {
			switch (keyCode) {
			case KeyEvent.KEYCODE_BUTTON_A:
				nativeOnKeyDown(KEY_GAMEPAD_A);
				return true;
			case KeyEvent.KEYCODE_BUTTON_B:
				nativeOnKeyDown(KEY_GAMEPAD_B);
				return true;
			case KeyEvent.KEYCODE_BUTTON_X:
				nativeOnKeyDown(KEY_GAMEPAD_X);
				return true;
			case KeyEvent.KEYCODE_BUTTON_Y:
				nativeOnKeyDown(KEY_GAMEPAD_Y);
				return true;
			case KeyEvent.KEYCODE_BUTTON_L1:
				nativeOnKeyDown(KEY_GAMEPAD_L);
				return true;
			case KeyEvent.KEYCODE_BUTTON_R1:
				nativeOnKeyDown(KEY_GAMEPAD_R);
				return true;
			default:
				break;
			}
			return super.onKeyDown(keyCode, event);
		}

		@Override
		public boolean onKeyUp(int keyCode, KeyEvent event) {
			switch (keyCode) {
			case KeyEvent.KEYCODE_BUTTON_A:
				nativeOnKeyUp(KEY_GAMEPAD_A);
				return true;
			case KeyEvent.KEYCODE_BUTTON_B:
				nativeOnKeyUp(KEY_GAMEPAD_B);
				return true;
			case KeyEvent.KEYCODE_BUTTON_X:
				nativeOnKeyUp(KEY_GAMEPAD_X);
				return true;
			case KeyEvent.KEYCODE_BUTTON_Y:
				nativeOnKeyUp(KEY_GAMEPAD_Y);
				return true;
			case KeyEvent.KEYCODE_BUTTON_L1:
				nativeOnKeyUp(KEY_GAMEPAD_L);
				return true;
			case KeyEvent.KEYCODE_BUTTON_R1:
				nativeOnKeyUp(KEY_GAMEPAD_R);
				return true;
			default:
				break;
			}
			return super.onKeyDown(keyCode, event);
		}

		@Override
		public boolean onGenericMotionEvent(MotionEvent event) {
			if ((event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK &&
				event.getAction() == MotionEvent.ACTION_MOVE) {
				float x1 = event.getAxisValue(MotionEvent.AXIS_X);
				float y1 = event.getAxisValue(MotionEvent.AXIS_Y);
				float x2 = event.getAxisValue(MotionEvent.AXIS_RX);
				float y2 = event.getAxisValue(MotionEvent.AXIS_RY);
				float l = event.getAxisValue(MotionEvent.AXIS_LTRIGGER);
				float r = event.getAxisValue(MotionEvent.AXIS_RTRIGGER);
				nativeOnGamepadAnalog(x1, y2, x2, y2, l, r);

				float hatX = event.getAxisValue(MotionEvent.AXIS_HAT_X);
				float hatY = event.getAxisValue(MotionEvent.AXIS_HAT_Y);
				if (hatX == -1.0f) {
					nativeOnKeyDown(KEY_GAMEPAD_LEFT);
					nativeOnKeyUp(KEY_GAMEPAD_RIGHT);
				} else if (hatX == 1.0f) {
					nativeOnKeyDown(KEY_GAMEPAD_RIGHT);
					nativeOnKeyUp(KEY_GAMEPAD_LEFT);
				} else {
					nativeOnKeyUp(KEY_GAMEPAD_RIGHT);
					nativeOnKeyUp(KEY_GAMEPAD_LEFT);
				}
				if (hatY == -1.0f) {
					nativeOnKeyDown(KEY_GAMEPAD_UP);
					nativeOnKeyUp(KEY_GAMEPAD_DOWN);
				} else if (hatY == 1.0f) {
					nativeOnKeyDown(KEY_GAMEPAD_DOWN);
					nativeOnKeyUp(KEY_GAMEPAD_UP);
				} else {
					nativeOnKeyUp(KEY_GAMEPAD_UP);
					nativeOnKeyUp(KEY_GAMEPAD_DOWN);
				}

				return true;
			}
			return super.onGenericMotionEvent(event);
		}
    }

    //
    // Video rendering view class
    //
    class VideoSurfaceView extends SurfaceView implements SurfaceHolder.Callback, View.OnTouchListener, Runnable {
        //
        // Constructor
        //
        public VideoSurfaceView(Context context) {
            super(context);
            SurfaceHolder holder = getHolder();
            holder.addCallback(this);
            setOnTouchListener(this);
        }

        //
        // Called when created.
        //
        @Override
        public void surfaceCreated(SurfaceHolder paramSurfaceHolder) {
			synchronized (syncObj) {
				if(video != null) {
					SurfaceHolder holder = videoView.getHolder();
					video.setDisplay(holder);
					setWillNotDraw(false);
				}
			}
        }

        //
        // Called when initialized or the screen size is changed.
        //
        @Override
        public void surfaceChanged(SurfaceHolder paramSurfaceHolder, int paramInt1, int paramInt2, int paramInt3) {
        }

        //
        // Set video rendering size.
        //
        @Override
        public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            int width = getDefaultSize(VIEWPORT_WIDTH, widthMeasureSpec);
            int height = getDefaultSize(VIEWPORT_HEIGHT, heightMeasureSpec);
            if(VIEWPORT_WIDTH > VIEWPORT_HEIGHT)
                width = (int)(height * ((float)VIEWPORT_WIDTH / (float)VIEWPORT_HEIGHT));
            else
                height = (int)(width * ((float)VIEWPORT_HEIGHT / (float)VIEWPORT_WIDTH));
            setMeasuredDimension(width, height);
            setForegroundGravity(Gravity.CENTER);
        }

        //
        // Called when destroyed.
        //
        @Override
        public void surfaceDestroyed(SurfaceHolder paramSurfaceHolder) {
        }

        //
        // Called when render to the screen.
        //
        @Override
        public void onDraw(Canvas canvas) {
            boolean ret = true;
            synchronized (syncObj) {
                if(video == null)
                    return;
                if(!isLoaded)
                    return;
                ret = nativeRunFrame();
                if(!ret)
                    nativeCleanup();
            }
            if (!ret) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
                    finishAndRemoveTask();
                isFinished = true;
            }
        }

        //
        // Video thread.
        //
        public void run() {
            //noinspection InfiniteLoopStatement
            do {
                synchronized (syncObj) {
                    if (video != null) {
                        new Handler(Looper.getMainLooper()).post(() -> {
							synchronized (syncObj) {
								if (video != null)
									videoView.invalidate();
							}
                        });
                    }
                }
                try {
                    //noinspection BusyWait
                    Thread.sleep(33);
                } catch (InterruptedException ignored) {
                }
            } while (true);
        }

        //
        // Called when touched during video playback.
        //
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            synchronized(syncObj) {
                nativeOnTouchStart(0, 0, 1);
                nativeOnTouchEnd(0, 0, 1);
            }
            return true;
        }
    }

    //
    // Helpers for JNI code
    //  - We name methods that are called from JNI code "bridge*()"
    //

    //
    // Start video playback.
    //
    private void bridgePlayVideo(String fileName, boolean isSkippable) {
		synchronized(syncObj) {
			if (video != null) {
				video.stop();
				video = null;
			}
			try {
				AssetFileDescriptor afd = getAssets().openFd("mov/" + fileName);
				video = new MediaPlayer();
				video.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
				video.prepare();
				new Handler(Looper.getMainLooper()).post(() -> {
					synchronized(syncObj) {
						view.setRenderMode(RENDERMODE_WHEN_DIRTY);
						setContentView(videoLayout);
						videoView.measure(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
						video.start();
					}
				});
			} catch(IOException e) {
				Log.e(APP_NAME, "Failed to play video " + fileName);
			}
		}
    }

    //
    // Stop video playback.
    //
    private void bridgeStopVideo() {
		synchronized(syncObj) {
			if(video != null) {
				video.stop();
				video.reset();
				video.release();
				video = null;
				new Handler(Looper.getMainLooper()).post(() -> {
						resumeFromVideo = true;
						setContentView(view);
						view.setRenderMode(RENDERMODE_CONTINUOUSLY);
				});
			}
		}
    }

    //
    // Return whether video is playing.
    //  - If a video playback was finished, this method returns false.
    //
    private boolean bridgeIsVideoPlaying() {
        synchronized (syncObj) {
            if (video != null) {
                int pos = video.getCurrentPosition();
                if (pos == 0)
                    return true;
                if (video.isPlaying())
                    return true;
                video.stop();
                video = null;
                new Handler(Looper.getMainLooper()).post(() -> {
                    resumeFromVideo = true;
                    setContentView(view);
                    view.setRenderMode(RENDERMODE_CONTINUOUSLY);
                });
            }
            return false;
        }
    }

    //
    // Check if an asset file exists.
    //
    private boolean bridgeCheckFileExists(String fileName) {
        byte[] buf = null;
        try {
            InputStream is = getResources().getAssets().open(fileName);
            is.close();
        } catch(IOException e) {
            return false;
        }
        return true;
    }

    //
    // Get an entire file content.
    //
    private byte[] bridgeGetFileContent(String fileName) {
        if(fileName.startsWith("sav/"))
            return getSaveFileContent(fileName.split("/")[1]);
        else
            return getAssetFileContent(fileName);
    }

    //
    // Get a save file content.
    //
    private byte[] getSaveFileContent(String fileName) {
        byte[] buf = null;
        try {
            FileInputStream fis = openFileInput(fileName);
            buf = new byte[fis.available()];
            //noinspection ResultOfMethodCallIgnored
            fis.read(buf);
            fis.close();
        } catch(IOException ignored) {
        }
        return buf;
    }

    //
    // Get an asset file content.
    //
    private byte[] getAssetFileContent(String fileName) {
        byte[] buf = null;
        try {
            InputStream is = getResources().getAssets().open(fileName);
            buf = new byte[is.available()];
            //noinspection ResultOfMethodCallIgnored
            is.read(buf);
            is.close();
        } catch(IOException e) {
            Log.e(APP_NAME, "Failed to read file " + fileName);
        }
        return buf;
    }

    //
    // Remove a save file.
    //
    private void bridgeRemoveSaveFile(String fileName) {
        File file = new File(fileName);
        //noinspection ResultOfMethodCallIgnored
        file.delete();
    }

    //
    // Open a save file for subsequent writes.
    //
    private OutputStream bridgeOpenSaveFile(String fileName) {
        try {
            return openFileOutput(fileName, 0);
        } catch(IOException e) {
            Log.e(APP_NAME, "Failed to write file " + fileName);
        }
        return null;
    }

    //
    // Write a byte to the opened save file.
    //
    private boolean bridgeWriteSaveFile(OutputStream os, int b) {
        try {
            os.write(b);
            return true;
        } catch(IOException e) {
            Log.e(APP_NAME, "Failed to write file.");
        }
        return false;
    }

    //
    // Close the save file.
    //
    private void bridgeCloseSaveFile(OutputStream os) {
        try {
            os.close();
        } catch(IOException e) {
            Log.e(APP_NAME, "Failed to write file.");
        }
    }
}
