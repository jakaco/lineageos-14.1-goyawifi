# LineageOS 14.1 for Galaxy Tab 3 Lite 7.0 (SM-T110)

Unofficial LineageOS 14.1 (Android 7.1.2) for the Samsung Galaxy Tab 3 Lite 7.0 Wi-Fi, **SM-T110** (`goyawifi`, Marvell PXA986).

| | |
|---|---|
| Android | 7.1.2, security patch 2021-06-05 |
| Kernel | Linux 3.4.113 (Samsung/Marvell 3.4.5 tree updated to the last 3.4 stable) |
| GPU | Vivante GC1000 with the stock Android 4.2 blobs |
| Status | Experimental, tested on one tablet |

**SM-T110 only.** Don't flash this on the SM-T111, T113, T116 or any other model. The installer checks the model and partition layout and stops if they don't match.

## Download

Get the latest zip from [Releases](https://github.com/jakaco/lineageos-14.1-goyawifi/releases). Each release lists its SHA-256.

## Working

- Display, hardware composition, brightness
- Touchscreen
- Wi-Fi
- Bluetooth (file transfer, A2DP audio)
- Audio: speaker, headphones, headset mic
- Rear camera: photos and video
- Hardware video decoding (720p)
- GPS (standalone)
- Accelerometer and auto-rotation
- microSD as portable storage
- MTP and PTP
- Charging, including the charging screen when off
- Deep sleep

## Known issues

- The launcher logs an "app not responding" shortly after each boot.
- The boot animation crashes as it closes. This is harmless.
- USB goes back to its default mode after each boot, so choose File transfer each time.
- microSD writes are slow, about 2.6 MB/s.
- Wi-Fi uses the US region, so channels 12 and 13 aren't available.
- GPS has no SUPL or XTRA assistance, and fix times read 18 seconds ahead.
- Per-app battery statistics are incomplete.
- SELinux is disabled and `ro.debuggable=1` is set (ADB still needs approval on the tablet). The vendor kernel and blobs predate SELinux enforcing, and `ro.debuggable=0` makes the framework restart.
- Don't use "Format as internal" for SD cards. Adoptable storage needs encryption that hasn't been tested.

Not tested yet: overnight battery drain and long-term stability.

## Installation

You need TWRP 3.x on the tablet.

1. In TWRP, back up System, Data and Boot (kernel) to a microSD card or a PC.
2. Coming from stock or another ROM: **Wipe > Advanced Wipe > Data**. Updating from an earlier release of this ROM: no wipe needed.
3. **Install** the zip. It takes up to 10 minutes and the screen may go dark; don't interrupt it.
4. Reboot to System. The first boot takes a few minutes.

The installer writes SYSTEM and KERNEL only. It never touches Recovery or your data partition, and it checks every image after writing.

Google apps aren't included and haven't been tested.

## Source

| Path | Contents |
|---|---|
| `device/samsung/goyawifi` | Device tree: audio, camera, Bluetooth, GPS, lights and video HALs, configs, overlays, `proprietary-files.txt` |
| `device/samsung/goyawifi-init` | Small bootstrap product that builds `init`, `adbd` and the binder shim |
| `kernel/` | Kernel source pin, `.config` and patches |
| `patches/` | Patches to LineageOS projects |
| `manifests/` | Pinned LineageOS manifest and extra source pins |
| `scripts/` | Build, image assembly and zip packaging |
| `packaging/twrp` | Installer |

See [BUILDING.md](BUILDING.md) to build it and [CHANGELOG.md](CHANGELOG.md) for release notes.

## Credits

- [LineageOS](https://lineageos.org)
- [T110-pmOS](https://github.com/T110-pmOS/linux_kernel_pxa988) for the kernel tree
- Samsung and Marvell for the kernel source and stock blobs
- [Bromite](https://github.com/bromite/bromite) for the System WebView
- kernel.org stable maintainers for 3.4.6 to 3.4.113

## License

Device tree, scripts and packaging: Apache-2.0. Patches keep the license of the project they apply to; the kernel is GPL-2.0. Stock blobs in the release zip belong to Samsung/Marvell. See [LICENSES.md](LICENSES.md).
