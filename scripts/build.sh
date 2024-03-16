#!/usr/bin/env bash
cd "$(dirname "$(readlink -f "$0")")/.." || exit 1

petalinux-build
petalinux-package --boot --u-boot --fpga --force
