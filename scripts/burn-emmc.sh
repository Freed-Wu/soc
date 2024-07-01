#!/usr/bin/env bash
set -e
cd "$(dirname "$(readlink -f "$0")")/.."

root=${2:-/run/media/$USER/root}
sudo install -Dm644 images/linux/image.ub images/linux/BOOT.BIN images/linux/boot.scr images/linux/rootfs.tar.gz -t "$root${3:-/home/root}"
sync
