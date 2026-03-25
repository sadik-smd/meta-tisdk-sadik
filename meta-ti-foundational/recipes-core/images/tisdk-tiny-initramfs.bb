SUMMARY = "TI SDK minimal image for initramfs"

DESCRIPTION = "Minimal TI SDK image for basic boot of Linux kernel. Intended as a bare system, \
this image does not package the kernel in the standard /boot folder in rootfs. Instead, it provides \
a base rootfs allowing the kernel to be deployed elsewhere (TFTP, separate boot partition, JTAG, etc.) \
and boot the image. This image is derived from arago-tiny-initramfs.\
"

require recipes-core/images/arago-tiny-initramfs.bb

# The jailhouse inmate runs as a standalone rootfs without a block device.
# The kernel is loaded separately by linux-loader.bin, so we list the boot
# packages explicitly without any kernel dependencies.

inherit core-image

# Enable passwordless root login (user types "root", no password asked)
IMAGE_FEATURES += "empty-root-password"

PACKAGE_INSTALL = " \
    base-files \
    base-passwd \
    busybox \
    netbase \
    shadow-base \
    update-alternatives-opkg \
"
