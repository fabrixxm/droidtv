LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_STATIC_LIBRARIES := libavformat libavcodec libavfilter libswscale libavutil
LOCAL_MODULE := libav
include $(BUILD_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
