# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := goyawifi_binder_probe
LOCAL_SRC_FILES := binder_probe.cpp
LOCAL_SHARED_LIBRARIES := libbinder libutils liblog
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)
LOCAL_MODULE := goyawifi_graphics_probe
LOCAL_SRC_FILES := graphics_probe.cpp
LOCAL_SHARED_LIBRARIES := libEGL libGLESv2 libhardware libdl liblog libui libutils
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)
LOCAL_MODULE := goyawifi_surface_probe
LOCAL_SRC_FILES := surface_probe.cpp
LOCAL_SHARED_LIBRARIES := libgui libui libbinder libutils libEGL libGLESv2 liblog
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
