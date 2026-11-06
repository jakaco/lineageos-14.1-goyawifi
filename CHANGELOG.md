# Changelog

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
