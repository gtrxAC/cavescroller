#!/bin/bash

[[ ! -e lib ]] && mkdir lib
[[ ! -e lib/web ]] && mkdir lib/web

if [[ ! -e raylib ]]; then
	if command -v git > /dev/null; then
		git clone https://github.com/raysan5/raylib --depth 1
	else
		echo "raylib directory not found, download it from https://github.com/raysan5/raylib or install git"
		exit
	fi
fi

if [[ ! -e emsdk ]]; then
	if command -v git > /dev/null; then
		git clone https://github.com/emscripten-core/emsdk --depth 1
		cd emsdk
		./emsdk install latest
		./emsdk activate latest
		cd ..
	else
		echo "emsdk directory not found, see https://emscripten.org/docs/getting_started/downloads.html or install git"
		exit
	fi
fi

source emsdk/emsdk_env.sh
cd raylib/src
make PLATFORM=PLATFORM_WEB || make PLATFORM=PLATFORM_WEB -e
mv libraylib.a ../../lib/web
make clean || make clean -e
cd ../..