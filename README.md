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

Some binary files needed by this repository:

- `system.xsa`: a zip file compressing FPGA's `*.bit` and some generated tcl/c files
- `weight.bin`: weight factors of neural network
- `quantify.bin`: quantification coefficiences

The repository doesn't contain:

- the code to build FPGA's `*.bit`
- the code to train neural network and generate `weight.bin` and
  `quantify.bin`, please see <https://gitlab.com/iWave/iwave>

All documents:

- [图像压缩软件设计报告大纲](docs/resources/design-report.md): written by Cheng Ran
- [搭载板图像加速处理软件与中心机的串口传输协议](docs/resources/serial-transmission-protocol.md):
  written by Li Yunfei
- [基于 linux 系统上与 tangxi 芯片通信的 AXI 驱动](docs/resources/build.md): written by Wang Yang
- [数据格式](docs/resources/format.md): written by Tao Jie

## Build for Local Machine

Dependencies:

- modules
  - [make](https://www.gnu.org/software/make)
  - [linux-headers](https://archlinux.org/packages/core/x86_64/linux-headers)
- apps
  - [cmake](https://cmake.org)
  - [pkgconf](https://gitea.treehouse.systems/ariadne/pkgconf)
  - [google test](https://github.com/google/googletest): optional, for unit test

<!-- markdownlint-disable MD013 -->

```sh
# modules
make
# apps
cmake -Bbuild/cmake
# or
# cmake -Bbuild/cmake -DCMAKE_BUILD_TYPE=Debug -DDRY_RUN=ON -DMASTER_TTY=/tmp/ttyS0 -DSLAVE_TTY=/tmp/ttyS1
cmake --build build/cmake
```

<!-- markdownlint-enable MD013 -->

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
petalinux-create -t project -n soc --template zynqMP
cd soc
# download source code to `soc/`
git clone --depth=1 --bare https://github.com/ustc-ivclab/deep-space-detection .git
git config core.bare false
git reset --hard
```

<!-- markdownlint-enable MD013 -->

### Get a Hardware Description File `system.xsa`

You can download `system.xsa` from
[Release](https://github.com/ustc-ivclab/deep-space-detection/releases)
to `project-spec/hw-description`.

Or you can generate it by yourself:

```sh
# make sure `vivado` and `xsct` in your `$PATH`
# wait about 4 minutes for `hw/build/project/system.xsa`
hw/generate-xsa.tcl
# or
vivado -mode batch -source hw/generate-xsa.tcl
# test can be skipped
hw/test-xsa.tcl
# or
xsct hw/test-xsa.tcl
cp hw/build/project/system.xsa project-spec/hw-description
```

### Configure

To fasten building, you need download:

- [u-boot-xlnx](https://github.com/Xilinx/u-boot-xlnx/tags)
- [linux-xlnx](https://github.com/Xilinx/linux-xlnx/tags)
- <https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html>:
  - [downloads](https://www.xilinx.com/member/forms/download/xef.html?filename=daily-downloads_2023.1_09121239.tar.gz)
  - [aarch64 sstate-cache](https://www.xilinx.com/member/forms/download/xef.html?filename=sstate_aarch64_2023.1_09121239.tar.gz)

Then extract them.

```sh
petalinux-config
```

![petalinux-config](https://github.com/ustc-ivclab/.github/assets/32936898/e0ddb510-d0fb-4db8-92aa-d6feac9da9db)

- Linux Components Selection
  - u-boot
    - [x] ext-local-src
  - External u-boot local source settings
    - External u-boot local source path
      - `/the/path/of/downloaded/` <https://github.com/Xilinx/u-boot-xlnx>
  - linux-kernel
    - [x] ext-local-src
  - External linux-kernel local source settings
    - External linux-kernel local source path
      - `/the/path/of/downloaded/` <https://github.com/Xilinx/linux-xlnx>
- FPGA Manager
  - [x] Fpga Manager
- Image Packaging Configuration
  - Root filesystem type
    - [x] EXT4 (SD/eMMC/SATA/USB): image is too large for initrd.
  - Device node of SD device
    - `/dev/mmcblk1p2`: 2nd partition of SD card. eMMC is `/dev/mmcblk0`.
- Yocto Settings
  - Add pre-mirror url
    - pre-mirror url path
      - `file:///the/path/of/downloaded/downloads`
  - Local sstate feeds settings
    - local sstate feeds url
      - `/the/path/of/downloaded/sstate-cache/of/aarch64`

```sh
petalinux-config -c rootfs
```

- Image Features
  - [x] package-management
  - [x] serial-autologin-root

```sh
scripts/config.sh
```

### Build

```sh
# on the first time it costs about 1 hour while it costs 2.5 minutes later
scripts/build.sh
```

### Burn

```sh
# insert SD card to your PC
# assume your SD card has 2 part: BOOT (vfat) and root (ext4)
# wait > 10 seconds
scripts/burn.sh
# insert SD card to your board
# reset the board
```

## Usage

### PC

```sh
# create two connected fake serials
socat pty,rawer,link=/tmp/ttyS0 pty,rawer,link=/tmp/ttyS1
# display log
journalctl -tslave0 -tmaster -fn0
main
master
```

### Embedded

```sh
# open serial debug helper
# assume your COM port is /dev/ttyUSB1
minicom -D /dev/ttyUSB1
```

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
