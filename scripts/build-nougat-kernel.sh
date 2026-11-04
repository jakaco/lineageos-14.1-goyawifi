#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
ROOT=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
SRC="$ROOT/kernel"
OUT="$ROOT/kernel-out"
TC=${ANDROID7_TOOLCHAIN_PREFIX:?Set ANDROID7_TOOLCHAIN_PREFIX to the arm-linux-androideabi prefix}
export KBUILD_BUILD_USER=android7 KBUILD_BUILD_HOST=goyawifi-builder
export KBUILD_BUILD_TIMESTAMP='Sun Nov 1 00:00:00 UTC 2026' KBUILD_BUILD_VERSION=1
cd "$SRC"
make O="$OUT" ARCH=arm CROSS_COMPILE="$TC" CC="${TC}gcc" CPP="${TC}gcc -E" HOSTCFLAGS=-fcommon goyawifi_defconfig > "$ROOT/reports/kernel-nougat-config.log" 2>&1
python3 - "$OUT/.config" "$ROOT/bringup/nougat-ramdisk.cpio.gz" <<'PY'
import re,sys
from pathlib import Path
p=Path(sys.argv[1]); text=p.read_text();size=Path(sys.argv[2]).stat().st_size
opts={k:'y' for k in ['AUDIT','SECURITY','SECURITYFS','SECURITY_NETWORK','SECURITY_PATH','SECURITY_SELINUX','SECURITY_SELINUX_BOOTPARAM','SECURITY_SELINUX_DEVELOP','DEFAULT_SECURITY_SELINUX','TMPFS_XATTR','EXT4_FS_SECURITY','CMDLINE_EXTEND']}
opts.update(SECURITY_SELINUX_BOOTPARAM_VALUE='1',SECURITY_SELINUX_CHECKREQPROT_VALUE='0',DEFAULT_SECURITY='"selinux"',DEFAULT_SECURITY_DAC='n',CMDLINE_FROM_BOOTLOADER='n',CMDLINE_FORCE='n',USB_ANDROID_SAMSUNG_MTP='n',LOCALVERSION='"-a7init1"',CMDLINE=f'"selinux=0 androidboot.selinux=disabled enforcing=0 goyawifi.initrd_size={size}"')
for key,value in opts.items():
 line=f'# CONFIG_{key} is not set' if value=='n' else f'CONFIG_{key}={value}'
 pattern=rf'^(?:CONFIG_{key}=.*|# CONFIG_{key} is not set)$'
 if re.search(pattern,text,re.M):text=re.sub(pattern,lambda _:line,text,flags=re.M)
 else:text+='\n'+line+'\n'
p.write_text(text)
PY
make O="$OUT" ARCH=arm CROSS_COMPILE="$TC" CC="${TC}gcc" CPP="${TC}gcc -E" HOSTCFLAGS=-fcommon oldnoconfig >> "$ROOT/reports/kernel-nougat-config.log" 2>&1
grep -q '^CONFIG_SECURITY_SELINUX=y$' "$OUT/.config"
grep -q '^CONFIG_CMDLINE_EXTEND=y$' "$OUT/.config"
make -j4 O="$OUT" ARCH=arm CROSS_COMPILE="$TC" CC="${TC}gcc" CPP="${TC}gcc -E" HOSTCFLAGS=-fcommon zImage modules > "$ROOT/reports/kernel-nougat-build.log" 2>&1
sha256sum "$OUT/arch/arm/boot/zImage" "$OUT/.config" > "$ROOT/reports/kernel-nougat-sha256.txt"
git diff > "$ROOT/reports/kernel-initrd-length.patch"
