How To Build Playfield Engine
=============================

Playfield Engine fully utilizes the CMake build system.

* Notes
    * Requires CMake 3.22 or later
    * Windows: Visual Studio 2022 (Community or higher, tested on x64 and arm64)
    * macOS: Tested on macOS 15 (Apple Silicon, Xcode required)
    * Linux: Tested on Ubuntu 22.04, 24.04 (X11 required)
    * A full build takes 10 seconds using 10 cores.

## Windows (Visual Studio)

### Prerequisite

* `Visual Studio 2022 Community` installed with C/C++ and CMake configured

### Steps

* Download the [Playfield Engine source code](https://github.com/awemorris/PlayfieldEngine/releases/tag/latest) and extract it.
* Open the top of the source code folder by Visual Studio.
* Choose the `VS2022 MSVC x64 Release` target. (Alternatively x86 and arm64 is available)
* Build the project.

The target file `out/build/windows-msvc-x64-release/playfield.exe` will be created.

---

## Windows (WSL2)

### Prerequisite

* A `Windows 11` PC with an Intel, AMD, or Arm64 processor
* A `WSL2` feature installed
* `Ubuntu` or `Debian` installed

### Steps

Open the terminal and type the following.

```
sudo apt-get install cmake mingw-w64

git clone --recursive https://github.com/awemorris/PlayfieldEngine.git
cd PlayfieldEngine
cmake --preset windows-mingw-x86_64
cmake --build --preset windows-mingw-x86_64
```

The target file `build-mingw-x86_64/playfield` will be created.

---

## Linux

### Prerequisite

On Debian, Ubuntu, or Raspberry Pi OS:
```
sudo apt-get install git cmake ninja-build build-essential libx11-dev libxpm-dev libasound2-dev mesa-common-dev
```

On RedHat, Rocky Linux, Fedora, etc.:
```
sudo dnf groupinstall "Development Tools" "Development Libraries"
sudo dnf install patch git cmake ninja-build libX11-devel libXpm-devel alsa-lib-devel mesa-libGL-devel
```

### Steps

Open the terminal and type the following.
```
git clone --recursive https://github.com/awemorris/PlayfieldEngine.git
cd PlayfieldEngine
cmake --preset linux
cmake --build --preset linux
```

The target file `build-linux/playfield` will be created.

---

## macOS

### Prerequisite

* A Mac with an Apple Silicon or Intel processor
* `macOS 15` installed
* `Xcode 16` installed

### Steps

Open the terminal and type the following.

```
git clone --recursive https://github.com/awemorris/PlayfieldEngine.git
cd PlayfieldEngine
cmake --preset macos
cmake --build --preset macos
```

The target `build-macos/Playfield.app` will be created.

---

## iOS

### Prerequisite

* A Mac with an Apple Silicon or Intel processor
* `macOS 15` installed
* `Xcode 16` installed

### Steps

* Pack assets into the `assets.arc` file.
* Download the [Playfield Engine binary](https://github.com/awemorris/PlayfieldEngine/releases/tag/latest) and extract it.
* Copy your `assets.arc` file into the `misc/ios/resources` folder.
* Open the `misc/ios` folder by Xcode.

---

## Android

### Prerequisite

* `Android Studio`

### Steps

* Download the [Playfield Engine binary](https://github.com/awemorris/PlayfieldEngine/releases/tag/latest) and extract it.
* Copy your asset files into the `misc/android/app/src/main/assets` folder.
* Open the `misc/android` folde by Android Studio

---

## WebAssembly

### Prerequisite

* `emsdk` installed.

### Steps

Open the terminal and type the following.

```
git clone --recursive https://github.com/awemorris/PlayfieldEngine.git
cd PlayfieldEngine
cmake --preset wasm
cmake --build --preset wasm
```

The target file `wasm/index.html` will be created.

---

## FreeBSD

### Prerequisite

* A `FreeBSD` machine
* `cmake` and `ninja` installed.

### Steps

Open the terminal and type the following.

```
git clone --recursive https://github.com/awemorris/PlayfieldEngine.git
cd PlayfieldEngine
cmake --preset freebsd
cmake --build --preset freebsd
```

The target file `build-freebsd/playfield` will be created.

---

## NetBSD

### Prerequisite

* A `NetBSD` machine
* `cmake` and `ninja` installed.

### Steps

Open the terminal and type the following.

```
git clone --recursive https://github.com/awemorris/PlayfieldEngine.git
cd PlayfieldEngine
cmake --preset netbsd
cmake --build --preset netbsd
```

The target file `build-netbsd/playfield` will be created.

---

## OpenBSD

### Prerequisite

* A `OpenBSD` machine
* `cmake` and `ninja` installed.

### Steps

Open the terminal and type the following.

```
git clone --recursive https://github.com/awemorris/PlayfieldEngine.git
cd PlayfieldEngine
cmake --preset openbsd
cmake --build --preset openbsd
```

The target file `build-openbsd/playfield` will be created.

---

## Unity Plugin

### Prerequisite

* Full build of `LLVM-21` installed.

### Steps

Open the terminal and type the following.

```
git clone --recursive https://github.com/awemorris/PlayfieldEngine.git
cd PlayfieldEngine
cmake --preset unity-win64
cmake --build --preset unity-win64
```

The target file `build-unity-win64/libplayfield.dll` will be created.

Note: Replace `win64` to one of `switch`, `ps5`, and `xbox`.
