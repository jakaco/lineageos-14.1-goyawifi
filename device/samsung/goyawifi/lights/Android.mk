# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := lights.default
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := goya-lights.c
LOCAL_CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter
LOCAL_SHARED_LIBRARIES := liblog
include $(BUILD_SHARED_LIBRARY)
