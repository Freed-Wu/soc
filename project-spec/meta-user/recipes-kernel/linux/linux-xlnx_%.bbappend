# FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# ERROR: linux-xlnx-6.1.5-xilinx-v2023.1+git999-r0 do_kernel_version_sanity_check: Package Version (6.1.5-xilinx-v2023.1+git999) does not match of kernel being built (6.1.30). Please update the PV variable to match the kernel source or set KERNEL_VERSION_SANITY_SKIP="1" in your recipe.
KERNEL_VERSION_SANITY_SKIP="1"
# SRC_URI:append = " file://bsp.cfg"
# KERNEL_FEATURES:append = " bsp.cfg"
