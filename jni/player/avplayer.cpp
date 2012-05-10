#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
} // end of extern C

#include "com_chrulri_droidtv_player_AVPlayer.h"
#include "mediaplayer.h"

#define UNUSED  __attribute__((unused))

#define LOG_TAG "AVPlayerNative"
#define LOGE(...)	__android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#ifdef DEBUG
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...)	__android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define LOGD(...)	((void)0)
#define LOGI(...)	((void)0)
#endif

#define CLASS_OUT_OF_MEMORY_ERROR	"java/lang/OutOfMemoryError"
#define CLASS_RUNTIME_EXCEPTION		"java/lang/RuntimeException"
#define CLASS_AVPLAYER				"com/chrulri/droidtv/player/AVPlayer"

static void avlibNotify(void* ptr, int level, const char* fmt, va_list vl);

static JavaVM* sVm;
struct fields_t {
	jclass clazz;
	jfieldID context;
	jmethodID postAudio;
	jmethodID postVideo;
	jmethodID postPrepare;
	jmethodID postNotify;
};
static fields_t fields;

JNIEnv* getJNIEnv() {
	JNIEnv* env;
	if (sVm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOGE("Failed to obtain JNIEnv");
		return NULL;
	}
	return env;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	LOGI("JNI_OnLoad(..) called");
	sVm = vm;

	av_register_all();
	av_log_set_callback(avlibNotify);

	JNIEnv* env = getJNIEnv();

	jclass clazz = env->FindClass(CLASS_AVPLAYER);
	if (clazz == NULL) {
		LOGE("Can't find AVPlayer class");
		return JNI_ERR;
	}
	fields.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(clazz));

	fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
	if (fields.context == NULL) {
		LOGE("Can't find AVPlayer.mNativeContext");
		return JNI_ERR;
	}

	fields.postAudio = env->GetMethodID(clazz, "postAudio", "([SI)I");
	if (fields.postAudio == NULL) {
		LOGE("Can't find AVPlayer.postAudio");
		return JNI_ERR;
	}

	fields.postVideo = env->GetMethodID(clazz, "postVideo", "()V");
	if (fields.postVideo == NULL) {
		LOGE("Can't find AVPlayer.postVideo");
		return JNI_ERR;
	}

	fields.postPrepare = env->GetMethodID(clazz, "postPrepare",
			"(II)Landroid/graphics/Bitmap;");
	if (fields.postPrepare == NULL) {
		LOGE("Can't find AVPlayer.postPrepare");
		return JNI_ERR;
	}

	fields.postNotify = env->GetMethodID(clazz, "postNotify", "(III)V");
	if (fields.postNotify == NULL) {
		LOGE("Can't find AVPlayer.postNotify");
		return JNI_ERR;
	}

	return JNI_VERSION_1_6;
}

void jniThrowException(JNIEnv* env, const char* className, const char* msg) {
	jclass exceptionClass = env->FindClass(className);
	if (exceptionClass == NULL) {
		LOGE("Unable to find exception class %s", className);
		return;
	}

	if (env->ThrowNew(exceptionClass, msg) != JNI_OK) {
		LOGE("Failed throwing '%s' '%s'", className, msg);
	}
}

class AVPlayerListener: public MediaPlayerListener {
public:
	AVPlayerListener(JNIEnv* env, jobject thiz, jobject weakRef);
	~AVPlayerListener();
	void postAudio(int16_t* buffer, int size);
	void postVideo();
	void postPrepare(int width, int height);
	void postNotify(int msg, int ext1, int ext2);
private:
	AVPlayerListener();
	jobject mPlayer;
};

AVPlayerListener::AVPlayerListener(JNIEnv* env, jobject thiz, jobject weakRef) {
	mPlayer = env->NewGlobalRef(weakRef);
}

AVPlayerListener::~AVPlayerListener() {
	JNIEnv *env = getJNIEnv();
	env->DeleteGlobalRef(mPlayer);
}

void AVPlayerListener::postAudio(int16_t* buffer, int size) {
	JNIEnv *env = getJNIEnv();
	jshortArray jBuffer = env->NewShortArray(size);
	env->SetShortArrayRegion(jBuffer, 0, size, buffer);
	env->CallVoidMethod(mPlayer, fields.postAudio, jBuffer, size);
}

void AVPlayerListener::postVideo() {
	JNIEnv *env = getJNIEnv();
	env->CallVoidMethod(mPlayer, fields.postVideo);
}

void AVPlayerListener::postPrepare(int width, int height) {
	JNIEnv *env = getJNIEnv();
	env->CallVoidMethod(mPlayer, fields.postPrepare, width, height);
}

void AVPlayerListener::postNotify(int msg, int ext1, int ext2) {
	JNIEnv *env = getJNIEnv();
	env->CallVoidMethod(mPlayer, fields.postNotify, msg, ext1, ext2);
}

static MediaPlayer* getNativeContext(JNIEnv* env, jobject thiz) {
	return (MediaPlayer*) env->GetIntField(thiz, fields.context);
}

static MediaPlayer* setNativeContext(JNIEnv* env, jobject thiz,
		MediaPlayer* player) {
	MediaPlayer* old = getNativeContext(env, thiz);
	if (old != NULL) {
		LOGI("freeing old media player object");
		delete (old);
	}
	env->SetIntField(thiz, fields.context, (jint) player);
	return old;
}

JNIEXPORT void JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1initialize(
		JNIEnv *env, jobject thiz, jobject weakRef) {
	LOGI("_initialize");
	MediaPlayer* mp = new MediaPlayer();
	AVPlayerListener * listener = new AVPlayerListener(env, thiz, weakRef);
	mp->setListener(listener);
	setNativeContext(env, thiz, mp);
}

JNIEXPORT jint JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1prepare(
		JNIEnv *env, jobject thiz, jstring jFileName) {
	LOGI("_prepare");
	int ret;

	const char* fileName = env->GetStringUTFChars(jFileName, NULL);

	MediaPlayer* mp = getNativeContext(env, thiz);
	ret = mp->prepare(fileName);

	env->ReleaseStringUTFChars(jFileName, fileName);
	return ret;
}

JNIEXPORT jint JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1start(
		JNIEnv *env, jobject thiz) {
	MediaPlayer* mp = getNativeContext(env, thiz);
	return mp->start();
}

JNIEXPORT jint JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1stop(
		JNIEnv *env, jobject thiz) {
	MediaPlayer* mp = getNativeContext(env, thiz);
	return mp->stop();
}

JNIEXPORT jint JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1getState(
		JNIEnv *env, jobject thiz) {
	MediaPlayer* mp = getNativeContext(env, thiz);
	return mp->getState();
}

static void avlibNotify(void* ptr, int level, const char* fmt, va_list vl) {
	LOGD("AVLib[%d]: %s", level, fmt);
}
