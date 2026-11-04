#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
ROOT=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
PORT=${GOYAWIFI_SOURCE_ROOT:?Set GOYAWIFI_SOURCE_ROOT to this repository root}
HOST="$ROOT/host/xenial"
DEVICE="$ROOT/src/device/samsung/goyawifi-init"
mkdir -p "$DEVICE"
cp "$PORT/device/samsung/goyawifi-init/"* "$DEVICE/"
cp -u "${ANDROID7_REFERENCE_ZIMAGE:?Set ANDROID7_REFERENCE_ZIMAGE}" "$DEVICE/kernel-reference"
chown -R 1000:1000 "$DEVICE"
chroot --userspec=1000:1000 "$HOST" /usr/bin/env HOME=/home/builder USER=builder LOGNAME=builder JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 PATH=/usr/lib/jvm/java-8-openjdk-amd64/bin:/usr/bin:/bin:/usr/sbin:/sbin LC_ALL=C /bin/bash -c '
cd /src
source build/envsetup.sh
lunch goyawifi_init-userdebug || exit $?
make -j4 init adbd checkpolicy sefcontext_compile
' > "$ROOT/reports/native-build.log" 2>&1
