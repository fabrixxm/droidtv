#ifndef DROIDTV_UTILS_H
#define DROIDTV_UTILS_H

#include <jni.h>
#include <android/log.h>

#define LOG_TAG "AVPlayerNative"
#define LOGE(TAG,...)	__android_log_print(ANDROID_LOG_ERROR,LOG_TAG "::" TAG,__VA_ARGS__)
#define LOGW(TAG,...)	__android_log_print(ANDROID_LOG_WARN,LOG_TAG "::" TAG,__VA_ARGS__)
#ifdef DEBUG
#define LOGD(TAG,...)	__android_log_print(ANDROID_LOG_DEBUG,LOG_TAG "::" TAG,__VA_ARGS__)
#define LOGDAV(...)		LOGD("LIBAV",__VA_ARGS__)
#define LOGI(TAG,...)	__android_log_print(ANDROID_LOG_INFO,LOG_TAG "::" TAG,__VA_ARGS__)
#else
#define LOGD(TAG,...)	((void)0)
#define LOGDAV(TAG,...)	((void)0)
#define LOGI(TAG,...)	((void)0)
#endif

JavaVM* getJVM();
JNIEnv* getJNIEnv();

#endif // DROIDTV_UTILS_H
