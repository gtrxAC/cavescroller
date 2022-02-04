#!/bin/sh
cc src/main.c lib/linux/libraylib.a -Llib/linux -Iraylib/src -o cavescroller \
	-DPLATFORM=PLATFORM_DESKTOP -DPLATFORM_DESKTOP \
	-lGL -lm -lpthread -ldl -lrt -lX11 -Ofast