#!/system/bin/sh
set -e
T=/system/bin/toybox
# Stock board configuration: AP master, codec slave. Applied before audioserver.
echo 1 > /proc/driver/ssp_master
echo 1 > /proc/driver/gssp_master
$T chown 0:1005 /dev/snd/*
$T chmod 660 /dev/snd/*
echo AUDIO_SETUP_READY
