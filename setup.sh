#!/bin/bash
_ () { [[ "${!1}" = "" ]] && export $1="$2"; }
# ______________________________________________________________________________
#
#  Set up raylib project
#
#  - Linux                   ./setup.sh
#  - Windows (w64devkit)     ./setup.sh
#  - Windows (cross compile) TARGET=Windows_NT ./setup.sh
#  - Web                     TARGET=Web ./setup.sh
#  - Android                 TARGET=Android ./setup.sh
# ______________________________________________________________________________
#

# Default to host platform
[[ "$TARGET" = "" ]] && TARGET=$(uname)

# Set up directory structure
mkdir -p include src lib assets lib/$TARGET

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
		make clean || make clean -e || rm -fv *.o
		cd ../..
		;;

	"Windows_NT")
		# Works on Windows (w64devkit) and Linux (mingw-w64)
		_ ARCH "x86_64"
		if command -v $ARCH-w64-mingw32-gcc > /dev/null; then
			cd raylib/src
			make CC=$ARCH-w64-mingw32-gcc AR=$ARCH-w64-mingw32-ar OS=Windows_NT || \
			make CC=$ARCH-w64-mingw32-gcc AR=$ARCH-w64-mingw32-ar OS=Windows_NT -e
			mv libraylib.a ../../lib/$TARGET
			cp raylib.h ../../include
			make clean || make clean -e || rm -fv *.o
			cd ../..
		else
			if [[ `uname` = "Linux" ]]; then
				if command -v apt > /dev/null; then
					sudo apt install mingw-w64
					source setup.sh
					exit
				else
					echo "Please install mingw-w64 using your package manager"
					exit 1
				fi
			else
				echo "Compiler for $ARCH not found, make sure you're using w64devkit.exe from https://github.com/skeeto/w64devkit/releases"
				exit 1
			fi
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
		make clean || make clean -e || rm -fv *.o
		cd ../..
		;;

	"Android")
		source android/setup.sh
		exit
		;;

	*)
		echo "Unsupported platform $TARGET"
		exit 1
		;;
esac