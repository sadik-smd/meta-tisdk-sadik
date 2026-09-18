# Override upstream lvgl-demo-fb to use TI's lv_port_linux fork

FILESEXTRAPATHS:prepend := "${THISDIR}/lvgl-demo-fb:"

BRANCH = "legacy"
BRANCH:am62lxx-evm = "version_9.4"

SRC_URI:remove = "git://github.com/lvgl/lv_port_linux_frame_buffer.git;protocol=https;branch=release/v9.4;name=demo"
SRC_URI:remove = "git://github.com/lvgl/lvgl;protocol=https;branch=release/v9.4;tag=v9.4.0;name=lvgl;subdir=${BB_GIT_DEFAULT_DESTSUFFIX}/lvgl"

SRC_URI:append = " \
    gitsm://github.com/TexasInstruments/lv_port_linux.git;protocol=https;branch=version_9.4;name=demo \
    git://github.com/sadik-smd/lv_demos.git;protocol=https;branch=${BRANCH};name=lvdemos;destsuffix=git-lvdemos \
    file://ti-high-res.cfg \
"

SRCREV_demo    = "5448e939cde01be46684e4fdfc661f849603bbc2"
SRCREV_lvdemos = "83cfa027a07ee8ecc2cce24c161540a3232c8003"
SRCREV_lvdemos:am62lxx-evm = "8e41791a2dce961ff9e157b413dcea2217301146"
SRCREV_FORMAT = "demo_lvdemos"

LIC_FILES_CHKSUM = "file://LICENSE;md5=802d3d83ae80ef5f343050bf96cce3a4 \
                    file://lvgl/LICENCE.txt;md5=4570b6241b4fced1d1d18eb691a0e083"

DEPENDS += "alsa-lib curl libdrm libevdev libsdl2 libsdl2-image mosquitto pkgconfig-native python3-native python3-pcpp-native wayland wayland-native wayland-protocols"
RDEPENDS:${PN} += "alsa-tools alsa-utils analytics-demo-data ca-certificates curl mosquitto-clients nnstreamer tensorflow-lite"

EXTRA_OECMAKE += " \
    -DUSE_EXTERNAL_DEMOS=ON \
    -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
    -DFETCHCONTENT_SOURCE_DIR_LV_DEMOS_EXT=${UNPACKDIR}/git-lvdemos \
"

TARGET_CC_ARCH:append = " -ffile-prefix-map=${UNPACKDIR}/git-lvdemos=/usr/src/debug/${PN}/${PV}"
