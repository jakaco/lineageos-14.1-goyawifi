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
| GPS | Fix received; three satellites on balcony | Owner test; no coordinate log |
| Accelerometer | Auto-rotation works after reboot | Owner confirmed portrait/landscape after persistent sysfs permission fix |

Not validated: suspend/resume, battery endurance, long-duration stability, broad Wi-Fi throughput/roaming, and all Bluetooth profiles. SELinux remains disabled; root ADB remains enabled.
