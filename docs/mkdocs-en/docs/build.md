Suika3 Build Instruction
========================

Suika3 fully utilizes the CMake build system.

* Notes
    * Requires CMake 3.22 or later
    * Windows: Visual Studio 2022 or 2026
    * macOS: Tested on macOS 26 (Xcode required)
    * Linux: Tested on Ubuntu 24.04 (X11 or Wayland required)
    * A full build takes 5 seconds using Intel Core Ultra 5 228V

## Windows (Visual Studio)

### Prerequisite

* `Visual Studio 2022` installed with C/C++ and CMake configured

### Steps

* Clone the repository.
* Open the top of the source code folder by Visual Studio.
* Choose the `VS2022 MSVC x64 Release` target.
* Build the project.

The target file `out/build/windows-msvc-x64-release/suika3.exe` will be created.

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

git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset windows-mingw-x86_64
cmake --build --preset windows-mingw-x86_64
```

The target file `build-mingw-x86_64/suika3.exe` will be created.

---

## Linux (X11)

### Prerequisite

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

---

## Linux (Wayland)

### Prerequisite

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

---

## macOS

### Prerequisite

* A Mac with an Apple Silicon or Intel processor
* `macOS 15` installed
* `Xcode 16` installed

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset macos
cmake --build --preset macos
```

The target `build-macos/Suika3.app` will be created.

---

## iOS

### Prerequisite

* A Mac with an Apple Silicon or Intel processor
* `macOS 15` installed
* `Xcode 16` installed

### Steps

* Pack assets into the `assets.arc` file.
* Download the [Suika binary](https://github.com/suika3-community/suika/releases) and extract it.
* Copy your `assets.arc` file into the `misc/ios/resources` folder.
* Open the `misc/ios` folder by Xcode.

---

## Android

### Prerequisite

* `Android Studio`

### Steps

* Download the [Suika binary](https://github.com/suika3-community/suika/releases) and extract it.
* Copy your asset files into the `misc/android/app/src/main/assets` folder.
* Open the `misc/android` folde by Android Studio

---

## WebAssembly

### Prerequisite

* `emsdk` installed.

### Steps

Open the terminal and type the following.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
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
git clone https://github.com/suika3-community/suika3.git
cd suika3
cmake --preset freebsd
cmake --build --preset freebsd
```

The target file `build-freebsd/suika3` will be created.

---

## NetBSD

### Prerequisite

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

### Prerequisite

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

### Prerequisite

* Full build of `LLVM-21` installed.

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
