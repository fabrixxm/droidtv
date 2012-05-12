MODULE_PATH := $(call my-dir)

#include $(MODULE_PATH)/../config-$(TARGET_ARCH).mak
include $(MODULE_PATH)/../config.mak

OBJS =
OBJS-yes =
SRC_PATH = $(MODULE_PATH)
include $(MODULE_PATH)/Makefile

# collect objects
OBJS += $(OBJS-yes)

ALL_S_FILES := $(wildcard $(MODULE_PATH)/$(TARGET_ARCH)/*.S)
ALL_S_FILES := $(addprefix $(TARGET_ARCH)/, $(notdir $(ALL_S_FILES)))

ifneq ($(ALL_S_FILES),)
	ALL_S_OBJS := $(patsubst %.S,%.o,$(ALL_S_FILES))
	C_OBJS := $(filter-out $(ALL_S_OBJS),$(OBJS))
	S_OBJS := $(filter $(ALL_S_OBJS),$(OBJS))
else
	C_OBJS := $(OBJS)
	S_OBJS :=
endif

C_FILES := $(patsubst %.o,%.c,$(C_OBJS))
S_FILES := $(patsubst %.o,%.S,$(S_OBJS))

FFFILES += $(addprefix $(notdir $(MODULE_PATH))/, $(sort $(S_FILES)) $(sort $(C_FILES)))
FFINCLUDES += $(MODULE_PATH)