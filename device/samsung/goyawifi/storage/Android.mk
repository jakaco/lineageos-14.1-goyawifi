# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := goya_storage_setup
LOCAL_SRC_FILES := storage-setup.c
include $(BUILD_EXECUTABLE)
