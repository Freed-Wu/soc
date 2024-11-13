#!/usr/bin/env bash
set -e
cd "$(dirname "$(dirname "$(readlink -f "$0")")")"

petalinux-build
kernel=--kernel
[[ -z "$(. scripts/get-root_dev.sh)" ]] || kernel=
# by default: --fsbl
# images/linux/zynqmp_fsbl.elf
# --u-boot
# images/linux/pmufw.elf
# images/linux/bl31.elf
# images/linux/system.dtb
# images/linux/u-boot.elf
# --fpga
# project-spec/hw-description/*.bit
# --kernel
# images/linux/image.ub
# images/linux/boot.scr
petalinux-package --boot --force --u-boot --fpga $kernel
