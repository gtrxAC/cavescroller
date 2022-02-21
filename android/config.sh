#!/bin/sh
# ______________________________________________________________________________
#
#  Build options for Android
#  Note: this script is sourced by android/setup.sh or android/build.sh
# ______________________________________________________________________________
#
# What Android API version to target
# Newer API versions don't seem to work for Android 5.1 and below
_ API_VERSION 31
_ MIN_API_VERSION 23

# The developer and package name: com.$DEV_NAME.$PKG_NAME
# Only one app with the same developer and package name can be installed at a time
_ DEV_NAME gtrxac
_ PKG_NAME cavescroller

# The name of the app shown in the launcher
_ APP_NAME CaveScroller

# App version, version code should be incremented by 1 and version name is the
# human readable version
_ VERSION_CODE 1
_ VERSION_NAME 1.0

# portrait or landscape
_ SCREEN_ORIENTATION landscape

# What architectures to build for
# arm64-v8a doesn't work but armeabi-v7a will work fine for 64-bit too
# error: 'aarch64': unable to pass LLVM bit-code files to linker
_ ABIS "armeabi-v7a x86 x86_64"

# Change the Java path if you're not on Linux or you installed Java somewhere else
_ SDK $(pwd)/android/sdk/`uname`
_ NDK $(pwd)/android/ndk/`uname`/android-ndk-r23b
_ JAVA /usr/lib/jvm/default-java
_ BUILD $(pwd)/android/build

_ BUILD_TOOLS $SDK/build-tools/29.0.3
_ TOOLCHAIN $NDK/toolchains/llvm/prebuilt/linux-x86_64
_ NATIVE_APP_GLUE $NDK/sources/android/native_app_glue
_ AR $TOOLCHAIN/bin/llvm-ar