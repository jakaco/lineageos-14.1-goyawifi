# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PACKAGE_NAME := GoyaCamera
LOCAL_SRC_FILES := CameraActivity.java
LOCAL_SDK_VERSION := 25
LOCAL_JAVA_LANGUAGE_VERSION := 1.8
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_MODULE_TAGS := optional
include $(BUILD_PACKAGE)
