LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE 		:= avplayer
LOCAL_SRC_FILES 	:= AVPlayer.cpp \
						MediaPlayer.cpp \
						Decoder.cpp \
						AudioDecoder.cpp \
						VideoDecoder.cpp 
LOCAL_LDLIBS   		:= -llog -ljnigraphics
LOCAL_CFLAGS		:= -D__STDC_CONSTANT_MACROS
LOCAL_CFLAGS		+= -DDEBUG
LOCAL_C_INCLUDES	:= $(LOCAL_PATH)/../libav
LOCAL_SHARED_LIBRARIES := libav
include $(BUILD_SHARED_LIBRARY)