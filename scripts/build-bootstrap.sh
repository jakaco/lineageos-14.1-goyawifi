#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Build init, adbd and the legacy binder shim from the goyawifi_init bootstrap product.
# assemble.py takes init from out/target/product/goyawifi-init/root.
set -euo pipefail
ROOT=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
HERE=$(cd "$(dirname "$0")/.." && pwd)
DEVICE="$ROOT/src/device/samsung/goyawifi-init"
mkdir -p "$DEVICE" "$ROOT/reports"
cp -r "$HERE/device/samsung/goyawifi-init/." "$DEVICE/"
cp -u "${ANDROID7_REFERENCE_ZIMAGE:?Set ANDROID7_REFERENCE_ZIMAGE to the built zImage}" "$DEVICE/kernel-reference"
chown -R 1000:1000 "$DEVICE"
chroot --userspec=1000:1000 "$ROOT/host/xenial" /usr/bin/env HOME=/home/builder USER=builder LOGNAME=builder \
    JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 PATH=/usr/lib/jvm/java-8-openjdk-amd64/bin:/usr/bin:/bin:/usr/sbin:/sbin LC_ALL=C \
    /bin/bash -c 'cd /src && source build/envsetup.sh && lunch goyawifi_init-userdebug && make -j4 init adbd libshim_goyawifi_binder' \
    > "$ROOT/reports/bootstrap-build.log" 2>&1
