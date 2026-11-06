# SPDX-License-Identifier: Apache-2.0
# Samsung Galaxy Tab 3 Lite 7.0 Wi-Fi (SM-T110, goyawifi), Marvell PXA986

TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a9
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_BOARD_PLATFORM := mrvl
TARGET_BOOTLOADER_BOARD_NAME := PXA986
TARGET_NO_BOOTLOADER := true

# The kernel and boot image are built by scripts/build-kernel.sh and packed by
# scripts/assemble.py, not by the platform build.
TARGET_NO_KERNEL := true
TARGET_PREBUILT_KERNEL := device/samsung/goyawifi-init/kernel-reference
KERNEL_HAS_FINIT_MODULE := false

# Partitions
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16777216
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1594884096
BOARD_USERDATAIMAGE_PARTITION_SIZE := 5368709120
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_CACHEIMAGE_PARTITION_SIZE := 209715200
BOARD_FLASH_BLOCK_SIZE := 131072
TARGET_RECOVERY_FSTAB := device/samsung/goyawifi/fstab.goyawifi

# SELinux stays disabled on the 3.4 vendor kernel.
GOYAWIFI_INIT_BRINGUP_NO_SELINUX := true

# Dex stays in jars; the ART boot image is built separately so SYSTEM fits.
WITH_DEXPREOPT := false

# Graphics: Vivante GC1000 with the Android 4.2 stock blobs
# Stock libGAL.so needs text relocations.
GOYAWIFI_LEGACY_VIVANTE_TEXTRELS := true
# Stock Marvell HWC1 layer ABI.
GOYAWIFI_HWC_LAYER_STRIDE_100 := true
# fb0 is BGRA.
BOARD_USE_BGRA_8888 := true

# Wi-Fi: Marvell SD8777 on cfg80211
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WIFI_DRIVER_STATE_CTRL_PARAM := /sys/class/rfkill/rfkill0/state
WIFI_DRIVER_STATE_ON := 1
WIFI_DRIVER_STATE_OFF := 0
WIFI_DRIVER_OPERSTATE_PATH := /sys/class/net/wlan0/operstate
