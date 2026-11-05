# Hardware validation snapshot

Device: SM-T110 / goyawifi, PXA986, Wi-Fi-only. Owner-visible checks were performed on the physical tablet.

| Area | Result | Evidence |
|---|---|---|
| Boot and UI | Boots internally; owner reported usable | Rebooted installed build |
| Display / HWC | Correct color order; hardware composition active; brightness works and backlight turns off | Owner-visible use |
| Wi-Fi | Connects and reaches Internet; reconnects after radio toggle and reboot | DNS, HTTP, HTTPS checks and owner confirmation |
| Audio | Speaker, wired headset, headset mic, volume and mute work | Owner recording/playback checks |
| Bluetooth | Phone pairing/file transfer and A2DP speaker playback work | Owner confirmation, including reboot reconnect |
| Camera | Rear preview, JPEG save/review, video recording and playback work | Owner-visible test and reboot capture checks |
| Video decode | 720p hardware decode displayed smoothly with correct color | Owner-visible test pattern |
| GPS | Accurate fix with many satellites; date corrected after a 1024-week rollover; fix time reads 18 s ahead | Owner test on r3; HAL log; no coordinate log |
| USB | MTP transfers match SHA-256 both ways on Windows; PTP and charging modes switch with ADB connected | Owner mode switches and hash checks on an r3 test build |
| Charging | USB and wall charger detected and reported as charging; charge rate not measured | Owner test and kernel log on an r3 test build |
| Suspend | Entered and resumed cleanly over 643 s unplugged on an earlier r3 build; not retested on the final image | Kernel log and owner wake check |
| Performance | Launcher scroll jank about 6% with a 312 MHz GPU floor, from 63% | Scripted scroll benchmark |
| Accelerometer | Auto-rotation works after reboot | Owner confirmed portrait/landscape after persistent sysfs permission fix |

Not validated: suspend/resume on the final r3 image, overnight sleep, battery endurance, long-duration stability, broad Wi-Fi throughput, and all Bluetooth profiles. This device configuration is Wi-Fi-only, with cellular voice/SMS UI disabled. The image sets `ro.debuggable=1`; ADB needs host authorization, and `adb root` stays refused unless enabled in Developer options. SELinux remains disabled.
