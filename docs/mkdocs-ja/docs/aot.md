How to Use AOT
==============

Suika3 supports **Ahead-of-Time (AOT) compilation** of scripts.
That is, an app may run completely native code instead of as a bytecode interpreter.

The `suika3-aotcomp` command converts `.ray` scripts into **ANSI C source code**.
The generated `library.c` file will be compiled with the entire engine.

---

## 1. Modify `main.ray`

Because the scripts will be compiled into native code,
loading the runtime library is no longer needed.

Open `main.ray` and comment out the loadLibrary() calls.

Example:
```
// Suika.loadPlugin("system")
```

Please note that you should not call `Suika.loadPlugin()` outside the
`main.ray` file.

---

## 2. Generate C Source

To compile scripts into C source code, run:

```sh
suika3-aotcomp main.ray script1.ray script2.ray ...
```

This command generates the following file:
```
library.c
```

The generated file contains the compiled script library.

> [!TIPS]
> Specify all script files in the command line, including `main.ray`.

Example:
```
suika3-aotcomp main.ray system.ray scenario1.ray scenario2.ray
```

--

## 3. Replace the Engine Library

Copy the generated `library.c` file to the engine source tree:
```
external/PlayfieldEngine/src/library.c
```

Overwrite the existing file.

---

## 4. Build the Engine

Build the Suika3 project using CMake as usual.

The compiled scripts will now be linked into the engine binary.

### iOS

To build static binaries, type:
```
cmake --preset ios-device
cmake --preset ios-simulator
cmake --build --preset ios-device
cmake --build --preset ios-simulator
```

After that, copy the static libraries to your iOS project:
* Copy `build-ios-device/libsuika3.a` to `Suika3.xcframework/ios-arm64/libsuika3.a`
* Copy `build-ios-simulator/libsuika3.a` to `Suika3.xcframework/ios-arm64_x86_64-simulator/libsuika3.a`

Overwrite the existing file.

### Android

To build shared binaries, type:
```
cmake --preset android-arm64
cmake --preset android-arvm7
cmake --preset android-x86
cmake --preset android-x86_64
cmake --build --preset android-arm64
cmake --build --preset android-arvm7
cmake --build --preset android-x86
cmake --build --preset android-x86_64
```

After that, copy the shared libraries to your Android project:
* Copy `build-android-arm64/libsuika3.so` to `app/src/main/jniLibs/arm64-v8a/libplayfield.so`
* Copy `build-android-armv7/libsuika3.so` to `app/src/main/jniLibs/armeabi-v7a/libplayfield.so`
* Copy `build-android-x86/libsuika3.so` to `app/src/main/jniLibs/x86/libplayfield.so`
* Copy `build-android-x86_64/libsuika3.so` to `app/src/main/jniLibs/x86_64/libplayfield.so`

Overwrite the existing file.

### HarmonyOS NEXT

To build shared binaries, type:
```
cmake --preset openharmony-arm64
cmake --preset openharmony-x86_64
cmake --build --preset openharmony-x86
cmake --build --preset openharmony-x86_64
```

After that, copy the shared libraries to your HarmonyOS NEXT project:
* Copy `build-openharmony-arm64/libsuika3.a` to `entry/libs/arm64-v8a/libsuika3.a`
* Copy `build-openharmony-x86_64/libsuika3.a` to `entry/libs/x86_64/libsuika3.a`

Overwrite the existing file.

### Unity Plugin

To build shared binaries, type:
```
cmake --preset unity-win64
cmake --build --preset unity-win64
```

After that, copy the libraries to your Unity project:
* Copy `build-unity-win64/libsuika3.dll` to `Assets/Plugins/x86_64/libplayfield.dll`

Overwrite the existing file.

---

## Results

Scripts are embedded directly into the executable, providing:

* No JIT (for store review)
* No runtime script loading
* Faster startup
