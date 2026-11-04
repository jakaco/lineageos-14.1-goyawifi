#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
ROOT=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
PORT=${GOYAWIFI_SOURCE_ROOT:?Set GOYAWIFI_SOURCE_ROOT to this repository root}
PORT=${GOYAWIFI_SOURCE_ROOT:?Set GOYAWIFI_SOURCE_ROOT to this repository root}
mkdir -p "$ROOT/src/device/samsung/goyawifi-init/shims"
cp "$PORT/device/samsung/goyawifi-init/shims/"* "$ROOT/src/device/samsung/goyawifi-init/shims/"
cp "$PORT/device/samsung/goyawifi-init/Android.mk" "$PORT/device/samsung/goyawifi-init/graphics_probe.cpp" "$PORT/device/samsung/goyawifi-init/BoardConfig.mk" "$ROOT/src/device/samsung/goyawifi-init/"
chown -R 1000:1000 "$ROOT/src/device/samsung/goyawifi-init"
chroot --userspec=1000:1000 "$ROOT/host/xenial" /usr/bin/env HOME=/home/builder USER=builder LOGNAME=builder JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 PATH=/usr/lib/jvm/java-8-openjdk-amd64/bin:/usr/bin:/bin:/usr/sbin:/sbin LC_ALL=C /bin/bash -c '
cd /src
source build/envsetup.sh
lunch goyawifi_init-userdebug || exit $?
make -j4 linker libui libEGL libGLESv1_CM libGLESv2 libhardware libhardware_legacy libshim_goyawifi_binder goyawifi_graphics_probe toybox
' > "$ROOT/reports/graphics-build.log" 2>&1
