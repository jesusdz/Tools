# Android APKs with the commandline

## Downloads

Download and unzip the following files:

* [Java - JDK version of JavaSE](https://www.oracle.com/java/technologies/downloads/)
* [Commandline tools](https://developer.android.com/studio) (at the bottom of the page)
* [Android NDK](https://developer.android.com/ndk/downloads)
* [Android SDK Platform tools](https://developer.android.com/studio/releases/platform-tools)
* [System.loadLibrary() couldn't find native librery](https://stackoverflow.com/questions/27421134/system-loadlibrary-couldnt-find-native-library-in-my-case)
* [AAPT2](https://developer.android.com/studio/command-line/aapt2)
* [AAPT Issue](https://stackoverflow.com/questions/23522153/manually-aapt-add-native-library-so-to-apk)

## Java

Unzip java in a directory and set the `JAVA_HOME` environment var. Adding the `bin` directory to the `PATH` can also come in handy.

## Get aapt

Unzip the **Commandline tools** in any random directory.

Create an empty directory for the Android SDK / NDK / etc. It will be our `ANDROID_SDK_ROOT`.

Go to `<androd-sdk-dir>\cmdline-tools\bin` and execute:
```
sdkmanager.bat --sdk_root=%ANDROID_SDK_ROOT% --install
sdkmanager.bat --sdk_root=%ANDROID_SDK_ROOT% --list
sdkmanager.bat --sdk_root=%ANDROID_SDK_ROOT% --install cmdline-tools;latest
```

Now we have retrieved the latest **Commandline tools** and the old ones can be removed.
Also, because the new commandline tools are part of the directory structure managed by the sdk manager, there is no need to specify the `--sdk_root` anymore.

We can add the `%ANDROID_SDK_ROOT\cmdline-tools\latest\bin%` to our `PATH` environment var.

Now, let's install the latest build tools (that include `aapt`, `aapt2`, `zipalign`...):
```
sdkmanager --install build-tools;33.0.1
```

Now, the platform tools (which contains `adb`):
```
sdkmanager --install platform-tools
```

And finally, install the latest NDK and an SDK:
```
sdkmanager --install ndk;25.2.9519653
sdkmanager --install platforms;android-33
```

