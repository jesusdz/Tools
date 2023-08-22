#!/bin/sh

##########################################################
# Environment variables
##########################################################

SHELL=/bin/bash
HOST=linux-x86_64
TARGET=aarch64-linux-android33
SDK=/home/jesus/Soft/AndroidSDK
NDK=${SDK}/ndk/25.2.9519653
TOOLCHAIN=${NDK}/toolchains/llvm/prebuilt/${HOST}
GCC="${TOOLCHAIN}/bin/clang -target ${TARGET} -march=armv8-a"
GXX="${TOOLCHAIN}/bin/clang++ -target ${TARGET} -march=armv8-a"
AR="${TOOLCHAIN}/bin/llvm-ar"
TOOLCHAIN_LIB_DIR=${TOOLCHAIN}/sysroot/usr/lib/aarch64-linux-android

NATIVE_APP_GLUE_DIR=${NDK}/sources/android/native_app_glue
OUT_LIB_DIR=lib/arm64-v8a


##########################################################
# Target to build
##########################################################

target=$1
if [[ ${target} = "" ]]; then
	target="build"
fi


##########################################################
# Compile the C native library
##########################################################
if [[ ${target} = "build" ]]; then

	rm -rf obj lib
	mkdir obj lib
	mkdir lib/arm64-v8a

	pushd obj

	echo "Compiling android_native_app_glue.c..."
	${GCC} -fPIC -c ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c -o android_native_app_glue.o

	echo "Archiving android_native_app_glue.a..."
	${AR} rcs libandroid_native_app_glue.a android_native_app_glue.o

	echo "Compiling main.cpp..."
	${GXX} -I${NATIVE_APP_GLUE_DIR} --sysroot=${TOOLCHAIN}/sysroot -std=gnu++11 -fPIC -c ../jni/main.cpp -o main.o

fi


##########################################################
# Make the APK (Android PacKage)
##########################################################
if [[ ${target} = "aok" ]]; then
	echo Building the APK
	# TODO
fi


##########################################################
# Install APK into the device
##########################################################
if [[ ${target} = "deploy" ]]; then
	echo Building the APK
	adb install -r bin/NativeActivity.apk
fi


##########################################################
# Clean APK and compilation data
##########################################################
if [[ ${target} = "cleanall" ]]; then
	rm -rf bin obj lib 2>/dev/null
	rm -rf bin intermediates 2>/dev/null
fi

if [[ ${target} = "clean" ]]; then
	rm -rf bin obj lib 2>/dev/null
fi

