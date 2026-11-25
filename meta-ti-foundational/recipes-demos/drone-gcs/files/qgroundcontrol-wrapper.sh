#!/bin/sh
# qgroundcontrol-wrapper.sh
#
# Launcher wrapper for QGroundControl on AM62P.
# Sets runtime environment variables and delegates to the QGC binary.

# Use pre-deployed settings if no user settings exist yet
if [ ! -f "${HOME}/.config/QGroundControl.org/QGroundControl.ini" ]; then
    mkdir -p "${HOME}/.config/QGroundControl.org"
    cp /etc/qgroundcontrol/QGroundControl.ini \
       "${HOME}/.config/QGroundControl.org/QGroundControl.ini"
fi

exec /usr/bin/QGroundControl "$@"
