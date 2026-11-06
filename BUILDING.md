# Building

This port doesn't build with a plain `brunch`. The system comes from a normal LineageOS 14.1 build, and then `scripts/assemble.py` puts it together with the kernel, the boot ramdisk, the stock blobs and a prebuilt ART boot image into the images the installer flashes. These are the steps used for the releases.

## Host

Ubuntu 16.04 (a chroot works), OpenJDK 8 and about 70 GB of disk. The scripts expect the build root at `/mnt/android7-jbuild` (override with `ANDROID7_BUILD_ROOT`), the Android tree at `$ROOT/src` and the chroot at `$ROOT/host/xenial`. The kernel needs the `arm-linux-androideabi-4.7` toolchain.

## 1. Kernel

```bash
git clone https://github.com/T110-pmOS/linux_kernel_pxa988 $ROOT/kernel-r5
cd $ROOT/kernel-r5
git checkout af48ce5b3b6ceb06963cc4aff0678c01c126c0f0
xz -dc /path/to/this/repo/kernel/patches/linux-3.4.113-goyawifi.patch.xz | git apply --binary
ANDROID7_TOOLCHAIN_PREFIX=/path/to/arm-linux-androideabi-4.7/bin/arm-linux-androideabi- scripts/build-kernel.sh
```

The patch turns the pinned commit into the exact r5 tree (git tree `89d202e3`). `kernel/patches/0001-*.patch` is the r5 display and resume fix on its own, for reading.

## 2. Android

```bash
repo init -u https://github.com/LineageOS/android.git -b cm-14.1
cp manifests/lineage-14.1.xml .repo/manifests/goyawifi.xml
repo init -m goyawifi.xml && repo sync
```

Apply each patch in `patches/` to its project (the file name says which, e.g. `frameworks-native.patch` goes to `frameworks/native`). Then:

```bash
scripts/build-bootstrap.sh   # init, adbd, binder shim
scripts/build-rom.sh         # lineage_goyawifi-user systemimage
```

## 3. Stock blobs

Mount the system image of stock firmware T110XXUANI7 and run:

```bash
device/samsung/goyawifi/extract-files.sh /path/to/stock/system
```

## 4. Images and zip

`scripts/package.sh` runs `assemble.py`, builds `system.ext4` and writes `system.ext4.gz` and `kernel-partition.img`. It reads the hardware runtime (blobs, rebuilt HALs, firmware, WebView, certificates, boot template) from `GOYAWIFI_WORK_DIR`; `assemble.py` lists every input and checks its hash.

Put the new image hashes in `packaging/twrp/release-metadata.json` and the installer, then:

```bash
scripts/make-twrp-zip.py --system-gz system.ext4.gz --kernel kernel-partition.img --output lineage-14.1-goyawifi-rN.zip
```
