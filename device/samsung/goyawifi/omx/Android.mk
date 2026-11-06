# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libstagefrighthw
LOCAL_SRC_FILES := goya-coda-omx.cpp
LOCAL_C_INCLUDES := frameworks/native/include/media/openmax
LOCAL_CPPFLAGS := -std=c++11
LOCAL_SHARED_LIBRARIES := libdl libutils liblog
include $(BUILD_SHARED_LIBRARY)
