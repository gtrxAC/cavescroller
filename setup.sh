#!/bin/bash
# ______________________________________________________________________________
#
#  Set up raylib project
#
#  - Linux                   ./setup.sh
#  - Windows (w64devkit)     ./setup.sh
#  - Windows (cross compile) TARGET=Windows_NT ./setup.sh
#  - Web                     TARGET=Web ./setup.sh
# ______________________________________________________________________________
#

[[ "$TARGET" = "" ]] && TARGET=`uname`

# Set up directory structure
[[ -e include ]] || mkdir include
[[ -e src ]] || mkdir src
[[ -e lib ]] || mkdir lib
[[ -e assets ]] || mkdir assets
[[ -e lib/$TARGET ]] || mkdir lib/$TARGET

# ______________________________________________________________________________
#
#  Install or update raylib
# ______________________________________________________________________________
#
if [[ -e raylib ]]; then
	if command -v git > /dev/null; then
		cd raylib
		git pull
		cd ..
	fi
else
	if command -v git > /dev/null; then
		git clone https://github.com/raysan5/raylib --depth 1
	else
		echo "raylib directory not found, download it from https://github.com/raysan5/raylib or install git"
		exit 1
	fi
fi

# ______________________________________________________________________________
#
#  Build raylib
# ______________________________________________________________________________
#
case "$TARGET" in
	"Linux")
		cd raylib/src
		make || make -e
		mv libraylib.a ../../lib/$TARGET
		cp raylib.h ../../include
		make clean || make clean -e || rm -v *.o
		cd ../..
		;;

	"Windows_NT")
		if [[ `uname` = "Linux" ]]; then
			if ! command -v wine > /dev/null; then
				echo "Wine is required for cross compilation."
				exit 1
			fi

			if [[ ! -e w64devkit ]]; then
				wget https://github.com/skeeto/w64devkit/releases/download/v1.10.0/w64devkit-1.10.0.zip
				unzip w64devkit-*.zip
				rm w64devkit-*.zip
			fi

			wine w64devkit/bin/bash.exe -c \
				"cd $(pwd) && PATH=Z:$(pwd)/w64devkit/bin\;\$PATH TARGET=Windows_NT ./setup.sh"
		else
			cd raylib/src
			mingw32-make || mingw32-make -e
			mv libraylib.a ../../lib/$TARGET
			cp raylib.h ../../include
			mingw32-make clean || mingw32-make clean -e || rm -v *.o
			cd ../..
		fi
		;;

	"Web")
		if [[ ! -e emsdk ]]; then
			if command -v git > /dev/null; then
				git clone https://github.com/emscripten-core/emsdk --depth 1
				cd emsdk
				./emsdk install latest
				./emsdk activate latest
				cd ..
			else
				echo "emsdk directory not found, see https://emscripten.org/docs/getting_started/downloads.html or install git"
				exit 1
			fi
		fi

		[[ -e src/shell.html ]] || cp raylib/src/minshell.html src/shell.html

		source emsdk/emsdk_env.sh
		cd raylib/src
		make PLATFORM=PLATFORM_WEB || make PLATFORM=PLATFORM_WEB -e
		mv libraylib.a ../../lib/$TARGET
		cp raylib.h ../../include
		make clean || make clean -e || rm -v *.o
		cd ../..
		;;

	*)
		echo "Unsupported OS $TARGET"
		exit 1
		;;
esac