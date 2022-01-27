#!/bin/bash
source emsdk/emsdk_env.sh
emcc src/main.c libraylib.a -Iraylib/src -o index.html -Os -s ASYNCIFY -s USE_GLFW=3 -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 -DPLATFORM=PLATFORM_WEB -DPLATFORM_WEB --shell-file src/shell.html --preload-file assets