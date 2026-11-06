#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Build the r5 kernel (Linux 3.4.113) exactly as released.
# Source: the revision in kernel/source.json with its patch applied, e.g.
#   git clone https://github.com/T110-pmOS/linux_kernel_pxa988 kernel-r5 && cd kernel-r5
#   git checkout af48ce5b3b6ceb06963cc4aff0678c01c126c0f0
#   xz -dc ../kernel/patches/linux-3.4.113-goyawifi.patch.xz | git apply --binary
# Assembly (scripts/assemble.py) takes zImage and modules from $OUT.
set -euo pipefail
HERE=$(cd "$(dirname "$0")/.." && pwd)
ROOT=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
SRC=${GOYAWIFI_KERNEL_SRC:-$ROOT/kernel-r5}
OUT=${GOYAWIFI_KERNEL_OUT:-$ROOT/kernel-out-r5}
TC=${ANDROID7_TOOLCHAIN_PREFIX:?Set ANDROID7_TOOLCHAIN_PREFIX to the arm-linux-androideabi-4.7 prefix}
export KBUILD_BUILD_USER=android7 KBUILD_BUILD_HOST=goyawifi-builder
export KBUILD_BUILD_TIMESTAMP='Sun Nov 1 00:00:00 UTC 2026' KBUILD_BUILD_VERSION=1
mkdir -p "$OUT" "$ROOT/reports"
cp "$HERE/kernel/config-3.4.113" "$OUT/.config"
cd "$SRC"
K="make O=$OUT ARCH=arm CROSS_COMPILE=$TC HOSTCFLAGS=-fcommon"
$K CC="${TC}gcc" CPP="${TC}gcc -E" oldnoconfig > "$ROOT/reports/kernel-r5-config.log" 2>&1
cmp -s <(grep -v '^#' "$HERE/kernel/config-3.4.113") <(grep -v '^#' "$OUT/.config") || { echo "config drifted from kernel/config-3.4.113" >&2; exit 1; }
$K CC="${TC}gcc" CPP="${TC}gcc -E" -j4 zImage modules > "$ROOT/reports/kernel-r5-build.log" 2>&1
sha256sum "$OUT/arch/arm/boot/zImage" | tee "$ROOT/reports/kernel-r5-sha256.txt"
