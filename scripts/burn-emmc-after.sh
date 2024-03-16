#!/usr/bin/env bash
set -e
cd "$(dirname "$(readlink -f "$0")")/.."

cp -r /home/root/* /run/media/mmcblk0p1
rm -rf /run/media/mmcblk0p2/*
# busybox doesn't support
# tar vxaCf /run/media/mmcblk0p2 /rootfs.tar.gz
tar vxaf /rootfs.tar.gz -C /run/media/mmcblk0p2
sync
