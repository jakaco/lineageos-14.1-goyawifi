# SPDX-License-Identifier: Apache-2.0
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, vendor/cm/config/common_full_tablet_wifionly.mk)
PRODUCT_NAME := lineage_goyawifi
PRODUCT_DEVICE := goyawifi-full
PRODUCT_BRAND := samsung
PRODUCT_MANUFACTURER := samsung
PRODUCT_MODEL := SM-T110
PRODUCT_BUILD_PROP_OVERRIDES += BUILD_FINGERPRINT=samsung/goyawifi/goyawifi:7.1.2/NJH47F/android7:user/test-keys
PRODUCT_CHARACTERISTICS := tablet
PRODUCT_AAPT_CONFIG := normal large xlarge mdpi hdpi
PRODUCT_AAPT_PREF_CONFIG := mdpi
PRODUCT_PACKAGES += SystemUI SettingsProvider CMSettingsProvider
PRODUCT_PACKAGES := $(filter-out Phone TeleService,$(PRODUCT_PACKAGES))
DEVICE_PACKAGE_OVERLAYS += device/samsung/goyawifi/overlay

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.secure=1 ro.debuggable=0 ro.adb.secure=1
# Match the exact SM-T110 stock display geometry and device-sized heap values.
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.hwrotation=270 ro.sf.lcd_density=160 ro.opengles.version=131072 \
    ro.bq.gpu_to_cpu_unsupported=1 \
    dalvik.vm.heapstartsize=8m dalvik.vm.heapgrowthlimit=96m dalvik.vm.heapsize=256m \
    dalvik.vm.heaptargetutilization=0.75 dalvik.vm.heapminfree=512k dalvik.vm.heapmaxfree=8m
# Initial framework tests use RAM and do not install/format internal storage.

# Legacy Vivante opens a non-inheritable galcore FD during EGL preload.
PRODUCT_PROPERTY_OVERRIDES += ro.zygote.disable_gl_preload=true

# First UI checkpoint: radio and location drivers are not ported yet.
PRODUCT_PROPERTY_OVERRIDES += config.disable_network=false config.disable_bluetooth=false config.disable_location=false

# Load the matched diagnostic ART image at its compiled base.
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.extra-opts=-Xnorelocate

# Old Vivante EGL cannot sustain HWUI preserved-buffer rendering.
PRODUCT_PROPERTY_OVERRIDES += debug.hwui.partial_updates=false

# Bound the stock EGL async queue handoff without enlarging BufferQueue.
PRODUCT_PROPERTY_OVERRIDES += ro.goya.legacy_egl_queue=true

# Keep normal colours and HWC active; GPU-only LiveDisplay calibration is unported.
PRODUCT_PROPERTY_OVERRIDES += ro.goya.no_gpu_color=true

# Complete shared Java dependency for the system-process fused provider.
PRODUCT_PACKAGES += CMParts FusedLocation com.android.location.provider com.android.location.provider.xml

# Retire the previous hardware-blit scanout buffer after vblank.
PRODUCT_PROPERTY_OVERRIDES += debug.goya.fb_retire=true

CM_BUILD := goyawifi
TARGET_SCREEN_WIDTH := 600
TARGET_SCREEN_HEIGHT := 1024
PRODUCT_BUILD_PROP_OVERRIDES += PRODUCT_NAME=goyawifi TARGET_DEVICE=goyawifi
PRODUCT_PROPERTY_OVERRIDES += ro.build.selinux=0 ro.goya.internal_install=1
PRODUCT_PACKAGES += GoyaCamera goya_storage_setup camera.mrvl
# Device-specific HALs, modules and setup files are overlaid from the verified
# runtime at image assembly; standard providers/apps come from the full build.
