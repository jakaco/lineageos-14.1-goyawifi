#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Build the LineageOS 14.1 system for goyawifi (user variant).
# The source tree in $ROOT/src is synced to manifests/lineage-14.1.xml with patches/ applied.
set -euo pipefail
ROOT=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
HERE=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$ROOT/src/device/samsung/goyawifi" "$ROOT/reports"
cp -r "$HERE/device/samsung/goyawifi/." "$ROOT/src/device/samsung/goyawifi/"
chown -R 1000:1000 "$ROOT/src/device/samsung/goyawifi"
chroot --userspec=1000:1000 "$ROOT/host/xenial" /usr/bin/env HOME=/home/builder USER=builder LOGNAME=builder \
    JACK_SERVER_VM_ARGUMENTS="-Dfile.encoding=UTF-8 -XX:+TieredCompilation -Xmx4g -Djava.security.properties=/src/device/samsung/goyawifi-init/jack.security" \
    JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 PATH=/usr/lib/jvm/java-8-openjdk-amd64/bin:/usr/bin:/bin:/usr/sbin:/sbin LC_ALL=C \
    /bin/bash -c 'cd /src && source build/envsetup.sh && lunch lineage_goyawifi-user && make -j4 systemimage' \
    > "$ROOT/reports/rom-build.log" 2>&1
