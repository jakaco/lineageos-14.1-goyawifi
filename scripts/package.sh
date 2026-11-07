#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
R=${ANDROID7_BUILD_ROOT:-/mnt/android7-jbuild}
W=${GOYAWIFI_WORK_DIR:?Set GOYAWIFI_WORK_DIR to local hardware overlays}
P=$R/src/out/target/product/goyawifi
if ! mountpoint -q "$R"; then
    mount -t ext4 -o loop,rw /mnt/j/Codex-builds/android7-goyawifi/nougat-build.ext4 "$R"
fi
if ! mountpoint -q "$R/host/xenial/src"; then
    mount --bind "$R/src" "$R/host/xenial/src"
fi
if ! mountpoint -q "$R/host/xenial/proc"; then
    mount -t proc proc "$R/host/xenial/proc"
fi
A=$P/goya-full-assembly
O=$W/full-lineage
if [ -L "$A" ]; then
    # An earlier assembly kept on another disk stays there; only the link goes.
    rm "$A"
elif [ -e "$A" ]; then
    # The previous assembly's images were already copied to $O; don't keep it.
    rm -rf "$A"
fi
if [ ! -e "$P/system/framework/arm/boot.art" ]; then
 sed 's/goyawifi-framework/goyawifi/g' "$W/build-art-boot-image.py" > "$R/reports/build-full-art.py"
 python3 "$R/reports/build-full-art.py"
fi
python3 "$(dirname "$(readlink -f "$0")")/assemble.py"
chroot "$R/host/xenial" /usr/bin/env LD_LIBRARY_PATH=/src/out/host/linux-x86/lib64 /src/out/host/linux-x86/bin/make_ext4fs -l 1594884096 -a system -L android_system /src/out/target/product/goyawifi/goya-full-assembly/system.ext4 /src/out/target/product/goyawifi/goya-full-assembly/system
e2fsck -fy "$A/system.ext4" || test $? -eq 1
e2fsck -fn "$A/system.ext4"
test "$(stat -c %s "$A/system.ext4")" = 1594884096
gzip -1 -c "$A/system.ext4" > "$O/system.ext4.gz.partial"
gzip -t "$O/system.ext4.gz.partial"
mv "$O/system.ext4.gz.partial" "$O/system.ext4.gz"
sha256sum "$A/system.ext4" "$O/system.ext4.gz" > "$O/system-sha256.txt"
cat "$O/system-sha256.txt"
