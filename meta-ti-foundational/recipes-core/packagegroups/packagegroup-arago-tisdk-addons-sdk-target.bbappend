PR:append = "_tisdk_0"

DEMOS = ""
DEMOS:append:am437x-evm = " ti-lvgl-demo"


UTILS:append:ti33x = " \
    opencv-dev \
"

UTILS:append:ti43x = " \
    opencv-dev \
"

EXTRA_LIBS:append:am335x-evm = " \
    pru-icss-dev \
    pru-icss-staticdev \
"

