SUMMARY = "QGroundControl - Ground Control Station for MAVLink drones"
DESCRIPTION = "QGroundControl provides full flight control and mission planning \
for any MAVLink-enabled drone. Built for the AM62P drone GCS device with \
Qt6/QML UI rendered on Wayland via PowerVR GPU, GStreamer H.264/H.265 video \
pipeline using TI VPE hardware decode, and built-in RTK/NTRIP corrections."
HOMEPAGE = "https://qgroundcontrol.com"

# QGC is dual-licensed: Apache-2.0 for libraries, GPL-3.0 for the application
LICENSE = "Apache-2.0 & GPL-3.0-only"
LIC_FILES_CHKSUM = " \
    file://LICENSE-APACHE;md5=86d3f3a95c324c9479bd8986968f4327 \
    file://LICENSE-GPL;md5=1ebbd3e34237af26da5dc08a4e440464 \
"

COMPATIBLE_MACHINE = "am62pxx"
PACKAGE_ARCH = "${MACHINE_ARCH}"

PV = "daily+git"
BRANCH = "master"

SRC_URI = " \
    gitsm://github.com/mavlink/qgroundcontrol.git;protocol=https;branch=${BRANCH} \
    file://qgc-settings.ini \
    file://qgroundcontrol.service \
    file://qgroundcontrol-wrapper.sh \
"

# Last commit with Qt minimum_version 6.11.0, compatible with meta-qt6's 6.11.3.
# The next commit (d93624d3e) bumped the minimum to 6.12.0 which meta-qt6 does
# not yet provide. Update once meta-qt6 ships Qt 6.12+.
SRCREV = "c15cbabf62de79122d9d2802e2b9a0aaaf1ded79"

# QGC requires Qt 6.12.0 exactly (from build-config.json).
# Package names follow meta-qt6 conventions; -native suffix = host tools.
# No libxcb: arago distro is Wayland-only (no DISTRO_FEATURES x11).
DEPENDS = " \
    qtbase \
    qtbase-native \
    qttools-native \
    qtdeclarative \
    qtdeclarative-native \
    qtshadertools \
    qtshadertools-native \
    qtquick3d \
    qtquick3d-native \
    qtlocation \
    qtpositioning \
    qtmultimedia \
    qtserialport \
    qtimageformats \
    qtconnectivity \
    qtwebsockets \
    qtwayland \
    qtgraphs \
    qtscxml \
    qtsensors \
    qtspeech \
    qthttpserver \
    gstreamer1.0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav \
    libsdl2 \
    zlib \
    openssl \
    python3-native \
"

RDEPENDS:${PN} = " \
    qtdeclarative \
    qtdeclarative-qmlplugins \
    qtlocation \
    qtpositioning \
    qtmultimedia \
    qtserialport \
    qtimageformats \
    qtconnectivity \
    qtwebsockets \
    qtwayland \
    qtwayland-qmlplugins \
    qtgraphs \
    qtscxml \
    qtsensors \
    qtspeech \
    qthttpserver \
    gstreamer1.0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav \
    ti-gst-plugins \
    libsdl2 \
    openssl \
    mavlink-router \
    powervr-graphics \
"

inherit cmake pkgconfig systemd

OECMAKE_GENERATOR = "Ninja"

EXTRA_OECMAKE = " \
    -DCMAKE_BUILD_TYPE=Release \
    -DOE_QMAKE_PATH_EXTERNAL_HOST_BINS=${STAGING_BINDIR_NATIVE} \
    -DQT_HOST_PATH=${RECIPE_SYSROOT_NATIVE}${prefix_native} \
    -DQGC_CUSTOM_DIR=custom-example \
    -DQGC_BUILD_TESTING=OFF \
    -DQGC_STABLE_BUILD=OFF \
    -DPython3_EXECUTABLE=${RECIPE_SYSROOT_NATIVE}${bindir_native}/python3-native/python3 \
    -DPYTHON_EXECUTABLE=${RECIPE_SYSROOT_NATIVE}${bindir_native}/python3-native/python3 \
    -DQGC_QT_MAXIMUM_VERSION=${QT_VERSION} \
"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${UNPACKDIR}/qgroundcontrol-wrapper.sh ${D}${bindir}/qgroundcontrol

    # QGC CMake installs the binary as QGroundControl
    if [ -f ${B}/QGroundControl ]; then
        install -m 0755 ${B}/QGroundControl ${D}${bindir}/QGroundControl
    fi

    # QML resources and assets (present when not using Qt resource bundles)
    install -d ${D}${datadir}/qgroundcontrol
    if [ -d ${B}/resources ]; then
        cp -r ${B}/resources ${D}${datadir}/qgroundcontrol/
    fi

    # Pre-configured first-run settings (comm links, video URI, RTK caster)
    install -d ${D}${sysconfdir}/qgroundcontrol
    install -m 0644 ${UNPACKDIR}/qgc-settings.ini \
        ${D}${sysconfdir}/qgroundcontrol/QGroundControl.ini

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/qgroundcontrol.service \
        ${D}${systemd_system_unitdir}/qgroundcontrol.service
}

SYSTEMD_SERVICE:${PN} = "qgroundcontrol.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

FILES:${PN} = " \
    ${bindir}/qgroundcontrol \
    ${bindir}/QGroundControl \
    ${datadir}/qgroundcontrol \
    ${sysconfdir}/qgroundcontrol \
    ${systemd_system_unitdir}/qgroundcontrol.service \
"

# QGC bundles some Qt plugins and links things statically in release builds
INSANE_SKIP:${PN} += "already-stripped dev-so rpaths"

PR = "r7"
