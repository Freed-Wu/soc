#!/usr/bin/env bash
set -e
cd "$(dirname "$(dirname "$(readlink -f "$0")")")"

root_dev="$(grep CONFIG_SUBSYSTEM_ROOTFS_EXT4=y project-spec/configs/config || true)"
[[ -z $root_dev ]] || root_dev="$(grep CONFIG_SUBSYSTEM_SDROOT_DEV project-spec/configs/config || true)"
root_dev="${root_dev#*\"}"
root_dev="${root_dev%\"}"
echo "$root_dev"
