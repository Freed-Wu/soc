#!/usr/bin/env bash
set -e
cd "$(dirname "$(dirname "$(readlink -f "$0")")")"

cp "${1:-/home/root}"/{BOOT.BIN,boot.scr,image.ub} /run/media/mmcblk0p1
rm -rf /run/media/mmcblk0p2/*
# busybox doesn't support
# tar vxaCf /run/media/mmcblk0p2 /home/root/rootfs.tar.gz
tar vxaf "${1:-/home/root}"/rootfs.tar.gz -C /run/media/mmcblk0p2
sync
