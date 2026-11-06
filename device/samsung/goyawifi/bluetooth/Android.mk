# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libbt-vendor
LOCAL_SRC_FILES := bluetooth-vendor.c
LOCAL_C_INCLUDES := system/bt
LOCAL_SHARED_LIBRARIES := liblog libcutils
include $(BUILD_SHARED_LIBRARY)
