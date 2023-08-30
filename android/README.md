# Android APKs with the commandline

## Downloads

Download and unzip the following files:

* [Java Development Kit 17 - JDK](https://www.oracle.com/java/technologies/downloads/)
  - On Linux, downloaded from [OpenLogic](https://www.openlogic.com/openjdk-downloads)
* [Commandline tools](https://developer.android.com/studio) (at the bottom of the page)

## Some reference links links

* [Android NDK](https://developer.android.com/ndk/downloads)
* [Android SDK Platform tools](https://developer.android.com/studio/releases/platform-tools)
* [System.loadLibrary() couldn't find native librery](https://stackoverflow.com/questions/27421134/system-loadlibrary-couldnt-find-native-library-in-my-case)
* [AAPT2](https://developer.android.com/studio/command-line/aapt2)
* [AAPT Issue](https://stackoverflow.com/questions/23522153/manually-aapt-add-native-library-so-to-apk)
* [Debugging C++ of an APK using lldb from the console](https://stackoverflow.com/questions/53448796/debugging-c-code-of-an-android-app-using-lldb-from-the-console)
* [Debugging C++ of an APK using lldb from the console 2](https://stackoverflow.com/questions/53733781/how-do-i-use-lldb-to-debug-c-code-on-android-on-command-line)
* [lldb-server manual page](https://lldb.llvm.org/man/lldb-server.html)
* [lldb manual page](https://lldb.llvm.org/man/lldb.html)
* [Debugging Android Native with LLDB](https://www.lili.kim/2019/01/28/android/Debug%20Android%20Native%20with%20LLDB/)
* [Useful thread where the last question is key](https://stackoverflow.com/questions/53733781/how-do-i-use-lldb-to-debug-c-code-on-android-on-command-line)

## Java

Unzip java in a directory and set the `JAVA_HOME` environment var. Adding the `bin` directory to the `PATH` can also come in handy.

## Android SDK

Create an empty directory for the Android SDK. It will be our `ANDROID_SDK_ROOT`.

Unzip the **Commandline tools** in any random directory (there's no need for it to be within the Android SDK root, this is just a bootstrap step and we will remove these downloaded commandline tools afterwards).

Go to `cmdline-tools\bin` and execute:
```
sdkmanager.bat --sdk_root=%ANDROID_SDK_ROOT% --install
sdkmanager.bat --sdk_root=%ANDROID_SDK_ROOT% --list
sdkmanager.bat --sdk_root=%ANDROID_SDK_ROOT% --install "cmdline-tools;latest"
```

Now we have retrieved the latest **Commandline tools** and the old ones (the downloaded ones) can be removed.
Also, because the new commandline tools are part of the directory structure managed by the sdk manager, there is no need to specify the `--sdk_root` to execute the `sdkmanager` anymore.

We can add the `%ANDROID_SDK_ROOT\cmdline-tools\latest\bin%` to our `PATH` environment var.

Now, let's install the latest build tools (that include `aapt`, `aapt2`, `zipalign`...):
```
sdkmanager --install "build-tools;33.0.2"
```

Now, the platform tools (which contains `adb`):
```
sdkmanager --install platform-tools
```

And finally, install the latest NDK and an SDK:
```
sdkmanager --install "ndk;25.2.9519653"
sdkmanager --install "platforms;android-33"
```

## Debugging an APK

- Make sure your APK is debuggable:
  - Open your `AndroidManifest.xml` file.
  - The `application` tag needs the `android:debuggable="true"` attribute.
- Make the APK and install it in your device.
- Open the Developer Settings:
  - Press *Select debug app* and select your app here.
  - Enable the *Wait for debugger* option.

## Commands

```
adb shell pm dump com.tools.game | grep versionCode

Server:
(pc) LLDB_SERVER_DIR=${TOOLCHAIN}/lib64/clang/14.0.7/lib/linux/aarch64
(pc) pushd ${LLDB_SERVER_DIR}
(pc) ${ADB} push lldb-server /data/local/tmp
(pc) popd
(pc) adb shell
(device) cd /data/local/tmp
(device) ./lldb-server platform --listen "*:9999" --server

Client:
(pc) adb devices // Note down the name of the device
(pc) adb shell pidof com.tools.game // Note down the pid of the process
(pc) adb forward tcp:9999 tcp:9999
(pc) lldb.sh
(lldb) platform select remote-android
(lldb) platform connect connect://YTLNLJEYTGVGJBH6:9999
(lldb) platform process attach --pid <pid-number> // fails with "Operation not permitted"
```

### Server terminal
```
./debug.sh
```

### Client terminal 1
```
lldb
(lldb) platform select remote-android
(lldb) platform connect unix-abstract-connect:///com.tools.game/debug.socket
(lldb) attach <pid>
```

### Client terminal 2
```
adb forward tcp:12345 jdwp:<pid>
jdb -attach localhost:12345
```

### Client terminal 1 (again)
```
(lldb) continue
// Then the application crashes
```
