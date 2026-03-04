Suika3
======

The next-generation visual novel engine for the 2030s and beyond.

`Suika3` is a high-performance visual novel engine developed by the
`Suika3 Community`. Built to be fast, native, and truly universal,
Suika3 aims to empower creators across the globe for the decades to
come.

Games made with Suika3 run on the `Suika3 JIT Virtual Machine` (VM).
The VM executes `NovelML 3.0` along with `SuikaScript 3.0`.

`NovelML 3.0` is a tag-based, human-readable markup for visual novels.
Familiar to developers from other engines, it uses concise, declarative tags
for dialogue and scenes, so scripts are readable at a glance.

`SuikaScript 3.0` is our original scripting language that runs at true native
speed within the VM. Inspired by JavaScript, SuikaScript offers a
streamlined syntax that is more intuitive for beginners yet powerful
enough for seasoned developers. Because both the language and VM are
developed in-house, you never have to worry about being left behind by
upstream language updates or breaking changes. Your scripts will remain
stable and compatible for the long term.

## Status: Implementing for 1.0.0

We are planning the 1.0.0 release at the end of March 2026.
Currently, We are working hard to implement this software.

**Our current status is:**
* ✅ We have implemented all APIs in C.
* ✅ We have implemented all tags in C.
* ❌ We are implementing API warppers for the scripting language.
    * ❌ Config
    * ❌ Input
    * ❌ Game
    * ❌ Image
    * ❌ Stage
    * ❌ Mixer
    * ❌ System Button
    * ❌ Text
    * ❌ Tag
    * ❌ Anime
    * ❌ Variable
    * ❌ Save
    * ❌ History
    * ❌ Seen
    * ❌ GUI
    * ❌ HAL

## Key Features

* **High Performance**: Powered by the Suika JIT VM, it delivers unparalleled execution speed.
* **Lightweight**: Engineered to run smoothly even on low-spec hardware and in developing regions.
* **Universal Platform**: Designed with a "port anywhere" strategy, ensuring compatibility with almost every modern platform.
* **Extensible**: NovelML can be seamlessly expanded by developers using SuikaScript.
* **Reliable**: Built on the proven codebase of Suika2 and integrated with the modern Playfield Engine.
* **Jot-and-Run**: A powerful markup language designed to bring your stories to life in an instant.

## Getting Started

A quick path to try Suika3 locally.

### Using prebuilt binary

Please download from [Releases](releases).

### Building locally

Refer to [build.md](docs/mkdocs-en/build.md) (when available) for platform-specific instructions.

```
git clone https://github.com/suika3-community/suika3.git
cd suika3
mkdir build
cd build
cmake ..
cmake --build .
cp -R ../game/* .
./suika3
```

## Quick Start

`start.novel` file looks like:
```
# Show a background image.
[bg file="bg/coast.png" time="0.5"]

# Show a character image.
[ch center="ch/midori-normal.png" time="0.5" fade="normal"]

# Show a question text.
[text text="Choose a name."]

# Show options.
[choose
  name="choose_result"
  text1="March"
  text2="April"
  text3="May"
]

# Branch by the choose's result.
[if lhs="${choose_result}" op="==" rhs="March"]
    [set name="name" value="March"]
[elseif lhs="${choose_result}" op="==" rhs="April"]
    [set name="name" value="April"]
[else]
    [set name="name" value="May"]
[endif]

# Show a text.
[text name="${name}" text="Hello! My name is ${name}."]
```

`main.pf` file looks like:
```
// Called when the window is opened.
func setup() {
    // Return the window settings.
    return {
        width:      1280,
        height:     720,
        title:      "My First Game",
	fullscreen: false
    };
}

// Called before the game starts.
func start() {
    // Do not delete the following line.
    Suika.start();
}

// Called before a frame rendering.
func update() {
    // Do not delete the following line.
    Suika.update();
}

// Called every frame rendering.
func render() {
    // Do not delete the following line.
    Suika.render();
}
```

## Examples

See the `game/` directory for:
* Minimal demo project
* Sample assets and NovelML snippets
* SuikaScript usage patterns

## Building

For contributors who want to build from source, see (docs/mkdocs-en/docs/build.md).

## Documentation

Documents are also available at the [Website](https://suika3.vn/docs/)

* [NovelML 3.0 reference](docs/mkdocs-en/docs/tags.md)
* [SuikaScript 3.0 reference](docs/mkdocs-en/docs/syntax.md)
* [API docs](docs/mkdocs-en/docs/api.md)
* [How to build](docs/mkdocs-en/docs/build.md)

## Supported Platforms

Suika3 targets desktop platforms (Windows, Linux, macOS) for game development,
with a clear migration path for mobile and embedded targets via Playfield Engine.

### Test Results

|Type       |OS         |Status       |Check Date       |Version                             |
|-----------|-----------|-------------|-----------------|------------------------------------|
|Desktop    |Windows    |✅ Works     |3rd March 2026   |Windows 11 x64                      |
|           |macOS      |✅ Works     |1st March 2026   ||macOS 15 Sequoia Arm64             |
|           |Linux      |✅ Works     |3rd March 2026   |Ubuntu 24.04 LTS x86_64             |
|Mobile     |iOS        |✅ Works     |                 |iPhone iOS 18                       |
|           |Android    |✅ Works     |                 |Android 16                          |
|BSD        |FreeBSD    |✅ Works     |                 |FreeBSD 14.3 amd64                  |
|           |NetBSD     |✅ Works     |                 |NetBSD 10.1 amd64                   |
|           |OpenBSD    |✅ Works     |                 |OpenBSD 7.7 amd64                   |
|Web        |WebAssembly|✅ Works     |                 |Chrome                              |
|           |Chromebook |✅ Works     |                 |Chrome                              |
|Other      |Unity      |✅ Works     |                 |Unity 6.2 Windows 11 x86_64         |

### Windows Compatibility List

| OS      | Version | Patch | CPU    | Runtimes                                                                             | 32-bit Binary | 64-bit Binary | Direct3D |
|---------|---------|-------|--------|--------------------------------------------------------------------------------------|---------------|---------------|----------|
| Windows | XP      | SP3   | x86    | DirectX End-User Runtimes (June 2010)                                                | ✅            | ❌            | 9.0      |
| Windows | XP      | SP3   | x64    | DirectX End-User Runtimes (June 2010)                                                | ✅            | ❌            | 9.0      |
| Windows | Vista   |       | x86    | DirectX End-User Runtimes (June 2010)                                                | ✅            | ❌            | 9.0      |
| Windows | Vista   |       | x64    | DirectX End-User Runtimes (June 2010)                                                | ✅            | ❌            | 9.0      |
| Windows | Vista   | SP1   | x86    | DirectX End-User Runtimes (June 2010)                                                | ✅            | ❌            | 9.0      |
| Windows | Vista   | SP1   | x64    | DirectX End-User Runtimes (June 2010), Microsoft Visual C++ Redistributable 2015     | ✅            | ✅            | 9.0      |
| Windows | Vista   | SP2   | x86    | DirectX 11 Platform Update                                                           | ✅            | ❌            | 11.0     |
| Windows | Vista   | SP2   | x64    | DirectX 11 Platform Update, Microsoft Visual C++ Redistributable 2015                | ✅            | ✅            | 11.0     |
| Windows | 7       |       | x86    |                                                                                      | ✅            | ❌            | 11.0     |
| Windows | 7       |       | x64    | Microsoft Visual C++ Redistributable 2015                                            | ✅            | ✅            | 11.0     |
| Windows | 7       | SP1   | x86    |                                                                                      | ✅            | ❌            | 11.0     |
| Windows | 7       | SP1   | x64    | Microsoft Visual C++ Redistributable 2015                                            | ✅            | ✅            | 11.0     |
| Windows | 8       |       | x86    |                                                                                      | ✅            | ❌            | 11.0     |
| Windows | 8       |       | x64    | Microsoft Visual C++ Redistributable 2015                                            | ✅            | ✅            | 11.0     |
| Windows | 8.1     |       | x86    |                                                                                      | ✅            | ❌            | 11.0     |
| Windows | 8.1     |       | x64    |                                                                                      | ✅            | ✅            | 11.0     |
| Windows | 10      |       | x86    |                                                                                      | ✅            | ❌            | 12.0     |
| Windows | 10      |       | x64    |                                                                                      | ✅            | ✅            | 12.0     |
| Windows | 10      |       | arm64  |                                                                                      | ✅            | ✅            | 12.0     |
| Windows | 11      |       | x64    |                                                                                      | ✅            | ✅            | 12.0     |
| Windows | 11      |       | arm64  |                                                                                      | ✅            | ✅            | 12.0     |

### macOS Compatibility List

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

### Linux Compatibility List

| Distribution      | Version   | CPU    | Status |
|-------------------|-----------|--------|--------|
| Raspberry Pi OS   | Debian 13 | arm64  | ✅     |
| Raspberry Pi OS   | Debian 13 | arm7   | ✅     |
| Ubuntu            | 24.04     | x86_64 | ✅     |
| Ubuntu            | 24.04     | arm64  | ✅     |
| Ubuntu            | 22.04     | x86_64 | ✅     |
| Ubuntu            | 22.04     | arm64  | ✅     |
| Ubuntu            | 20.04     | x86_64 | ✅     |
| Ubuntu            | 20.04     | arm64  | ✅     |
| Kubuntu           | 25.04     | x86_64 | ✅     |
| Linux Mint        | 22.3      | x86_64 | ✅     |
| Fedora            | 43        | x86_64 | ✅     |

### iOS Compatibility List

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

### Android Compatibility List

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

## Lineage: The Grand Journey

Suika3 represents the culmination of over two decades of relentless innovation and development:

* **Suika Studio (2001–2004)**: The origin of our codebase, featuring our first GUI-based editors. 
* **Unfruitiful (2005–2015)**: A decade of R&D focused on establishing a robust portability layer for cross-platform support.
* **Suika2 (2016–2024)**: The cornerstone of our current architecture and the fruit of a decade of dedicated R&D.
  It delivers a full visual novel experience with extensive platform compatibility.
* **Playfield Engine (2025–)**: A versatile 2D engine derived from the core portability layer of the Suika series.
* **Suika3 (2026–)**: By synthesizing these legacies and introducing **NovelML 3.0 / SuikaScript 3.0**,
  Suika3 delivers the rock-solid stability of its predecessors alongside the cutting-edge flexibility of modern technology.

## Localization / Internationalization

Suika3 supports the following languages, and the translation is in progress.

| Language               | Locale Code | Translation Type         | Scope of Translation             | Progress      |
|------------------------|-------------|--------------------------|----------------------------------|---------------|
| English                | `en-US`     | Original                 | UI / Docs / Samples              | Baseline      |
| Spanish                | `es-ES`     | Machine Translation      | UI                               | Not Yet       |
| French                 | `fr-FR`     | Machine Translation      | UI                               | Not Yet       |
| Italian                | `it-IT`     | Machine Translation      | UI                               | Not Yet       |
| German                 | `de-DE`     | Machine Translation      | UI                               | Not Yet       |
| Greek                  | `el-GR`     | Machine Translation      | UI                               | Not Yet       |
| Russian                | `ru-RU`     | Machine Translation      | UI                               | Not Yet       |
| Simplified Chinese     | `zh-CN`     | Machine Translation      | UI                               | Not Yet       |
| Traditional Chinese    | `zh-TW`     | Machine Translation      | UI                               | Not Yet       |
| Japanese               | `ja-JP`     | Professional Translation | UI / Docs                        | Not Yet       |

**How to switch languages**  
Playfield Engine uses the system locale.
To switch languages, set your system locale.

**Contributing Translations**  
Translation files are located under the three `i18n/` directories. (main, HAL and NoctLang)
To propose a new translation or update an existing one, please open an Issue or submit a Pull Request.

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

## CMake Presets

Suika3 ships with CMake presets covering various platforms and build configurations.

|Preset                         |Platform       |Compiler   |Directory                              |Target           |Type           |
|-------------------------------|---------------|-----------|---------------------------------------|-----------------|---------------|
|windows-vs2022-x86-debug       |Windows        |MSVC       |out/build/windows-vs2022-x86-debug     |playfield.exe    |Executable     |
|windows-vs2022-x86-release     |Windows        |MSVC       |out/build/windows-vs2022-x86-release   |playfield.exe    |Executable     |
|windows-vs2022-x64-debug       |Windows        |MSVC       |out/build/windows-vs2022-x64-debug     |playfield.exe    |Executable     |
|windows-vs2022-x64-release     |Windows        |MSVC       |out/build/windows-vs2022-x64-release   |playfield.exe    |Executable     |
|windows-vs2022-arm64-debug     |Windows        |MSVC       |out/build/windows-vs2022-arm64-debug   |playfield.exe    |Executable     |
|windows-vs2022-arm64-release   |Windows        |MSVC       |out/build/windows-vs2022-arm64-release |playfield.exe    |Executable     |
|windows-vs2022-gdk-desktop     |Windows        |MSVC       |out/build/windows-vs2022-gdk-desktop   |playfield.exe    |Executable     |
|windows-vs2022-gdk-xbox-xs     |Windows        |MSVC       |out/build/windows-vs2022-gdk-xbox-xs   |playfield.exe    |Executable     |
|windows-vs2026-x86-debug       |Windows        |MSVC       |out/build/windows-vs2026-x86-debug     |playfield.exe    |Executable     |
|windows-vs2026-x86-release     |Windows        |MSVC       |out/build/windows-vs2026-x86-release   |playfield.exe    |Executable     |
|windows-vs2026-x64-debug       |Windows        |MSVC       |out/build/windows-vs2026-x64-debug     |playfield.exe    |Executable     |
|windows-vs2026-x64-release     |Windows        |MSVC       |out/build/windows-vs2026-x64-release   |playfield.exe    |Executable     |
|windows-vs2026-arm64-debug     |Windows        |MSVC       |out/build/windows-vs2026-arm64-debug   |playfield.exe    |Executable     |
|windows-vs2026-arm64-release   |Windows        |MSVC       |out/build/windows-vs2026-arm64-release |playfield.exe    |Executable     |
|windows-vs2026-gdk-desktop     |Windows        |MSVC       |out/build/windows-vs2026-gdk-desktop   |playfield.exe    |Executable     |
|windows-vs2026-gdk-xbox-xs     |Windows        |MSVC       |out/build/windows-vs2026-gdk-xbox-xs   |playfield.exe    |Executable     |
|windows-mingw-x86              |Windows        |MinGW      |build-mingw-x86                        |playfield.exe    |Executable     |
|windows-mingw-x86_64           |Windows        |MinGW      |build-mingw-x86_64                     |playfield.exe    |Executable     |
|windows-mingw-arm64            |Windows        |MinGW-LLVM |build-mingw-arm64                      |playfield.exe    |Executable     |
|macos                          |macOS          |Clang      |build-macos                            |Playfield.app    |App Bundle     |
|linux-x11                      |Linux          |GCC        |build-linux                            |playfield        |Executable     |
|linux-wayland                  |Linux          |GCC        |build-linux                            |playfield        |Executable     |
|linux-gdm                      |Linux          |GCC        |build-linux                            |playfield        |Executable     |
|linux-gdm-rot90                |Linux          |GCC        |build-linux                            |playfield        |Executable     |
|freebsd                        |FreeBSD        |Clang      |build-freebsd                          |playfield        |Executable     |
|netbsd                         |NetBSD         |GCC        |build-netbsd                           |playfield        |Executable     |
|openbsd                        |OpenBSD        |Clang      |build-openbsd                          |playfield        |Executable     |
|wasm                           |WebAssembly    |Emscripten |build-wasm                             |index.html       |HTML + Wasm    |
|wasm-local                     |Chromebook     |Emscripten |build-wasm-local                       |index.html       |HTML + Wasm    |
|ios-device                     |iOS Device     |Clang      |build-ios-device                       |libplayfield.a   |Static Library |
|ios-simulator                  |iOS Simulator  |Clang      |build-ios-simulator                    |libplayfield.a   |Static Library |
|android-x86                    |Android x86    |Clang      |build-android-x86                      |libplayfield.so  |Shared Library |
|android-x86_64                 |Android x86_64 |Clang      |build-android-x86_64                   |libplayfield.so  |Shared Library |
|android-armv7                  |Android armv7  |Clang      |build-android-armv7                    |libplayfield.so  |Shared Library |
|android-arm64                  |Android arm64  |Clang      |build-android-arm64                    |libplayfield.so  |Shared Library |
|unity-win64                    |Unity Plugin   |Clang-CL   |build-unity-win64                      |libplayfield.dll |DLL Plugin     |
|unity-switch                   |Unity Plugin   |Clang      |build-unity-switch                     |libplayfield.a   |Static Library |
|unity-ps5                      |Unity Plugin   |Clang      |build-unity-ps5                        |libplayfield.a   |Static Library |
|unity-xbox                     |Unity Plugin   |Clang      |build-unity-xbox                       |libplayfield.a   |Static Library |

---

## Technical Overview

Suika3 is not merely a wrapper around SDL combined with a scripting language.
It features its own rendering and audio backends, and its own scripting language,
positioning it as a fully independent game engine.

### Core Architecture

Suika3 is based on `PlayField Engine`, a comprehensive 2D game engine.
This means that Suika3 is fully extensible using the Playfield API.
(Did you know that Playfield Engine was actually developed for Suika3?)

```
+-----------------------+
|        Suika3         |
+-----------------------+
|    Playfield Engine   |
+-----------+-----------+
| StratoHAL | NoctLang  |
+-----------+-----------+
|   Operating System    |
+-----------------------+
```

* **Scripting**:
  Integrates [NoctLang](https://github.com/awemorris/NoctLang),
  a tiny yet mighty language designed for game scripting.

* **Rendering**:
  Supports native DirectX 9/11/12, Metal, OpenGL, OpenGL ES, and WebGL for wide compatibility.

* **Audio**:
  Provides lightweight audio support through native DirectSound or XAudio2 (Windows),
  Audio Unit (macOS/iOS), ALSA (Linux), OSS (BSD), and other APIs.

### StratoHAL

StratoHAL originated as the codebase of a 2D game engine developed since 2001,
and has been open-sourced with a proven record of exceptional stability.
Evolving over a quarter century from the Windows 9x era,
StratoHAL has grown to support macOS, Linux, iOS, Android, WebAssembly, and Unity.
It has been running reliably on smartphones for more than a decade.
While SDL3 already exists as a popular open-source alternative,
StratoHAL covers the same major platforms as SDL3 — and uniquely provides console support through Unity without relying on any NDA-restricted code.

### Platform Support and Comonents

|Platform Type  |OS / Platform       |Graphics                 |Sound                 |SDK                  |
|---------------|--------------------|-------------------------|----------------------|---------------------|
|Desktop        |Windows             |DirectX 12/11/9          |DirectSound           |Win32 API            |
|               |macOS               |Metal                    |Audio Unit            |AppKit (Objective-C) |
|               |ChromeOS            |WebGL 2                  |OpenAL (Emscripten)   |Emscripten (C)       |
|               |Linux               |OpenGL 3                 |ALSA                  |C, X11 or Wayland    |
|               |*BSD                |OpenGL 3                 |OSS (/dev/dsp)        |C, X11               |
|               |Qt                  |OpenGL 3                 |Qt Sound              |Qt                   |
|Mobile         |iOS                 |Metal                    |Audio Unit            |UIKit (Objective-C)  |
|               |Android             |OpenGL ES 2              |OpenSL ES             |Android NDK          |
|Web            |WebAssembly (Wasm)  |WebGL 2                  |OpenAL (Emscripten)   |Emscripten (C)       |
|Console        |Unity               |Unity                    |Unity                 |Unity Native Plugin  |
|               |Xbox Series X\|S    |DirectX 12               |XAudio2 (GDK)         |Microsft GDK         |

### Console Unity Plugin Notice

Playfield Engine provides Unity Plugin binaries for platforms
including Windows 64-bit and game consoles.  These binaries are built
entirely with the open-source version of LLVM/Clang toolchain (no
proprietary SDKs).

For Xbox series, you can use the native Microsoft GDK port directly, not via Unity.

### NoctLang

```
SuikaScript = NoctLang + Suika3 API
```

**NoctLang** is a lightweight scripting language designed for in-app scripting.
With a game-oriented syntax, it emphasizes clarity, instant startup, and tight integration with the engine.

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
|WebAssembly    |Interpreter or AOT |
|Unity Plugin   |Interpreter or AOT |
|Xbox           |Interpreter or AOT |

### Runtime Footprint

|Platform       |Binary Size    |Memory Usage (GC)                                              |
|---------------|---------------|---------------------------------------------------------------|
|All targets    |~2 MB          |Allocates 256 MB at startup for GC; does not exceed that limit |

The garbage collector allocates 256 MB at startup because currently 256 MB is the maximum heap size.

### Garbage Collection

Suika3 implements a high-performance garbage collector similar to the
HotSpot VM in Java, so you generally don’t need to worry much about
delays caused by garbage collection.

* The garbage collector uses a generational GC, managing objects in young and old generations.
* Young-generation GC runs extremely fast using a copying algorithm (semi-space copy GC).
* Old-generation GC uses a mark-and-sweep algorithm and performs compaction when necessary (mark-sweep-compact GC).

## License

This software is released under the `zlib License`.

```
Suika3
Copyright (c) 2026 The Suika3 Community
```

## Support & Contact

For issues, open an issue on GitHub. For larger discussions and design
proposals, check the community channels (Discord coming soon).

## Community

Our official Discord server is coming soon! We are hard at work
creating a space for creators to connect, share, and grow. Stay tuned
for updates.

## Contribution

We welcome contributions from everyone! Whether you are fixing a bug,
improving documentation, or proposing new features, your help is what
drives **Suika3** forward.

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for our contribution
guidelines and code of conduct.

Please also see [MANIFESTO.md](MANIFESTO.md) for our manifesto.
