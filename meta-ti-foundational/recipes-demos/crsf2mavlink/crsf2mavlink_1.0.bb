SUMMARY = "CRSF to MAVLink bridge daemon for AM62P drone GCS"
DESCRIPTION = "Reads CRSF (Crossfire / ExpressLRS) RC frames from a UART \
serial port and emits MAVLink RC_CHANNELS_OVERRIDE messages over UDP to \
mavlink-router on localhost:14540. Runs as a systemd service, started after \
mavlink-router. Configured via /etc/crsf2mavlink/crsf2mavlink.conf."
HOMEPAGE = "https://github.com/TexasInstruments"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e0f87c3fe67b5f5e08e80e6534c10dc9"

COMPATIBLE_MACHINE = "am62pxx"

PV = "1.0"

# All source is local - no upstream git repo
SRC_URI = " \
    file://LICENSE \
    file://crsf2mavlink.c \
    file://crsf.h \
    file://mavlink_bridge.h \
    file://CMakeLists.txt \
    file://crsf2mavlink.conf \
    file://crsf2mavlink.service \
"

S = "${UNPACKDIR}"

inherit cmake pkgconfig systemd

EXTRA_OECMAKE = "-DCMAKE_BUILD_TYPE=Release"

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${B}/crsf2mavlink ${D}${sbindir}/crsf2mavlink

    install -d ${D}${sysconfdir}/crsf2mavlink
    install -m 0644 ${UNPACKDIR}/crsf2mavlink.conf \
        ${D}${sysconfdir}/crsf2mavlink/crsf2mavlink.conf

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/crsf2mavlink.service \
        ${D}${systemd_system_unitdir}/crsf2mavlink.service
}

SYSTEMD_SERVICE:${PN} = "crsf2mavlink.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

FILES:${PN} = " \
    ${sbindir}/crsf2mavlink \
    ${sysconfdir}/crsf2mavlink/crsf2mavlink.conf \
    ${systemd_system_unitdir}/crsf2mavlink.service \
"

PR = "r0"
