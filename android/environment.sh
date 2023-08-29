#!/bin/sh

##########################################################
# Environment variables
##########################################################

# Check JAVA_HOME environment var exists
if [ -z ${JAVA_HOME} ]; then
	echo "Could not find environment var JAVA_HOME"
	exit -1
fi

# Check ANDROID_SDK_ROOT environment var exists
if [ -z ${ANDROID_SDK_ROOT} ]; then
	echo "Could not find environment var ANDROID_SDK_ROOT"
	exit -1
fi

PACKAGE_NAME=com.tools.game

# Directories
ANDROID_HOME=${ANDROID_SDK_ROOT}
BUILD_TOOLS=${ANDROID_HOME}/build-tools/33.0.2
PLATFORM_TOOLS=${ANDROID_HOME}/platform-tools
ANDROID_PLATFORM_DIR=${ANDROID_HOME}/platforms/android-33
APPLICATION_REL_PATH=com/tools/game
HOST=linux-x86_64
TARGET=aarch64-linux-android33
NDK=${ANDROID_HOME}/ndk/25.2.9519653
TOOLCHAIN=${NDK}/toolchains/llvm/prebuilt/${HOST}
TOOLCHAIN_LIB_DIR=${TOOLCHAIN}/sysroot/usr/lib/aarch64-linux-android
NATIVE_APP_GLUE_DIR=${NDK}/sources/android/native_app_glue
OUT_LIB_DIR=lib/arm64-v8a

# Tools
GCC="${TOOLCHAIN}/bin/clang -target ${TARGET} -march=armv8-a --sysroot=${TOOLCHAIN}/sysroot"
GXX="${TOOLCHAIN}/bin/clang++ -target ${TARGET} -march=armv8-a --sysroot=${TOOLCHAIN}/sysroot"
AR="${TOOLCHAIN}/bin/llvm-ar"
ADB=${PLATFORM_TOOLS}/adb
AAPT=${BUILD_TOOLS}/aapt
JAVAC=${JAVA_HOME}/bin/javac
D8=${BUILD_TOOLS}/d8
KEYTOOL=${JAVA_HOME}/bin/keytool
APKSIGNER=${BUILD_TOOLS}/apksigner
ZIPALIGN=${BUILD_TOOLS}/zipalign
LLDB_SERVER=${TOOLCHAIN}/lib64/clang/14.0.7/lib/linux/aarch64/lldb-server

