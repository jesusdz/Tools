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

INCLUDES=-I"../../"

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

	echo "Pushd..."
	pushd obj
	echo ""

	echo "android_native_app_glue.o:"
	echo ${GCC} -fPIC -c ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c -o android_native_app_glue.o
	${GCC} -fPIC -c ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c -o android_native_app_glue.o
	echo ""

	echo "android_native_app_glue.a:"
	echo ${AR} rcs libandroid_native_app_glue.a android_native_app_glue.o
	${AR} rcs libandroid_native_app_glue.a android_native_app_glue.o
	echo ""

	echo "main.o:"
	echo ${GXX} -I${NATIVE_APP_GLUE_DIR} -std=gnu++11 -fPIC ${INCLUDES} -c ../jni/main.cpp -o main.o
	${GXX} -I${NATIVE_APP_GLUE_DIR} -std=gnu++11 -fPIC ${INCLUDES} -c ../jni/main.cpp -o main.o
	echo ""

	echo "libgame.so:"
	echo ${GXX} main.o -L. -landroid_native_app_glue -u ANativeActivity_onCreate -landroid -lEGL -lGLESv1_CM -llog -shared -o ../${OUT_LIB_DIR}/libgame.so
	${GXX} main.o -L. -landroid_native_app_glue -u ANativeActivity_onCreate -landroid -lEGL -lGLESv1_CM -llog -shared -o ../${OUT_LIB_DIR}/libgame.so
	echo ""

	echo "Popd..."
	popd
	echo ""

fi


##########################################################
# Make the APK (Android PacKage)
##########################################################

if [[ ${target} = "apk" ]]; then
	echo Building the APK

    # ----------------------------------------------------
    # Start with a fresh bin directory
	rm -rf bin
	mkdir bin

	# ----------------------------------------------------
	# Generate the R.java file into the src directory

	${AAPT} package -v -f -m -S res -J src -M AndroidManifest.xml -I ${ANDROID_PLATFORM_DIR}/android.jar

	# ----------------------------------------------------
	# Compile java code from 'src' to 'obj' directory

	${JAVAC} -source 1.7 -target 1.7 -d obj -classpath ${ANDROID_PLATFORM_DIR}/android.jar -sourcepath src src/${APPLICATION_REL_PATH}/R.java
	${JAVAC} -source 1.7 -target 1.7 -d obj -classpath ${ANDROID_PLATFORM_DIR}/android.jar -sourcepath src src/${APPLICATION_REL_PATH}/MainActivity.java

	# ----------------------------------------------------
	# Compile java bytecode to dex bytecode. Reference
	# https://developer.android.com/studio/command-line/d8

	cp ${BUILD_TOOLS}/lib/d8.jar ${BUILD_TOOLS}/d8.jar
	${D8} obj/${APPLICATION_REL_PATH}/* --classpath ${ANDROID_PLATFORM_DIR}/android.jar --output bin/
	rm ${BUILD_TOOLS}/d8.jar

	# ----------------------------------------------------
	# Generate the first unsigned version of the APK

	${AAPT} package -v -f -M AndroidManifest.xml -S res -I ${ANDROID_PLATFORM_DIR}/android.jar -F bin/NativeActivity.unaligned.apk bin/

	# ------------------------------------------------------
	# Add the native activity and other libs to the APK

	${AAPT} add bin/NativeActivity.unaligned.apk "${OUT_LIB_DIR}/libgame.so"
	cp "${TOOLCHAIN_LIB_DIR}/libc++_shared.so" ${OUT_LIB_DIR}
	${AAPT} add bin/NativeActivity.unaligned.apk "${OUT_LIB_DIR}/libc++_shared.so"


	# ------------------------------------------------------
	# Align the resources of the final APK to 4 bytes

	${ZIPALIGN} -v -f 4 bin/NativeActivity.unaligned.apk bin/NativeActivity.apk #|| goto ERROR


	# ------------------------------------------------------
	# Create a key and sign the APK with it

	if [ ! -f "Tools.keystore" ]; then
		${KEYTOOL} -genkeypair -validity 1000 -dname "CN=tools,O=jesusdz,C=ES" -keystore Tools.keystore -storepass pass4tools -keypass pass4tools -alias ToolsKey -keyalg RSA
	fi
	${APKSIGNER} sign --ks Tools.keystore --ks-key-alias ToolsKey --ks-pass pass:"pass4tools" bin/NativeActivity.apk

fi


##########################################################
# Install APK into the device
##########################################################

if [[ ${target} = "deploy" ]]; then
	echo Building the APK
	# -k argument would keep the data and cache directories
	${ADB} uninstall com.tools.game
	${ADB} install -r bin/NativeActivity.apk
fi


##########################################################
# Clean APK and compilation data
##########################################################

if [[ ${target} = "cleanall" ]]; then
	rm -rf bin obj lib 2>/dev/null
	rm -rf bin intermediates 2>/dev/null
	rm -rf src/com/tools/game/R.java
	rm -rf Tools.keystore
fi

if [[ ${target} = "clean" ]]; then
	rm -rf bin obj lib 2>/dev/null
fi

