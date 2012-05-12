/******************************************************************************
 *  DroidTV, live TV on Android devices with host USB port and a DVB tuner    *
 *  Copyright (C) 2012  Christian Ulrich <chrulri@gmail.com>                  *
 *                                                                            *
 *  This program is free software: you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by      *
 *  the Free Software Foundation, either version 3 of the License, or         *
 *  (at your option) any later version.                                       *
 *                                                                            *
 *  This program is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *  GNU General Public License for more details.                              *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License         *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 ******************************************************************************/

package com.chrulri.droidtv.player;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Matrix.ScaleToFit;
import android.graphics.Paint;
import android.graphics.RectF;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;
import android.view.SurfaceHolder;

public final class AVPlayer {
	static final String TAG = AVPlayer.class.getSimpleName();

	public static final int STATE_UNKNOWN = 0;
	public static final int STATE_UNINITIALIZED = 1;
	public static final int STATE_PREPARING = 2;
	public static final int STATE_PREPARED = 3;
	public static final int STATE_PLAYING = 4;
	public static final int STATE_ERROR = 5;
	public static final int STATE_FINISHED = 6;
	public static final int STATE_STARTING = 7;
	public static final int STATE_STOPPING = 8;

	static {
		System.loadLibrary("av");
		System.loadLibrary("avplayer");
	}

	private int mNativeContext = 0;

	private AudioTrack mAudioTrack;
	private Bitmap mBitmap;
	private Matrix mMatrix = new Matrix();
	private RectF mBounds = new RectF();
	private RectF mSource = new RectF();

	private SurfaceHolder mSurface;
	private OnUpdateListener mListener;
	private String mFileName;

	public AVPlayer(SurfaceHolder surface, OnUpdateListener listener) {
		mSurface = surface;
		mListener = listener;

		mSurface.addCallback(new SurfaceHolder.Callback() {
			@Override
			public void surfaceDestroyed(SurfaceHolder holder) {
				// ignore
			}

			@Override
			public void surfaceCreated(SurfaceHolder holder) {
				performRender();
			}

			@Override
			public void surfaceChanged(SurfaceHolder holder, int format,
					int width, int height) {
				mBounds.set(0, 0, width, height);
				mMatrix.setRectToRect(mSource, mBounds, ScaleToFit.CENTER);
				performRender();
			}
		});

		_initialize();
	}

	@Override
	protected void finalize() throws Throwable {
		_finalize();
	}

	private native void _initialize();

	private native void _finalize();

	private native int _prepare(String fileName);

	private native int _start();

	private native int _stop();

	private native int _getState();

	private native void _drawFrame(Bitmap bitmap);

	private Bitmap postPrepareVideo(int width, int height) {
		Log.d(TAG, "postPrepareVideo");
		mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
		return mBitmap;
	}

	private boolean postPrepareAudio(int sampleRate) {
		Log.d(TAG, "postPrepareAudio");

		final int channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
		final int audioFormat = AudioFormat.ENCODING_PCM_16BIT;

		try {
			mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
					channelConfig, audioFormat, AudioTrack.getMinBufferSize(
							sampleRate, channelConfig, audioFormat),
					AudioTrack.MODE_STREAM);
		} catch (Throwable t) {
			Log.e(TAG, "postPrepareAudio", t);
			return false;
		}
		return true;
	}

	private int postAudio(short[] buffer, int bufsize) {
		// Log.d(TAG, "postAudio");
		return mAudioTrack.write(buffer, 0, bufsize);
	}

	private void postVideo() {
		// Log.d(TAG, "postVideo");
		performRender();
	}

	private void postNotify(int msg, int ext1, int ext2) {
		Log.d(TAG, "postNotify");
		// TODO implement
	}

	private void performRender() {
		if (mBitmap != null) {
			_drawFrame(mBitmap);
			Canvas c = mSurface.lockCanvas();
			if (c != null) {
				c.drawPaint(new Paint());
				c.drawBitmap(mBitmap, mMatrix, null);
				mSurface.unlockCanvasAndPost(c);
			}
		}
	}

	public void prepare(String fileName) throws IOException {
		if (fileName == null || !new File(fileName).canRead()) {
			throw new FileNotFoundException();
		}
		int ret = _prepare(fileName);
		if (ret != 0) {
			throw new IOException("prepare[" + ret + "]");
		}
	}

	public interface OnUpdateListener {
		// TODO implement
	}

	public void start() throws IOException {
		mAudioTrack.play();
		// TODO check if current state is valid
		int ret = _start();
		if (ret != 0) {
			throw new IOException("start[" + ret + "]");
		}
	}

	public void stop() throws IOException {
		mAudioTrack.stop();
		// TODO check if current state is valid
		int ret = _stop();
		if (ret != 0) {
			throw new IOException("stop[" + ret + "]");
		}
	}

	public boolean isPlaying() {
		return _getState() == STATE_PLAYING;
	}
}
