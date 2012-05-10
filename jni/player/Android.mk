LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE 		:= avplayer
LOCAL_SRC_FILES 	:= avplayer.cpp mediaplayer.cpp
LOCAL_LDLIBS   		:= -llog -ljnigraphics -lz
LOCAL_CFLAGS		:= -D__STDC_CONSTANT_MACROS
#LOCAL_CFLAGS		+= -DDEBUG
LOCAL_C_INCLUDES	:= $(LOCAL_PATH)/../libav
LOCAL_STATIC_LIBRARIES := libav
include $(BUILD_SHARED_LIBRARY)