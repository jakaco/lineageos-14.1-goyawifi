# LineageOS 14.1 for Galaxy Tab 3 Lite 7.0 (SM-T110)

Unofficial LineageOS 14.1 (Android 7.1.2) for the Samsung Galaxy Tab 3 Lite 7.0 Wi-Fi, **SM-T110** (`goyawifi`, Marvell PXA986).

| | |
|---|---|
| Android | 7.1.2, security patch 2021-06-05 |
| Kernel | Linux 3.4.113 (Samsung/Marvell 3.4.5 tree updated to the last 3.4 stable) |
| GPU | Vivante GC1000 with the stock Android 4.2 blobs |
| Status | Experimental, tested on one tablet |

**Made and tested on the SM-T110.** The installer also accepts the SM-T111 with a warning (untested; no calls, SMS or mobile data). It refuses the Galaxy Tab 3 7.0 (T210/T211/T217), the T113, T116 and anything else, and stops if the partition layout doesn't match the T110's. Back up System and Boot in TWRP before flashing.

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
- Root (LineageOS su)
- Google apps (OpenGApps pico)

## Known issues

- The boot animation crashes as it closes. This is harmless.
- USB goes back to its default mode after each boot, so choose File transfer each time.
- microSD writes are slow, about 2.6 MB/s.
- Wi-Fi uses the US region (channels 1–11) unless you set your country. Android 7 has no menu for it, so set it once with ADB, for example `adb shell settings put global wifi_country_code DE`, then reboot. Most countries then get channels 1–13.
- GPS has no SUPL or XTRA assistance. Fix times read 18 seconds ahead before r6; r6 corrects that, but the correction hasn't been confirmed with a real fix yet.
- Per-app battery use in Settings is an estimate. The tablet can't measure current, so Android splits the drain using typical values for the screen, CPU and Wi-Fi.
- SELinux is disabled and `ro.debuggable=1` is set (ADB still needs approval on the tablet). The vendor kernel and blobs predate SELinux enforcing, and `ro.debuggable=0` makes the framework restart.
- Don't use "Format as internal" for SD cards. Adoptable storage needs encryption that hasn't been tested.

Standby: 85% to 83% over 11 hours asleep with Wi-Fi on (about 0.2% per hour). Long-term stability hasn't been tested yet.

## Installation

You need TWRP 3.x on the tablet.

1. In TWRP, back up System, Data and Boot (kernel) to a microSD card or a PC.
2. Coming from stock or another ROM: **Wipe > Advanced Wipe > Data**. Updating from an earlier release of this ROM: no wipe needed.
3. **Install** the zip. It takes up to 10 minutes and the screen may go dark; don't interrupt it.
4. Reboot to System. The first boot takes a few minutes.

The installer writes SYSTEM and KERNEL only. It never touches Recovery or your data partition, and it checks every image after writing.

## Root

Use LineageOS root:

1. Install `addonsu-14.1-arm-goyawifi.zip` from the release in TWRP. LineageOS's own addonsu zip won't install here, because this TWRP can't set SELinux labels.
2. In Settings > About tablet, tap Build number 7 times.
3. Set Settings > Developer options > Root access to Apps, or Apps and ADB.

Magisk 25.2 boots and its daemon runs, but root through it doesn't work yet: the app shows "Installed: N/A" and its Superuser prompt is invisible. Magisk 31 hangs at the Samsung logo.

## Google apps

[OpenGApps](https://opengapps.org) ARM 7.1 pico works. Install it in TWRP right after the ROM, before the first boot, together with a data wipe. The first boot then takes about 6 to 7 minutes.

Gesture typing isn't available: Google's gesture library uses an instruction this CPU doesn't have, so the keyboard ignores it. Reinstall GApps after every ROM update, because the update replaces SYSTEM.

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
