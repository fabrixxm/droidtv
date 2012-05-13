LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
#LOCAL_MODULE		:= libav_vfpv3
LOCAL_MODULE		:= libav
LOCAL_ARM_MODE		:= arm
LOCAL_LDLIBS		:= -lz

LOCAL_CFLAGS		:= -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 
LOCAL_CFLAGS		+= -DHAVE_AV_CONFIG_H -Wno-sign-compare -Wno-switch -Wno-pointer-sign
LOCAL_CFLAGS		+= -DTARGET_CONFIG=\"config-$(TARGET_ARCH).h\" -Wno-long-long
LOCAL_LDFLAGS		:= -Wl,--fix-cortex-a8
#LOCAL_LDFLAGS 		+= -Wl,-soname,libav.so

FFFILES :=
FFINCLUDES :=
include $(call all-makefiles-under,$(LOCAL_PATH))

LOCAL_SRC_FILES		:= $(FFFILES)
LOCAL_C_INCLUDES	:= $(FFINCLUDES) $(LOCAL_PATH)
LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)


## NEON
#include $(CLEAR_VARS)
#LOCAL_MODULE		:= libav_neon
#LOCAL_ARM_MODE		:= arm
#LOCAL_LDLIBS		:= -lz
#
#LOCAL_CFLAGS		:= -march=armv7-a -mfloat-abi=softfp -mfpu=neon
#LOCAL_CFLAGS		+= -DHAVE_AV_CONFIG_H -Wno-sign-compare -Wno-switch -Wno-pointer-sign
#LOCAL_CFLAGS		+= -DTARGET_CONFIG=\"config-$(TARGET_ARCH).h\" -Wno-long-long
#LOCAL_LDFLAGS		:= -Wl,--fix-cortex-a8
#LOCAL_LDFLAGS 		+= -Wl,-soname,libav.so
#
#LOCAL_ARM_NEON		:= true
#HAVE_NEON			:= yes
#LOCAL_CFLAGS		+= -DNEON=1
#
#FFFILES :=
#FFINCLUDES :=
#include $(call all-makefiles-under,$(LOCAL_PATH))
#
#LOCAL_SRC_FILES		:= $(FFFILES)
#LOCAL_C_INCLUDES	:= $(LOCAL_PATH) $(FFINCLUDES)
#LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)
#include $(BUILD_SHARED_LIBRARY)
