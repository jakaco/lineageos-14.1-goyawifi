# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := $(call my-dir)

ifeq ($(WPA_SUPPLICANT_VERSION),VER_0_8_X)
WPA_SUPPL_DIR := external/wpa_supplicant_8
include $(WPA_SUPPL_DIR)/wpa_supplicant/android.config

include $(CLEAR_VARS)
LOCAL_MODULE := lib_driver_cmd_goyawifi
LOCAL_SRC_FILES := driver_cmd_goyawifi.c
LOCAL_C_INCLUDES := \
    $(WPA_SUPPL_DIR)/src \
    $(WPA_SUPPL_DIR)/src/common \
    $(WPA_SUPPL_DIR)/src/drivers \
    $(WPA_SUPPL_DIR)/src/l2_packet \
    $(WPA_SUPPL_DIR)/src/utils \
    $(WPA_SUPPL_DIR)/src/wps \
    $(WPA_SUPPL_DIR)/wpa_supplicant \
    external/libnl/include
LOCAL_CFLAGS := -DANDROID_P2P -DCONFIG_DRIVER_NL80211 -Wno-unused-parameter
ifdef CONFIG_ANDROID_LOG
LOCAL_CFLAGS += -DCONFIG_ANDROID_LOG
endif
ifdef CONFIG_P2P
LOCAL_CFLAGS += -DCONFIG_P2P
endif
LOCAL_SHARED_LIBRARIES := libc libcutils
include $(BUILD_STATIC_LIBRARY)
endif
