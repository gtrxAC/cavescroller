#!/bin/bash

[[ ! -e lib ]] && mkdir lib
[[ ! -e lib/win ]] && mkdir lib/win

if [[ ! -e raylib ]]; then
	if command -v git > /dev/null; then
		git clone https://github.com/raysan5/raylib --depth 1
	else
		echo "raylib directory not found, download it from https://github.com/raysan5/raylib or install git"
		exit
	fi
fi

cd raylib/src
mingw32-make || mingw32-make -e
mv libraylib.a ../../lib/win
rm -rf *.o
cd ../..