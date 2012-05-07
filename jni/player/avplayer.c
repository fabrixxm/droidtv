#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <errno.h>
#include <stdlib.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include "utils.h"

static void avlibNotify(void* ptr, int level, const char* fmt, va_list vl);

/* *** INITIALIZATION *** */
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	LOGD("JNI_OnLoad(..) called");

	av_register_all();
	av_log_set_callback(avlibNotify);

	return JNI_VERSION_1_6;
}

static JNIEnv*				sEnv;
static jobject				sPlayer;
static jmethodID			mCallbackAudio;
static jmethodID			mCallbackVideo;
static jmethodID			mCallbackPrepareBitmap;

static AVFormatContext*		sFile;
static int					sCurrentState;
static int					sAudioStreamIndex;
static int					sVideoStreamIndex;
static AVFrame*				sFrame;
static struct SwsContext*	sCtxConvert;
static jobject				sBitmap;
static AndroidBitmapInfo	sBitmapInfo;

JNIEXPORT jint JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1prepare
  (JNIEnv *env, jobject jPlayer, jstring jFileName) {
	sCurrentState = com_chrulri_droidtv_player_AVPlayer_STATE_PREPARING;
    int ret, i;

    jclass clazz = (*env)->GetObjectClass(env, jPlayer);
    mCallbackAudio = (*env)->GetMethodID(env, clazz, "callbackAudio", "([SI)I");
	mCallbackVideo = (*env)->GetMethodID(env, clazz, "callbackVideo", "()V");
	mCallbackPrepareBitmap = (*env)->GetMethodID(env, clazz, "callbackPrepareBitmap", "(II)Landroid/graphics/Bitmap;");
    sPlayer = jPlayer;
    sEnv = env;

	const char* fileName = (*env)->GetStringUTFChars(env, jFileName, NULL);

    // open file
	if ((ret = avformat_open_input(&sFile, fileName, NULL, NULL)) != 0) {
		return ret;
	}
	
	(*env)->ReleaseStringUTFChars(env, jFileName, fileName);
	
	// get stream information
	if ((ret = avformat_find_stream_info(sFile, NULL)) < 0) {
		return ret;
	}
	
	AVStream* stream;
	AVCodecContext* codec_ctx;
	AVCodec* codec;
	
	// prepare audio
	sAudioStreamIndex = -1;
	for (i = 0; i < sFile->nb_streams; i++) {
		if (sFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			sAudioStreamIndex = i;
			break;
		}
	}
	
	if (sAudioStreamIndex == -1) {
		return -1;
	}

	stream = sFile->streams[sAudioStreamIndex];
	codec_ctx = stream->codec;
	codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return -1;
	}

	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		return -1;
	}
	
	// prepare video
	sVideoStreamIndex = -1;
	for (i = 0; i < sFile->nb_streams; i++) {
		if (sFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			sVideoStreamIndex = i;
			break;
		}
	}
	
	if (sVideoStreamIndex == -1) {
		return -1;
	}
	
	stream = sFile->streams[sVideoStreamIndex];
	codec_ctx = stream->codec;
	codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return -1;
	}
	
	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		return -1;
	}
	
	int videoWidth = codec_ctx->width;
	int videoHeight = codec_ctx->height;
	//sDuration =  sFile->duration;
	
	sCtxConvert = sws_getContext(stream->codec->width,
								 stream->codec->height,
								 stream->codec->pix_fmt,
								 stream->codec->width,
								 stream->codec->height,
								 PIX_FMT_RGB565,
								 SWS_POINT,
								 NULL,
								 NULL,
								 NULL);

	if (sCtxConvert == NULL) {
		return -1;
	}

	sFrame = avcodec_alloc_frame();
	if (sFrame == NULL) {
		return -1;
	}
	
	sBitmap = (*env)->CallObjectMethod(env, jPlayer, mCallbackPrepareBitmap, videoWidth, videoHeight);
	
	if ((ret = AndroidBitmap_getInfo(env, sBitmap, &sBitmapInfo))) {
		LOGE("AndroidBitmap_getInfo(..) failed: 0x%x", ret);
		return ret;
	}
	
	if(sBitmapInfo.format != ANDROID_BITMAP_FORMAT_RGB_565) {
		LOGE("Bitmap format is not RGB_565!");
		return -1;
	}
	
	sCurrentState = com_chrulri_droidtv_player_AVPlayer_STATE_PREPARED;
    return 0;
}

JNIEXPORT jint JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1start
  (JNIEnv *env, jobject player) {
  // TODO implement
  return -1;
}

JNIEXPORT jint JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1stop
  (JNIEnv *env, jobject player) {
  // TODO implement
  return -1;
}

JNIEXPORT jint JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1getState
  (JNIEnv *env UNUSED, jobject player UNUSED) {
  return sCurrentState;
}

static void redraw(AVFrame* frame) {
	int ret;
	void* pixels;

	if ((ret = AndroidBitmap_lockPixels(sEnv, sBitmap, &pixels))) {
		LOGE("AndroidBitmap_lockPixels(..) failed: 0x%x", ret);
		return;
	}
	
	avpicture_fill((AVPicture *) sFrame,
				   (uint8_t *) pixels,
				   PIX_FMT_RGB565,
				   sBitmapInfo.width,
				   sBitmapInfo.height);
	
	sws_scale(sCtxConvert,
		      frame->data,
		      frame->linesize,
			  0,
			  sBitmapInfo.height,
			  sFrame->data,
			  sFrame->linesize);
	
	if ((ret = AndroidBitmap_unlockPixels(sEnv, sBitmap))) {
		LOGE("AndroidBitmap_unlockPixels(..) failed: 0x%x", ret);
		return;
	}
	
	(*sEnv)->CallVoidMethod(sEnv, sPlayer, mCallbackVideo);
}

static void avlibNotify(void* ptr, int level, const char* fmt, va_list vl) {
	// TODO implement
}
