SUMMARY = "AM62P Drone GCS image - QGC + MAVLink middleware stack"
DESCRIPTION = "Full drone ground control station image for the AM62P. \
Extends tisdk-default-image with QGroundControl, mavlink-router, and the \
supporting middleware stack for MAVLink routing, RC/CRSF bridging, \
RTK-GNSS corrections, and GStreamer H.264/H.265 video."

require recipes-core/images/tisdk-default-image.bb

COMPATIBLE_MACHINE = "am62pxx"

IMAGE_INSTALL:append = " \
    \
    mavlink-router \
    crsf2mavlink \
    \
    qgroundcontrol \
    \
    gpsd \
    gpsd-machine-conf \
    \
    gstreamer1.0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav \
    gstreamer1.0-rtsp-server \
    ti-gst-plugins \
    ti-gst-utils \
    \
    sdl2 \
    \
    openssl \
    ca-certificates \
    \
    iw \
    iproute2 \
    net-tools \
"
