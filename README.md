Suika3
======

<img src="https://raw.githubusercontent.com/suika3-community/suika3/refs/heads/main/docs/img/logo-small.png" alt="Suika3 Logo" align="left" width="120" hspace="20">

Suika3 is a portable visual novel engine designed to run on a wide
range of platforms (PC/iOS/Android/Web/etc.) using a custom JIT/AOT
hybrid scripting infrastructure and C89-based rendering & audio
runtime for mobile app store compliance.

Our slogan is "Visual Novel Anywhere!"

<br clear="left">

<div align="center">
  <img src="https://github.com/suika3-community/suika3/blob/main/docs/img/screenshot-20260317.webp" alt="Suika3 Screensho"><br>
</div>

---

<div align="center">
  <img src="https://github.com/suika3-community/suika3/blob/main/docs/img/platforms.png" alt="Suika3 runs everywhere."><br>
</div>

---

## TL;DR

- **Target:** Mobile-first VN engine that runs anywhere
- **High-Performance:** Native, written in C
- **Hybrid Scripting:** JIT VM with AOT fallback for App Store compliance
- **Runs Everywhere:**
    * **Desktop:** Windows, macOS, Linux, Chromebook, Raspberry Pi, *BSD, Solaris
    * **Mobile:** iOS, Android, HarmonyOS NEXT
    * **Web:** WebAssembly
    * **Console:** Xbox GDK, PS5 and Switch via Unity Integration
- **Users:** Professional creators, indie studios, enterprise companies, and hobbyists.
- [Join Discord](https://discord.gg/YZsq9u9Mgr)

To build and run:
```
./configure --prefix=/usr/local
make
make install
cd game
suika3
```

---

## Introduction

Suika3 is a production-grade visual novel engine specifically
engineered for commercial mobile app development, maintained by the
`Suika3 Community`, led by Awe Morris.

Games made with Suika3 are powered by `NovelML 3.0` and `SuikaScript 3.0`.

- **NovelML 3.0**: A tag-based, human-readable, extensible DSL
  designed specifically for visual novels. It features concise,
  declarative tags for seamless dialogue and scene control. Developers
  can add custom tags by SuikaScript.

- **SuikaScript 3.0**: The robust extension language for Suika3. It
  utilizes a high-speed JIT compiler on desktop platforms and can be
  compiled into native binaries for production or mobile
  environments.

Although Suika3 is designed as a professional tool, you are absolutely
welcome to use it just for fun — after all, fun is the driving force
behind every great community.

---

## Key Features

With lead developer's 10+ years of R&D experience in computer science,
Suika3 is engineered with modern techniques, and delivers:

- **High Performance**: Powered by the Suika JIT VM, it delivers
  2.5-4.5x faster execution speed compared to our interpreter backend
  in synthetic benchmarks.

- **Lightweight**: Engineered to run smoothly at 60 fps even on
  low-spec hardware such as Raspberry Pi in developing regions.

- **True Portability**: Designed with a "port anywhere" strategy,
  ensuring compatibility with almost every modern platform.

- **Extensible**: NovelML can be seamlessly extended using
  SuikaScript. Developers can implement custom tags simply by writing
  a SuikaScript function named `Tag_*()`.

- **Reliable**: We do enterprise-grade Quality Assurance to deliver
  reliable software.

- **Mobile UI/UX:** The default UX is mobile-first with a hamburger
  menu. Desktop-style tiny buttons on message windows are not a
  primary target due to mobile usability and store review risks.

- **Jot-and-Run**: A powerful markup language designed to bring your
  stories to life in an instant.

- **Battery-Friendly:** Idle CPU and GPU usages are around 1%, making
  it perfect for long-play on mobile devices.

- **Long-Term Support:** Our codebase is so portable and designed to
  support future platforms through the 2030s, 2040s, and beyond.

---

## Index

- [Kanban](#kanban-status)
- [Getting Started Guide](#getting-started-guide)
- [Building Locally](#building-locally)
- [Quick Look](#quick-look)
- [Examples](#examples)
- [Why SuikaScript?](#why-suikascript)
- [Technical Overview](#technical-overview)
- [Garbage Collection](#garbage-collection)
- [Compatibility List](#compatibility-list)
- [Documentation](#documentation)
- [Internationalization](#internationalization)
- [Third-party Libraries](#third-party-libraries)
- [CMake Presets](#cmake-presets)
- [Codebase & Maturity](#codebase--maturity)
- [Quality Assurance](#quality-assurance)
- [Adoption Status](#adoption-status)
- [Repository Structure](#repository-structure)
- [Asset File Formats](#asset-file-formats)
- [Game Packaging & Distribution](#game-packaging-distribution)
- [Engine Feature List](#engine-feature-list)
- [Screenshots](#screenshots)
- [Versioning Policy](#versioning-policy)
- [License](#license)
- [Support & Contact](#support--contact)
- [Community](#community)
- [Contribution](#contribution)
- [Lineage: The Grand Journey](#lineage-the-grand-journey)
- [Why Suika3?: Our Philosophy](#why-suika3-our-philosophy)
- [FAQ](#faq)

---

## Kanban (Status)

We are planning our first stable release `Suika3 26.07.0` on July 1 2026.

The program has been already implemented, and currently we are working
on sample games and documentation alongwith debugging to complete the
stable release.

**Our current status is:**

- ✅ Cleaning up the Suika2 codebase. (OpenNovel) (June 2024 - November 2024)
- ✅ Implementing the scripting language. (NoctLang) (December 2024 - March 2025)
- ✅ Implementing the 2D game engine. (Playfield Engine) (March 2025 - October 2025)
- ✅ Implementing the tag execution engine. (Suika3) (January 2026 - February 2026)
- ✅ Refactoring all C functions into a stable "C API". (January 2026 - February 2026)
- ✅ Implementing all tags in C. (January 2026 - February 2026)
- ✅ Wrapping "C API" for SuikaScript. (February 2026 - March 2026)
- ✅ API Freeze (March 7, 2026)
- ✅ GUI Animation Implementation (March 10, 2026)
- ✅ Code Freeze (March 12, 2026)
- Remaining Tasks
    - Sample Game Creation (March 30, 2026)
    - Documentation (March 15 - June 30, Simultaneous with Quality Stabilization)
        - ✅ Website
        - ✅ `SuikaScript Syntax`
        - ✅ `SuikaScript API`
        - ✅ `NovelML Tags`
        - ✅ `Build Instructions`
        - ✅ `NovelML Syntax`
        - ✅ `Getting Started`
        - `Plugin Development`
        - `SRS: System Requirement Specification`
        - `SDS: System Design Specification`
- Quality Stabilization Period (March 30 - June 30, 2026)
    - Milestone: `Suika3 26.07.0 RC1` (March 30, 2026)
    - Milestone: `Suika3 26.07.0 RC2` (April 30, 2026)
    - Milestone: `Suika3 26.07.0 RC3` (May 30, 2026)
    - Milestone: `Suika3 26.07.0 RC4/GM` (June 30, 2026)
- `Suika3 26.07.0` Release (July 1, 2026)

---

## Getting Started Guide

This guide will help you jump-start your very first visual novel
project in just a few easy steps.

### 1. Your First Run

Let's get the engine running so you can see the magic happen!

- **Download:** Grab the latest release ZIP file from the official source.
- **Extract:** Right-click the ZIP file and select "Extract All" to a folder of your choice.
- **Launch:** Open the folder and run `suika3.exe`. A sample game will start immediately!

### 2. Personalize Your Story (`start.novel`)

Now, let's make the game say exactly what you want.

- **Open:** Find the `start.novel` file in your project folder and
open it with your favorite text editor. It is written in
`NovelML`.
- **Edit:** Replace the existing text with the following command:
```
[text text="Hello, world! This is my first game."]
```
- **Test:** Save the file and run `suika3.exe` again. You should see your new message on the screen!

### 3. Customize the Screen (`main.pf`)

You can easily change the look and feel of your game window.

- **Locate:** Open the `main.pf` file in your editor.
- **Modify:** Look for the `func setup()` section. You can change the resolution and the title of your window here:
```
// Called when the window is opened.
func setup() {
    return {
        width:      1280,            // The width of your game
        height:     720,             // The height of your game
        title:      "My First Game", // Your game's title
        fullscreen: false            // Set to true for full-screen mode
    };
}
```

### 4. Under the Hood (Advanced Tips)

The bottom part of your `main.pf` file contains the core engine logic
in `SuikaScript`. It's best to leave these functions as they are
unless you are doing advanced customization:

- `func start()`: This is called once when your game launches.
- `func update()`: This runs every single frame to handle game logic.
- `func render()`: This draws everything on the screen after the update is done.

```
// Called before the game starts.
func start() {
    // Load plugins here.
    // Suika.loadPlugin("testplugin");

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

> [!TIP]
> These functions are the core mechanism of the `Playfield Engine` that
> powers Suika3. Suika.start(), Suika.update(), and Suika.render() must
> remain in place for the game to function properly.

---

## Quick Look

### NovelML

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

---

## Build Locally

Refer to [build.md](docs/mkdocs-en/docs/build.md) for platform-specific instructions.

---

## Examples

A sample game is planned to be built in the end of March 2026.

See the `game/` directory for:
- Minimal demo project
- Sample assets and NovelML snippets
- SuikaScript usage patterns

---

### Why SuikaScript?

- **Instant Iteration:** No compile cycles, no waiting. The built-in
  JIT compiler runs your scripts immediately after editing, so you can
  tweak gameplay and see results in real-time.

- **True Native Performance:** Run extensions at full speed on iOS,
  Android, and consoles. Even on platforms where JIT is restricted,
  you never have to sacrifice performance thanks to AOT.

- **Sandbox Model:** Scripts cannot access arbitrary
files/network. Only game assets and save data are accessible via
engine-managed APIs.

- **Smooth Store Approval:** Dramatically lower the risk of
  rejection. Since final builds consist of native code via AOT, you'll
  breeze through App Store and console certifications.

- **Easy to Learn, Powerful to Use:** The JavaScript-inspired syntax
  is friendly for beginners, yet offers the deep flexibility
  experienced developers expect for complex systems.

- **Long-term Stability:** Because we develop the language in-house,
  you're shielded from breaking changes in upstream projects. We own
  and control the full stack, so your scripts stay compatible forever.

---

## Technical Overview

If these specifications excite you, you're probably in the right
place.

Suika3 is not merely a wrapper around SDL combined with a scripting
language. It features its own rendering and audio backends, and its
own scripting language, positioning it as a fully independent game
engine.

### Core Architecture

Suika3 is based on `PlayField Engine`, a comprehensive 2D game engine.
This means that Suika3 is fully extensible using the Playfield API.
(And Playfield Engine was actually developed just for Suika3.)

```
+------------------------------+
|        NovelML (Tags)        |
+------------------------------+
| Plugin Tags (by SuikaScript) | --> SuikaScript can be compiled into a native binary!
+------------------------------+
|       Base Tags (by C)       |
+------------------------------+
|    SuikaScript Wrapper API   |
+------------------------------+
|         Suika3 C API         |
+------------------------------+
|       Playfield C API        |
+---------------+--------------+
|   StratoHAL   |   NoctLang   |
+---------------+--------------+
|       Operating System       |
+------------------------------+
```

- **Scripting**: Integrates
  [NoctLang](https://github.com/awemorris/NoctLang), a tiny yet mighty
  language designed for game scripting.

* **Rendering**: Supports native DirectX 9/11/12, Metal, OpenGL,
  OpenGL ES, and WebGL for wide compatibility.

* **Audio**: Provides lightweight audio support through native
  DirectSound or XAudio2 (Windows), Audio Unit (macOS/iOS), ALSA
  (Linux), OSS (BSD), and other APIs.

### StratoHAL

StratoHAL originated as the codebase of a 2D game engine developed
since 2001, and has been Zlib-licensed with a proven record of
exceptional stability.

Evolving over a quarter century from the Windows 9x era, StratoHAL has
grown to support macOS, Linux, iOS, Android, WebAssembly, and Unity.
It has been running reliably on smartphones for more than a decade.

While SDL3 already exists as a popular free/libre software
alternative, StratoHAL covers:

- The same major platforms as SDL3.

- Uniquely provides console support through Unity Integration without
  relying on any NDA-restricted code.

- Software 3D rendering for older platforms.

### Platform Support and Components

|Platform Type  |OS / Platform       |Graphics                 |Sound                 |SDK                  |
|---------------|--------------------|-------------------------|----------------------|---------------------|
|Desktop        |Windows             |DirectX 12/11/9          |DirectSound           |Win32 API            |
|               |macOS               |Metal                    |Audio Unit            |AppKit (Objective-C) |
|               |ChromeOS            |WebGL 2                  |OpenAL (Emscripten)   |Emscripten (C)       |
|               |Linux               |OpenGL 3                 |ALSA                  |C, X11 or Wayland    |
|               |*BSD                |OpenGL 3                 |OSS (/dev/dsp)        |C, X11               |
|               |Solaris 11          |Software                 |OSS (/dev/dsp)        |C, X11               |
|               |Qt                  |OpenGL 3                 |Qt Sound              |Qt                   |
|Mobile         |iOS                 |Metal                    |Audio Unit            |UIKit (Objective-C)  |
|               |Android             |OpenGL ES 2              |OpenSL ES             |Android NDK          |
|               |HarmonyOS NEXT      |OpenGL ES 2              |OpenSL ES             |OpenHarmony SDK      |
|Web            |WebAssembly (Wasm)  |WebGL 2                  |OpenAL (Emscripten)   |Emscripten (C)       |
|Console        |Unity               |Unity                    |Unity                 |Unity Native Plugin  |
|               |Xbox Series X\|S    |DirectX 12               |XAudio2               |Microsoft GDK        |

### Console Unity Plugin Notice

Playfield Engine provides Unity Plugin binaries for platforms
including Windows 64-bit and game consoles.  These binaries are built
entirely with the official version of LLVM/Clang toolchain (no
proprietary SDKs).

For Xbox series, you can use the native Microsoft GDK port directly, not via Unity.

### NoctLang

```
SuikaScript = NoctLang + Suika3 API
```

**NoctLang** is a lightweight scripting language designed for in-app
scripting.  With a game-oriented syntax, it emphasizes clarity,
instant startup, and tight integration with the engine.

The built-in JIT compiler supports a wide range of CPU architectures
to cover most gaming consoles and smartphones, including:

- ✅ Intel x86 (Xbox) [jit-x86.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-x86.c)
- ✅ AMD64/x86_64 (PS4/PS5/Xbox One/Xbox series X|S) [jit-x86_64.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-x86_64.c)
- ✅ ARMv5-ARMv7 (Nintendo DS/PS Vita) [jit-arm32.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-arm32.c)
- ✅ Arm64 (Switch/Switch2) [jit-arm64.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-arm64.c)
- ✅ PowerPC 32 (Wii/GameCube) [jit-ppc32.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-ppc32.c)
- ✅ PowerPC 64 (PS3/Xbox 360) [jit-ppc64.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-ppc64.c)
- ✅ MIPS32 (PS1/PSP) [jit-mips32.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-mips32.c)
- ✅ MIPS64 (N64/PS2) [jit-mips64.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-mips64.c)
- ✅ RISC-V 32 (for future devices) [jit-riscv32.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-riscv32.c)
- ✅ RISC-V 64 (for future devices) [jit-riscv64.c](https://github.com/suika3-community/suika3/blob/main/external/PlayfieldEngine/external/NoctLang/src/core/jit-riscv64.c)

These architectures are well-supported, we can say at least they all pass [the test suite](external/PlayfieldEngine/external/NoctLang/tests/run-syntax.sh).

However, the following are not supported yet because of the lack of development machines:

- ❌ SH-4 (Dreamcast)
- ❌ Sun SPARC
- ❌ HP PA-RISC
- ❌ Motorola 68000
- ❌ Loongson
- ❌ IBM Z
- **Challenge:** If you have one, please provide a ssh access to the development environment for 3 days. We can port there.

### AOT Compilation

For platforms where JIT is restricted (such as mobile or consoles),
NoctLang can fall back to interpreter mode, and AOT (ahead-of-time)
compilation using a C source backend — ensuring full compatibility
even in tightly controlled environments.

Please see [AOT](docs/mkdocs-en/docs/aot.md) for more details.

### Script Execution Mode

|Platform          |Mode               |
|------------------|-------------------|
|Windows x86       |JIT                |
|Windows x64       |JIT                |
|Windows arm64     |JIT                |
|macOS x86_64      |JIT                |
|macOS arm64       |JIT                |
|Linux x86         |JIT                |
|Linux x86_64      |JIT                |
|Linux armv7       |JIT                |
|Linux arm64       |JIT                |
|iOS               |Interpreter or AOT |
|Android           |Interpreter or AOT |
|HarmonyOS NEXT    |Interpreter or AOT |
|WebAssembly       |Interpreter or AOT |
|Unity Plugin      |Interpreter or AOT |
|Xbox              |Interpreter or AOT |
|FreeBSD           |JIT                |
|NetBSD            |JIT                |
|OpenBSD           |Interpreter or AOT |
|Solaris 11 x86_64 |JIT                |
|Solaris 11 Sparc  |Interpreter or AOT |

### Runtime Footprint

The footprint of Suika3 is very small.

| OS           | Binary Size  | Memory Usage                |
|--------------|--------------|-----------------------------|
| Windows 11   | 2 MB         | 300 MB in the task manager  |
| Windows 2000 | 3 MB         | 8 MB in the task manager    |

> [!TIP]
> Due to the modern graphics architecture, game applications using
> DirectX 12 typically consume at least 300 MB of memory even before
> loading any game assets. The engine itself consumes only 8 MB on
> Windows 2000.

---

## Garbage Collection

Suika3 features a high-performance generational garbage collector,
inspired by the architecture of the Java HotSpot VM. This design
ensures that developers can focus on creation without being
interrupted by the dreaded "GC spikes" or frame drops.

Objects in scripts (strings, dictionaries) are managed by GC,
while large assets such as textures are managed by native code.

### Core Mechanism: Generational GC

The system categorizes objects into two groups to optimize memory
management:

* Young Generation: Most objects die young. Suika3 handles these using
  a high-speed Copying Algorithm (Semi-space Copy GC), which clears
  out temporary data in a flash. (typically < 0.1ms)

* Old Generation: Long-lived objects are moved here. This area uses a
  Mark-Sweep-Compact GC algorithm, which periodically reorganizes
  memory to prevent fragmentation. (typically 10-300ms)

For more detailed implementation, please check
[gc.h](external/PlayfieldEngine/external/NoctLang/src/core/gc.h) and
[gc.c](external/PlayfieldEngine/external/NoctLang/src/core/gc.c).

### Frame-Synchronized Latency Hiding

The real "magic" of Suika3 lies in its timing. By executing the
Young-generation Copy GC every single frame, the system effectively
hides the GC processing time within the natural frame interval.

Thanks to this generational strategy, the heavier Old-generation
Mark-and-Sweep is rarely triggered, maintaining a constant 60 fps
experience for the player.

---

## Compatibility List

### Platform Availability at a Glance

`Supported` means the upstream (`Playfield Engine`) fully supports it.

| Category    | OS / Environment   | Status       | Last Checked | Checked On                       |
|-------------|--------------------|--------------|--------------|----------------------------------|
| **Desktop** | Windows            | ✅ Supported | 3 Mar 2026   | Windows 11 (x64/Arm64)           |
|             | macOS              | ✅ Supported | 1 Mar 2026   | macOS 26 Tahoe (Apple Silicon)   |
|             | Linux              | ✅ Supported | 3 Mar 2026   | Ubuntu 24.04 LTS (x86_64)        |
| **Mobile**  | iOS                | ✅ Supported | -            | iOS 18                           |
|             | Android            | ✅ Supported | -            | Android 15                       |
|             | HarmonyOS NEXT     | ✅ Supported | -            | API 10+                          |
| **BSD**     | FreeBSD            | ✅ Supported | -            | 14.0-RELEASE amd64               |
|             | NetBSD             | ✅ Supported | -            | 10.0 amd64, aarch64, armv7       |
|             | OpenBSD            | ✅ Supported | -            | 10.0 amd64, aarch64, armv7       |
| **Web**     | WebAssembly (Wasm) | ✅ Supported | -            | Chrome, Edge, Safari             |
|             | Chromebook         | ✅ Supported | -            | Chrome Browser / Linux Container |
| **Other**   | Unity Integration  | ✅ Supported | -            | Unity 6.2 (Windows x86_64)       |

### 64-bit Windows Compatibility List

The official recommended binary is the 64-bit version.

| OS      | Version                     | Patch | CPU    | Runtimes                                        | 64-bit Binary | Direct3D |
|---------|-----------------------------|-------|--------|-------------------------------------------------|---------------|----------|
| Windows | 11                          |       | x64    | (None required)                                 | ✅            | 12.0     |
| Windows | 11                          |       | arm64  | (None required)                                 | ✅            | 12.0     |
| Windows | 10                          |       | x64    | (None required)                                 | ✅            | 12.0     |
| Windows | 10                          |       | arm64  | (None required)                                 | ✅            | 12.0     |
| Windows | 8.1                         |       | x64    | (None required)                                 | ✅            | 11.0     |
| Windows | 8                           |       | x64    | Microsoft Visual C++ Redistributable 2015       | ✅            | 11.0     |
| Windows | 7                           |       | x64    | UCRT Update (KB2999226)                         | ✅            | 11.0     |
|         |                             |       |        | Microsoft Visual C++ Redistributable 2015       |               |          |
| Windows | 7                           | SP1   | x64    | Microsoft Visual C++ Redistributable 2015       | ✅            | 11.0     |
| Windows | Vista                       |       | x64    | (No UCRT)                                       | ❌            | -        |
| Windows | Vista                       | SP1   | x64    | DirectX End-User Runtimes (June 2010)           | ✅            | 9.0      |
|         |                             |       |        | Microsoft Visual C++ Redistributable 2015       |               |          |
| Windows | Vista                       | SP2   | x64    | Platform Update for Windows Vista               | ✅            | 11.0     |
|         |                             |       |        | Microsoft Visual C++ Redistributable 2015       |               |          |
| Windows | XP Professional x64 Edition |       | x64    | (No UCRT)                                       | ❌            | -        |
| Windows | XP Professional x64 Edition | SP1   | x64    | (No UCRT)                                       | ❌            | -        |
| Windows | XP Professional x64 Edition | SP2   | x64    | (No UCRT)                                       | ❌            | -        |

### 32-bit Windows Compatibility List

Suika3 provides the 32-bit binary `suika3-32.exe` for backward compatibility,
supporting legacy systems through older runtimes.

| OS      | Version                     | Patch | CPU    | Runtimes                               | 32-bit Binary | Direct3D |
|---------|-----------------------------|-------|--------|----------------------------------------|---------------|----------|
| Windows | 11                          |       | x64    | (None required)                        | ✅            | 12.0     |
| Windows | 11                          |       | arm64  | (None required)                        | ✅            | 12.0     |
| Windows | 10                          |       | x86    | (None required)                        | ✅            | 12.0     |
| Windows | 10                          |       | x64    | (None required)                        | ✅            | 12.0     |
| Windows | 10                          |       | arm64  | (None required)                        | ✅            | 12.0     |
| Windows | 8.1                         |       | x86    | (None required)                        | ✅            | 11.0     |
| Windows | 8.1                         |       | x64    | (None required)                        | ✅            | 11.0     |
| Windows | 8                           |       | x86    | (None required)                        | ✅            | 11.0     |
| Windows | 8                           |       | x64    | (None required)                        | ✅            | 11.0     |
| Windows | 7                           |       | x86    | (None required)                        | ✅            | 11.0     |
| Windows | 7                           |       | x64    | (None required)                        | ✅            | 11.0     |
| Windows | 7                           | SP1   | x86    | (None required)                        | ✅            | 11.0     |
| Windows | 7                           | SP1   | x64    | (None required)                        | ✅            | 11.0     |
| Windows | Vista                       |       | x86    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |
| Windows | Vista                       |       | x64    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |
| Windows | Vista                       | SP1   | x86    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |
| Windows | Vista                       | SP1   | x64    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |
| Windows | Vista                       | SP2   | x86    | DirectX 11 Platform Update             | ✅            | 11.0     |
| Windows | Vista                       | SP2   | x64    | DirectX 11 Platform Update             | ✅            | 11.0     |
| Windows | XP                          |       | x86    | DirectX 9.0b Runtime                   | ✅            | 9.0      |
| Windows | XP                          | SP1   | x86    | DirectX 9.0b Runtime                   | ✅            | 9.0      |
| Windows | XP                          | SP2   | x86    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |
| Windows | XP                          | SP3   | x86    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |
| Windows | XP Professional x64 Edition |       | x64    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |
| Windows | XP Professional x64 Edition | SP1   | x64    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |
| Windows | XP Professional x64 Edition | SP2   | x64    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |

### 32-bit Classic Windows Compatibility List

Suika3 provides the 32-bit classic binary `suika3-95.exe` for early Win32 compatibility.

| OS      | Version | Patch | CPU    | Runtimes                               | 32-bit Binary | Direct3D       |
|---------|---------|-------|--------|----------------------------------------|---------------|----------------|
| Windows | 2000    |       | x86    | DirectX End-User Runtimes (Dec 2006)   | ✅            | 9.0            |
| Windows | 2000    | SP1   | x86    | DirectX End-User Runtimes (Dec 2006)   | ✅            | 9.0            |
| Windows | 2000    | SP2   | x86    | DirectX End-User Runtimes (Dec 2006)   | ✅            | 9.0            |
| Windows | 2000    | SP3   | x86    | DirectX End-User Runtimes (Dec 2006)   | ✅            | 9.0            |
| Windows | 2000    | SP4   | x86    | DirectX End-User Runtimes (Dec 2006)   | ✅            | 9.0            |
| Windows | Me      |       | x86    |                                        | ✅            | soft rendering |
| Windows | 98      |       | x86    |                                        | ✅            | soft rendering |
| Windows | 98      | SE    | x86    |                                        | ✅            | soft rendering |
| Windows | 95      |       | x86    |                                        | ✅            | soft rendering |
| Windows | 95      | OSR2  | x86    |                                        | ✅            | soft rendering |
| Windows | NT 4.0  |       | x86    |                                        | ❌            | soft rendering |
| Windows | NT 3.51 |       | x86    |                                        | ❌            | soft rendering |

### macOS Compatibility List

| OS                     | Version | Mac CPU              | Status                       |
|------------------------|---------|----------------------|------------------------------|
| macOS Tahoe            | 26.0    | arm64 / x86_64       | ✅ OK                        |
| macOS Sequoia          | 15.0    | arm64 / x86_64       | ✅ OK                        |
| macOS Sonoma           | 14.0    | arm64 / x86_64       | ✅ OK                        |
| macOS Ventura          | 13.0    | arm64 / x86_64       | ✅ OK                        |
| macOS Monterey         | 12.0    | arm64 / x86_64       | ✅ OK                        |
| macOS Big Sur          | 11.0    | arm64 / x86_64       | ✅ OK                        |
| macOS Catalina         | 10.15   | x86_64               | ✅ OK                        |
| macOS Mojave           | 10.14   | x86_64               | ✅ OK                        |
| macOS High Sierra      | 10.13   | x86_64               | ✅ OK                        |
| macOS Sierra           | 10.12   | x86_64               | ✅ OK                        |
| OS X El Capitan        | 10.11   | x86_64               | ✅ OK                        |
| OS X Yosemite          | 10.10   | x86_64               | ❌ No Metal                  |
| OS X Mavericks         | 10.9    | x86_64               | ❌ No Metal                  |
| OS X Mountain Lion     | 10.8    | x86_64               | ❌ No Metal                  |
| Mac OS X Lion          | 10.7    | x86_64               | ❌ No Metal                  |
| Mac OS X Snow Leopard  | 10.6    | i386 / x86_64        | ❌ No Metal, No AVFoundation |
| Mac OS X Leopard       | 10.5    | ppc / ppc64 / i386   | ❌ No Metal, No AVFoundation |
| Mac OS X Tiger         | 10.4    | ppc / ppc64 / i386   | ❌ No Metal, No AVFoundation |
| Mac OS X Panther       | 10.3    | ppc                  | ❌ No Metal, No AVFoundation |
| Mac OS X Jaguar        | 10.2    | ppc                  | ❌ No Metal, No AVFoundation |
| Mac OS X Puma          | 10.1    | ppc                  | ❌ No Metal, No AVFoundation |
| Mac OS X Cheetah       | 10.0    | ppc                  | ❌ No Metal, No AVFoundation |

### Linux Compatibility List

| Distribution      | Version               | CPU             | Status | Graphics                      |
|-------------------|-----------------------|-----------------|--------|-------------------------------|
| Raspberry Pi OS   | Debian 13             | arm64, armv7    | ✅     | X11 + OpenGL ES               |
| Ubuntu            | 24.04 / 22.04 / 20.04 | x86_64, arm64   | ✅     | X11 + OpenGL 3                |
| Kubuntu           | 25.04                 | x86_64, arm64   | ✅     | Wayland + OpenGL 3            |
| Linux Mint        | 22.3                  | x86_64          | ✅     | X11 + OpenGL 3                |
| Fedora            | 43                    | x86_64          | ✅     | X11 + OpenGL 3                |
| Odroid            |                       | arm64           | ✅     | GBM + DRM + EGL + OpenGL ES 2 |

### iOS Compatibility List

| OS     | Status   |
|--------|----------|
| iOS 26 | ✅       |
| iOS 18 | ✅       |
| iOS 17 | ✅       |
| iOS 16 | ✅       |
| iOS 15 | ✅       |
| iOS 14 | ✅       |
| iOS 13 | ✅       |
| iOS 12 | ✅       |
| iOS 11 | ✅       |
| iOS 10 | ?        |
| iOS 9  | ?        |
| iOS 8  | ?        |
| iOS 7  | No Metal |

### Android Compatibility List

| OS          | Status | Special                      |
|-------------|--------|------------------------------|
| Android 16  | ✅     |                              |
| Android 15  | ✅     |                              |
| Android 14  | ✅     |                              |
| Android 13  | ✅     |                              |
| Android 12  | ✅     |                              |
| Android 11  | ✅     |                              |
| Android 10  | ✅     |                              |
| Android 9   | ✅     |                              |
| Android 8   | ✅     |                              |
| Android 7   | ✅     | Change the build properties. |
| Android 6   | ✅     | Change the build properties. |
| Android 5   | ✅     | Change the build properties. |
| Android 4.4 | ✅     | Change the build properties. |

### HarmonyOS / OpenHarmony Compatibility List

| OS         | Status |
|------------|--------|
| API 20     | ✅     |
| API 12     | ✅     |

### Solaris

| OS         | Status |
|------------|--------|
| Solaris 11 | ✅     |

---

## Documentation

Documentation is also available at [the Website](https://suika3.vn/docs/)
and it is synchronized to the latest.

- For Basic Usage:
    - [Getting Started Guide](docs/mkdocs-en/docs/getting-started.md)
    - [NovelML 3.0 Syntax Reference](docs/mkdocs-en/docs/novelml-syntax.md)
    - [NovelML 3.0 Tag Reference](docs/mkdocs-en/docs/tags.md)
- For Advanced Usage:
    - [SuikaScript 3.0 Syntax Reference](docs/mkdocs-en/docs/syntax.md)
    - [SuikaScript 3.0 API Reference](docs/mkdocs-en/docs/api.md)
    - [Plugin Development](docs/mkdocs-en/docs/plugin.md)
    - [AOT Instructions](docs/mkdocs-en/docs/aot.md)
- For Community Developers:
    - [Build Instruction](docs/mkdocs-en/docs/build.md)

---

## Internationalization

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

### How to switch languages

Playfield Engine uses the system locale.  To switch languages, set
your system locale.

### Contributing Translations

We welcome community contributions to improve our translations!
If you want to help us reach more developers, here is how you can contribute:

0.  Do `grep -r _TR() .` and detect original messages.
1.  **Locate the Files**: Translation assets are managed across three core directories:
    - `resources/i18n/message.txt` (Suika3 part, `S3_TR()`)
    - `external/PlayfieldEngine/resources/i18n/message.txt` (Playfield Engine part, `PF_TR()`)
    - `external/PlayfieldEngine/external/NoctLang/i18n/message.txt` (NoctLang part, `N_TR()`)
    - `external/PlayfieldEngine/external/StratoHAL/i18n/message.txt` (StratoHAL part, `HAL_TR()`)
2.  **Submit Your Changes**: 
    - Found a typo or a weird machine translation? Feel free to open a **Pull Request**.
    - Want to add a new language? Please open an **Issue** first so we can coordinate!

> [!TIP]
> Manual/Professional translations are prioritized over
> machine-generated content. Your human touch makes a huge difference!

---

## Third-party Libraries

Our engine is built upon several free/libre software libraries. To
ensure build reproducibility and long-term maintainability, all
necessary source code tarballs and patches are bundled within this
repository under the
`external/PlayfieldEngine/external/StratoHAL/lib/` directory.

### Core Libraries

| Library          | Purpose             | Key Feature                                            |
|------------------|---------------------|--------------------------------------------------------|
| Playfield Engine | 2D Game Engine      | Our base game engine. (in-house)                       |
| NoctLang         | Scripting Language  | Our scripting language. (in-house)                     |
| zlib             | Compression         | Deflate algorithm for general data compression.        |
| libpng           | Image               | Reference library for supporting PNG images.           |
| JPEG9            | Image               | Industry-standard JPEG image decompression.            |
| libwebp          | Image               | Modern, high-efficiency image format support.          |
| FreeType2        | Font                | High-quality font rendering and glyph processing.      |
| libogg           | Audio Container     | Bitstream handling for Ogg multimedia files.           |
| libvorbis        | Audio Codec         | Lossy audio compression used for BGM and SE.           |
| brotli           | Compression         | High-ratio compression for web and data assets. (WOFF) |
| bzip2            | Compression         | High-quality data compressor for archives.             |

### License Compliance

Each library is used according to its respective free/libre software
license. Please refer to the `NOTICE` file in this repository for the
full text of each license and copyright notice.

---

## CMake Presets

Suika3 ships with CMake presets covering various platforms and build
configurations.

|Preset                         |Platform              |Compiler   |Directory                              |Target        |Type           |
|-------------------------------|----------------------|-----------|---------------------------------------|--------------|---------------|
|windows-vs2022-x86-debug       |Windows               |MSVC       |out/build/windows-vs2022-x86-debug     |suika3.exe    |Executable     |
|windows-vs2022-x86-release     |Windows               |MSVC       |out/build/windows-vs2022-x86-release   |suika3.exe    |Executable     |
|windows-vs2022-x64-debug       |Windows               |MSVC       |out/build/windows-vs2022-x64-debug     |suika3.exe    |Executable     |
|windows-vs2022-x64-release     |Windows               |MSVC       |out/build/windows-vs2022-x64-release   |suika3.exe    |Executable     |
|windows-vs2022-arm64-debug     |Windows               |MSVC       |out/build/windows-vs2022-arm64-debug   |suika3.exe    |Executable     |
|windows-vs2022-arm64-release   |Windows               |MSVC       |out/build/windows-vs2022-arm64-release |suika3.exe    |Executable     |
|windows-vs2022-gdk-desktop     |Windows               |MSVC       |out/build/windows-vs2022-gdk-desktop   |suika3.exe    |Executable     |
|windows-vs2022-gdk-xbox-xs     |Windows               |MSVC       |out/build/windows-vs2022-gdk-xbox-xs   |suika3.exe    |Executable     |
|windows-vs2026-x86-debug       |Windows               |MSVC       |out/build/windows-vs2026-x86-debug     |suika3.exe    |Executable     |
|windows-vs2026-x86-release     |Windows               |MSVC       |out/build/windows-vs2026-x86-release   |suika3.exe    |Executable     |
|windows-vs2026-x64-debug       |Windows               |MSVC       |out/build/windows-vs2026-x64-debug     |suika3.exe    |Executable     |
|windows-vs2026-x64-release     |Windows               |MSVC       |out/build/windows-vs2026-x64-release   |suika3.exe    |Executable     |
|windows-vs2026-arm64-debug     |Windows               |MSVC       |out/build/windows-vs2026-arm64-debug   |suika3.exe    |Executable     |
|windows-vs2026-arm64-release   |Windows               |MSVC       |out/build/windows-vs2026-arm64-release |suika3.exe    |Executable     |
|windows-vs2026-gdk-desktop     |Windows               |MSVC       |out/build/windows-vs2026-gdk-desktop   |suika3.exe    |Executable     |
|windows-vs2026-gdk-xbox-xs     |Windows               |MSVC       |out/build/windows-vs2026-gdk-xbox-xs   |suika3.exe    |Executable     |
|windows-mingw-x86              |Windows               |MinGW      |build-mingw-x86                        |suika3.exe    |Executable     |
|windows-mingw-x86_64           |Windows               |MinGW      |build-mingw-x86_64                     |suika3.exe    |Executable     |
|windows-mingw-arm64            |Windows               |MinGW-LLVM |build-mingw-arm64                      |suika3.exe    |Executable     |
|macos                          |macOS                 |Clang      |build-macos                            |Suika3.app    |App Bundle     |
|linux-x11                      |Linux                 |GCC        |build-linux                            |suika3        |Executable     |
|linux-wayland                  |Linux                 |GCC        |build-linux                            |suika3        |Executable     |
|linux-gdm                      |Linux                 |GCC        |build-linux                            |suika3        |Executable     |
|linux-gdm-rot90                |Linux                 |GCC        |build-linux                            |suika3        |Executable     |
|freebsd                        |FreeBSD               |Clang      |build-freebsd                          |suika3        |Executable     |
|netbsd                         |NetBSD                |GCC        |build-netbsd                           |suika3        |Executable     |
|openbsd                        |OpenBSD               |Clang      |build-openbsd                          |suika3        |Executable     |
|solaris11                      |Solaris11             |SunCC      |build-solaris11                        |suika3        |Executable     |
|wasm                           |WebAssembly           |Emscripten |build-wasm                             |index.html    |HTML + Wasm    |
|wasm-local                     |Chromebook            |Emscripten |build-wasm-local                       |index.html    |HTML + Wasm    |
|ios-device                     |iOS Device            |Clang      |build-ios-device                       |libsuika3.a   |Static Library |
|ios-simulator                  |iOS Simulator         |Clang      |build-ios-simulator                    |libsuika3.a   |Static Library |
|android-x86                    |Android x86           |Clang      |build-android-x86                      |libsuika3.so  |Shared Library |
|android-x86_64                 |Android x86_64        |Clang      |build-android-x86_64                   |libsuika3.so  |Shared Library |
|android-armv7                  |Android armv7         |Clang      |build-android-armv7                    |libsuika3.so  |Shared Library |
|android-arm64                  |Android arm64         |Clang      |build-android-arm64                    |libsuika3.so  |Shared Library |
|openharmony-arm64              |HarmonyOS NEXT arm64  |Clang      |build-openharmony-arm64                |libsuika3.a   |Static Library |
|openharmony-armv7              |HarmonyOS NEXT armv7  |Clang      |build-openharmony-armv7                |libsuika3.a   |Static Library |
|openharmony-x86_64             |HarmonyOS NEXT x86_64 |Clang      |build-openharmony-x86_64               |libsuika3.a   |Static Library |
|unity-win64                    |Unity Plugin          |Clang-CL   |build-unity-win64                      |libsuika3.dll |DLL Plugin     |
|unity-switch                   |Unity Plugin          |Clang      |build-unity-switch                     |libsuika3.a   |Static Library |
|unity-ps5                      |Unity Plugin          |Clang      |build-unity-ps5                        |libsuika3.a   |Static Library |
|unity-xbox                     |Unity Plugin          |Clang      |build-unity-xbox                       |libsuika3.a   |Static Library |

---

## Codebase & Maturity

Suika3 is a robust visual novel engine with over 100k lines of code (LoC).
This is not a weekend project but a mature codebase evolved over 25+ years.

- **Proven Stability:** Includes core modules refined since 2001.

- **Modern Architecture:** Features a clean HAL (Hardware Abstraction
 Layer) separated in 2016 and high-performance GPU rendering
 implemented in 2022.

- **Native Multi-Platform:** Built mostly with C, and the platform
 native languages, including C++ (DirectX), Swift (macOS/iOS),
 Objective-C (macOS/iOS), C# (Unity) and Shaders (HLSL/GLSL/Metal).

---

## Quality Assurance

In `Software Engineering`, reliability is a function of time. As
Suika3 is currently in its pre-release phase, we define its cumulative
operational time as zero. Consequently, formal quality metrics are not
yet applicable.

### Our Path to Stability

We are committed to delivering a production-grade engine. Our QA
roadmap is as follows:

1. **Release Candidate (RC) Phase**: Upon reaching the RC milestone,
  we will initiate rigorous tracking of all identified issues.
  See [BUGS.md](BUGS.md)

2. **Data-Driven Hardening**: We will analyze bug discovery and
  resolution rates to quantify the software's stability.

3. **Commercial-Grade Standards**: Our final goal is to achieve a
  level of robustness that meets the requirements of commercial visual
  novel productions.

While the engine is still "young" in terms of uptime, the underlying
architecture inherits over two decades of development philosophy,
ensuring a solid foundation for the upcoming QA process.

---

## Adoption Status

We aim to create a new market for "Mobile Visual Novels" and have not
yet established a track record.

If you are interested in helping shape this new market as an **Early
Adopter**, we warmly invite you to join our community.

If you prefer to adopt tools with an established track record, you may
wish to revisit the project once Suika3 has matured further.

---

## Repository Structure

```
include/                           # Public headers
src/                               # Engine source code
resources/                         # Assets and resources for the engine
  projects/                          # Official project base for iOS, Android, etc.
cmake/                             # CMake configuration files and presets
docs/                              # Documentation source files (MkDocs)
external/                          # Third-party libraries and dependencies
  PlayfieldEngine/                   # The core 2D game engine
    external/                          # Sub-dependencies of Playfield Engine
      NoctLang/                          # Scripting language used by the engine
        include/                           # Public headers for NoctLang
        src/                               # Source code for NoctLang
          core/                              # Core implementation of NoctLang
      StratoHAL/                         # Hardware Abstraction Layer for cross-platform support
        include/                           # Public headers for StratoHAL
        src/                               # Source code for StratoHAL
```

---

## Asset File Formats

- Images:
    - Supported formats: PNG, JPEG, WebP.
- Audio:
    - Supported formats: Ogg Vorbis, 44100Hz, Stereo or Mono.
- Fonts:
    - Supported formats: TrueType (TTF), OpenType (OTF)

---

## Game Packaging & Distribution

To generate `assets.arc` from your game assets, please use the
`suika3-pack` tool provided in this repository. This tool will create
an obfuscated archive of your assets that can be efficiently loaded by
the engine. (The obfuscation algorithm is a key rotating XOR, and it
is not intended for strong security. It can help prevent casual
tampering. The obfuscation key can be changed in "key.h".)

| OS                 | Assets                                                                                                                         |
|--------------------|--------------------------------------------------------------------------------------------------------------------------------|
| Windows            | Assets should be stored into a file name `assets.arc`.                                                                         |
| macOS              | Assets should be stored into `Contents/Resources/assets.arc` inside the app bundle.                                            |
| Linux              | Assets should be stored into a file name `assets.arc`.                                                                         |
| iOS                | Assets should be stored into `Contents/Resources/assets.arc` inside the app bundle.                                            |
| Android            | Assets must not be packed. Please add your assets to the `app/src/main/assets/` folder directly as normal files.               |
| HarmonyOS NEXT     | Assets must not be packed. Please add your assets to the `entry/src/main/resources/rawfiles/` folder directly as normal files. |
| WebAssembly (Wasm) | Assets should be stored into `assets.arc` and placed alongside `index.html`.                                                   |
| Unity              | Assets must not be packed. Please add your assets to the `Assets/Resources/StreamingAssets/` folder directly as normal files.  |

---

## Engine Feature List

- Message display
- Option selection
- Background and character images
- Image transition
- Save / Load
- Skip mode
- Auto mode
- Backlog
- CG Gallery
- Localization
- UI construction

---

## Screenshots

---

## Versioning Policy

`Year.Month.Patch-Level`

- 26.07.0: Initial release of Suika3 (July 2026)
- 27.01.0: First major update with new features and improvements (January 2027)
- 27.07.0: Second major update with additional features and optimizations (July 2027)
- 28.01.0: Third major update with further enhancements and bug fixes (January 2028)
- ...

---

## License

Suika3 is a permissive
[free/libre software](https://www.gnu.org/philosophy/free-sw.en.html),
released under the permissive `ZLib License`.

* You can use Suika3 for commercial games.
* You do not need to open-source your game code.
* You may modify and redistribute the engine.
* Attribution is appreciated but not required.

```
Suika3
Copyright (c) 2026 The Suika3 Community
```

The full license is available at [LICENSE](LICENSE) and [NOTICE](NOTICE).

---

## Support & Contact

For general inquiries, bug reports, and technical assistance:

- **GitHub Issues**: Preferred for formal bug tracking and feature
  requests.

- **[Discord Server](https://discord.gg/YZsq9u9Mgr)**: The best place
  for real-time discussions and quick questions.

We strive to provide timely support and help creators bring their
stories to life.

### Professional & Enterprise Support

While the **Suika3 Community** provides active support through GitHub
and Discord, we understand that commercial productions often require
dedicated support or guaranteed response times.

For studios and developers requiring **formal maintenance contracts,
priority bug fixes, or private consultations**, please contact us
directly via email. We are committed to helping ensure your project's
success in a high-stakes production environment.

- **Contact**: Awe Morris <awe@noctvm.io>

**Not looking for a formal contract? No problem.** If you are an indie
developer or hobbyist and prefer to keep things casual, just ping us
on Discord with any questions. We're always happy to chat, and we'll
do our best to respond or push a fix as soon as possible! ;-)

---

## Community

[Discord Server](https://discord.gg/YZsq9u9Mgr)

### Recruiting

We are currently looking for contributors in the following areas:

* Documentation writers
* HarmonyOS NEXT engineer
* Android engineers
* iOS engineers

### Our Commitment to Inclusivity

We are committed to providing a welcoming and safe environment for
everyone. Any form of discrimination is strictly prohibited.

Our community warmly welcomes developers of all backgrounds, including
but not limited to:

- **Identity & Expression:** Race, ethnicity, nationality, language,
  gender identity and expression, sexual orientation, and sex
  characteristics.

- **Personal Attributes:** Age, body size, personal appearance,
  illness, and neurodiversity.

- **Life Context:** Faiths, beliefs, disability (e.g. visibility, but
  not limited), level of experience, education, and socio-economic
  status.

- **Justice & Inclusion:** We explicitly welcome individuals with past
  criminal records, firmly believing in the power of second chances
  and the inherent value of every contributor who participates
  respectfully.

### Governance

Suika3 provides a clean upstream codebase intended to serve as a
reliable foundation for downstream customizations. Users are
encouraged to maintain private forks tailored to their own needs.
Creating your own rebranded version is a natural and fully supported
use of the project.

Because of this philosophy, Suika3 does not employ a formal governance
structure. Instead, the project is guided by its lead maintainers in
close collaboration with the community. We together with a community
of contributors.

### Lead Maintainers

- Awe Morris (IEEE Member / IEEE Computer Society Member)
- Aitsuki

---

## Contribution

We welcome contributions from everyone! Whether you are fixing a bug,
improving documentation, or proposing new features, your help is what
drives **Suika3** forward.

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for our contribution
guidelines and code of conduct.

Please also see [MANIFESTO.md](MANIFESTO.md) for our manifesto.

---

## Lineage: The Grand Journey

Suika3 represents the culmination of over two decades of relentless innovation and development:

- **Suika Studio (2001–2004)**: The origin of our codebase, featuring
  our first GUI-based editors.

- **Unfruitiful (2005–2015)**: A decade of R&D focused on establishing
  a robust portability layer for cross-platform support.

- **Suika2 (2016–2024)**: The cornerstone of our current architecture
  and the fruit of a decade of dedicated R&D. It delivers a full
  visual novel experience with extensive platform compatibility.
  Several games with Suika3 were sold on App Store.

- **Playfield Engine (2025–)**: A versatile 2D engine derived from the
  core portability layer of the Suika series.

- **Suika3 (2026–)**: By synthesizing these legacies and introducing
  `NovelML` and `SuikaScript`, Suika3 delivers the stability of its
  predecessors alongside the flexibility of modern technology.

---

## Why Suika3?: Our Philosophy

We are not looking at the existing VN market. We aim to pioneer a
mobile VN market that does not yet exist.

Creators with different goals may also find excellent resources in
other engines like Ren'Py, Unity, or Godot.

### Mission: Building a Sustainable Ecosystem

Our dedication to the commercial success of visual novels is a
strategic response to the long-term health of the medium. We believe
that for visual novels to flourish in the 2030s, they must transcend
hobbyist boundaries and re-establish themselves as a thriving creative
industry.

Since the 2010s, the visual novel market has faced significant growth
challenges. One critical bottleneck has been the **lack of
high-performance software engines** capable of delivering a seamless
**native experience on iOS and Android**. While developers typically
work on PCs, for most people today, the primary personal computer is a
smartphone.

Without accessible, professional-grade tools for modern mobile
platforms, many creators have been restricted to limited distribution
channels. As a result, the medium has struggled to reach a global,
mobile-first audience, slowing its overall expansion.

While free and hobbyist projects are culturally valuable and
indispensable, they alone may not always sustain a large creative
industry. A truly healthy ecosystem may require:

- **Commercially Viable Games** — Titles that reach players on their
  primary devices and generate the economic activity necessary for
  creators to build long-term careers.

- **Economic Independence** — Empowering talent, particularly in Asia
  and the Global South, to overcome economic barriers through
  storytelling.

- **A Living Industry** — Transforming the medium from a niche
  interest into a sustainable market where creativity can lead to
  self-reliance.

In our view, it is a natural professional expectation for commercial
success.

Please note that this philosophy does not reject hobbyist or
experimental projects. We simply focus on a different problem space:
sustainability at scale.

### Our Vision: Catalyst for Growth

We do not aim to provide just another tool. Our goal is to provide a
**catalyst for growth**.

By offering a "port-anywhere" engine that delivers native performance
without the overhead of heavy frameworks, we enable developers to
focus on what matters most: **telling stories that endure.**

Because we love visual novels, we are committed to pushing the medium
forward and ensuring it remains a vibrant and economically viable art
form for decades to come.

### Our Values: Empowering Talents for Sustainable Careers

Our value is to empower talent—particularly in Asia and the Global
South—to build sustainable careers through their storytelling. We
believe that with the right tools, creativity can overcome economic
barriers.

Our commitment to the permissive license is for those people who
publish and sell original visual novels on the App Store and Google
Play, regardless of their software, hardware, and budget limitations.

---

## FAQ

- [What's this?](#whats-this)
- [Is this a competitor to existing engines such as Ren'Py, Unity, or Godot?](#is-this-a-competitor-to-existing-engines-such-as-renpy-unity-or-godot)
- [Is this okay to use for now?](#is-this-okay-to-use-for-now)
- [Doesn't "over 25 years of maturity" contradict "zero operational use"?](#doesnt-over-25-years-of-maturity-contradict-zero-operational-use)
- [Oh it's bus factor 1, isn't it?](#oh-its-bus-factor-1-isnt-it)
- [Can companies maintain their own forks?](#can-companies-maintain-their-own-forks)
- [It says community-led, but who makes the decisions?](#it-says-community-led-but-who-makes-the-decisions)
- [Is Suika3 open source software? Can it be used commercially?](#is-suika3-open-source-software-can-it-be-used-commercially)
- [What about the license for third-party libraries?](#what-about-the-license-for-third-party-libraries)
- [JIT is disabled on iOS/console, so what should I do?](#jit-is-disabled-on-iosconsole-so-what-should-i-do)
- [Are scripts safe? Can they access files or the network?](#are-scripts-safe-can-they-access-files-or-the-network)
- [Is it easy to pass the store review?](#is-it-easy-to-pass-the-store-review)
- [Is "Supports all modern platforms" really true?](#is-supports-all-modern-platforms-really-true)
- [What does "via Unity" mean for console support?](#what-does-via-unity-mean-for-console-support)
- [To what extent is HarmonyOS NEXT supported?](#to-what-extent-is-harmonyos-next-supported)
- [Can you reproduce "2.5-4.5x times faster"? What are the measurement conditions?](#can-you-reproduce-25-45x-times-faster-what-are-the-measurement-conditions)
- [Old GC takes 10 to 300ms, but does it cause frame drops?](#old-gc-takes-10-to-300ms-but-does-it-cause-frame-drops)
- [How do I start making games? Any samples?](#how-do-i-start-making-games-any-samples)
- [How do I go about developing plugins?](#how-do-i-go-about-developing-plugins)
- [Can I migrate from existing assets (like Ren'Py or Unity)?](#can-i-migrate-from-existing-assets-like-renpy-or-unity)
- [Where's the document? Is it the latest version?](#wheres-the-document-is-it-the-latest-version)
- [Is Japanese/Chinese/... supported?](#is-japanesechinese-supported)
- [Where should I go if I'm in trouble?](#where-should-i-go-if-im-in-trouble)
- [What about SLAs and maintenance contracts for businesses?](#what-about-slas-and-maintenance-contracts-for-businesses)
- [What are the minimum requirements?](#what-are-the-minimum-requirements)
- [What does "Supported" mean?](#what-does-supported-mean)
- [Will there be breaking changes? What is the compatibility policy?](#will-there-be-breaking-changes-what-is-the-compatibility-policy)

### What's this?

Suika3 is a next-generation visual novel engine designed with
professional studios and commercial app development in mind. It aims
for high performance, long-term maintainability, and broad platform
support through native implementations (primarily in C).

### Is this a competitor to existing engines such as Ren'Py, Unity, or Godot?

Rather than being competitors, we target different problem
areas. Suika3 clearly states its direction as "creating a
mobile-centric VN market that can withstand commercial use," and
respects existing engines as viable options.

### Is this okay to use for now?

Since actual operational uptime is zero, quality metrics are still
pending, it states honestly. Users can choose whether to "participate
as an Early Adopter" or "wait several years" before making an
adoption decision.

### Doesn't "over 25 years of maturity" contradict "zero operational use"?

Suika3 itself is a new integration, but it builds upon a long lineage
including Suika2 and StratoHAL. In other words, it's a "new package
with an old foundation."

### Oh it's bus factor 1, isn't it?

The policy explicitly states "encouraging forks and rebrands" and
"maintaining a clean upstream without formal governance," fostering an
approach that makes downstream maintenance easier.

### Can companies maintain their own forks?

With both "Software Requirement Specification" (SRS) and "Software
Design Specification" (SDS) provided, it is ideal for enterprise
maintenance. Furthermore, "professional/enterprise support" is
available, enabling the transfer of necessary know-how to the adopting
organization.

### It says community-led, but who makes the decisions?

The community handles maintenance, while lead members take the
lead. We do not implement formal governance.

### Is Suika3 open source software? Can it be used commercially?

Yes, it is OSS and can be used commercially. Since it is a permissive
license, there is no requirement to disclose the source code when
modifying it.

### What about the license for third-party libraries?

See [NOTICE](NOTICE) for the full text of each license and copyright
notice for the third-party libraries used in Suika3.

### JIT is disabled on iOS/console, so what should I do?

The official binaries use an interpreter in those environments. While
we believe our interpreter is sufficiently fast, if you require speed
or if even the interpreter poses a problem for a store review, please
use [AOT](docs/mkdocs-en/aot.md). This is a perfect solution boasting
2.5-4.5x the speed compared to the interpreter.

### Are scripts safe? Can they access files or the network?

No, scripts run in a sandboxed environment and cannot access files
outside the resource bundle or the network. It can access only the
APIs exposed by the engine, which are designed to be safe for game
logic.

### Is it easy to pass the store review?

We believe that using AOT compilation dramatically lowers the hurdle
for store reviews, as it essentially becomes a native app simply
executing tags, not running a generic-purpose scripting. However, we
cannot guarantee the outcome of store reviews, as they can be up to
the game quality including graphical design, user experience, and
content.

### Is "Supports all modern platforms" really true?

It's understandable that you find this hard to believe, but everything
is working according to our checks. However, when you ship it as a
product, please perform QA/QC at your own responsibility.

### What does "via Unity" mean for console support?

Even if we port Suika3 to consoles, we cannot disclose its source code
due to the NDA. However, dedicated versions of Unity are available for
consoles. Therefore, we ported Suika3 into a Unity plugin. Using this
plugin allows you to run Suika3 within Unity. In other words, if you
have the console version of Unity and a "devkit" machine, you can run
Suika3 on consoles as well.

Please understand that the console has UI/UX requirements, so you will
need to implement them additionally.

### To what extent is HarmonyOS NEXT supported?

All of the core developer resides outside China and does not possess
the actual hardware sold in China. Consequently, functionality has
only been verified using an emulator.

### Can you reproduce "2.5-4.5x times faster"? What are the measurement conditions?

This is a synthetic benchmark designed to measure the performance difference.

Measurement code:
```
func main() {
    var sum = 0;
    for(i in 0..10000) {
        for(j in 0..100000) {
            sum = sum + 1;
        }
    }
}
```

| Machine                 | JIT (s)      | Interpreter (s)       | Scaling (JIT vs Interpreter) |
|-------------------------|--------------|-----------------------|------------------------------|
| Intel Core i9 12900H    | 3.32         | 13.2s                 | 4.0x                         |
| Intel Core Ultra 5 228V | 5.78s        | 15.6                  | 2.7x                         |
| Intel Xeon Silver 4114  | 8.08         | 36.4s                 | 4.5x                         |
| Apple M5                | 2.77         | 10.6s                 | 3.8x                         |
| IBM POWER8              | 43.719       | 117.8s                | 2.7x                         |

In a real game app, the performance difference may vary depending on
the your logic and the amount of script execution. However, we have
observed that in typical visual novel scenarios, the performance
improvement can be significant, often around 5x faster when using AOT
compared to the interpreter.

### Old GC takes 10 to 300ms, but does it cause frame drops?

The Old GC is rarely invoked. Large buffers like images are managed by
C and are properly freed manually. The targets for GC are strings and
dictionary objects in scripts, and the Old GC is not invoked unless
something unusual occurs.

### How do I start making games? Any samples?

Please see the
[Getting Started Guide](docs/mkdocs-en/docs/getting-started.md)
for a step-by-step guide to creating your first game with Suika3.
We also provide sample games in the `game/` directory of this
repository, which you can use as references or starting points for
your own projects.

### How do I go about developing plugins?

Please see the [Plugin Development Guide](docs/mkdocs-en/docs/plugin.md)
for detailed instructions on how to create plugins for Suika3.

### Can I migrate from existing assets (like Ren'Py or Unity)?

Only consider migration when targeting mobile is necessary and your
existing engine makes it difficult. Otherwise, migration may not be
worth the effort.

Images and audio can be migrated by batch converting their formats,
but the UI requires manual porting. Scenarios also need manual
porting, though AI could enable some degree of automated conversion.

### Where's the document? Is it the latest version?

See [Documentation](docs/mkdocs-en/docs/index.md) for the latest
documentation.

### Is Japanese/Chinese/... supported?

Games are Unicode compliant, so you can use any language that does not
use Unicode composition in your game content. The engine itself is
internationalized, and error messages are shown in local languages.

For Japanese and CJK games, Suika3 supports:
* Kinsoku characters including line breaking rules
* Ruby annotations
* Vertical writing

Note that Right-To-Left writing system is currently not implemented
yet because of the lack of the speakers in the development team.

### Where should I go if I'm in trouble?

Please ask questions on GitHub or Discord. If you're unfamiliar with
GitHub and don't like Discord, email is fine too ;-)

### What about SLAs and maintenance contracts for businesses?

Please request the service level you require. For a contract covering
only bug fixes, the cost is typically around 100 USD per month. For a
comprehensive contract including feature additions, it is
approximately 5000 USD per game title. If you are based in an emerging
country and cannot cover these costs, please reach out to discuss
options.

### What are the minimum requirements?

Please see the [Compatibility List](#compatibility-list) for detailed
information on supported platforms and their requirements.

- Official Configuration
    - Display: 640x480 24bpp
    - CPU: 1GHz
    - RAM: 512MB
    - GPU: DirectX 9 / OpenGL 3.0 / OpenGL ES 2.0 / Metal 1.0
    - Disk: 2MB + assets

- Minimum Configuration for Embedded Devices
    - Display: Any size, 24bpp required
    - CPU: 200MHz
    - RAM: 32MB
    - GPU: Not required
    - Disk: 2MB + assets

### What does "Supported" mean?

This indicates that the sample game has passed the verification
process to ensure it runs on either the actual hardware or an
emulator.

It just means we have a working codebase for the platform, and you
need a final QA for your game.

### Will there be breaking changes? What is the compatibility policy?

No backward-breaking changes to API specification will be made to past
releases. Existing API functions will not be changed, and new APIs
will be added instead. If behavior must be altered, a flag will be
provided to configure the behavior mode. Note that save data
specifications may not be compatible across releases. For a single
product, please fix and lock the major release version to be used.
