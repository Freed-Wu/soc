#!/usr/bin/env xsct
# https://docs.xilinx.com/r/en-US/ug1400-vitis-embedded
cd [file dirname [info script]]
exec rm -r build/workspace
exec mkdir -p build/workspace
cd build/workspace

set platform platform
set app app
set hw ../project/system.xsa

setws
# `-out .` and `active` is unnecessary in most situations
platform create -name $platform -hw $hw -out . -proc psu_cortexa53_0 -os standalone -arch 64-bit
platform active $platform
platform config -extra-compiler-flags fsbl {-MMD -MP -Wall -fmessage-length=0 -DARMA53_64 -DFSBL_DEBUG_INFO_VAL=1 -Os -flto -ffat-lto-objects}
# build fsbl.elf
platform generate

app create -name $app -template {Empty Application (C++)} -lang c++
# -soft-link break `sysproj build`
# exec cmake -Bbuild
# catch {importsources -name $app -path ../src}
# importsources -name $app -path ../../src
# build $app.elf, called by `sysproj build`
# app build -name $app

# after building fsbl.elf, build BOOT.BIN
# please ignore:
#   readelf: Error: 'platform/export/platform/sw/fsbl.elf': No such file
sysproj build -name ${app}_system

# must set encoding switch: 0000 (JTAG)
# else
#   WARNING: [Xicom 50-100] The current boot mode is QSPI32.
#   Flash programming is not supported with the selected boot mode.If flash programming fails, configure device for JTAG boot mode and try again.
# bpremove -all
# else
#   ERROR: [Xicom 50-331] Timed out while waiting for FSBL to complete.
#   Problem in Initializing Hardware
#   Flash programming initialization failed.
exec program_flash -f ${app}_system/Debug/sd_card/BOOT.BIN -fsbl $platform/export/$platform/sw/$platform/boot/fsbl.elf -offset 0 -flash_type qspi-x8-dual_parallel -verify

exit

# for debug
connect
targets -set -filter {name =~ {Cortex-A53 #0}}
rst -processor -clear-registers

# fpga [glob $platform/hw/*.bit]
source $platform/hw/psu_init.tcl
# avoid Memory write error at 0x0. Blocked address 0x0. DDR controller is not initialized
psu_init
psu_ps_pl_isolation_removal
psu_ps_pl_reset_config
dow $app/Debug/$app.elf
bpadd &main
con
