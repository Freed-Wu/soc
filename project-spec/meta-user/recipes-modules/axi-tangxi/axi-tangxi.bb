SUMMARY = "Recipe for  build an external axi-tangxi Linux kernel module"
SECTION = "PETALINUX/modules"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${FILE_DIRNAME}/../../../../LICENSE;md5=1ebbd3e34237af26da5dc08a4e440464"

inherit module

INHIBIT_PACKAGE_STRIP = "1"

SRC_URI = "file://${FILE_DIRNAME} \
	"

S = "${WORKDIR}/${FILE_DIRNAME}"
