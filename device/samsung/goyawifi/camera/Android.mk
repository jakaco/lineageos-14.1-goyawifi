# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := camera.mrvl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := camera-hal.cpp
LOCAL_C_INCLUDES := system/media/camera/include external/libyuv/files/include external/libjpeg-turbo
LOCAL_CPPFLAGS := -std=c++11
LOCAL_SHARED_LIBRARIES := libutils libcutils liblog libbinder libui libcamera_client libjpeg libc++
LOCAL_STATIC_LIBRARIES := libyuv_static
include $(BUILD_SHARED_LIBRARY)
