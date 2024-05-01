# https://stackoverflow.com/a/73509979/16027269
SRC := $(patsubst %/,%,$(dir $(realpath $(lastword $(MAKEFILE_LIST)))))
DRIVER_NAME := $(notdir $(SRC))
$(DRIVER_NAME)-objs := axi_tangxi_platform.o axitangxi_dev.o axi_tangxi.o
ifneq ($(KERNELRELEASE),)
	ccflags-y += -g -DDEBUG
	obj-m := $(DRIVER_NAME).o
else
# https://docs.yoctoproject.org/1.9/ref-manual/ref-manual.html#var-KERNEL_SRC
ifeq ($(KERNEL_SRC),)
ifeq ($(shell test -d /usr/src/linux || echo 0),)
	KERNEL_SRC := /usr/src/linux
else
	KERNEL_SRC := /lib/modules/$(shell uname -r)/build
# https://nixos.wiki/wiki/Linux_kernel#Developing_out-of-tree_kernel_modules
ifeq ($(shell test -f /run/current-system/nixos-version || echo 0),)
	KERNEL_SRC := $(shell nix eval --impure --expr '"$${(import <nixpkgs> {}).linuxPackages_latest.kernel.dev}"')$(KERNEL_SRC)
endif
endif
endif
KERNEL_MAKE := $(MAKE) -C$(KERNEL_SRC) M=$(SRC)
RULES := all modules modules_prepare modules_check modules_install clean
.PHONY: default $(RULES)
default:
	$(KERNEL_MAKE) && $(MAKE) $(SRC)/compile_commands.json
$(RULES):
	$(KERNEL_MAKE) $@ && $(MAKE) $(SRC)/compile_commands.json
$(SRC)/compile_commands.json: $(patsubst %,$(SRC)/%,$($(DRIVER_NAME)-objs)) $(SRC)/modules.order
	$(KERNEL_SRC)/scripts/clang-tools/gen_compile_commands.py -d$(KERNEL_SRC) -o$(SRC)/compile_commands.json $(SRC)
endif
