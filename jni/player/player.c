#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <errno.h>
#include <stdlib.h>

#include <libavcodec/avcodec.h>

#define UNUSED  __attribute__((unused))

#define SET_BIT(var,pos)	((var) |=  (1<<(pos)))
#define CLEAR_BIT(var,pos)	((var) &= ~(1<<(pos)))
#define CHECK_BIT(var,pos)	((var) &   (1<<(pos)))

#define LOG_TAG "DroidTvPlayerNative"
#define LOGE(...)	__android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#ifdef DEBUG
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#else
#define LOGD(...)	((void)0)
#endif
/* *** INITIALIZATION *** */
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	LOGD("JNI_OnLoad(..) called");

	av_register_all();

	return JNI_VERSION_1_6;
}
