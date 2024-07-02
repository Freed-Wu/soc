# deep-space-detection

The repository contains:

- [A Linux driver](project-spec/meta-user/recipes-modules) which supports:
  - mmap: map a memory between user space and kernel space
  - ioctl:
    - copy a memory between kernel space (PS's storage) and a peripheral's
      storage (PL's storage)
    - write neural network accelerators' registers to drive it to process
- [A Linux application](project-spec/meta-user/recipes-apps) which
  will be started automatically when OS booted.
  - communicate with center board to download images
  - call driver to process images to get the processed bit streams
  - entropy encode the bit streams
  - communicate with center board to upload the bit streams

Linux driver and Linux application share `axitangxi_ioctl.h`.

The repository doesn't contain:

- the code to build FPGA's `*.bit`
- the code to train neural network and generate `weight.bin` and
  `quantify.bin`, please see <https://gitlab.com/iWave/iwave>

## Build for Local Machine

Dependencies:

- modules
  - [make](https://www.gnu.org/software/make)
  - [linux-headers](https://archlinux.org/packages/core/x86_64/linux-headers)
- apps
  - [meson](https://mesonbuild.com/) or [cmake](https://cmake.org)
  - [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
  - [google test](https://github.com/google/googletest): optional, for unit test

Some required binary files `*.bin` can be seen [here](#binary-files).

```sh
# modules
make
# apps
meson setup project-spec/meta-user/recipes-apps/autostart/build project-spec/meta-user/recipes-apps/autostart
meson compile -Cproject-spec/meta-user/recipes-apps/autostart/build
# or
cmake -Bproject-spec/meta-user/recipes-apps/autostart/build
cmake --build project-spec/meta-user/recipes-apps/autostart/build
```

## Cross Compile for Develop Board

Dependencies:

- [vitis](https://aur.archlinux.org/packages/vitis): contains
  [vivado](https://aur.archlinux.org/packages/vivado).
  - `source /opt/xilinx/vitis/settings.sh`
- [petalinux](https://aur.archlinux.org/packages/petalinux)
  - `source /opt/petalinux/settings.sh`
- [minicom](https://archlinux.org/packages/extra/x86_64/minicom) or
  [picocom](https://archlinux.org/packages/extra/x86_64/picocom): optional,
  serial debug

### Create a Project

<!-- markdownlint-disable MD013 -->

```sh
petalinux-create -tproject -nsoc --template zynqMP
cd soc
# download source code to `soc/`
git clone --depth=1 --bare https://github.com/ustc-ivclab/deep-space-detection .git
git config core.bare false
git reset --hard
```

<!-- markdownlint-enable MD013 -->

### Configure

#### Binary files

Before building, some required binary files can be downloaded from
[Release](https://github.com/ustc-ivclab/deep-space-detection/releases):

- `system.xsa`: a zip file compressing FPGA's `*.bit` and some generated tcl/c
  files, which is called as hardware description file
- `weight.bin`: weight factors of neural network
- `exp.bin`: gauss function factors for speeding up entropy encoding
- `cdf.bin`: cdf function factors for speeding up entropy encoding

Currently, `system.xsa` has hard encoded some coefficiences, so the following
files are unnecessary temporally:

- `quantify.bin`: quantification coefficiences

#### Third-party Files

To speed up building, you need to download:

- [u-boot-xlnx](https://github.com/Xilinx/u-boot-xlnx/tags)
- [linux-xlnx](https://github.com/Xilinx/linux-xlnx/tags)
- <https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html>:
  - [downloads](https://www.xilinx.com/member/forms/download/xef.html?filename=daily-downloads_2023.1_09121239.tar.gz)
  - [aarch64 sstate-cache](https://www.xilinx.com/member/forms/download/xef.html?filename=sstate_aarch64_2023.1_09121239.tar.gz)

Then extract them to some paths. Refer
[assets/configs/example/config](assets/configs/example/config) to modify these
paths.

```sh
install -Dm644 /the/path/of/system.xsa -t project-spec/hw-description
install -Dm644 /the/path/of/{weight,cdf,exp}.bin -t project-spec/meta-user/recipes-apps/autostart/assets/bin
# wait > 60 seconds
# for SD
scripts/config.sh assets/configs/config assets/configs/example/config assets/configs/sd/config
# for eMMC
scripts/config.sh assets/configs/config assets/configs/example/config assets/configs/emmc/config
```

### Build

Every time source code is modified, you need to rerun:

```sh
# on the first time it costs about 1 hour while it costs 2.5 minutes later
scripts/build.sh
```

### Burn

Assume your SD card has 2 parts:

1. BOOT: vfat
2. rootfs: ext4

If not, you can generate it by:

```sh
sudo scripts/create-disk.sh /dev/sdb assets/sfdisk/example.yaml
```

Dependencies:

- [util-linux](https://github.com/util-linux/util-linux)
- [dosfstools](https://github.com/dosfstools/dosfstools)
- [e2fsprogs](http://e2fsprogs.sourceforge.net)

Insert the SD card to your PC. For eMMC, make sure the SD card has been burn.

Run:

```sh
# wait > 10 seconds
scripts/burn.sh
```

Then:

For SD:

1. insert the SD card to your board
2. set switch to 0101
3. reset the board

For eMMC:

1. insert the SD card to your board
2. set switch to 0101
3. reset the board
4. run `post-burn-emmc.sh`
5. set switch to 0110
6. reset the board

## Usage

### PC

```sh
# run the following commands in different ttys
# create two connected fake serials
socat pty,rawer,link=/tmp/ttyS0 pty,rawer,link=/tmp/ttyS1
main -dt/tmp/ttyS0
master -t/tmp/ttyS1 /the/path/of/test.yuv
# display log
journalctl -tslave0 -tmaster -fn0
```

### Embedded

```sh
# run the following commands in different ttys
# open serial debug helper to see information
# assume your COM port is /dev/ttyUSB0
minicom -D/dev/ttyUSB0
# assume your COM port is /dev/ttyUSB1
master /the/path/of/test.yuv
# display log
journalctl -tmaster -fn0
```

## Debug

### App

Add [example kconfig](assets/configs/rootfsconfigs/Kconfig.user) to
[kconfig](project-spec/configs/rootfsconfigs/Kconfig.user).

Add the following code to [rootfs_config](project-spec/configs/rootfs_config):

```config
CONFIG_autostart-dbg=y
```

Rebuild and reburn it.

Connect board and PC by a WLAN, then add IP address in board and PC,
such as:

Board:

```sh
ip a add 192.168.151.112/24 dev eth0
gdbserver 192.168.151.112:6666 main
```

PC:

```sh
cgdb -darm-none-eabi-gdb
```

### Hardware description file

You can generate an example `system.xsa` which doesn't contain the code of
neural network.

Make sure `vivado` and `xsct` in your `$PATH`.

```sh
# wait about 4 minutes
hw/generate-xsa.tcl
# test, it can be skipped
hw/test-xsa.tcl
cp hw/build/project/system.xsa project-spec/hw-description
```

## Documents

- [图像压缩软件设计报告大纲](docs/resources/design-report.md): written by Cheng Ran
- [搭载板图像加速处理软件与中心机的串口传输协议](docs/resources/serial-transmission-protocol.md):
  written by Li Yunfei
- [基于 linux 系统上与 tangxi 芯片通信的 AXI 驱动](docs/resources/build.md): written by Wang Yang
- [数据格式](docs/resources/format.md): written by Tao Jie

## References

- Vivado
  - [vivado system level design entry](https://docs.xilinx.com/r/en-US/ug895-vivado-system-level-design-entry)
  - [vivado tcl commands](https://docs.xilinx.com/r/en-US/ug835-vivado-tcl-commands)
- Vitis
  - [vitis embedded](https://docs.xilinx.com/r/en-US/ug1400-vitis-embedded)
- PetaLinux
  - [petalinux-tools-reference-guide](https://docs.xilinx.com/r/en-US/ug1144-petalinux-tools-reference-guide/Menuconfig-Corruption-for-Kernel-and-U-Boot)
  - [Embedded-Design-Tutorials](https://xilinx.github.io/Embedded-Design-Tutorials/docs/2021.2/build/html/docs/Introduction/ZynqMPSoC-EDT/1-introduction.html)
  - [PetaLinux Yocto Tips](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842475/PetaLinux+Yocto+Tips)
- Linux Driver
  - [Linux-Device-Driver](https://github.com/d0u9/Linux-Device-Driver)
  - [Linux 设备驱动程序](<https://github.com/stelectronic/doc/blob/master/Linux%E8%AE%BE%E5%A4%87%E9%A9%B1%E5%8A%A8%E7%A8%8B%E5%BA%8F(%E4%B8%AD%E6%96%87%E7%89%88%E7%AC%AC%E4%B8%89%E7%89%88%E5%AE%8C%E7%BE%8E%E7%BC%96%E8%BE%91%E5%B8%A6%E4%BA%8C%E7%BA%A7%E4%B9%A6%E7%AD%BE).pdf>)
- Device Tree
  - [devicetree specification](https://devicetree-specification.readthedocs.io/)
  - [Device Tree Usage](https://elinux.org/Device_Tree_Usage)
