SUMMARY = "axi-tangxi"
DESCRIPTION = "Recipe for build an external axi-tangxi Linux kernel module"
HOMEPAGE = "https://localhost"
BUGTRACKER = "https://localhost"
SECTION = "petalinux/modules"
LICENSE = "GPLv2"
CVE_PRODUCT = ""
# nooelint: oelint.var.licenseremotefile
LIC_FILES_CHKSUM = "file://${FILE_DIRNAME}/../../../../LICENSE;md5=1ebbd3e34237af26da5dc08a4e440464"
SRC_URI = "file://${FILE_DIRNAME}"
S = "${WORKDIR}/${FILE_DIRNAME}"
KERNEL_MODULE_AUTOLOAD += "axi-tangxi"
BBCLASSEXTEND = ""

inherit module

INHIBIT_PACKAGE_STRIP = "1"
