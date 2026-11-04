# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libshim_goyawifi_binder
LOCAL_SRC_FILES := legacy_binder.c
LOCAL_SHARED_LIBRARIES := libbinder
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall -Wextra -Werror
include $(BUILD_SHARED_LIBRARY)
