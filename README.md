Playfield Engine
================

[![License: zlib](https://img.shields.io/badge/License-zlib-blue.svg)](LICENSE)

`Playfield Engine` is a 2D game scripting runtime that works everywhere.
With scripts written in `Playfield Script`,
it can render graphics and play sounds seamlessly across platforms.

[Official Page](https://noctvm.io/playfield/)
 | 
[Getting Started](docs/mkdocs-en/docs/gettingstarted.md)
 |
[Japanese Page](https://noctvm.io/playfield-ja/)

## Download v0.8.16 now!

| Platform                                                  | Downloads                                                                                                                |
|-----------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------|
| <img src="docs/img/logo-win.png" align="middle"> Windows  | [Windows 64-bit Installer (For Windows 10/11)](https://noctvm.io/dl/playfield/playfield-installer-0.8.16.exe)            |
|                                                           | [Windows 64-bit ZIP (For Windows 10/11)](https://noctvm.io/dl/playfield/playfield-win64-0.8.16.zip)                      |
|                                                           | [Windows Arm64 ZIP (For Windows 11 Arm64)](https://noctvm.io/dl/playfield/playfield-winarm64-0.8.16.zip)                 |
|                                                           | [Windows 32-bit ZIP (For Windows XP)](https://noctvm.io/dl/playfield/playfield-win32-0.8.16.zip)                         |
|                                                           | [Windows Pack Tool (For All Windows)](https://noctvm.io/dl/playfield/playfield-pack-0.8.16.zip)                          |
| <img src="docs/img/logo-mac.png" align="middle"> Mac      | [macOS Arm64/Intel DMG (For macOS 10.12+)](https://noctvm.io/dl/playfield/Playfield-0.8.16.dmg)                          |
| <img src="docs/img/logo-linux.png" align="middle"> Linux  | [Linux x86_64 AppImage](https://noctvm.io/dl/playfield/Playfield-0.8.16-x86_64.AppImage)                                 |
|                                                           | [Linux aarch64 AppImage (For Raspberry Pi OS 64-bit)](https://noctvm.io/dl/playfield/Playfield-0.8.16-aarch64.AppImage)  |
| Others                                                    | [WebAssembly (HTML)](https://noctvm.io/dl/playfield/playfield-wasm-0.8.16.zip)                                           |
|                                                           | [iOS (Xcode)](https://noctvm.io/dl/playfield/playfield-ios-0.8.16.zip)                                                   |
|                                                           | [Android (Android Studio)](https://noctvm.io/dl/playfield/playfield-android-0.8.16.zip)                                  |
|                                                           | [HarmonyOS NEXT](https://noctvm.io/dl/playfield/playfield-openharmony-0.8.16.zip)                                        |
|                                                           | [Unity Plugin](https://noctvm.io/dl/playfield/playfield-unity-0.8.16.zip)                                                |
| Samples                                                   | [Sample Games](https://noctvm.io/dl/playfield/playfield-samples-0.8.16.zip)                                              |
| Source Code                                               | [Source Code](https://noctvm.io/dl/playfield/playfield-src-0.8.16.zip)                                                   |


## Feature Highlights

Built completely from scratch, Playfield Engine features:

- **Comfortable Scripting:**
    - Powered by Playfield Script, a C/JavaScript-like scripting language designed in-house.
    - Native performance with a built-in JIT compiler.
    - Write a script and run it instantly — **Jot and Run, Boom!**
- **Portable Rendering & Audio:**
    - Powered by StratoHAL, a portable game library also designed in-house.
    - Runs virtually everywhere: Windows, macOS, Linux, Chromebook, WebAssembly, iOS, Android, and even consoles.
    - Deploy to platforms you don't even own — **Go Far, Stay Close!**
- **Minimal Runtime:**
    - The runtime is only ~1.4 MB on Windows.
    - Perfect for mobile apps — **Little Core, Big Hits!**

![platforms](https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/platforms.png)

---

## Why Playfield Engine?

_"If a game engine could run anywhere, how would the world change?"_

Playfield Engine was born from this single question.

Creativity shouldn't depend on the power of your computer, where you
live, or what tools you have.  Playfield Engine was created with the
belief that every creator deserves space to explore, no matter their
surroundings.

It runs smoothly even on older or low-spec machines — if you have a
computer, there's a place for you to create.  It's a helpful companion
for creators everywhere — especially in places with fewer resources.
If you have something to make, Playfield Engine will be here to walk
with you.

Whether you're using a Raspberry Pi in a classroom, or using a
UN-issued Chromebook in a temporary shelter, Playfield Engine offers
gentle tools to help bring your ideas to life — in a way that feels
simple, thoughtful, and your own.

The goal is simple:
**To make game development accessible, thoughtful, and within reach
for everyone.**

**Dreams fuel it — and they're the only requirement.**

---

## Live Web Demo

These are the sample games that work on Web browsers.

[![Small Shoot](https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-4.png)](https://noctvm.io/g/4)

[![Heartbeat Clock](https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-2.png)](https://noctvm.io/g/2)

[![Mini Shoot](https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-3.png)](https://noctvm.io/g/3)

[![DON'T RUSH IN COMIKET](https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-1.png)](https://noctvm.io/g/1)

---

## Gallery

Playfield Engine running on Windows 11:

<img src="https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-win11.png" width="320" alt="Playfield Engine running on Windows 11">

Playfield Engine running on macOS:

<img src="https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-macos.png" width="320" alt="Playfield Engine running on macOS">

Playfield Engine running on Linux:

<img src="https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-linux.png" width="320" alt="Playfield Engine running on Linux">

Playfield Engine running on iOS simulator:

<img src="https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-ios.png" width="320" alt="Playfield Engine running on iOS simulator">

Playfield Engine running on Android emulator:

<img src="https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-android.png" width="320" alt="Playfield Engine running on Android emulator">

Playfield Engine running on HarmonyOS NEXT emulator:

<img src="https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-harmonyosnext.png" width="320" alt="Playfield Engine running on HarmonyOS NEXT emulator">

Playfield Engine running on Windows XP — because why not:

<img src="https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-xp.png" width="320" alt="Playfield Engine running on Windows XP">

Playfield Engine running on NetBSD — Of course it runs!: 

<img src="https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/screenshot-netbsd.png" width="320" alt="Playfield Engine running on NetBSD">

---

## Status

Actively developing on the 0.8.x branch. Stable 1.0.0 release planned.

Currently we are focusing on `C API` for use with derrived engines.

**The nightly builds actually work on:**

|Type       |OS              |Status       |Confirm Date |Confirmed On                        |
|-----------|----------------|-------------|-------------|------------------------------------|
|Desktop    |Windows         |✅ Works     |1 March 2026 |Windows 11 x64                      |
|           |macOS           |✅ Works     |1 March 2026 |macOS 26 Arm64                      |
|           |Linux           |✅ Works     |1 March 2026 |Ubuntu 24.04 LTS x86_64             |
|Mobile     |iOS             |✅ Works     |1 March 2026 |iPhone iOS 26                       |
|           |Android         |✅ Works     |22 Sep 2025  |Android 16                          |
|           |HarmonyOS NEXT  |✅ Initial   |9 March 2026 |DevEco Studio 6                     |
|BSD        |FreeBSD         |✅ Works     |22 Sep 2025  |FreeBSD 14.3 amd64                  |
|           |NetBSD          |✅ Works     |22 Sep 2025  |NetBSD 10.1 amd64                   |
|           |OpenBSD         |✅ Works     |22 Sep 2025  |OpenBSD 7.7 amd64                   |
|Others     |WebAssembly     |✅ Works     |22 Sep 2025  |Chrome                              |
|           |Chromebook      |✅ Works     |22 Sep 2025  |Chrome [Here](https://noctvm.io/w/) |
|           |Unity           |✅ Works     |22 Sep 2025  |Unity 6.2 Windows 11 x86_64         |

---

## Example

Here's a simple example that displays an image following the mouse
cursor. This shows the basic lifecycle:
`setup` (window config),
`start` (asset loading),
`update` (per-frame logic and texture loading), and
`render` (per-frame rendering).

```
// Called when the window is created.
func setup() {
    // Return the window configuration.
    return {
        width: 1280,
        height: 720,
        title: "My First Game"
    };
}

// Called once when the game starts.
func start() {
    // Create a white 100x100 texture.
    tex = Engine.createColorTexture({
        width: 100,
        height: 100,
        r: 255, g: 255, b: 255, a: 255
    });
}

// Called every frame before rendering.
func update() {
    posX = Engine.mousePosX;
    posY = Engine.mousePosY;
}

// Called every frame to render graphics.
func render() {
    Engine.draw({ texture: tex, x: posX, y: posY });
}
```

---

## Quick Start

Precompiled binaries are available for Windows, macOS, Linux, and Chromebook!

### 1. Download the latest binary:

- Download the `All-in-One` file at the top of this page.
- Extract the downloaded file.

### 2. Run a game script

- For Windows:
    - Double-click the `playfield.exe` file.
    - The game will be started.
- For macOS:
    - Open the `misc` folder.
    - Open the `macos` folder.
    - Open the `Playfield.dmg` file.
    - Copy the application inside the dmg file alongside the `main.pf` file.
    - Double-click the copied application file.
    - The game will be started.
- For Linux:
    - Open the `misc` folder.
    - Open the `linux` folder.
    - Copy the `PlayfieldEngine-x86_64.AppImage` file alongside the `main.pf` file.
    - Double-click the copied AppImage file.
    - The game will be started.
- For Chromebook:
    - Open [this URL](https://noctvm.io/w/) on the browser.
    - Press the `Open Folder` button.
    - Select the folder where the `main.pf` file exists.
    - The game will be started.

### 3. Edit a game script

- With Playfield Editor (Currently Windows only):
    - Double-click the `playfield-editor.exe` file to run the editor.
    - Open the `main.pf` file from the editor.
    - Edit the file.
    - Press Ctrl+R to run the game. (Press R key while holding Ctrl key down)
- With your favorite editor:
    - Prior to editing, copy the engine alongside the `main.pf` file.
    - Open your favorite editor and load the `main.pf` file.
    - Edit the file and save it.
    - Run the engine.

### 4. Explore more features

- [Getting Started](docs/mkdocs-en/docs/gettingstarted.md)
- [Playfield Script Syntax](docs/mkdocs-en/docs/syntax.md)
- [Playfield Script API](docs/mkdocs-en/docs/api.md)
- [How To Build Playfield Engine](docs/mkdocs-en/docs/build.md)

---

## Technical Overview

Playfield Engine is not merely a wrapper around SDL combined with a
scripting language. It features its own rendering and audio backends,
and its own scripting language, positioning it as a fully independent
game engine.

### Core Architecture

```
+----------------------------------------------------------+
|                       User Scripts                       |
+----------------------------------------------------------+
|                 Playfield Script Runtime                 |
+--------------------------------+-------------------------+
|                    Playfield C Runtime                   |
+--------------------------------+-------------------------+
|  StratoHAL (Rendering/Audio)   |    NoctLang VM (JIT)    |
+--------------------------------+-------------------------+
```

* **Scripting**:
  Integrates [NoctLang](https://github.com/awemorris/NoctLang), a
  tiny yet mighty language designed for game scripting.

* **Rendering**:
  Supports native DirectX 9/11/12, Metal, OpenGL, OpenGL ES, and WebGL for wide compatibility.

* **Audio**:
  Provides lightweight audio support through native DirectSound or XAudio2 (Windows),
  Audio Unit (macOS/iOS), ALSA (Linux), OSS (BSD), and other APIs.

### StratoHAL

StratoHAL originated as the codebase of a 2D game engine developed
since 2001, and has been open-sourced with a proven record of
exceptional stability. Evolving over a quarter century from the
Windows 9x era, StratoHAL has grown to support macOS, Linux, iOS,
Android, WebAssembly, and Unity. It has been running reliably on
smartphones for more than a decade. While SDL3 already exists as a
popular open-source alternative, StratoHAL covers the same major
platforms as SDL3 — and uniquely provides console support through
Unity without relying on any NDA-restricted code.

### Platform Support

|Platform Type  |OS / Platform       |Notes                                     |
|---------------|--------------------|------------------------------------------|
|Desktop        |Windows             |DirectX 12/11/9, DirectSound, XAudio2     |
|               |macOS               |Metal, Audio Unit                         |
|               |ChromeOS            |OpenGL, ALSA                              |
|               |Linux               |OpenGL, ALSA                              |
|               |*BSD                |OpenGL, OSS (/dev/dsp)                    |
|               |Qt                  |Qt OpenGL                                 |
|Mobile         |iOS                 |Metal, Audio Unit                         |
|               |Android             |OpenGL, OpenSL ES, NDK                    |
|               |HarmonyOS NEXT      |OpenGL, OpenSL ES                         |
|Web            |WebAssembly         |WebGL, OpenAL, via Emscripten             |
|Console        |Unity               |Unity Plugin                              |
|               |Xbox Series X\|S    |Xbox GDK Native (DirectX 12 + XAudio2)    |

### Console Unity Plugin Notice

Playfield Engine provides Unity Plugin binaries for platforms
including Windows 64-bit and game consoles.  These binaries are built
entirely with the open-source version of LLVM/Clang toolchain (no
proprietary SDKs).

For Xbox series, you can use the native Microsoft GDK port, not via Unity.

### NoctLang

```
Playfield Script = NoctLang + Playfield API
```

**NoctLang** is a lightweight scripting language designed specifically
for Playfield Engine. With a game-oriented syntax, it emphasizes
clarity, instant startup, and tight integration with the engine.

The built-in JIT compiler supports a wide range of CPU architectures,
including both 32-bit and 64-bit of: **Intel, Arm, RISC-V, PowerPC, MIPS.**

For platforms where JIT is restricted (such as mobile or consoles),
NoctLang can fall back to interpreter mode, and AOT (ahead-of-time)
compilation using a C source backend — ensuring full compatibility
even in tightly controlled environments.

### Script Execution Mode

|Platform       |Mode               |
|---------------|-------------------|
|Windows x86    |JIT                |
|Windows x64    |JIT                |
|Windows arm64  |JIT                |
|macOS x86_64   |JIT                |
|macOS arm64    |JIT                |
|Linux x86      |JIT                |
|Linux x86_64   |JIT                |
|Linux armv7    |JIT                |
|Linux arm64    |JIT                |
|FreeBSD x86_64 |JIT                |
|iOS            |Interpreter or AOT |
|Android        |Interpreter or AOT |
|HarmonyOS      |Interpreter or AOT |
|WebAssembly    |Interpreter or AOT |
|Unity Plugin   |Interpreter or AOT |

### Runtime Footprint

|Platform       |Binary Size    |Memory Usage (GC)                                              |
|---------------|---------------|---------------------------------------------------------------|
|All targets    |~2 MB          |Allocates 256 MB at startup for GC; does not exceed that limit |

The garbage collector allocates 256 MB at startup because currently 256 MB is the maximum heap size.

### Garbage Collection

Playfield Engine implements a high-performance garbage collector
similar to the HotSpot VM in Java, so you generally don’t need to
worry much about delays caused by garbage collection.

* The garbage collector uses a generational GC, managing objects in young and old generations.
* Young-generation GC runs extremely fast using a copying algorithm (semi-space copy GC).
* Old-generation GC uses a mark-and-sweep algorithm and performs compaction when necessary (mark-sweep-compact GC).

---

## Tests

Playfield Engine is tested on the following environments in the
development.

|Type       |OS              |Version |Architecture    |
|-----------|----------------|--------|----------------|
|Desktop    |Windows         |11      |x64             |
|           |                |        |arm64           |
|           |macOS           |15      |arm64           |
|           |                |12      |x86_64          |
|           |Ubuntu          |24.04   |x86_64          |
|           |                |        |arm64           |
|           |Kubuntu         |25.04   |x86_64          |
|Browser    |Chrome          |139     |WebAssembly     |
|           |Safari          |18.6    |WebAssembly     |
|Smartphone |iOS             |18      |Simulator       |
|           |Android         |16      |Simulator       |
|           |HarmonyOS NEXT  |20      |Simulator       |
|Console    |Unity           |6.2     |Windows x64     |

---

## CMake Presets

Playfield Engine ships with CMake presets covering various platforms
and build configurations.

|Preset                         |Platform         |Compiler   |Directory                              |Target           |Type           |
|-------------------------------|-----------------|-----------|---------------------------------------|-----------------|---------------|
|windows-vs2022-x86-debug       |Windows          |MSVC       |out/build/windows-vs2022-x86-debug     |playfield.exe    |Executable     |
|windows-vs2022-x86-release     |Windows          |MSVC       |out/build/windows-vs2022-x86-release   |playfield.exe    |Executable     |
|windows-vs2022-x64-debug       |Windows          |MSVC       |out/build/windows-vs2022-x64-debug     |playfield.exe    |Executable     |
|windows-vs2022-x64-release     |Windows          |MSVC       |out/build/windows-vs2022-x64-release   |playfield.exe    |Executable     |
|windows-vs2022-arm64-debug     |Windows          |MSVC       |out/build/windows-vs2022-arm64-debug   |playfield.exe    |Executable     |
|windows-vs2022-arm64-release   |Windows          |MSVC       |out/build/windows-vs2022-arm64-release |playfield.exe    |Executable     |
|windows-vs2022-gdk-desktop     |Windows          |MSVC       |out/build/windows-vs2022-gdk-desktop   |playfield.exe    |Executable     |
|windows-vs2022-gdk-xbox-xs     |Windows          |MSVC       |out/build/windows-vs2022-gdk-xbox-xs   |playfield.exe    |Executable     |
|windows-vs2026-x86-debug       |Windows          |MSVC       |out/build/windows-vs2026-x86-debug     |playfield.exe    |Executable     |
|windows-vs2026-x86-release     |Windows          |MSVC       |out/build/windows-vs2026-x86-release   |playfield.exe    |Executable     |
|windows-vs2026-x64-debug       |Windows          |MSVC       |out/build/windows-vs2026-x64-debug     |playfield.exe    |Executable     |
|windows-vs2026-x64-release     |Windows          |MSVC       |out/build/windows-vs2026-x64-release   |playfield.exe    |Executable     |
|windows-vs2026-arm64-debug     |Windows          |MSVC       |out/build/windows-vs2026-arm64-debug   |playfield.exe    |Executable     |
|windows-vs2026-arm64-release   |Windows          |MSVC       |out/build/windows-vs2026-arm64-release |playfield.exe    |Executable     |
|windows-vs2026-gdk-desktop     |Windows          |MSVC       |out/build/windows-vs2026-gdk-desktop   |playfield.exe    |Executable     |
|windows-vs2026-gdk-xbox-xs     |Windows          |MSVC       |out/build/windows-vs2026-gdk-xbox-xs   |playfield.exe    |Executable     |
|windows-mingw-x86              |Windows          |MinGW      |build-mingw-x86                        |playfield.exe    |Executable     |
|windows-mingw-x86_64           |Windows          |MinGW      |build-mingw-x86_64                     |playfield.exe    |Executable     |
|windows-mingw-arm64            |Windows          |MinGW-LLVM |build-mingw-arm64                      |playfield.exe    |Executable     |
|macos                          |macOS            |Clang      |build-macos                            |Playfield.app    |App Bundle     |
|linux-x11                      |Linux            |GCC        |build-linux                            |playfield        |Executable     |
|linux-wayland                  |Linux            |GCC        |build-linux                            |playfield        |Executable     |
|linux-gdm                      |Linux            |GCC        |build-linux                            |playfield        |Executable     |
|linux-gdm-rot90                |Linux            |GCC        |build-linux                            |playfield        |Executable     |
|freebsd                        |FreeBSD          |Clang      |build-freebsd                          |playfield        |Executable     |
|netbsd                         |NetBSD           |GCC        |build-netbsd                           |playfield        |Executable     |
|openbsd                        |OpenBSD          |Clang      |build-openbsd                          |playfield        |Executable     |
|wasm                           |WebAssembly      |Emscripten |build-wasm                             |index.html       |HTML + Wasm    |
|wasm-local                     |Chromebook       |Emscripten |build-wasm-local                       |index.html       |HTML + Wasm    |
|ios-device                     |iOS Device       |Clang      |build-ios-device                       |libplayfield.a   |Static Library |
|ios-simulator                  |iOS Simulator    |Clang      |build-ios-simulator                    |libplayfield.a   |Static Library |
|android-x86                    |Android x86      |Clang      |build-android-x86                      |libplayfield.so  |Shared Library |
|android-x86_64                 |Android x86_64   |Clang      |build-android-x86_64                   |libplayfield.so  |Shared Library |
|android-armv7                  |Android armv7    |Clang      |build-android-armv7                    |libplayfield.so  |Shared Library |
|android-arm64                  |Android arm64    |Clang      |build-android-arm64                    |libplayfield.so  |Shared Library |
|openharmony-arm64              |HarmonyOS arm64  |Clang      |build-openharmony-arm64                |libplayfield.a   |Static Library |
|openharmony-armv7              |HarmonyOS armv7  |Clang      |build-openharmony-armv7                |libplayfield.a   |Static Library |
|openharmony-x86_64             |HarmonyOS x86_64 |Clang      |build-openharmony-x86_64               |libplayfield.a   |Static Library |
|unity-win64                    |Unity Plugin     |Clang-CL   |build-unity-win64                      |libplayfield.dll |DLL Plugin     |
|unity-switch                   |Unity Plugin     |Clang      |build-unity-switch                     |libplayfield.a   |Static Library |
|unity-ps5                      |Unity Plugin     |Clang      |build-unity-ps5                        |libplayfield.a   |Static Library |
|unity-xbox                     |Unity Plugin     |Clang      |build-unity-xbox                       |libplayfield.a   |Static Library |

---

## Samples

Samples are added timely.

|Name          |Directory            |Description                      |
|--------------|---------------------|---------------------------------|
|bouncer       |samples/bouncer      |A block bounce demo.             |
|rush          |samples/rush         |A jumping game demo.             |
|heartbeatclock|samples/hearbeatclock|A clock.                         |
|shoot         |samples/shoot        |A mini shooting.                 |

---

## AI Support

To generate game templates with AI, please let it load the following references:

- [Playfield Script Syntax](docs/mkdocs-en/docs/syntax.md)
- [Playfield Script API](docs/mkdocs-en/docs/api.md)

AI can then create runnable Playfield Engine games directly.

**Note:** In ChatGPT, you need to click the **"+"** button and enable
**"Web Search"** so that it can read the above references.

---

## Localization / Internationalization

Playfield Engine supports the following locales for the UI messages.

| Language               | Locale Code | Translation Type         | Scope of Translation             |
|------------------------|-------------|--------------------------|----------------------------------|
| English                | `en-US`     | Original                 | UI / Docs / Samples              |
| Spanish                | `es-ES`     | Machine Translation      | UI                               |
| French                 | `fr-FR`     | Machine Translation      | UI                               |
| Italian                | `it-IT`     | Machine Translation      | UI                               |
| German                 | `de-DE`     | Machine Translation      | UI                               |
| Greek                  | `el-GR`     | Machine Translation      | UI                               |
| Russian                | `ru-RU`     | Machine Translation      | UI                               |
| Simplified Chinese     | `zh-CN`     | Machine Translation      | UI                               |
| Traditional Chinese    | `zh-TW`     | Machine Translation      | UI                               |
| Japanese               | `ja-JP`     | Professional Translation | UI / Docs                        |

**How to switch languages**  
Playfield Engine uses the system locale.
To switch languages, set your system locale.

**Contributing Translations**  
Translation files are located under the three `i18n/` directories. (main, HAL and NoctLang)
To propose a new translation or update an existing one, please open an Issue or submit a Pull Request.

---

## Third-party Libraries

Playfield Engine uses the following libraries.
The source code tarballs for the libraries are included in this repository.

- zlib
- libpng
- libogg
- libvorbis
- JPEG9
- libwebp
- brotli
- bzip2
- FreeType2

---

## Compatibily List

**Windows:**

| OS      | Version | Patch | CPU    | Runtimes                                                                             | 32-bit Binary | 64-bit Binary | Direct3D |
|---------|---------|-------|--------|--------------------------------------------------------------------------------------|---------------|---------------|----------|
| Windows | XP      | SP3   | x86    | DirectX End-User Runtimes (June 2010)                                                | ✅            | -             | 9.0      |
| Windows | XP      | SP3   | x64    | DirectX End-User Runtimes (June 2010)                                                | ✅            | ❌            | 9.0      |
| Windows | Vista   |       | x86    | DirectX End-User Runtimes (June 2010)                                                | ✅            | -             | 9.0      |
| Windows | Vista   |       | x64    | DirectX End-User Runtimes (June 2010)                                                | ✅            | ❌            | 9.0      |
| Windows | Vista   | SP1   | x86    | DirectX End-User Runtimes (June 2010)                                                | ✅            | -             | 9.0      |
| Windows | Vista   | SP1   | x64    | DirectX End-User Runtimes (June 2010), Microsoft Visual C++ Redistributable 2015     | ✅            | ✅            | 9.0      |
| Windows | Vista   | SP2   | x86    | DirectX 11 Platform Update                                                           | ✅            | -             | 11.0     |
| Windows | Vista   | SP2   | x64    | DirectX 11 Platform Update, Microsoft Visual C++ Redistributable 2015                | ✅            | ✅            | 11.0     |
| Windows | 7       |       | x86    |                                                                                      | ✅            | -             | 11.0     |
| Windows | 7       |       | x64    | Microsoft Visual C++ Redistributable 2015                                            | ✅            | ✅            | 11.0     |
| Windows | 7       | SP1   | x86    |                                                                                      | ✅            | -             | 11.0     |
| Windows | 7       | SP1   | x64    | Microsoft Visual C++ Redistributable 2015                                            | ✅            | ✅            | 11.0     |
| Windows | 8       |       | x86    |                                                                                      | ✅            | -             | 11.0     |
| Windows | 8       |       | x64    | Microsoft Visual C++ Redistributable 2015                                            | ✅            | ✅            | 11.0     |
| Windows | 8.1     |       | x86    |                                                                                      | ✅            | -             | 11.0     |
| Windows | 8.1     |       | x64    |                                                                                      | ✅            | ✅            | 11.0     |
| Windows | 10      |       | x86    |                                                                                      | ✅            | -             | 12.0     |
| Windows | 10      |       | x64    |                                                                                      | ✅            | ✅            | 12.0     |
| Windows | 10      |       | arm64  |                                                                                      | ✅            | ✅            | 12.0     |
| Windows | 11      |       | x64    |                                                                                      | ✅            | ✅            | 12.0     |
| Windows | 11      |       | arm64  |                                                                                      | ✅            | ✅            | 12.0     |

Note: playfield.exe is the 64-bit binary and playfield32.exe is the 32-bit one.

**macOS:**

| OS                | Version | CPU    | Status |
|-------------------|---------|--------|--------|
| OS X El Capitan   | 10.11   | x86_64 | ✅     |
| macOS Sierra      | 10.12   | x86_64 | ✅     |
| macOS High Sierra | 10.13   | x86_64 | ✅     |
| macOS Mojave      | 10.14   | x86_64 | ✅     |
| macOS Catalina    | 10.15   | x86_64 | ✅     |
| macOS Big Sur     | 11.0    | arm64  | ✅     |
| macOS Big Sur     | 11.0    | x86_64 | ✅     |
| macOS Monterey    | 12.0    | arm64  | ✅     |
| macOS Monterey    | 12.0    | x86_64 | ✅     |
| macOS Ventura     | 13.0    | arm64  | ✅     |
| macOS Ventura     | 13.0    | x86_64 | ✅     |
| macOS Sonoma      | 14.0    | arm64  | ✅     |
| macOS Sonoma      | 14.0    | x86_64 | ✅     |
| macOS Sequoia     | 15.0    | arm64  | ✅     |
| macOS Sequoia     | 15.0    | x86_64 | ✅     |
| macOS Tahoe       | 26.0    | arm64  | ✅     |
| macOS Tahoe       | 26.0    | x86_64 | ✅     |

**Linux:**

* One of the following:
    * X11 (X11 + GLX + OpenGL 3) [Default]
    * Wayland (Wayland + EGL + OpenGL ES 2) [Optional]
    * Framebuffer (GBM + DRM + EGL + OpenGL ES 2) [Optional]

**iOS:**

| OS     | Status |
|--------|--------|
| iOS 11 | ✅     |
| iOS 12 | ✅     |
| iOS 13 | ✅     |
| iOS 14 | ✅     |
| iOS 15 | ✅     |
| iOS 16 | ✅     |
| iOS 17 | ✅     |
| iOS 18 | ✅     |
| iOS 26 | ✅     |

**Android:**

| OS         | Status |
|------------|--------|
| Android 6  | ✅     |
| Android 7  | ✅     |
| Android 8  | ✅     |
| Android 9  | ✅     |
| Android 10 | ✅     |
| Android 11 | ✅     |
| Android 12 | ✅     |
| Android 13 | ✅     |
| Android 14 | ✅     |
| Android 15 | ✅     |
| Android 16 | ✅     |

**OpenHarmony / HarmonyOS NEXT:**

| OS         | Status |
|------------|--------|
| API 20     | ✅     |
| API 12     | ✅     |

---

## NoctVM Ecosystem

Playfield Engine is a part of the [NoctVM](https://noctvm.io/) ecosystem:

- **NoctLang** — portable scripting language (used in this project)
- **Playfield Engine* — go-anywhere 2D game engine (this project)
- NoctUI — a declarative UI library (not used in this project)

Together, they form the foundation of the Noct ecosystem.
NoctLang itself is just the core language engine with no complex runtime.
Playfield Engine is one of the runtimes that embeds and extends the NoctLang core.

---

## License

```
Playfield Engine
© 2025, 2026 Awe Morris
```

Playfield is available as open-source software under the **zlib License**.
You're free to use, modify, and share it — even in commercial projects.

---

## Contact

Email: Awe Morris <awe@noctvm.io>
Project Page: https://noctvm.io/playfield/

---

## Join Us

Playfield Engine is still in its early stages. We're building the game
engine with care, hoping it will inspire and empower creators around
the world.

If you're interested in contributing — whether it's code,
documentation, testing, or ideas — we'd be happy to have you with us.

Every small step helps shape what Playfield Engine can become. You're
welcome to join us on this journey.

[Join us on Discord](https://discord.gg/ybHWSqDVEX)
