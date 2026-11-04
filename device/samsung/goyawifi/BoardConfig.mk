# SPDX-License-Identifier: Apache-2.0
# Retain every tested downstream kernel/GC1000 compatibility switch.
include device/samsung/goyawifi-framework/BoardConfig.mk
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1594884096
BOARD_USERDATAIMAGE_PARTITION_SIZE := 5368709120
BOARD_FLASH_BLOCK_SIZE := 131072
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_CACHEIMAGE_PARTITION_SIZE := 209715200
TARGET_RECOVERY_FSTAB := device/samsung/goyawifi/fstab.goyawifi
WITH_DEXPREOPT := false
