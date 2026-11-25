SUMMARY = "MAVLink Router - routes MAVLink packets between endpoints"
DESCRIPTION = "MAVLink Router distributes MAVLink messages between multiple \
endpoints (UART, UDP, TCP). Used as the central MAVLink broker on the AM62P \
drone GCS device, sitting between the telemetry radio, RC bridge, QGC, and \
any MAVSDK companion applications."
HOMEPAGE = "https://github.com/mavlink-router/mavlink-router"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=136e8d82ef1e53c3c2d60ee07fe8b3b4"

COMPATIBLE_MACHINE = "am62pxx"

PV = "4.0.0+git"
BRANCH = "master"

SRC_URI = " \
    gitsm://github.com/mavlink-router/mavlink-router.git;protocol=https;branch=${BRANCH} \
    file://mavlink-router.conf \
    file://mavlink-router.service \
"

# Update with: git ls-remote https://github.com/mavlink-router/mavlink-router master
SRCREV = "5e38c7a32391a1ba75cde05c61825257ad617411"

DEPENDS = " \
    meson-native \
    ninja-native \
    systemd \
"

inherit meson pkgconfig systemd

# Disable logging to stderr in release builds; keep systemd journal support
EXTRA_OEMESON = " \
    -Dsystemdsystemunitdir=${systemd_system_unitdir} \
    --buildtype=release \
"

do_install:append() {
    # Main config file
    install -d ${D}${sysconfdir}/mavlink-router
    install -m 0644 ${UNPACKDIR}/mavlink-router.conf \
        ${D}${sysconfdir}/mavlink-router/main.conf

    # Drop-in config directory for per-link overrides
    install -d ${D}${sysconfdir}/mavlink-router/config.d
}

SYSTEMD_SERVICE:${PN} = "mavlink-router.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

FILES:${PN} += " \
    ${sysconfdir}/mavlink-router/main.conf \
    ${sysconfdir}/mavlink-router/config.d \
    ${systemd_system_unitdir}/mavlink-router.service \
"

PR = "r0"
