#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
ROOT=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
PORT=${GOYAWIFI_SOURCE_ROOT:?Set GOYAWIFI_SOURCE_ROOT to this repository root}
cp "$PORT/device/samsung/goyawifi-init/Android.mk" "$PORT/device/samsung/goyawifi-init/binder_probe.cpp" "$ROOT/src/device/samsung/goyawifi-init/"
chown 1000:1000 "$ROOT/src/device/samsung/goyawifi-init/"{Android.mk,binder_probe.cpp}
chroot --userspec=1000:1000 "$ROOT/host/xenial" /usr/bin/env HOME=/home/builder USER=builder LOGNAME=builder JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 PATH=/usr/lib/jvm/java-8-openjdk-amd64/bin:/usr/bin:/bin:/usr/sbin:/sbin LC_ALL=C /bin/bash -c '
cd /src
source build/envsetup.sh
lunch goyawifi_init-userdebug || exit $?
make -j4 servicemanager bctest logd logcat linker toolbox libbinder libstdc++ goyawifi_binder_probe
' > "$ROOT/reports/services-build.log" 2>&1
