#!/system/bin/sh
set -e
exec >/tmp/bluetooth-setup.log 2>&1
T=/system/bin/toybox
# Native Bluetooth alarm scheduling uses Android's WAKE_LOCK permission group.
$T chown 1000:3010 /sys/power/wake_lock /sys/power/wake_unlock
$T chmod 660 /sys/power/wake_lock /sys/power/wake_unlock
$T touch /data/misc/goya-radio.lock
$T chown 1000:1002 /data/misc/goya-radio.lock
$T chmod 660 /data/misc/goya-radio.lock
$T mkdir -p /data/misc/bluedroid /data/misc/bluetooth /data/misc/bluetooth/logs
$T chown 1002:1002 /data/misc/bluedroid /data/misc/bluetooth /data/misc/bluetooth/logs
$T chmod 770 /data/misc/bluedroid /data/misc/bluetooth /data/misc/bluetooth/logs
MACFILE=/data/misc/bluedroid/factory-address
valid_mac() { [ -f "$1" ] && $T grep -Eq '^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$' "$1"; }
# Retry until a valid address is saved: an empty or bad file must never stop Bluetooth for good.
if ! valid_mac "$MACFILE"; then
    NEW=/tmp/goya-bt-addr
    $T rm -f "$NEW"
    $T mkdir -p /tmp/goya-efs-bt
    if $T mount -t ext4 -o ro,noload /dev/block/mmcblk0p4 /tmp/goya-efs-bt; then
        $T cat /tmp/goya-efs-bt/bluetooth/bt_addr > "$NEW" 2>/dev/null || true
        $T umount /tmp/goya-efs-bt || true
    fi
    if ! valid_mac "$NEW"; then
        # EFS has no usable address: derive a stable, locally administered one from the serial number.
        SERIAL=$($T grep -o 'androidboot.serialno=[^ ]*' /proc/cmdline | $T cut -d= -f2)
        H=$(echo -n "goya-bt-$SERIAL" | $T md5sum | $T cut -c1-10)
        echo "02:$(echo $H | $T cut -c1-2):$(echo $H | $T cut -c3-4):$(echo $H | $T cut -c5-6):$(echo $H | $T cut -c7-8):$(echo $H | $T cut -c9-10)" > "$NEW"
        echo GOYA_BT_GENERATED_ADDRESS
    fi
    valid_mac "$NEW"
    $T cp "$NEW" "$MACFILE"
fi
valid_mac "$MACFILE"
$T chown 1002:1002 "$MACFILE"
$T chmod 600 "$MACFILE"
MAC=$($T cat "$MACFILE")
$T insmod /system/lib/modules/mbt8xxx.ko drv_mode=1 bt_mac="$MAC"
for node in /sys/class/rfkill/rfkill*; do
    if [ "$($T cat "$node/type")" = bluetooth ]; then
        $T chown 1002:1000 "$node/state"
        $T chmod 660 "$node/state"
    fi
done
echo BLUETOOTH_SETUP_READY
