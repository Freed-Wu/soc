#!/usr/bin/env bash
set -e
cd "$(dirname "$(dirname "$(readlink -f "$0")")")"

config="$(grep CONFIG_SUBSYSTEM_SDROOT_DEV project-spec/configs/config)"
config="${config#*\"}"
config="${config%\"}"

root=${2:-/run/media/$USER/root}
if [[ $config == /dev/mmcblk0p2 ]]; then
	# use sudo to avoid wrong privilege
	sudo install -Dm644 images/linux/image.ub images/linux/BOOT.BIN images/linux/boot.scr images/linux/rootfs.tar.gz -t "$root${3:-/home/root}"
else
	boot=${1:-/run/media/$USER/BOOT}
	cp images/linux/image.ub images/linux/BOOT.BIN images/linux/boot.scr "$boot"
	sudo rm -rf "$root"/*
	sudo tar vxaCf "$root" images/linux/rootfs.tar.gz
fi
sync
