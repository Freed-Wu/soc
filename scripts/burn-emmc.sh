#!/usr/bin/env bash
set -e
cd "$(dirname "$(readlink -f "$0")")/.."

root=${2:-/run/media/$USER/root}
sudo cp images/linux/image.ub images/linux/BOOT.BIN images/linux/boot.scr "$root/home/root"
sudo cp images/linux/rootfs.tar.gz "$root"
sync
