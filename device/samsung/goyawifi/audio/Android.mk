# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := goya_audio_route
LOCAL_SRC_FILES := audio-route.c
LOCAL_C_INCLUDES := external/tinyalsa/include
LOCAL_SHARED_LIBRARIES := libtinyalsa
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)
LOCAL_MODULE := audio.primary.mrvl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := audio-hal.c
LOCAL_C_INCLUDES := external/tinyalsa/include
LOCAL_SHARED_LIBRARIES := libtinyalsa liblog libcutils
LOCAL_CFLAGS := -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-unused-label
include $(BUILD_SHARED_LIBRARY)
