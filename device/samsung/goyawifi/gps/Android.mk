# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := gps.default
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := goya-gps-hal-compat.c
LOCAL_CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter
LOCAL_SHARED_LIBRARIES := libhardware liblog libdl
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
include $(BUILD_SHARED_LIBRARY)

