#!/usr/bin/env bash
cd "$(dirname "$(readlink -f "$0")")/.." || exit 1

cp images/linux/image.ub images/linux/BOOT.BIN images/linux/boot.scr /run/media/$USER/${1:-BOOT}
# use sudo to avoid wrong privilege
sudo rm -r /run/media/$USER/${2:-root}/*
sudo tar vxaCf /run/media/$USER/${2:-root} images/linux/rootfs.tar.gz
sync
