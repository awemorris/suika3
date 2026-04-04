Suika3 Build Instructions
========================

Suika3 fully utilizes the CMake build system.

* Notes
    * Requires CMake 3.22 or later
    * Windows: Visual Studio 2022 or 2026 required
    * macOS: Xcode 8.2.1 or later required
    * Linux: GCC 4.4 or later required (Clang is also supported)
    * A full build takes:
        * 20 seconds for Visual Studio 2026 on Intel Core i5 13400
        * 5 seconds for Linux on Intel Core i5 13400
        * 5 seconds for macOS on Apple M5

---

## Windows (Visual Studio 2026)

Visual Studio is the recommended build environment for Windows, and is used for the official binary.

### Prerequisites

* A `Windows 11` PC with an Intel, AMD, or Arm64 processor
* `Visual Studio 2026` installed with C/C++ and CMake configured

### Steps

* Clone the repository.
* Open the root of the source code folder in Visual Studio.
* Wait for the CMake configuration to complete.
* Choose the `VS2026 x64 Release` target.
* Build the project.

The target file `out/build/windows-vs2026-x64-release/suika3.exe` will be created.

---

## Windows (WSL2)

This method uses MinGW on WSL2 to create a Windows exe file.
Note that it is possible to build Suika3 using MinGW,
but it is not recommended due to false positives in antivirus software.

### Prerequisites

* A `Windows 11` PC with an Intel, AMD, or Arm64 processor
* The `WSL2` feature installed
* `Ubuntu` or `Debian` installed on WSL2

```
sudo apt-get install cmake ninja-build mingw-w64
```

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset windows-mingw-x86_64
cmake --build --preset windows-mingw-x86_64
```

The target file `build-mingw-x86_64/suika3.exe` will be created.

---

## Linux (X11)

### Prerequisites

* A `Linux` machine with any processor
* `X11` installed

On Debian, Ubuntu, or Raspberry Pi OS:
```
sudo apt-get install git cmake ninja-build build-essential libx11-dev libxpm-dev libasound2-dev mesa-common-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-x
```

On RedHat, Rocky Linux, Fedora, etc.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build libX11-devel libXpm-devel alsa-lib-devel mesa-libGL-devel gstreamer1.0-devel gstreamer1.0-plugins-base-devel
```

### Steps

Open the terminal and type the following.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-x11
cmake --build --preset linux-x11
```

The target file `build-linux-x11/suika3` will be created.

If you want to debug Suika3 with GDB, you can use the `linux-x11-debug` preset instead of `linux-x11`.

---

## Linux (Wayland)

Note that Wayland support is still experimental.
It is compatible with KDE, but has problems with showing windows on GNOME.

### Prerequisites

* A `Linux` machine with any processor
* `Wayland` installed

On Debian or Ubuntu:
```
sudo apt-get install git cmake ninja-build build-essential libwayland-dev wayland-protocols libegl1-mesa-dev libasound2-dev
```

On RedHat, Rocky Linux, Fedora, etc.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build wayland-devel wayland-protocols-devel mesa-libEGL-devel alsa-lib-devel
```

### Steps

Open the terminal and type the following.
```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset linux-wayland
cmake --build --preset linux-wayland
```

The target file `build-linux-wayland/suika3` will be created.

If you want to debug Suika3 with GDB, you can use the `linux-wayland-debug` preset instead of `linux-wayland`.

---

## macOS (App Bundle)

This method creates an app bundle for macOS, and is used for the official binary.

### Prerequisites

* A Mac with an Apple Silicon or Intel processor
* `macOS 11` or later installed
* `Xcode` installed

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos
cmake --build --preset macos
```

The target `build-macos/Suika3.app` will be created.

### DMG Packaging

If you want to distribute Suika3 for macOS, you need to create a DMG file with code signing and notirization.
You need to have a `Developer ID Application` certificate in the keychain and be logged in with an Apple Developer account on Xcode.
Please note that an app bundle distributed via a zip file cannot access outside of the app bundle, so we use DMG here.

```
cd macos

# Sign the app.
codesign --timestamp --options runtime --entitlements ../resources/macos/macos.entitlements --deep --force --sign "Developer ID Application" Suika3.app

# Notarize the app. (takes some time)
ditto -c -k --sequesterRsrc --keepParent Suika3.app Suika3.zip
xcrun notarytool submit Suika3.zip --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$APP_SECRET" --wait
xcrun stapler staple Suika3.app

# Create a DMG file.
mkdir tmp
cp -Rv Suika3.app tmp/Suika3.app
hdiutil create -fs HFS+ -format UDBZ -srcfolder tmp -volname Suika3 Suika3.dmg

# Sign the DMG file to allow access to files outside the bundle (avoid Gatekeeper issues).
codesign --sign "Developer ID Application" Suika3.dmg
```

---

## macOS (CLI)

This method creates a command-line interface (CLI) version of Suika3 for macOS.
It is useful for debugging and development, but is not recommended for distribution.

### Prerequisites

* A Mac with an Apple Silicon or Intel processor
* `macOS 11` or later installed
* `Xcode` installed

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos-cli
cmake --build --preset macos-cli
```

The target `build-macos/Suika3.app` will be created.

If you want to debug Suika3 with LLDB, you can use the `macos-cli-debug` preset instead of `macos`.

---

## iOS

### Prerequisites

* A Mac with an Apple Silicon or Intel processor
* `macOS 11` or later installed
* `Xcode` installed

### Steps

* Pack assets into the `assets.arc` file.
* Download [the official binary](https://github.com/suika3-community/suika/releases) and extract it.
* Copy your `assets.arc` file into the `misc/ios/resources` folder.
* Open the `misc/ios` folder in Xcode.
* Build and run.

### Build from Scratch

If you want to build from scratch, use `cmake --preset ios-device` or `cmake --preset ios-simulator`,
then copy the built `libsuika3.a` file into the `misc/ios/lib` folder, and open the `misc/ios` folder in Xcode.

---

## Android

### Prerequisites

* `Android Studio`

### Steps

* Download [the official binary](https://github.com/suika3-community/suika/releases) and extract it.
* Copy your asset files into the `misc/android/app/src/main/assets` folder.
* Open the `misc/android` folder in Android Studio.

### Build from Scratch

If you want to build from scratch, use `cmake --preset android-arm64`, then copy the built `libsuika3.so` file into the `misc/android/app/src/main/jniLibs/arm64-v8a` folder, and open the `misc/android` folder in Android Studio.

---

## WebAssembly

### Prerequisites

* `emsdk` installed. (Any OS will do.)

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset wasm
cmake --build --preset wasm
```

The target file `wasm/index.html` will be created.

### Testing

To run the app, place your `assets.arc` file in the `wasm` folder.

After that, type `python -m http.server` and open `http://localhost:8000` in a browser window.

On Windows, you can use `suika3-web.exe` instead of `python`. It runs a small web server and opens the browser automatically.

---

## FreeBSD

### Prerequisites

* A `FreeBSD` machine
* `cmake` and `ninja` installed.

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset freebsd
cmake --build --preset freebsd
```

The target file `build-freebsd/suika3` will be created.

---

## NetBSD

### Prerequisites

* A `NetBSD` machine
* `cmake` and `ninja` installed.

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset netbsd
cmake --build --preset netbsd
```

The target file `build-netbsd/suika3` will be created.

---

## OpenBSD

### Prerequisites

* A `OpenBSD` machine
* `cmake` and `ninja` installed.

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset openbsd
cmake --build --preset openbsd
```

The target file `build-openbsd/suika3` will be created.

---

## Unity Plugin

### Prerequisites

* Full build of `LLVM-22` installed.

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset unity-win64
cmake --build --preset unity-win64
```

The target file `build-unity-win64/libsuika3.dll` will be created.

Note: Replace `win64` to one of `switch`, `ps5`, and `xbox`.

To use a vendor official SDK, open `cmake/toolchains/unity-*.cmake` and replace `CMAKE_C_COMPILER`, `CMAKE_CXX_COMPILER` and `CMAKE_AR`.
