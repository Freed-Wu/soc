#!/usr/bin/env bash
set -e
cd "$(dirname "$(readlink -f "$0")")/.."

boot=${1:-/run/media/$USER/BOOT}
root=${2:-/run/media/$USER/root}
cp images/linux/image.ub images/linux/BOOT.BIN images/linux/boot.scr "$boot"
# use sudo to avoid wrong privilege
sudo rm -rf "$root"/*
sudo tar vxaCf "$root" images/linux/rootfs.tar.gz
sync
