#!/bin/bash
source emsdk/emsdk_env.sh
cd raylib/src
make PLATFORM=PLATFORM_WEB || make PLATFORM=PLATFORM_WEB -e
mv libraylib.a ../../
make clean || make clean -e
cd ../..