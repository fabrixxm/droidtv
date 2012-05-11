#include <jni.h>

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#include "MediaPlayer.h"
#include "utils.h"

#include "com_chrulri_droidtv_player_AVPlayer.h"

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
	jmethodID postPrepareVideo;
	jmethodID postPrepareAudio;
	jmethodID postNotify;
};
static fields_t fields;

JavaVM* getJVM() {
	return sVm;
}

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

	fields.postPrepareVideo = env->GetMethodID(clazz, "postPrepareVideo",
			"(II)Landroid/graphics/Bitmap;");
	if (fields.postPrepareVideo == NULL) {
		LOGE("Can't find AVPlayer.postPrepareVideo");
		return JNI_ERR;
	}

	fields.postPrepareAudio = env->GetMethodID(clazz, "postPrepareAudio",
			"(I)Z");
	if (fields.postPrepareAudio == NULL) {
		LOGE("Can't find AVPlayer.postPrepareAudio");
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
	jint postAudio(int16_t* buffer, int size);
	void postVideo();
	jobject postPrepareVideo(int width, int height);
	jboolean postPrepareAudio(int sampleRate);
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

jint AVPlayerListener::postAudio(int16_t* buffer, int size) {
	JNIEnv *env = getJNIEnv();
	jshortArray jBuffer = env->NewShortArray(size);
	env->SetShortArrayRegion(jBuffer, 0, size, buffer);
	return env->CallIntMethod(mPlayer, fields.postAudio, jBuffer, size);
}

void AVPlayerListener::postVideo() {
	JNIEnv *env = getJNIEnv();
	env->CallVoidMethod(mPlayer, fields.postVideo);
}

jobject AVPlayerListener::postPrepareVideo(int width, int height) {
	JNIEnv *env = getJNIEnv();
	return env->CallObjectMethod(mPlayer, fields.postPrepareVideo, width, height);
}

jboolean AVPlayerListener::postPrepareAudio(int sampleRate) {
	JNIEnv *env = getJNIEnv();
	return env->CallBooleanMethod(mPlayer, fields.postPrepareAudio, sampleRate);
}

void AVPlayerListener::postNotify(int msg, int ext1, int ext2) {
	JNIEnv *env = getJNIEnv();
	env->CallVoidMethod(mPlayer, fields.postNotify, msg, ext1, ext2);
}

static MediaPlayer* getNativeContext(JNIEnv* env, jobject thiz) {
	return (MediaPlayer*) env->GetIntField(thiz, fields.context);
}

static void freeNativeContext(JNIEnv* env, jobject thiz) {
	MediaPlayer* old = getNativeContext(env, thiz);
	if (old != NULL) {
		LOGI("freeing old media player object");
		delete old;
	}
}

static void setNativeContext(JNIEnv* env, jobject thiz,
		MediaPlayer* player) {
	freeNativeContext(env, thiz);
	env->SetIntField(thiz, fields.context, (jint) player);
}

JNIEXPORT void JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1initialize(
		JNIEnv *env, jobject thiz, jobject weakRef) {
	LOGI("_initialize");
	MediaPlayer* mp = new MediaPlayer();
	AVPlayerListener * listener = new AVPlayerListener(env, thiz, weakRef);
	mp->setListener(listener);
	setNativeContext(env, thiz, mp);
}

JNIEXPORT void JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1finalize(
		JNIEnv *env, jobject thiz) {
	freeNativeContext(env, thiz);
}

JNIEXPORT jint JNICALL Java_com_chrulri_droidtv_player_AVPlayer__1prepare(
		JNIEnv *env, jobject thiz, jstring jFileName) {
	LOGI("_prepare");
	int ret;

	const char* fileName = env->GetStringUTFChars(jFileName, NULL);

	MediaPlayer* mp = getNativeContext(env, thiz);
	ret = mp->prepare(env, fileName);

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
	LOGD("AVLib[%s]:",
			level == AV_LOG_PANIC ? "PANIC" :
			level == AV_LOG_FATAL ? "FATAL" :
			level == AV_LOG_ERROR ? "ERROR" :
			level == AV_LOG_WARNING ? "WARNING" :
			level == AV_LOG_INFO ? "INFO" :
			level == AV_LOG_VERBOSE ? "VERBOSE" :
			level == AV_LOG_DEBUG ? "DEBUG" :
			"UNKNOWN");
	LOGDAV(fmt, vl);
}
