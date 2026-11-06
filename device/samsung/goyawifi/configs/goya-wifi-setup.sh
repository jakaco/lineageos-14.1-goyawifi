#!/system/bin/sh
set -e
exec >/tmp/wifi-setup.log 2>&1
T=/system/bin/toybox
$T mkdir -p /data/misc/net
$T chown 0:2000 /data/misc/net
$T chmod 750 /data/misc/net
if [ -e /proc/net/xt_qtaguid/ctrl ]; then $T chown 0:3007 /proc/net/xt_qtaguid/ctrl; fi
if [ -e /proc/net/xt_qtaguid/stats ]; then $T chown 0:3006 /proc/net/xt_qtaguid/stats; fi
if [ -e /dev/xt_qtaguid ]; then $T chmod 644 /dev/xt_qtaguid; fi
for node in /sys/kernel/ipv4/tcp_[rw]mem_*; do
    if [ -e "$node" ]; then $T chown 1000:1000 "$node"; fi
done
for module in xt_mark xt_connmark xt_IDLETIMER xt_conntrack; do
    $T insmod /system/lib/modules/$module.ko
done
$T mkdir -p /data/misc/wifi /data/misc/wifi/sockets
# Remove only the legacy per-interface socket created by the first trial.
# Nougat sends IFNAME-prefixed commands to the global abstract control socket.
$T rm -f /data/system/wpa_supplicant/wlan0
if [ -d /data/system/wpa_supplicant ]; then $T rmdir /data/system/wpa_supplicant; fi
$T chown 1010:1000 /data/misc/wifi
$T chmod 770 /data/misc/wifi
$T chown 1010:1010 /data/misc/wifi/sockets
$T chmod 771 /data/misc/wifi/sockets
MACFILE=/data/misc/wifi/factory-mac-address
if [ ! -f "$MACFILE" ]; then
    $T mkdir -p /tmp/goya-efs-wifi
    if $T mount -t ext4 -o ro,noload /dev/block/mmcblk0p4 /tmp/goya-efs-wifi; then
        $T cat /tmp/goya-efs-wifi/wifi/.mac.info > "$MACFILE" || true
        $T umount /tmp/goya-efs-wifi
    fi
fi
$T grep -Eq '^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$' "$MACFILE"
MAC=$($T cat "$MACFILE")
$T chown 0:0 "$MACFILE"
$T chmod 600 "$MACFILE"
$T chown 1000:1000 /sys/class/rfkill/rfkill0/state
$T chmod 660 /sys/class/rfkill/rfkill0/state
$T insmod /system/lib/modules/mlan.ko
$T insmod /system/lib/modules/sd8xxx.ko sta_name=wlan cfg80211_wext=12 drv_mode=1 mac_addr="$MAC"
$T setprop wifi.interface wlan0
echo WIFI_SETUP_READY
