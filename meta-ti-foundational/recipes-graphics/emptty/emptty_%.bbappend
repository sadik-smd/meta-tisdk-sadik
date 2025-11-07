PR:append = "_tisdk_0"

# Disable emptty for tisdk-display-cluster-image
SYSTEMD_AUTO_ENABLE:${PN} = "${@oe.utils.conditional("DISPLAY_CLUSTER_ENABLE", "1", "disable", "enable", d)}"

# Always disable emptty for AM62SIP
SYSTEMD_AUTO_ENABLE:${PN}:am62xxsip-evm = "disable"

# Always disable emptty for AM62L
SYSTEMD_AUTO_ENABLE:${PN}:am62lxx-evm = "disable"

# Always disable emptty for AM64X
SYSTEMD_AUTO_ENABLE:${PN}:am64xx-evm = "disable"

# Always disable emptty for AM335X
#SYSTEMD_AUTO_ENABLE:${PN}:am335x-evm = "disable"

# Always disable emptty for AM437X
#SYSTEMD_AUTO_ENABLE:${PN}:am437x-evm = "disable"

# Always disable emptty for AM65X
#SYSTEMD_AUTO_ENABLE:${PN}:am65xx-evm = "disable"
