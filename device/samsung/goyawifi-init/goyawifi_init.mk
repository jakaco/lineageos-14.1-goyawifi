# SPDX-License-Identifier: Apache-2.0
# Source-built native bring-up product. No filesystem installer or Android system.
PRODUCT_NAME := goyawifi_init
PRODUCT_DEVICE := goyawifi-init
PRODUCT_BRAND := samsung
PRODUCT_MANUFACTURER := samsung
PRODUCT_MODEL := SM-T110
PRODUCT_CHARACTERISTICS := tablet
PRODUCT_PACKAGES := init adbd
PRODUCT_DEFAULT_PROPERTY_OVERRIDES := ro.secure=0 ro.debuggable=1 ro.adb.secure=0
