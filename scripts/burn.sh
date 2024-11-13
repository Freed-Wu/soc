#!/usr/bin/env bash
set -e
cd "$(dirname "$(dirname "$(readlink -f "$0")")")"

root_dev=$(. scripts/get-root_dev.sh)
root=${2:-/run/media/$USER/root}
# emmc
if [[ $root_dev == /dev/mmcblk0p2 ]]; then
	# use sudo to avoid wrong privilege
	sudo install -Dm644 images/linux/image.ub images/linux/BOOT.BIN images/linux/boot.scr images/linux/rootfs.tar.gz -t "$root${3:-/home/root}"
# sd
elif [[ $root_dev == /dev/mmcblk1p2 ]]; then
	sudo rm -rf "$root"/*
	sudo tar vxaCf "$root" images/linux/rootfs.tar.gz
	boot=${1:-/run/media/$USER/BOOT}
	cp images/linux/image.ub images/linux/BOOT.BIN images/linux/boot.scr "$boot"
# initramfs
else
	boot=${1:-/run/media/$USER/BOOT}
	cp images/linux/BOOT.BIN "$boot"
fi
sync
