# Changelog

## r6 (2026-11-07)

- Root: LineageOS root works with `addonsu-14.1-arm-goyawifi.zip` (Developer options > Root access)
- Google apps: OpenGApps pico installs and the Play Store works. The keyboard no longer crashes when GApps adds Google's gesture-typing library, which this CPU can't run.
- Fixed the long freezes during boot and right after the first unlock. Every app process set up the GPU as soon as it started, even background ones that never draw; on this GPU driver about 20 of them doing it at once froze SystemUI and the launcher for up to a minute. Only apps that draw set it up now, so it takes 1–2 s instead of about 20 s.
- Compressed swap in RAM (zram, 256 MB) now works: the kernel no longer carries Samsung's changes that stopped normal swapping. The storage scheduler is `deadline`, so screen updates don't wait behind background reads.
- Power off really switches the tablet off. Since the 3.4.113 kernel update it only halted, and the battery drained overnight while it looked off.
- 32 MB more RAM for apps (868 MB instead of 835 MB): the kernel no longer sets memory aside for a modem the Wi-Fi tablet doesn't have
- Settings > Battery shows which apps and parts use the battery. The tablet now has its own power profile (3600 mAh, CPU speeds, screen and Wi-Fi draw) instead of Android's placeholder.
- Fixed `adb shell screencap` crashing
- Wi-Fi country codes work: `settings put global wifi_country_code XX` reaches the driver, and most countries get channels 1–13 instead of the US 1–11
- Android can kill an app together with every process it started (per-app process groups)
- Battery statistics include kernel wakelocks
- Fixed Settings > Security and Settings > Backup & reset crashing, opening the microSD card in Settings > Storage, and the lock screen notification screen that apps can open crashing
- Declares the tablet's standard features (touchscreen, microphone, keyboard support, home screen, backup and more). Without them the Play Store hid most apps.
- TWRP installer: progress bar, clearer messages, re-flashes once if a check fails, accepts TWRP builds that report the T110 as `goya`, and accepts the SM-T111 with an untested-model warning
- Native crash reports work (kernel backport), and suspend no longer prints a kernel warning every time
- Kernel boots ramdisks of any size, and the boot image is a standard one that tools such as Magisk can unpack
- Boot ramdisk now includes the built SELinux policy (SELinux stays off) and fires the standard post-fs-data and nonencrypted triggers
- init starts services that carry an SELinux label

## r5 (2026-11-06)

- Kernel updated from Linux 3.4.5 to 3.4.113, the last 3.4 stable release, in four tested steps (3.4.30, 3.4.60, 3.4.90, 3.4.113)
- Waking from sleep is much faster: the kernel resumes in about 0.75 s instead of about 2.5 s. The display driver was waiting on I²C transfers that couldn't complete.
- Fixed white flashes at boot and after a reboot
- Screen-off uses a plain fade; the old effect drew the screen squashed and mirrored
- No mobile or SIM icons, "No SIM" text or mobile data settings
- Fixed the setup wizard being skipped or hanging on a new install
- Removed bring-up leftovers from the system image
- Kernel source pin corrected (see `kernel/source.json`)

## r4 (2026-11-05)

- microSD cards mount, and apps and MTP can use them
- Charging screen when plugged in while off
- Touch boost lasts for the whole drag
- Apps are compiled at install and while idle, so they start faster
- Bromite SystemWebView 81 replaces Chromium 52 WebView, and WebView memory sharing works
- Current Mozilla root certificates
- Bluetooth setup recovers from a bad saved address
- Removed Updater, Email and Exchange

## r3 (2026-11-05)

- MTP and PTP file transfer; Developer options no longer crashes Settings
- Deep sleep works
- Fixed the framework restart about 90 s after boot
- Fixed `adb shell` and `adb reboot`
- USB and wall chargers show as charging
- GPS dates corrected (week rollover)
- 312 MHz GPU floor and interactive CPU governor: launcher scroll jank down from 63% to about 6%
- Missing AOSP kernel settings added

## r2 (2026-11-04)

- Removed the phone app and SIM alerts
- User build with ADB authorization

Don't use r2: its system image is missing the ART boot image.

## r1 (2026-11-04)

- First release
