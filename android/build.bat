@echo off


REM ######################################################
REM Environment variables
REM ######################################################

set ANDROID_HOME=C:\Users\jesus\Soft\android-sdk
set BUILD_TOOLS= %ANDROID_HOME%\build-tools\33.0.1
set PLATFORM_TOOLS=%ANDROID_HOME%\platform-tools
set ANDROID_PLATFORM_DIR=%ANDROID_HOME%\platforms\android-33
set APPLICATION_REL_PATH=com\tools\game
set HOST=windows-x86_64

REM clang.exe --print-targets to see a full list of targets
REM set TARGET=arm64-linux-android33
set TARGET=aarch64-linux-android33

REM NDK
set NDK=%ANDROID_HOME%\ndk\25.2.9519653
set TOOLCHAIN=%NDK%\toolchains\llvm\prebuilt\%HOST%
REM set GCC=%TOOLCHAIN%\bin\%TARGET%-clang
REM set GXX=%TOOLCHAIN%\bin\%TARGET%-clang++
set GCC=%TOOLCHAIN%\bin\clang --target=%TARGET% -march=armv8-a
set GXX=%TOOLCHAIN%\bin\clang++ --target=%TARGET% -march=armv8-a
set AR=%TOOLCHAIN%\bin\llvm-ar
set TOOLCHAIN_LIB_DIR=%TOOLCHAIN%\sysroot\usr\lib\aarch64-linux-android

REM Include dir for native_app_glue sources .h/.cpp
set NATIVE_APP_GLUE_DIR=%NDK%\sources\android\native_app_glue

set OUT_LIB_DIR=lib\arm64-v8a
REM set OUT_LIB_DIR=lib\armeabi-v7a
REM set OUT_LIB_DIR=lib\x86
REM set OUT_LIB_DIR=lib\x86_64

REM We avoid backslashes for the AAPT command
set AAPT_LIB_DIR=%OUT_LIB_DIR:\=/%



REM ######################################################
REM Target to build
REM ######################################################

set target=%1
if [%target%] == [] goto build
if "%target%" == "build" goto build
if "%target%" == "apk" goto apk
if "%target%" == "deploy" goto deploy
if "%target%" == "clean" goto clean
if "%target%" == "cleanall" goto cleanall

echo Invalid target
exit /b 0



REM ######################################################
REM Compile the C native library
REM ######################################################
: build

rmdir /S /Q obj lib 2> nul
mkdir obj lib 2> nul
mkdir %OUT_LIB_DIR% 2> nul

pushd obj

REM Have a look at this compile arguments
REM /Users/thomas/Documents/android-ndk-r5b/toolchains/arm-eabi-4.4.0/prebuilt/darwin-x86/bin/arm-eabi-g++ --sysroot=/Users/thomas/Documents/android-ndk-r5b/platforms/android-8/arch-arm -march=armv7-a -mfloat-abi=softfp -mfpu=neon -Wl,--fix-cortex-a8 -fno-exceptions -fno-rtti -nostdlib -fpic -shared -o GLmove.so -O3

echo %GCC% -fPIC -c %NATIVE_APP_GLUE_DIR%\android_native_app_glue.c -o android_native_app_glue.o
call %GCC% -fPIC -c %NATIVE_APP_GLUE_DIR%\android_native_app_glue.c -o android_native_app_glue.o

echo %AR% rcs libandroid_native_app_glue.a android_native_app_glue.o
call %AR% rcs libandroid_native_app_glue.a android_native_app_glue.o

echo %GXX% -I%NATIVE_APP_GLUE_DIR% --sysroot=%TOOLCHAIN%\sysroot -std=gnu++11 -fPIC -c ..\jni\main.cpp -o main.o
call %GXX% -I%NATIVE_APP_GLUE_DIR% --sysroot=%TOOLCHAIN%\sysroot -std=gnu++11 -fPIC -c ..\jni\main.cpp -o main.o

echo %GXX% main.o -L. -landroid_native_app_glue -u ANativeActivity_onCreate -landroid -lEGL -lGLESv1_CM -llog -shared -o ..\%OUT_LIB_DIR%\libgame.so
call %GXX% main.o -L. -landroid_native_app_glue -u ANativeActivity_onCreate -landroid -lEGL -lGLESv1_CM -llog -shared -o ..\%OUT_LIB_DIR%\libgame.so

popd

exit /b 0




REM ######################################################
REM Make the APK (Android PacKage)
REM ######################################################
: apk


REM ------------------------------------------------------
REM Start with a fresh bin directory

rmdir /Q /S bin 2> nul
mkdir bin 2> nul


REM ------------------------------------------------------
REM Generate the R.java file into the src directory

call %BUILD_TOOLS%\aapt package -v -f -m  -S res -J src -M AndroidManifest.xml -I %ANDROID_PLATFORM_DIR%\android.jar


REM ------------------------------------------------------
REM Compile java code from 'src' to 'obj' directory
REM  -source 1.7 -target 1.7
REM  -source 1.7 -target 1.7
call %JAVA_HOME%\bin\javac -source 1.7 -target 1.7 -d obj -classpath %ANDROID_PLATFORM_DIR%\android.jar -sourcepath src src\%APPLICATION_REL_PATH%\R.java
call %JAVA_HOME%\bin\javac -source 1.7 -target 1.7 -d obj -classpath %ANDROID_PLATFORM_DIR%\android.jar -sourcepath src src\%APPLICATION_REL_PATH%\MainActivity.java


REM ------------------------------------------------------
REM Compile java bytecode to dex bytecode. Reference:
REM https://developer.android.com/studio/command-line/d8

set JAVA_HOME_OLD=%JAVA_HOME%
set JAVA_HOME=C:\Users\jesus\Soft\jdk-1.8
call %BUILD_TOOLS%\d8 obj\%APPLICATION_REL_PATH%\* --classpath %ANDROID_PLATFORM_DIR%\android.jar --output bin\
set JAVA_HOME=%JAVA_HOME_OLD%


REM ------------------------------------------------------
REM Generate the first unsigned version of the APK

call %BUILD_TOOLS%\aapt package -v -f -M AndroidManifest.xml -S res -I %ANDROID_PLATFORM_DIR%\android.jar -F bin\NativeActivity.unaligned.apk bin\


REM ------------------------------------------------------
REM Add the native activity and other libs to the APK

call %BUILD_TOOLS%\aapt add bin\NativeActivity.unaligned.apk "%AAPT_LIB_DIR%/libgame.so"

copy "%TOOLCHAIN_LIB_DIR%\libc++_shared.so" %OUT_LIB_DIR%\
call %BUILD_TOOLS%\aapt add bin\NativeActivity.unaligned.apk "%AAPT_LIB_DIR%/libc++_shared.so"


REM ------------------------------------------------------
REM Align the resources of the final APK to 4 bytes

call %BUILD_TOOLS%\zipalign -v -f 4 bin\NativeActivity.unaligned.apk bin\NativeActivity.apk || goto ERROR


REM ------------------------------------------------------
REM Create a key and sign the APK with it

if not exist "Tools.keystore" call %JAVA_HOME%\bin\keytool -genkeypair -validity 1000 -dname "CN=tools,O=jesusdz,C=ES" -keystore Tools.keystore -storepass pass4tools -keypass pass4tools -alias ToolsKey -keyalg RSA
call %BUILD_TOOLS%\apksigner sign --ks Tools.keystore --ks-key-alias ToolsKey --ks-pass pass:"pass4tools" bin\NativeActivity.apk

exit /b 0




REM ######################################################
REM Install APK into the device
REM ######################################################
: deploy

call %PLATFORM_TOOLS%\adb install -r bin\NativeActivity.apk
exit /b 0




REM ######################################################
REM Clean APK and compilation data
REM ######################################################
: cleanall
echo Cleaning APK data...
rmdir /Q /S bin 2> nul

: clean
echo Cleaning compilation data...
rmdir /Q /S obj lib 2> nul

exit /b 0




: ERROR

echo The last command finished with error



