#!/usr/bin/env bash
set -e
cd "$(dirname "$(dirname "$(readlink -f "$0")")")"

mkdir -p images/linux
scp ${1:-_server:Desktop/soc/}images/linux/image.ub ${1:-_server:Desktop/soc/}images/linux/BOOT.BIN ${1:-_server:Desktop/soc/}images/linux/boot.scr ${1:-_server:Desktop/soc/}images/linux/rootfs.tar.gz images/linux
shift || true
[[ $# == 0 ]] && set -- "/run/media/$USER"/*
exec scripts/burn.sh "$@"
