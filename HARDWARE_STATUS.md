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
| Suspend | r4: about 984 s of continuous suspend in a 17-minute unplugged test; Wi-Fi and Bluetooth back after wake; wake takes about 3 s | Timestamp logger, kernel log and owner wake check |
| Performance | Launcher scroll jank about 6% with a 312 MHz GPU floor, from 63% | Scripted scroll benchmark |
| microSD | 32 GB SDHC card mounts as public storage; apps and MTP can use it; 8 MB write verified (about 2.6 MB/s) | r4 test builds |
| Off-mode charging | Plugging in while powered off shows the charging screen; holding Power boots Android | Owner test on an r4 test build |
| WebView | Bromite 81 loads in Jelly; Wikipedia scroll p50 19 ms against 16 ms on stock 52 | Scripted scroll benchmark and owner use |
| Drag performance | Touch boost renews during drags; slow drags clearly smoother | Owner comparison |
| Accelerometer | Auto-rotation works after reboot | Owner confirmed portrait/landscape after persistent sysfs permission fix |

Not validated: overnight sleep, battery endurance, long-duration stability, broad Wi-Fi throughput, and all Bluetooth profiles. This device configuration is Wi-Fi-only, with cellular voice/SMS UI disabled. The image sets `ro.debuggable=1`; ADB needs host authorization, and `adb root` stays refused unless enabled in Developer options. SELinux remains disabled.
