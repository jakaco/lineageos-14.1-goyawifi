#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
ROOT=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
chroot --userspec=1000:1000 "$ROOT/host/xenial" /usr/bin/env HOME=/home/builder USER=builder LOGNAME=builder JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 PATH=/usr/lib/jvm/java-8-openjdk-amd64/bin:/usr/bin:/bin:/usr/sbin:/sbin LC_ALL=C JACK_SERVER_VM_ARGUMENTS="-Dfile.encoding=UTF-8 -XX:+TieredCompilation -Xmx4g -Djava.security.properties=/src/device/samsung/goyawifi-init/jack.security" /bin/bash -c '
cd /src
source build/envsetup.sh
lunch goyawifi_init-userdebug || exit $?
make -j4 core-oj core-libart libopenjdk libjavacore libopenjdkjvm tzdata
' > "$ROOT/reports/core-build.log" 2>&1
