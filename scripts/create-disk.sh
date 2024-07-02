#!/usr/bin/env bash
set -e
cd "$(dirname "$(dirname "$(readlink -f "$0")")")"

umount "${1:-/dev/sdb}"* || true
sfdisk "${1:-/dev/sdb}" <"${2:-assets/sfdisk/example.yaml}"
mkfs.vfat "${1:-/dev/sdb}1"
# /dev/sdb2 contains `Targa image data - Color 3 x 15648 x 16 +17 +38112 - 1-bit alpha' data
# Proceed anyway? (y,N)
echo y | mkfs.ext4 "${1:-/dev/sdb}2"
fatlabel "${1:-/dev/sdb}1" BOOT
e2label "${1:-/dev/sdb}2" rootfs
