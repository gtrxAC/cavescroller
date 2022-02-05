#!/bin/sh
cc src/main.c lib/win/libraylib.a -Llib/win -Iraylib/src -o cavescroller.exe \
	-DPLATFORM=PLATFORM_DESKTOP -DPLATFORM_DESKTOP \
	-lopengl32 -lgdi32 -lwinmm -Wl,--subsystem,windows -Ofast