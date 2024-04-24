#!/usr/bin/env bash
set -e
cd "$(dirname "$(readlink -f "$0")")/.."

mkfs.vfat "${1:-/dev/sdb}1"
mkfs.ext4 "${1:-/dev/sdb}2"
fatlabel "${1:-/dev/sdb}1" BOOT
e2label "${1:-/dev/sdb}2" rootfs
