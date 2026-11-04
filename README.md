# LineageOS 14.1 port for Samsung SM-T110 (goyawifi)

**Status: experimental, hardware-tested on one SM-T110.** This is a downstream Android 7.1.2 / LineageOS 14.1 port, not an official LineageOS release. The only tested target is the Wi-Fi-only Samsung SM-T110 (`goyawifi`, PXA986). Do not flash on SM-T111, SM-T113, SM-T116, or another device.

## Download

The TWRP installer is attached to the GitHub Release. It writes SYSTEM (`mmcblk0p15`) and KERNEL (`mmcblk0p10`). It does not format USER/data (`mmcblk0p16`) or write RECOVERY (`mmcblk0p9`). Its updater checks the exact model, device, partition map, partition sizes, recovery fingerprint, and payload hashes.

Before flashing, make a TWRP backup of SYSTEM, USER, and KERNEL to removable storage or a PC. In TWRP, choose **Install**, select the release ZIP, confirm, then reboot System. If any check fails, the updater stops before writing.

## Hardware status

Confirmed on the owner's tablet: boot and UI use, Wi-Fi Internet, display colors and hardware composition, brightness and backlight-off behavior, speaker/headphone/microphone audio, Bluetooth file transfer and A2DP speaker playback, rear camera photos and video recording/playback, smooth 720p hardware video decoding, GPS fix (three satellites on a balcony), and accelerometer-driven portrait/landscape rotation after reboot.

This is a Wi-Fi-only tablet build: cellular voice/SMS capability and phone/carrier UI are disabled. ADB requires host authorization and `adb root` is disabled. SELinux remains disabled, so this end-of-life Android 7.1.2 build is still experimental and not suitable for sensitive daily use. Suspend/resume, long-duration stability, and battery life have not been validated. Some UI animation jank remains. This release does not provide OTA updates or encryption migration.

## Source tree

This repository carries the device configuration, source patches, pinned upstream manifest, and build/package scripts. It does not mirror the full Android source tree or commit stock vendor binaries. The flashable release image includes device-specific runtime components; see `LICENSES.md` for provenance and license boundaries. A clean source sync uses the revisions in `sources/lineage-14.1-resolved-manifest.xml` and `sources/kernel-source.json`.

Build host used for the validated image: isolated Ubuntu 16.04, OpenJDK 8, downstream Linux 3.4.5 toolchain, and an ext4 build volume with about 64 GiB of source/build space. Scripts in `scripts/` show the recorded build and assembly steps; the original host paths must be supplied for another build. Proprietary components must be supplied from a lawful SM-T110 firmware source.

## Release checksums

See `packaging/twrp/release-metadata.json` for raw image hashes. The GitHub Release asset includes `release-metadata.json` and a `SHA256SUMS` file for payload verification.
