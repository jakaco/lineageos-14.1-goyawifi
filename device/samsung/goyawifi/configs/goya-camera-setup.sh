#!/system/bin/sh
set -e
T=/system/bin/toybox
$T chown 1047:1006 /dev/video0
$T chmod 660 /dev/video0
# Graphics also uses ION; add camera through the graphics supplementary group.
$T chown 0:1003 /dev/ion
$T chmod 660 /dev/ion
# Give the unprivileged codec service access only to the CODA UIO node and ION.
if [ -c /dev/uio0 ] && [ "$(cat /sys/class/uio/uio0/name)" = pxa-coda7542 ]; then
    $T chown 1046:1046 /dev/uio0
    $T chmod 660 /dev/uio0
    $T chown 1046:1003 /dev/ion
fi
echo CAMERA_SETUP_READY
