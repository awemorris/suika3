Suika3
======

<div align="center">
  <img src="https://raw.githubusercontent.com/suika3-community/suika3/refs/heads/main/docs/img/logo-small.png">
</div>

A next-generation visual novel engine for professional creators,
designed to run natively across virtually all platforms,
ensuring long-term support for the 2030s and beyond.

![platforms](https://github.com/awemorris/PlayfieldEngine/blob/main/docs/img/platforms.png)

---

## TL;DR

* **High-Performance:** Native, written in C
* **Better Scripting:** JIT VM with AOT fallback for App Store compliance
* **Virtuall All Platforms:**
    * **Desktop:** Windows, macOS, Linux, Chromebook, Raspberry Pi
    * **Mobile:** iOS, Android, HarmonyOS NEXT
    * **Web:** WebAssembly
    * **Console:** Xbox GDK, PS5 and Switch via Unity Integration
* Target: Empowering both commercial and indie studios

[Join Discord](https://discord.gg/YZsq9u9Mgr)

---

## Introduction

Suika3 is a high-performance, production-grade visual novel engine
specifically engineered for commercial app development, maintained by
the `Suika3 Community` and led by Awe Morris.

Engineered with state-of-the-art techniques in computer science,
Suika3 delivers native performance and true portability, empowering
creators worldwide to build timeless experiences that endure for
decades.

Games made with Suika3 are powered by `NovelML 3.0` and `SuikaScript 3.0`.

* **NovelML 3.0**: A tag-based, human-readable markup language
  designed specifically for visual novels. It features concise,
  declarative tags for seamless dialogue and scene
  control. Developers can also extend its capabilities by defining
  custom tags using SuikaScript.

* **SuikaScript 3.0**: The robust extension language for Suika3. It
  utilizes a high-speed JIT compiler on desktop platforms and can be
  compiled into native binaries for production or mobile
  environments.

---

## Kanban (Status): Final Polishing for 1.0.0!

We are planning the 1.0.0 release on July 1 2026.
Currently we are working very hard to complete Suika3.

**Our current status is:**

* ✅ Cleaning up the Suika2 code base. (OpenNovel) (June 2024 - November 2024)
* ✅ Implementing the scripting language. (NoctLang) (December 2024 - March 2025)
* ✅ Implementing the 2D game engine. (Playfield Engine) (March 2025 - October 2025)
* ✅ Implementing the tag execution engine. (Suika3) (January 2026 - February 2026)
* ✅ Refactoring all C APIs. (January 2026 - February 2026)
* ✅ Implementing all tags in C. (January 2026 - February 2026)
* ✅ Wrapping all APIs in SuikaScript. (February 2026 - March 2026)

* Remaining Tasks
    * ✅ Final checking for API consistency and freeze the API. (March 7, 2026)
    * Adjusting GUI functionality including GUI animations. (March 10, 2026)
    * Engine Feature Freeze (March 15, 2026)
    * Adding the exporter tool. (March 20, 2026)
    * Adding a sample game. (March 30, 2026)

* Quality Stabilization Period (March 30 - May 30, 2026)
    * Milestone: Release Candidate 1 (March 30, 2026)
    * Milestone: Release Candidate 2 (April 30, 2026)
    * Milestone: Release Candidate 3 (May 30, 2026)
    * Milestone: Release Candidate 4 / Golden Master (June 30, 2026)

* Documentation (Simultaneous with Quality Stabilization)
    * `C API`
    * ✅ `SuikaScript Syntax`
    * ✅ `SuikaScript API`
    * `NovelML Syntax`
    * ✅ `NovelML Tags`
    * `Getting Started`
    * `Plugin Development`
    * ✅ `Build Instruction`
    * `System Requirement Specification`
    * `System Design Specification`
    * ✅ Website

* 1.0.0 Release (July 1, 2026)

---

## Key Features

* **High Performance**: Powered by the Suika JIT VM, it delivers
    5~10x execution speed compared to interpreter.

* **Lightweight**: Engineered to run smoothly at 60 fps even on low-spec
    hardware such as Raspberry Pi in developing regions.

* **Universal Platform**: Designed with a "port anywhere" strategy,
    ensuring compatibility with almost every modern platform.

* **Extensible**: NovelML can be seamlessly expanded by developers
    using SuikaScript.

* **Reliable**: Built on the proven codebase of Suika2 and integrated
    with the modern Playfield Engine.

* **Jot-and-Run**: A powerful markup language designed to bring your
    stories to life in an instant.

---

## Getting Started

A quick path to try Suika3 locally.

### Using prebuilt binary

Please download from [Releases](releases).

### Building locally

Refer to [build.md](docs/mkdocs-en/build.md) for platform-specific instructions.

**Generic instruction:**
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

### SuikaScript

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

---

## Examples

Sorry we are currently preparing examples.

See the `game/` directory for:
* Minimal demo project
* Sample assets and NovelML snippets
* SuikaScript usage patterns

---

### Why SuikaScript?

* **Instant Iteration:** No compile cycles, no waiting. The built-in
    JIT compiler runs your scripts immediately after editing, so you
    can tweak gameplay and see results in real-time.

* **True Native Performance:** Run extensions at full speed on iOS,
    Android, and consoles. Even on platforms where JIT is restricted,
    you never have to sacrifice performance thanks to AOT.

* **Smooth Store Approval:** Dramatically lower the risk of
    rejection. Since final builds consist of native code via AOT,
    you'll breeze through App Store and console certifications.

* **Easy to Learn, Powerful to Use:** The JavaScript-inspired syntax
    is friendly for beginners, yet offers the deep flexibility
    experienced developers expect for complex systems.

* **Long-term Stability:** Because we develop the language in-house,
    you're shielded from breaking changes in upstream projects. We own
    and control the full stack, so your scripts stay compatible forever.

---

## Why Suika3?: Our Philosophy

### Mission: Building a Sustainable Ecosystem

Our dedication to the commercial success of visual novels is a
strategic response to the long-term health of the medium. We believe
that for visual novels to flourish in the 2030s, they must transcend
hobbyist boundaries and re-establish themselves as a thriving creative
industry.

Since the 2010s, the visual novel market has faced significant growth
challenges. One critical bottleneck has been the **lack of
high-performance free/libre software engines** capable of delivering a
seamless **native experience on iOS and Android**.

Without accessible, professional-grade tools for modern mobile
platforms, many creators have been restricted to limited distribution
channels. As a result, the medium has struggled to reach a global,
mobile-first audience, slowing its overall expansion.

While free and hobbyist projects are culturally indispensable, we
believe they may not be sufficient on their own to sustain an entire
creative industry. A truly healthy ecosystem may require:

* **Commercially Viable Games** — Titles that reach players on their
    primary devices and generate the economic activity necessary for
    creators to build long-term careers.

* **Economic Independence** — Empowering talent, particularly in Asia
    and the Global South, to overcome economic barriers through
    storytelling.

* **A Living Industry** — Transforming the medium from a niche
    interest into a sustainable market where creativity can lead to
    self-reliance.

...Is this overly assertive? In our view, it is a natural professional
expectation for commercial success.

### Our Vision: Catalyst for Growth

We do not aim to provide just another tool. Our goal is to provide a
**catalyst for growth**.

By offering a "port-anywhere" engine that delivers native performance
without the overhead of heavy commercial frameworks, we enable
developers to focus on what matters most: **telling stories that
endure.**

Because we love visual novels, we are committed to pushing the medium
forward and ensuring it remains a vibrant and economically viable art
form for decades to come.

### Our Values: Empowering Talents for Sustainable Careers

Suika3 is dedicated to creators who dream of publishing and selling
original visual novels on the App Store and Google Play, regardless of
their hardware limitations.

Our value is to empower talent—particularly in Asia and the Global
South—to build sustainable careers through their storytelling. We
believe that with the right tools, creativity can overcome economic
barriers.

Our commitment to free/libre software is rooted in a simple, practical
goal: fostering sustainable economic independence. This mission is
shaped by our core members' personal experiences in Asia and their
journeys in overcoming adversity. We simply believe that talent, when
equipped with the right tools, inevitably leads to self-reliance.

Creators with different goals may also find excellent resources in
other engines like Unity, Godot, or Ren'Py.

---

## Technical Overview

If these specifications excite you, you're probably in the right place.

Suika3 is not merely a wrapper around SDL combined with a scripting language.
It features its own rendering and audio backends, and its own scripting language,
positioning it as a fully independent game engine.

### Core Architecture

Suika3 is based on `PlayField Engine`, a comprehensive 2D game engine.
This means that Suika3 is fully extensible using the Playfield API.
(Playfield Engine was actually developed just for Suika3.)

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

* **Scripting**:
  Integrates [NoctLang](https://github.com/awemorris/NoctLang),
  a tiny yet mighty language designed for game scripting.

* **Rendering**:
  Supports native DirectX 9/11/12, Metal, OpenGL, OpenGL ES, and WebGL
  for wide compatibility.

* **Audio**:
  Provides lightweight audio support through native DirectSound or XAudio2 (Windows),
  Audio Unit (macOS/iOS), ALSA (Linux), OSS (BSD), and other APIs.

### StratoHAL

StratoHAL originated as the codebase of a 2D game engine developed
since 2001, and has been Zlib-licensed with a proven record of
exceptional stability.  Evolving over a quarter century from the
Windows 9x era, StratoHAL has grown to support macOS, Linux, iOS,
Android, WebAssembly, and Unity.  It has been running reliably on
smartphones for more than a decade.  While SDL3 already exists as a
popular free/libre software alternative, StratoHAL covers the same
major platforms as SDL3 — and uniquely provides console support
through Unity without relying on any NDA-restricted code.

### Platform Support and Components

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

The built-in JIT compiler supports a wide range of CPU architectures, including:

* Intel x86 (32-bit)
* AMD64/x86_64 (64-bit)
* ARMv7 (32-bit)
* Arm64 (64-bit)
* RISC-V (32-bit/64-bit)
* PowerPC/POWER (32-bit/64-bit)
* MIPS (32-bit/64-bit)

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
|iOS            |Interpreter or AOT |
|Android        |Interpreter or AOT |
|HarmonyOS NEXT |Interpreter or AOT |
|WebAssembly    |Interpreter or AOT |
|Unity Plugin   |Interpreter or AOT |
|Xbox           |Interpreter or AOT |
|FreeBSD        |JIT                |
|NetBSD         |JIT                |
|OpenBSD        |Interpreter or AOT |

### Runtime Footprint

|Platform       |Binary Size    |Memory Usage (GC)                                              |
|---------------|---------------|---------------------------------------------------------------|
|All targets    |~2 MB          |Allocates 256 MB at startup for GC; does not exceed that limit |

The garbage collector commits 256 MB at startup because currently 256
MB is the maximum heap size in the default configuration.  So the
required system memory size is 256 MB. (However, this can be adjusted
to a smaller or larger values.)

---

## Garbage Collection (GC) in Suika3

Suika3 features a high-performance generational garbage collector,
inspired by the architecture of the Java HotSpot VM. This design
ensures that developers can focus on creation without being
interrupted by the dreaded "GC spikes" or frame drops.

### Core Mechanism: Generational GC

The system categorizes objects into two groups to optimize memory
management:

* Young Generation: Most objects die young. Suika3 handles these using
  a high-speed Copying Algorithm (Semi-space Copy GC), which clears
  out temporary data in a flash. (typically < 0.1ms)

* Old Generation: Long-lived objects are moved here. This area uses a
  Mark-Sweep-Compact GC algorithm, which periodically reorganizes
  memory to prevent fragmentation. (typically 10-300ms)

### Frame-Synchronized Latency Hiding

The real "magic" of Suika3 lies in its timing. By executing the
Young-generation Copy GC every single frame, the system effectively
hides the GC processing time within the natural frame interval.

Thanks to this generational strategy, the heavier Old-generation
Mark-and-Sweep is rarely triggered, maintaining a constant 60 fps
experience for the player.

---

## Supported Platforms

Suika3 primarily targets desktop environments (Windows, Linux, macOS)
while providing a robust migration path for mobile and embedded
systems via the Playfield Engine.

### Platform Availability at a Glance

`Supported` means the upstream (`Playfield Engine`) fully supports it.

| Category    | OS / Environment   | Status       | Last Checked | Checked On                       |
| :---        | :---               | :---         | :---         | :---                             |
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

Suika3 provides the 32-bit binary for backward compatibility,
supporting legacy systems through older runtimes. If you change the
source code to remove the XInput support, you can run Suika3 even on
Windows 2000.

| OS      | Version                     | Patch | CPU    | Runtimes                               | 32-bit Binary | Direct3D | Special   |
|---------|-----------------------------|-------|--------|----------------------------------------|---------------|----------|-----------|
| Windows | 11                          |       | x64    | (None required)                        | ✅            | 12.0     |           |
| Windows | 11                          |       | arm64  | (None required)                        | ✅            | 12.0     |           |
| Windows | 10                          |       | x86    | (None required)                        | ✅            | 12.0     |           |
| Windows | 10                          |       | x64    | (None required)                        | ✅            | 12.0     |           |
| Windows | 10                          |       | arm64  | (None required)                        | ✅            | 12.0     |           |
| Windows | 8.1                         |       | x86    | (None required)                        | ✅            | 11.0     |           |
| Windows | 8.1                         |       | x64    | (None required)                        | ✅            | 11.0     |           |
| Windows | 8                           |       | x86    | (None required)                        | ✅            | 11.0     |           |
| Windows | 8                           |       | x64    | (None required)                        | ✅            | 11.0     |           |
| Windows | 7                           |       | x86    | (None required)                        | ✅            | 11.0     |           |
| Windows | 7                           |       | x64    | (None required)                        | ✅            | 11.0     |           |
| Windows | 7                           | SP1   | x86    | (None required)                        | ✅            | 11.0     |           |
| Windows | 7                           | SP1   | x64    | (None required)                        | ✅            | 11.0     |           |
| Windows | Vista                       |       | x86    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |           |
| Windows | Vista                       |       | x64    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |           |
| Windows | Vista                       | SP1   | x86    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |           |
| Windows | Vista                       | SP1   | x64    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |           |
| Windows | Vista                       | SP2   | x86    | DirectX 11 Platform Update             | ✅            | 11.0     |           |
| Windows | Vista                       | SP2   | x64    | DirectX 11 Platform Update             | ✅            | 11.0     |           |
| Windows | XP                          |       | x86    | DirectX 9.0b Runtime                   | ❌            | 9.0      | No XInput |
| Windows | XP                          | SP1   | x86    | DirectX 9.0b Runtime                   | ✅            | 9.0      |           |
| Windows | XP                          | SP2   | x86    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |           |
| Windows | XP                          | SP3   | x86    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |           |
| Windows | XP Professional x64 Edition |       | x64    | DirectX End-User Runtimes (June 2010)  | ❌            | 9.0      | No XInput |
| Windows | XP Professional x64 Edition | SP1   | x64    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |           |
| Windows | XP Professional x64 Edition | SP2   | x64    | DirectX End-User Runtimes (June 2010)  | ✅            | 9.0      |           |
| Windows | 2000                        |       | x86    | DirectX End-User Runtimes (Dec 2006)   | ❌            | 9.0      | No XInput |
| Windows | 2000                        | SP1   | x86    | DirectX End-User Runtimes (Dec 2006)   | ❌            | 9.0      | No XInput |
| Windows | 2000                        | SP2   | x86    | DirectX End-User Runtimes (Dec 2006)   | ❌            | 9.0      | No XInput |
| Windows | 2000                        | SP3   | x86    | DirectX End-User Runtimes (Dec 2006)   | ❌            | 9.0      | No XInput |
| Windows | 2000                        | SP4   | x86    | DirectX End-User Runtimes (Dec 2006)   | ❌            | 9.0      | No XInput |

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
| Raspberry Pi OS   | Debian 13             | arm64, armv7    | ✅     | X11, OpenGL ES                |
| Ubuntu            | 24.04 / 22.04 / 20.04 | x86_64, arm64   | ✅     | X11, OpenGL 3                 |
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

---

## Documentation

Documentation is also available at [the Website](https://suika3.vn/docs/)
and it is synchronized to the latest.

* [NovelML 3.0 Syntax Reference](docs/mkdocs-en/docs/tags.md) (In-Progress)
* [NovelML 3.0 Tag Reference](docs/mkdocs-en/docs/tags.md)
* [SuikaScript 3.0 Syntax Reference](docs/mkdocs-en/docs/syntax.md)
* [SuikaScript 3.0 API Reference](docs/mkdocs-en/docs/api.md)
* [Build Instruction](docs/mkdocs-en/docs/build.md)

---

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

### How to switch languages

Playfield Engine uses the system locale.  To switch languages, set
your system locale.

### Contributing Translations

We welcome community contributions to improve our translations!
If you want to help us reach more developers, here is how you can contribute:

0.  Do `grep -r _TR()` and detect original messages.
1.  **Locate the Files**: Translation assets are managed across three core directories:
    * `resources/i18n/message.txt` (Suika3 part, `S3_TR()`)
    * `external/PlayfieldEngine/resources/i18n/message.txt` (Playfield Engine part, `PF_TR()`)
    * `external/PlayfieldEngine/external/NoctLang/i18n/message.txt` (NoctLang part, `N_TR()`)
    * `external/PlayfieldEngine/external/StratoHAL/i18n/message.txt` (StratoHAL part, `HAL_TR()`)
2.  **Submit Your Changes**: 
    * Found a typo or a weird machine translation? Feel free to open a **Pull Request**.
    * Want to add a new language? Please open an **Issue** first so we can coordinate!

> [!TIP]
> Manual/Professional translations are prioritized over machine-generated content. Your human touch makes a huge difference!

---

## Third-party Libraries

Our engine is built upon several free/libre software libraries. To
ensure build reproducibility and long-term maintainability, all
necessary source code tarballs and patches are bundled within this
repository under the
`external/PlayfieldEngine/external/StratoHAL/lib/` directory.

### Core Libraries

| Library          | Purpose             | Key Feature                                            |
| :---             | :---                | :---                                                   |
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

|Preset                         |Platform              |Compiler   |Directory                              |Target           |Type           |
|-------------------------------|----------------------|-----------|---------------------------------------|-----------------|---------------|
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

* **Proven Stability:** Includes core modules refined since 2001.

* **Modern Architecture:** Features a clean HAL (Hardware Abstraction
    Layer) separated in 2016 and high-performance GPU rendering
    implemented in 2022.

* **Native Multi-Platform:** Built mostly with C, and the platform
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

1.  **Release Candidate (RC) Phase**: Upon reaching the RC milestone,
    we will initiate rigorous tracking of all identified issues.

2.  **Data-Driven Hardening**: We will analyze bug discovery and
    resolution rates to quantify the software's stability.

3.  **Commercial-Grade Standards**: Our final goal is to achieve a
    level of robustness that meets the requirements of commercial
    visual novel productions.

While the engine is still "young" in terms of uptime, the underlying
architecture inherits over two decades of development philosophy,
ensuring a solid foundation for the upcoming QA process.

---

## License

Suika3 is
[free/libre software](https://www.gnu.org/philosophy/free-sw.en.html),
released under the permissive `ZLib License`.

```
Suika3
Copyright (c) 2026 The Suika3 Community
```

We see Suika3 as part of a broader effort to help build a sustainable
creative ecosystem for visual novels, guided by a clear mission,
vision, and values. At the same time, you are absolutely welcome to
use Suika3 simply as a useful open-source component within your own
projects.

---

## Support & Contact

For general inquiries, bug reports, and technical assistance:

* **GitHub Issues**: Preferred for formal bug tracking and feature
    requests.

* **[Discord Server](https://discord.gg/YZsq9u9Mgr)**: The best place
    for real-time discussions and quick questions.

We strive to provide timely support and help creators bring their
stories to life.

### Professional & Enterprise Support

While the **Suika3 Community** provides active support through GitHub
and Discord, we recognize that **commercial projects** often require
dedicated support or guaranteed response times.

For studios and developers requiring **formal maintenance contracts,
priority bug fixes, or private consultations**, please contact us
directly via email. We are committed to helping ensure your project's
success in a high-stakes production environment.

* **Contact**: Awe Morris <awe@noctvm.io>

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

* **Identity & Expression:** Race, ethnicity, nationality, language,
    gender identity and expression, sexual orientation, and sex
    characteristics.

* **Personal Attributes:** Age, body size, personal appearance, illness,
    and neurodiversity.

* **Life Context:** Faiths, beliefs, disability (e.g. visibility, but
    not limited), level of experience, education, and socio-economic
    status.

* **Justice & Inclusion:** We explicitly welcome individuals with past
    criminal records, firmly believing in the power of second chances
    and the inherent value of every contributor who participates
    respectfully.

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

* **Suika Studio (2001–2004)**: The origin of our codebase, featuring
  our first GUI-based editors.

* **Unfruitiful (2005–2015)**: A decade of R&D focused on establishing
  a robust portability layer for cross-platform support.

* **Suika2 (2016–2024)**: The cornerstone of our current architecture
  and the fruit of a decade of dedicated R&D. It delivers a full
  visual novel experience with extensive platform compatibility.
  Several games with Suika3 were sold on App Store.

* **Playfield Engine (2025–)**: A versatile 2D engine derived from the
  core portability layer of the Suika series.

* **Suika3 (2026–)**: By synthesizing these legacies and introducing
  **NovelML 3.0 / SuikaScript 3.0**, Suika3 delivers the rock-solid
  stability of its predecessors alongside the cutting-edge flexibility
  of modern technology.
