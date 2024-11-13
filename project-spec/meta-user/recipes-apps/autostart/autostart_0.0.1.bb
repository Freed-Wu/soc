SUMMARY = "autostart"
DESCRIPTION = "Simple autostart application"
HOMEPAGE = "https://github.com/Freed-Wu/soc"
BUGTRACKER = "https://github.com/Freed-Wu/soc/issues"
SECTION = "petalinux/apps"
LICENSE = "GPL-3.0-only"
CVE_PRODUCT = ""
# must start from a file to avoid oelint's warning
LIC_FILES_CHKSUM = "file://${FILE_DIRNAME}/../../../../LICENSE;md5=1ebbd3e34237af26da5dc08a4e440464"
SRC_URI = "file://${FILE_DIRNAME}"
S = "${WORKDIR}/${FILE_DIRNAME}"

inherit meson pkgconfig update-rc.d

INITSCRIPT_NAME = "autostart"
INITSCRIPT_PARAMS = "start 99 S ."

FILES:${PN} += "${systemd_unitdir}/* ${libdir}/*"
BBCLASSEXTEND = ""
