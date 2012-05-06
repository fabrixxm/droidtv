LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := player
LOCAL_SRC_FILES := player.c
#LOCAL_CFLAGS	+= -DDEBUG
LOCAL_LDLIBS    := -llog -ljnigraphics -lz
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../libav
LOCAL_STATIC_LIBRARIES := libav
include $(BUILD_SHARED_LIBRARY)