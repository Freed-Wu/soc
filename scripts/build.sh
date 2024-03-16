#!/usr/bin/env bash
set -e
cd "$(dirname "$(readlink -f "$0")")/.."

petalinux-build
petalinux-package --boot --u-boot --fpga --force
