#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
ROOT=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
PORT=${GOYAWIFI_SOURCE_ROOT:?Set GOYAWIFI_SOURCE_ROOT to this repository root}
WORK=${GOYAWIFI_WORK_DIR:?Set GOYAWIFI_WORK_DIR to local hardware overlays}
D="$ROOT/src/device/samsung/goyawifi-camera"
mkdir -p "$D"
cp "$PORT/device/samsung/goyawifi-camera/camera-hal.cpp" "$D/"
chroot --userspec=1000:1000 "$ROOT/host/xenial" /usr/bin/env HOME=/home/builder USER=builder LOGNAME=builder JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 PATH=/usr/lib/jvm/java-8-openjdk-amd64/bin:/usr/bin:/bin:/usr/sbin:/sbin LC_ALL=C /bin/bash -c 'cd /src; source build/envsetup.sh; lunch goyawifi_framework-userdebug; make -j4 camera.mrvl cameraserver' > "$ROOT/reports/camera-source-build.log" 2>&1
cp "$ROOT/src/out/target/product/goyawifi-framework/system/lib/hw/camera.mrvl.so" "$WORK/camera-runtime/camera.source.so"
