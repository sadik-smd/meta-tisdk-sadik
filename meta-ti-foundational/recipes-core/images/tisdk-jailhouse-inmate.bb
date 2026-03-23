SUMMARY = "TI SDK minimal image for Jailhouse Linux inmate demo"

DESCRIPTION = "Minimal TI SDK image for basic boot of Linux inmate for Jailhouse. \
This image is derived from arago-jailhouse-inmate and includes additional packages \
for out-of-box demo.\
"

require recipes-core/images/tisdk-tiny-initramfs.bb

PACKAGE_INSTALL += "jailhouse-inmate jailhouse-oob perf"
