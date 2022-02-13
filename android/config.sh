# What Android API version to target (raylib requires at least 16)
API_VERSION=29

# The developer and package name: com.$DEV_NAME.$PKG_NAME
DEV_NAME=gtrxac
PKG_NAME=cavescroller

# The name of the app shown in the launcher
APP_NAME=CaveScroller

# App version, version code should be incremented by 1 and version name is the
# human readable version
VERSION_CODE=2
VERSION_NAME=1.0.1

# portrait or landscape
SCREEN_ORIENTATION=landscape

# What architectures to build for
ABIS="armeabi-v7a arm64-v8a x86 x86_64"

SDK=$(pwd)/android/sdk/`uname`
NDK=$(pwd)/android/ndk/`uname`/android-ndk-r23b
JAVA=/usr/lib/jvm/default-java  # Change this if you're not on Linux or you installed Java somewhere else
BUILD=$(pwd)/android/build

BUILD_TOOLS=$SDK/build-tools/29.0.3
TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-x86_64
NATIVE_APP_GLUE=$NDK/sources/android/native_app_glue
AR=$TOOLCHAIN/bin/llvm-ar