SUMMARY = "Simple autostart application"
SECTION = "PETALINUX/apps"
LICENSE = "GPL-3.0-only"
LIC_FILES_CHKSUM = "file://${FILE_DIRNAME}/../../../../LICENSE;md5=1ebbd3e34237af26da5dc08a4e440464"

SRC_URI = "file://${FILE_DIRNAME}"

S = "${WORKDIR}/${FILE_DIRNAME}"

inherit cmake pkgconfig

FILES:${PN} += "${systemd_unitdir}/* \
    ${libdir}/*"

FILES:${PN}-dev = ""
