#ifndef DROIDTV_UTILS_H
#define DROIDTV_UTILS_H

#include <jni.h>
#include <android/log.h>

#define LOG_TAG "AVPlayerNative"
#define LOGE(...)	__android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#ifdef DEBUG
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGDAV(...)	__android_log_vprint(ANDROID_LOG_DEBUG,LOG_TAG "::avlib",__VA_ARGS__)
#define LOGI(...)	__android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define LOGD(...)	((void)0)
#define LOGDAV(...)	((void)0)
#define LOGI(...)	((void)0)
#endif

JavaVM* getJVM();

#endif // DROIDTV_UTILS_H
