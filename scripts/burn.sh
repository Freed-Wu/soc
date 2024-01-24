#!/usr/bin/env bash
cd "$(dirname "$(readlink -f "$0")")/.." || exit 1

boot=${1:-/run/media/$USER/BOOT}
root=${2:-/run/media/$USER/root}
cp images/linux/image.ub images/linux/BOOT.BIN images/linux/boot.scr "$boot"
# use sudo to avoid wrong privilege
sudo rm -r "$root"/*
sudo tar vxaCf "$root" images/linux/rootfs.tar.gz
sync
