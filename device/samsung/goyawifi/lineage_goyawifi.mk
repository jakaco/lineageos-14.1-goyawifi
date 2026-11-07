# SPDX-License-Identifier: Apache-2.0
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, vendor/cm/config/common_full_tablet_wifionly.mk)

PRODUCT_NAME := lineage_goyawifi
PRODUCT_DEVICE := goyawifi
PRODUCT_BRAND := samsung
PRODUCT_MANUFACTURER := samsung
PRODUCT_MODEL := SM-T110
PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME=goyawifi \
    TARGET_DEVICE=goyawifi \
    BUILD_FINGERPRINT=samsung/goyawifi/goyawifi:7.1.2/NJH47F/android7:user/test-keys
CM_BUILD := goyawifi

PRODUCT_CHARACTERISTICS := tablet
PRODUCT_AAPT_CONFIG := normal large xlarge mdpi hdpi
PRODUCT_AAPT_PREF_CONFIG := mdpi
TARGET_SCREEN_WIDTH := 600
TARGET_SCREEN_HEIGHT := 1024
DEVICE_PACKAGE_OVERLAYS += device/samsung/goyawifi/overlay

# No modem
PRODUCT_PACKAGES := $(filter-out Phone TeleService,$(PRODUCT_PACKAGES))
PRODUCT_PROPERTY_OVERRIDES += ro.radio.noril=true

# Device HALs and apps built from this tree
PRODUCT_PACKAGES += \
    audio.primary.mrvl \
    camera.mrvl \
    gps.default \
    libbt-vendor \
    libstagefrighthw \
    lights.default \
    goya_storage_setup \
    GoyaCamera

PRODUCT_PACKAGES += \
    SystemUI \
    SettingsProvider \
    CMSettingsProvider \
    CMParts \
    FusedLocation \
    com.android.location.provider \
    com.android.location.provider.xml

# ro.debuggable=0 makes the framework restart about 90 s after boot on this
# kernel. ADB still needs host authorization and adb root stays off.
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.secure=1 ro.debuggable=1 ro.adb.secure=1
PRODUCT_PROPERTY_OVERRIDES += ro.build.selinux=0

# Display and memory, matching stock
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.hwrotation=270 \
    ro.sf.lcd_density=160 \
    ro.opengles.version=131072 \
    ro.bq.gpu_to_cpu_unsupported=1 \
    dalvik.vm.heapstartsize=8m \
    dalvik.vm.heapgrowthlimit=96m \
    dalvik.vm.heapsize=256m \
    dalvik.vm.heaptargetutilization=0.75 \
    dalvik.vm.heapminfree=512k \
    dalvik.vm.heapmaxfree=8m

# Vivante GC1000 with Android 4.2 EGL
PRODUCT_PROPERTY_OVERRIDES += \
    ro.zygote.disable_gl_preload=true \
    debug.hwui.partial_updates=false \
    ro.goya.legacy_egl_queue=true \
    ro.goya.no_gpu_color=true \
    debug.goya.fb_retire=true

# The prebuilt ART boot image is loaded at its compiled base address.
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.extra-opts=-Xnorelocate

PRODUCT_PROPERTY_OVERRIDES += \
    config.disable_bluetooth=false \
    ro.goya.internal_install=1

# Configs
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,device/samsung/goyawifi/configs/goya-audio,system/etc/goya-audio) \
    device/samsung/goyawifi/configs/audio_policy.conf:system/etc/audio_policy.conf \
    device/samsung/goyawifi/configs/media_codecs.xml:system/etc/media_codecs.xml \
    device/samsung/goyawifi/configs/media_profiles.xml:system/etc/media_profiles.xml \
    device/samsung/goyawifi/configs/gps.conf:system/etc/gps.conf \
    device/samsung/goyawifi/configs/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    device/samsung/goyawifi/configs/bluetooth/bt_stack.conf:system/etc/bluetooth/bt_stack.conf \
    device/samsung/goyawifi/configs/bluetooth/bt_did.conf:system/etc/bluetooth/bt_did.conf \
    device/samsung/goyawifi/configs/permissions/goya-camera.xml:system/etc/permissions/goya-camera.xml \
    device/samsung/goyawifi/configs/permissions/goyawifi-core-hardware.xml:system/etc/permissions/goyawifi-core-hardware.xml \
    device/samsung/goyawifi/configs/idc/sec_touchscreen.idc:system/usr/idc/sec_touchscreen.idc \
    device/samsung/goyawifi/configs/keylayout/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
    device/samsung/goyawifi/configs/goya-audio-setup.sh:system/etc/goya-audio-setup.sh \
    device/samsung/goyawifi/configs/goya-bluetooth-setup.sh:system/etc/goya-bluetooth-setup.sh \
    device/samsung/goyawifi/configs/goya-camera-setup.sh:system/etc/goya-camera-setup.sh \
    device/samsung/goyawifi/configs/goya-wifi-setup.sh:system/etc/goya-wifi-setup.sh

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.software.app_widgets.xml:system/etc/permissions/android.software.app_widgets.xml

# Stock Samsung/Marvell blobs, see proprietary-files.txt and extract-files.sh
$(call inherit-product-if-exists, vendor/samsung/goyawifi/goyawifi-vendor.mk)
