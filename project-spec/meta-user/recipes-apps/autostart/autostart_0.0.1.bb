SUMMARY = "autostart"
DESCRIPTION = "Simple autostart application"
HOMEPAGE = "https://localhost"
BUGTRACKER = "https://localhost"
SECTION = "petalinux/apps"
LICENSE = "GPL-3.0-only"
CVE_PRODUCT = ""
# nooelint: oelint.var.licenseremotefile
LIC_FILES_CHKSUM = "file://${FILE_DIRNAME}/../../../../LICENSE;md5=1ebbd3e34237af26da5dc08a4e440464"
SRC_URI = "file://${FILE_DIRNAME}"
S = "${WORKDIR}/${FILE_DIRNAME}"

inherit meson pkgconfig

FILES:${PN} += "${systemd_unitdir}/* ${libdir}/*"
# nooelint: oelint.var.filesoverride
FILES:${PN}-dev = ""
BBCLASSEXTEND = ""
