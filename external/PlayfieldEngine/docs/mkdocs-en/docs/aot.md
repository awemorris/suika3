How To Use AOT
==============

`Playfield Engine` supports **Ahead-of-Time (AOT) compilation** of scripts.
That is, an app may run as a complete native code instead of a bytecode interpreter.

The `playfield-aot` command converts `.pf` scripts into **ANSI C source**.
The generated `library.c` file will be compiled with the entire engine source.

---

## 1. Modify `main.pf`

Because the scripts will be compiled into native code,
the runtime library loading will be no longer needed.

Open `main.pf` and comment out the `Engine.loadLibrary()` calls.

Example:
```
// Engine.loadLibrary("system")
```

Please note that do not call `Engine.loadLibrary()` outside the
`main.pf` file for convenience.

---

## 2. Generate C Source

To compile scripts into C source code, run:

```sh
playfield-aot script1.pf script2.pf ...
```

This command generates the following file:
```
library.c
```

The generated file contains the compiled script library.

> [!TIPS]
> Specify all script files in the command line, including `main.pf`.

Example:
```
playfield-aot main.pf system.pf scenario1.pf scenario2.pf
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

Build the Playfield Engine project using CMake as usual.

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
* Copy `build-ios-device/libplayfield.a` to `Playfield.xcframework/ios-arm64/libplayfield.a`
* Copy `build-ios-simulator/libplayfield.a` to `Playfield.xcframework/ios-arm64_x86_64-simulator/libplayfield.a`

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
* Copy `build-android-arm64/libplayfield.so` to `app/src/main/jniLibs/arm64-v8a/libplayfield.so`
* Copy `build-android-armv7/libplayfield.so` to `app/src/main/jniLibs/armeabi-v7a/libplayfield.so`
* Copy `build-android-x86/libplayfield.so` to `app/src/main/jniLibs/x86/libplayfield.so`
* Copy `build-android-x86_64/libplayfield.so` to `app/src/main/jniLibs/x86_64/libplayfield.so`

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
* Copy `build-openharmony-arm64/libplayfield.a` to `entry/libs/arm64-v8a/libplayfield.a`
* Copy `build-openharmony-x86_64/libplayfield.a` to `entry/libs/x86_64/libplayfield.a`

Overwrite the existing file.

### Unity Plugin

To build shared binaries, type:
```
cmake --preset unity-win64
cmake --build --preset unity-win64
```

After that, copy the libraries to your Unity project:
* Copy `build-unity-win64/libsuika3.dll` to `Assets/Plugins/x86_64/libsuika3.dll`

Overwrite the existing file.

---

## Result

Scripts are embedded directly into the executable, providing:

* Faster startup
* No runtime script loading
* Easier deployment
