System Requirement Specifications for Suika3
===========================================

## 1. Overview

Suika3 is a high-performance scripting runtime optimized for Visual
Novels (VN) and 2D games. It provides a multi-layered DSL environment
to balance ease of use with professional-grade extensibility.

## 2. Core Components (The DSL Stack)

Suika3 empowers creators through four specialized languages:

- NovelML (Tag-based DSL): A straightforward markup language using
  `[]` tags for rapid VN scenario development.

- Anime (Animation DSL): A dedicated system for layer-based raster
  image animations, controlling affine transform sequences.

- GUI (UI/UX DSL): A flexible toolset for building interactive screens
   with buttons optimized for VN requirements.

- Ray (General-purpose Scripting): A powerful scripting
  language with VN API.
    - Customization: Define bespoke NovelML tags.
    - Performance: JIT-compiled on PC for fast iteration; AOT-compiled
      to native binaries for iOS compliance.
    - Low-level Access: Direct hooks into the Suika3 Core C APIs.

## 3. Key Objectives & Design Philosophy

- Mobile-First Experience: Designed with the belief that smartphones
  are a primary computing device. It avoids PC-centric UI/UX in
  favor of a native mobile feel.

- Store Publishing Compatibility: Fully compliant with iOS/Android store
  policies via AOT compilation and responsive design.

- High Portability:
    - Tier 1: iOS, Android, HarmonyOS NEXT, Windows, macOS, Linux
    - Tier 2: Gaming Consoles
    - Tier 3: Chromebook, Wasm (WebAssembly)

- Beyond Visual Novels: While VN-centric, the underlying 2D foundation
  allows for genre-fusing (e.g., VN + RPG or Action).

## 4. Out of Scope / Constraints

To maintain portability and performance, Suika3 explicitly excludes:

- PC-Exclusive Features: Suika3 is not a replacement for legacy
  PC-only VN engines.

- Full-scale Web Deployment: The Wasm port is intended for demos, not
  primary distribution.

- 3D Graphics: Currently focused on 2D (future 3D support is planned
  alongside AI-driven asset generation).

- Proprietary Middleware: No support for closed technologies like
  Live2D to ensure maximum engine portability.

## 5. NovelML

## 6. Ray

## 7. Anime

## 8. GUI

## 9. Config

