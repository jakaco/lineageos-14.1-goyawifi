# TWRP package contents

This package targets **Samsung SM-T110 / goyawifi only**. The updater checks the model, device, partition map, partition sizes, recovery fingerprint, and image checksums before writing. It writes SYSTEM (`mmcblk0p15`) and KERNEL (`mmcblk0p10`) only. USER/data and RECOVERY are not flashed or formatted.

Before installation, make a TWRP backup of SYSTEM, USER, and KERNEL to removable storage or a PC. Use TWRP 3.x, copy the release ZIP to the tablet, choose Install, and reboot System after success. The package is experimental and unsigned; install only if you understand the risk and can restore your backup.

The included system image contains Android 7.1.2 / LineageOS 14.1, device-specific hardware support, and stock-derived components. Cellular voice/SMS capability and phone/carrier UI are disabled for this Wi-Fi-only tablet. ADB authentication is enabled and `adb root` is disabled. SELinux is disabled in this experimental build.
