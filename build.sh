#!/bin/bash
# ______________________________________________________________________________
#
#  Compile raylib project
#
#  - Linux                   ./build.sh
#  - Windows (w64devkit)     ./build.sh
#  - Windows (cross compile) TARGET=Windows_NT ./build.sh
#  - Web                     TARGET=Web ./build.sh
#  - Android                 TARGET=Android ./build.sh
#
#  - Debug                   DEBUG=1 ./build.sh
#  - Build and run           ./build.sh -r
# ______________________________________________________________________________
#

# Default build options, override options from the command line

# Platform, one of Windows_NT, Linux, Web, Android. Defaults to your OS.
[[ "$TARGET" = "" ]] && TARGET=`uname`

# Executable name, extension is added depending on target platform.
[[ "$NAME" = "" ]] && NAME="cavescroller"

# Compiler flags.
[[ "$FLAGS" = "" ]] && FLAGS=""

RELEASEFLAGS="-Os -flto -s"
DEBUGFLAGS="-O0 -g -Wall -Wextra -Wpedantic"

# ______________________________________________________________________________
#
#  Compile
# ______________________________________________________________________________
#
TYPEFLAGS=$RELEASEFLAGS
[[ "$DEBUG" != "" ]] && TYPEFLAGS=$DEBUGFLAGS

[[ -e lib/$TARGET ]] || ./setup.sh

case "$TARGET" in
	"Windows_NT")
		# change i686 to x86_64 for 64-bit build
		CC="i686-w64-mingw32-gcc"
		EXT=".exe"
		PLATFORM="PLATFORM_DESKTOP"
		TARGETFLAGS="-lopengl32 -lgdi32 -lwinmm -Wl,--subsystem,windows"
		;;

	"Linux")
		CC="gcc"
		PLATFORM="PLATFORM_DESKTOP"
		TARGETFLAGS="-lGL -lm -lpthread -ldl -lrt -lX11"
		;;

	"Web")
		CC="emcc"
		EXT=".html"
		PLATFORM="PLATFORM_WEB"
		TARGETFLAGS="-s ASYNCIFY -s USE_GLFW=3 -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 --shell-file src/shell.html --preload-file assets"
		source emsdk/emsdk_env.sh
		;;

	"Android")
		source android/build.sh
		exit
		;;

	*)
		echo "Unsupported OS $TARGET"
		exit 1
		;;
esac

$CC src/*.c -Iinclude -Llib/$TARGET -o $NAME$EXT \
	-lraylib -D$PLATFORM $FLAGS $TYPEFLAGS $TARGETFLAGS

# itch.io expects html5 games to be named index.html, the names of js/data/wasm
# files can stay the same
[[ "$TARGET" = "Web" ]] && mv $NAME.html index.html

# ______________________________________________________________________________
#
#  Run if -r was specified
# ______________________________________________________________________________
#
if [[ "$1" = "-r" ]]; then
	case "$TARGET" in
		"Windows_NT") ([[ `uname` = "Linux" ]] && wine $NAME$EXT) || $NAME$EXT;;
		"Linux") ./$NAME;;
		"Web") emrun index.html;;
	esac
fi