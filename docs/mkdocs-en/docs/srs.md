System Requirement Specifications for Suika3
============================================

## 1. Overview

Suika3 is a full-stack visual novel (VN) engine optimized for cross-platform deployment,
including mobile-first environments. It provides a multi-layered DSL environment designed
to balance ease of authoring with professional-grade extensibility and portability.

**Target Audience**

This document is intended for engineers at organizations that fork Suika3 to build
an in-house engine. It specifies the behavioral, architectural, and interface requirements
that any conformant implementation must satisfy. Game developers using the engine as-is
should refer to the tutorial and tag reference documentation instead.

**Scope**

This document covers:

- Engine architecture and layer responsibilities
- Boot and execution sequence
- DSL specifications (NovelML, Anime, GUI, Ray)
- Platform requirements
- Out-of-scope constraints

For the architectural rationale and design discussion, see [design.md](design.md).
For the full tag reference, see [novelml-tags.md](novelml-tags.md).
For the API reference, see [ray-vn-api.md](ray-vn-api.md) and [ray-2d-api.md](ray-2d-api.md).
Full tag and API requirements in SRS form are given in Appendices A, B, and C (forthcoming).

---

## 2. Design Philosophy

### 2.1 Mobile-First

Suika3 shall treat smartphones and tablets as primary target devices.
UI/UX decisions shall prioritize touch-first interaction patterns over PC-centric conventions.
Specifically:

- The engine shall not place small, persistent button clusters around the message window.
- The system menu shall be accessed via a single hamburger-style button (SysBtn) that
  appears on interaction and auto-hides after inactivity.
- This behavior shall be hard-coded to ensure compliance with mobile store guidelines.
  It may be disabled only for kiosk or demo builds via configuration.

### 2.2 Store Publishing Compatibility

The engine shall be deployable to all major app stores without policy violations.

- On iOS, Ray scripts shall support Ahead-of-Time (AOT) compilation to ANSI C,
  satisfying the App Store prohibition on runtime code generation.
- The engine shall not depend on any proprietary middleware (e.g., Live2D) that would
  restrict store submission or redistribution.
- UI layout and interaction models shall comply with store human interface guidelines
  on all Tier 1 platforms.

### 2.3 High Portability

The engine shall be portable across a wide range of platforms through a hardware
abstraction architecture. Platform support is tiered by required compliance level:

| Tier | Platforms                                              |
|------|--------------------------------------------------------|
| 1    | iOS, Android, HarmonyOS NEXT, Windows, macOS, Linux    |
| 2    | Gaming consoles (via Unity integration)                |
| 3    | WebAssembly, Chromebook                                |

Tier 1 platforms shall receive full feature support and be tested against every release.
Tier 2 and 3 platforms shall receive best-effort support.

### 2.4 Extensibility Beyond Visual Novels

While the engine is optimized for visual novels, the underlying 2D engine layer shall
expose sufficient APIs to support genre-fusing titles (e.g., VN combined with RPG or
action mechanics) without requiring engine modification.

### 2.5 Exclusion of Proprietary Technologies

The engine shall depend exclusively on open, royalty-free technologies.
Closed-source middleware, proprietary asset formats, and patented compression schemes
shall not be introduced into the core engine.

---

## 3. Architecture

### 3.1 Layered Component Model

Suika3 is structured as four discrete layers stacked vertically. Each layer exposes a
public C API to the layer immediately above it and consumes only the C API of the layer
immediately below it. No layer shall bypass this constraint by calling across non-adjacent
layers or accessing internal symbols of another layer.

```
+----------------------------------------+
| VN Engine (Suika3)                     |  Runs .novel (NovelML)
+----------------------------------------+
| 2D Game Engine (Playfield Engine)      |  Runs .ray (Ray / NoctLang)
+----------------------------------------+
| Scripting Language (NoctLang)          |  JIT / AOT infrastructure
+----------------------------------------+
| Hardware Abstraction Layer (StratoHAL) |  OS and hardware portability
+----------------------------------------+
```

This architecture is referred to as the **Layered Component Model**.

The constraints on inter-layer communication shall be:

- Each layer shall implement exactly one area of concern.
- Each layer shall expose its functionality through a public C language API.
- Each layer shall call only the public C API of the layer one level below it.
- Direct access to internal functions or data of non-adjacent layers is prohibited.

This one-to-one dependency model is chosen over object-oriented many-to-many inheritance
to minimize coupling, simplify porting, and enable independent layer replacement.

### 3.2 StratoHAL

StratoHAL (Hardware Abstraction Layer) is the lowest layer. It shall:

- Abstract all OS-specific and hardware-specific operations, including windowing,
  input, audio, file I/O, and rendering surface creation.
- Expose a uniform C API to Playfield Engine regardless of the underlying platform.
- Define the platform-dependent `main()` entry point via the `PF_DEFINE_MAIN` macro.
- Provide the `hal_main()` function as the engine's actual runtime entry point.

### 3.3 NoctLang

NoctLang is a dynamically typed scripting language and its runtime. It shall:

- Provide a sandboxed virtual machine (VM) for executing `.ray` scripts.
- Support Just-In-Time (JIT) compilation on PC platforms for development performance.
- Support Ahead-of-Time (AOT) compilation to ANSI C for native binary deployment,
  required for iOS App Store compliance.
- Expose a C API for VM creation, script loading, function invocation, and value exchange.

### 3.4 Playfield Engine

Playfield Engine is the 2D game engine layer. It shall:

- Load and execute `main.ray` through the NoctLang VM.
- Register StratoHAL event callbacks (setup, start, update, render).
- Expose the low-level 2D rendering and audio API (`Engine.*`) to Ray scripts.
- Drive the frame loop by invoking `update()` and `render()` in `main.ray` each frame.

### 3.5 VN Engine (Suika3)

The VN Engine is the topmost layer. It shall:

- Implement the NovelML interpreter, including all standard tags.
- Manage the layer system (stage, character layers, message box, etc.).
- Implement the Anime, GUI, and Config DSLs.
- Expose the VN-level API (`Suika.*`) to Ray scripts for custom tag development.
- Manage save/load state, including execution position and variable state.

---

## 4. Boot and Execution Sequence

### 4.1 Entry Point Definition

The engine entry point shall be defined in `game.c` using the macro:

```c
PF_DEFINE_MAIN(pf_init_hook, init_aot_code)
```

This macro shall expand to a platform-appropriate `main()` function (or equivalent,
such as `WinMain()` on Windows or `UIApplicationMain()` on iOS).
The `pf_init_hook` parameter registers engine-level initialization.
The `init_aot_code` parameter registers AOT-compiled script code when AOT mode is active;
it shall be `NULL` in interpreter mode.

### 4.2 Bootstrap Sequence

The bootstrap sequence shall proceed as follows:

1. `main()` calls `hal_main()` in StratoHAL.
2. `hal_main()` calls `hal_bootstrap()` in Playfield Engine.
3. `hal_bootstrap()` registers StratoHAL event callbacks for the start, update, and render events.
4. `hal_bootstrap()` creates a NoctLang VM instance.
5. The NoctLang VM loads `main.ray`.
6. `hal_bootstrap()` invokes `setup()` in `main.ray`.
7. `setup()` returns a dictionary containing the window width, height, and title string.
8. `hal_bootstrap()` returns these values to `hal_main()`.
9. `hal_main()` creates the application window at the specified dimensions with the specified title.

### 4.3 Start Sequence

After window creation, StratoHAL fires the start event:

1. StratoHAL invokes the registered start callback in Playfield Engine.
2. Playfield Engine invokes `start()` in `main.ray` via the NoctLang VM.
3. `start()` calls `Suika.start()`.
4. `Suika.start()` loads and parses `config.ini`.
5. `Suika.start()` loads `start.novel` and sets the execution position to the first tag.
6. `Suika.start()` returns. Control returns up the call chain to StratoHAL.

### 4.4 Game Loop

After the start sequence completes, StratoHAL enters the game loop.
Each frame shall proceed as follows:

1. StratoHAL invokes the update callback in Playfield Engine.
2. Playfield Engine invokes `update()` in `main.ray`.
3. `update()` calls `Suika.update()`.
4. `Suika.update()` reads the current tag name and parameters from `start.novel`
   (or the currently loaded NovelML file).
5. `Suika.update()` resolves and calls the corresponding `Tag_*()` function.
6. `Tag_*()` executes one frame of the tag's behavior.
   If the tag's work is complete, it calls `Suika.moveToNextTag()` and returns `true`.
   If more frames are needed, it returns `false` without advancing.
7. Control returns to StratoHAL.
8. StratoHAL invokes the render callback in Playfield Engine.
9. Playfield Engine invokes `render()` in `main.ray`.
10. `render()` calls `Suika.render()`.
11. `Suika.render()` composites and draws all stage layers to the framebuffer.
12. StratoHAL presents the framebuffer and begins the next frame.

---

## 5. DSL Stack

Suika3 exposes four domain-specific languages to game creators. Each targets a distinct
authoring concern and is processed by a distinct subsystem of the engine.

### 5.1 NovelML

NovelML is a tag-based markup language for authoring VN scenarios.

- File extension: `.novel`
- Entry point: `start.novel` (loaded by `Suika.start()`)
- Syntax: A sequence of tags of the form `[tagname param="value" ...]`
- Execution: Tags execute sequentially, one per update cycle (or across multiple frames
  for time-based tags). A single execution position is maintained at all times.

NovelML is the primary authoring surface for game creators.
It is intentionally simple and does not expose general-purpose programming constructs
directly; branching and variables are provided through specific tags (`[if]`, `[set]`, etc.).

### 5.2 Anime

Anime is a declarative language for describing layer-based raster image animations.

- File extension: `.anime`
- Loaded by: `[anime]` tag or indirectly by GUI button state transitions
- Syntax: Named blocks, each describing affine transform sequences for one layer
- Execution: Blocks in a single file may execute in parallel

Anime targets artists and designers who need precise control over timing and
layer transforms without writing code.

### 5.3 GUI

GUI is a declarative language for defining interactive button screens.

- File extension: `.gui`
- Loaded by: `[gui]` tag in NovelML
- Syntax: Named blocks, each defining one button with type, images, position, and behavior
- Execution: Synchronous — `[gui]` blocks until a button is clicked or the screen is dismissed

The GUI DSL is deliberately synchronous to avoid the complexity of asynchronous
callback models, which are difficult for non-programmer authors to reason about.

### 5.4 Ray (NoctLang)

Ray is the general-purpose scripting layer, built on NoctLang.

- File extension: `.ray`
- Entry point: `main.ray` (loaded at bootstrap)
- Syntax: NoctLang (dynamically typed, C-family syntax)
- Execution: JIT on PC; AOT to native binary for iOS

Ray serves two roles:
1. **Engine extension**: Authors define custom NovelML tags as `Tag_*()` functions.
2. **Low-level access**: Direct access to `Suika.*` VN API and `Engine.*` 2D API.

---

## 6. NovelML Requirements

### 6.1 Execution Model

- The engine shall maintain exactly one **execution position** in the currently loaded
  NovelML file at any time.
- Tags shall execute in the order they appear in the file, from top to bottom.
- Execution shall advance to the next tag only when the current tag calls
  `Suika.moveToNextTag()`.
- Time-based tags (e.g., `[bg]` with a fade duration) shall span multiple frames
  before advancing execution.

### 6.2 File Loading

- The engine shall load `start.novel` automatically on startup via `Suika.start()`.
- The `[load]` tag shall replace the current execution context with a new NovelML file.
- The `[load]` tag shall optionally jump to a named label within the loaded file.
- Loaded files shall share the same variable namespace as the calling file.

### 6.3 Variable Model

- The engine shall support a flat, dynamically typed variable namespace shared
  across all loaded NovelML files within a session.
- Variables shall be set and read by tags (`[if]`, `[choose]`, etc.) using named identifiers.
- The variable namespace shall be saved and restored as part of save/load state.

### 6.4 Localization

- The `[text]` tag shall support locale-suffixed `text-<locale>` parameters.
- The `[choose]` tag shall support locale-suffixed `text<N>-<locale>` parameters.
- The engine shall resolve the best-matching locale using the following priority:
  1. Most specific locale variant (e.g., `text-en-gb`)
  2. Language-level fallback (e.g., `text-en`)
  3. Unsuffixed default (`text`)
- The engine shall use the locale specified by `game.locale` in `config.ini`,
  or the OS system locale if `game.locale` is empty.
- Changing `game.locale` at runtime (via `[config]` or `Suika.setConfig()`) shall
  take effect on all subsequent tag executions without requiring a restart.

### 6.5 Macro System

- The engine shall support macro definition via `[defmacro]` / `[endmacro]`.
- A macro shall be callable via `[callmacro]` and shall return via `[returnmacro]`.
- Macros shall support parameterized invocation.

### 6.6 Plugin Tag Extension

- Any Ray function named `Tag_<name>()` that is loaded into the NoctLang VM shall
  become available as the NovelML tag `[<name>]`.
- Plugin tags shall follow the same execution contract as built-in tags:
  they shall call `Suika.moveToNextTag()` when complete and return `true`.
- Plugins shall be loaded via `Suika.loadPlugin({name: "<name>"})` in `main.ray`.
- Plugin files shall reside at `system/plugin/<name>/<name>.ray`.
- A plugin shall define a `plugin_init_<name>()` function that is called on load.

### 6.7 Save and Load

- The engine shall be capable of saving the current execution position (file path and
  tag index) and the full variable namespace to a save slot.
- On load, the engine shall restore execution to the saved position and restore all variables.
- The engine shall support multiple save slots.
- In release mode (`release_mode.enable=true`), save data shall be written to the
  OS-designated user data directory rather than the game folder.

---

## 7. Ray Requirements

### 7.1 NoctLang Language Requirements

The NoctLang VM embedded in the engine shall support the following language features:

- **Dynamic typing**: Variables shall not require type declarations and shall accept
  any value type at any time.
- **Primitive types**: Integer, floating-point, string, boolean, and null.
- **Composite types**: Arrays (ordered, integer-indexed) and dictionaries (string-keyed).
- **Control flow**: `if`/`else`, `for`/`while` loops, `break`, `continue`, `return`.
- **Functions**: Named functions, anonymous functions (lambdas), and first-class function values.
- **Closures**: Functions shall capture variables from their enclosing scope.
- **Range iteration**: The `..` operator shall produce an iterable integer range.
- **Global variables**: Variables declared outside any function shall be accessible
  from all functions in the same script file.

### 7.2 JIT Execution Requirements

- On PC platforms (Windows, macOS, Linux), the NoctLang VM shall support
  Just-In-Time compilation for performance during development iteration.
- JIT compilation shall be transparent to the script author.

### 7.3 AOT Execution Requirements

- The `suika3-aotcomp` tool shall convert `.ray` scripts into ANSI C source code.
- The generated `library.c` shall be compiled together with the engine into a
  single native binary.
- In AOT mode, calls to `Suika.loadPlugin()` in `main.ray` shall be disabled,
  as plugin scripts are compiled directly into the binary.
- AOT mode is required for iOS App Store compliance and shall be supported on
  all Tier 1 platforms.
- See [aot.md](aot.md) for the AOT compilation workflow.

### 7.4 Plugin System Requirements

- The engine shall support loading Ray plugin files at runtime via `Suika.loadPlugin()`.
- Each plugin shall reside at `system/plugin/<name>/<name>.ray`.
- Each plugin shall define a function `plugin_init_<name>()` that is called
  immediately after the plugin is loaded.
- All `Tag_*()` functions defined in a loaded plugin shall become available as
  NovelML tags after the plugin is loaded.
- In AOT mode, plugins shall be compiled into the binary and their init functions
  called at startup instead of being loaded at runtime.

---

## 8. Anime Requirements

### 8.1 File Structure

- An Anime file shall consist of one or more named blocks.
- Each block shall describe a transform sequence for exactly one named layer.
- Block names shall be arbitrary identifiers with no semantic effect on execution.

### 8.2 Layer Properties

Each block shall support the following animatable properties:

| Property         | Description                                     |
|------------------|-------------------------------------------------|
| `from-x` / `to-x` | Horizontal position in pixels                 |
| `from-y` / `to-y` | Vertical position in pixels                   |
| `from-a` / `to-a` | Alpha (transparency), 0–255                   |
| `from-sx` / `to-sx` | Horizontal scale factor                     |
| `from-sy` / `to-sy` | Vertical scale factor                       |
| `from-r` / `to-r`   | Rotation in degrees                         |

### 8.3 Coordinate System

- Absolute coordinates shall be specified as plain integers (e.g., `100`).
- Relative coordinates shall be prefixed with `r` (e.g., `r0` = current position,
  `r-100` = 100 pixels left of current position).

### 8.4 Timing

- Each block shall specify `start:` and `end:` times in seconds, relative to
  the start of the anime file's playback.
- The engine shall interpolate property values linearly between `from-*` and `to-*`
  values over the specified time range.

### 8.5 Parallel Execution

- All blocks within a single Anime file shall execute in parallel, not sequentially.
- The `[anime]` tag shall block NovelML execution until all blocks in the file complete.

---

## 9. GUI Requirements

### 9.1 Button Definition Model

- A GUI file shall consist of a `global` block and one or more named button blocks.
- The `global` block shall specify fade-in and fade-out durations for the GUI screen.
- Each button block shall specify at minimum: `type`, position (`x`, `y`),
  and idle/hover image paths.

### 9.2 Execution Model

- The `[gui]` tag shall display the defined buttons and block NovelML execution.
- Execution shall resume only when the player clicks a button or dismisses the GUI.
- Asynchronous button callbacks (buttons active during normal tag execution) are
  explicitly not supported and shall not be implemented.

### 9.3 Button Types

The engine shall support the following button types:

| Type          | Behavior                                                       |
|---------------|----------------------------------------------------------------|
| `label`       | Jump to a named label in the current NovelML script.           |
| `save`        | Open the save screen.                                          |
| `load`        | Open the load screen.                                          |
| `quit`        | Exit the application.                                          |
| `language`    | Switch the active locale to the value specified by `lang:`.    |
| `config`      | Open the configuration screen.                                 |
| `history`     | Open the dialogue history screen.                              |
| `auto`        | Toggle auto-advance mode.                                      |
| `skip`        | Toggle skip mode.                                              |
| `preview`     | Display a text preview area within the GUI.                    |
| `custom`      | Invoke a named Ray function.                                   |

### 9.4 Language Button Requirements

- A `language` button shall display an `image-active` image when its `lang:` value
  matches the current `game.locale`.
- Clicking a `language` button shall set `game.locale` to the button's `lang:` value
  and take effect immediately on all subsequent tag executions.
- The new locale shall persist across sessions (saved to global save data).

### 9.5 Animation Integration

- Each button shall optionally specify `idle_anime` and `hover_anime` paths.
- The engine shall play `hover_anime` when the pointer enters the button,
  and resume `idle_anime` when the pointer leaves.
- Button state animations shall loop continuously.

---

## 10. Config Requirements

### 10.1 File Format

- The engine shall load `config.ini` from the game root directory on startup.
- The file shall use a flat `key=value` format, one entry per line.
- Lines beginning with `#` shall be treated as comments and ignored.
- Keys shall be organized by namespace prefix (e.g., `msgbox.*`, `character.*`).

### 10.2 Runtime Modification

- The `[config]` NovelML tag shall allow any config key to be modified at runtime.
- The `Suika.setConfig()` Ray API function shall provide the same capability from scripts.
- Runtime modifications shall take effect immediately on subsequent engine behavior.
- Modified values shall be persisted in global save data so they survive session restarts.

### 10.3 Save Data Classification

Config keys shall be classified into two categories:

- **Global save keys**: Persisted across all save slots (e.g., volume, locale, seen-message flags).
  These are written to a single global save file.
- **Local save keys**: Persisted per save slot (e.g., current chapter name for slot thumbnails).
  These are written alongside the save slot data.

The engine shall expose `Suika.isGlobalSaveConfig()` and `Suika.isLocalSaveConfig()`
to allow Ray scripts to query the classification of any key.

### 10.4 Release Mode

- When `release_mode.enable=true`, the engine shall write all save data to the
  OS-designated user data directory (e.g., `%APPDATA%` on Windows, `~/Library` on macOS).
- When `release_mode.enable=false` (default), save data shall be written to the
  game's working directory.
- Release mode shall be required for store submissions on all Tier 1 platforms.

---

## 11. Platform Requirements

### 11.1 Common Requirements (All Platforms)

The following requirements apply to all supported platforms:

- **Unicode**: The engine shall render text using Unicode-encoded TrueType fonts.
  All UI text, dialogue, and file paths shall support Unicode.
- **Localization**: The engine shall detect the OS system locale on startup and
  use it as the default `game.locale` if not overridden in `config.ini`.
- **Save data compatibility**: Save data written on one Tier 1 platform shall be
  loadable on any other Tier 1 platform without modification, provided the game
  files are identical.
- **Asset packaging**: The engine shall support loading game assets from either
  a packed `assets.arc` archive or loose files in the working directory.
  `assets.arc` takes precedence if both are present.

### 11.2 Tier 1 Platform Requirements

#### Windows

- The engine shall build using Visual Studio 2022 or later, or GCC/Clang on WSL.
- Game files shall be distributed as `suika3.exe` and `assets.arc` in the same folder.
- Save data shall be written to `%APPDATA%` when release mode is enabled.

#### macOS

- The engine shall build using Xcode 8.2.1 or later.
- Simple distribution shall be supported via `Suika3.dmg` paired with `assets.arc`.
- Full distribution (embedded assets, custom icon) shall be supported via the
  Xcode project at `SDK/macos/project/`.
- Save data shall be written to `~/Library/Application Support/` when release mode is enabled.

#### Linux

- The engine shall build using GCC 4.4 or later, or Clang, with CMake 3.22 or later.
- The engine depends on GStreamer for audio/video; Flatpak is the recommended
  distribution method to ensure dependency availability on player systems.
- For SteamOS, the recommended Valve packaging method for Steam Deck shall be followed.

#### iOS

- Ray scripts shall be compiled via AOT before submission to the App Store.
- The engine shall build using the Xcode project at `SDK/ios/`.
- A VS Code build task (`Build iOS IPA`) shall be provided for automated on-device deployment.
- The device shall be registered as a development device in Xcode prior to use of
  the VS Code task.

#### Android

- Game files shall be deployed as loose files into `SDK/android/app/src/main/assets/`.
  `assets.arc` is not used on Android.
- The engine shall build using Android Studio or the VS Code `Build Android APK` task.
- The VS Code task shall automatically download OpenJDK and the Android SDK if absent.

#### HarmonyOS NEXT (OpenHarmony)

- Game files shall be deployed as loose files into
  `SDK/openharmony/entry/src/main/resources/rawfile/`.
  `assets.arc` is not used on HarmonyOS NEXT.
- The engine shall build using DevEco Studio.

### 11.3 Tier 2 Platform Requirements (Consoles via Unity)

- The engine shall be integrable into a Unity project via the plugin at `SDK/unity/`.
- Game files shall be deployed as loose files into `SDK/unity/Assets/StreamingAssets/`.
- The Unity project shall require **Allow unsafe code** enabled in Player Settings.
- The Unity project shall require the audio sampling rate set to **44100 Hz**.
- Console-specific certification and submission requirements are outside the scope
  of the engine and are the responsibility of the integrating party.

### 11.4 Tier 3 Platform Requirements (WebAssembly)

- The engine shall be deployable as a WebAssembly application via `SDK/wasm/index.html`
  paired with `assets.arc` on a static web server.
- The WebAssembly build is intended for demos and browser-based previews,
  not as a primary distribution channel.
- A local test server (`suika3-web.exe` on Windows; `python3 -m http.server` on
  macOS/Linux) shall be provided for development use.

---

## 12. Out of Scope

The following are explicitly outside the scope of Suika3 and shall not be introduced
into the core engine without a revision to this specification:

### 12.1 PC-Exclusive Features

Suika3 is not a replacement for legacy PC-only VN engines.
Features that cannot be implemented portably across all Tier 1 platforms
shall not be added to the core engine.

### 12.2 3D Graphics

The engine is currently focused on 2D rendering.
3D graphics support is not in scope for the current version.
Future versions may introduce 3D support alongside AI-driven asset generation pipelines,
but this requires a separate specification revision.

### 12.3 Proprietary Middleware

Integration with proprietary middleware (e.g., Live2D, Spine, FMOD) shall not be
added to the core engine. Such integrations may be implemented as external plugins
by the integrating party, but shall not introduce store submission restrictions or
royalty obligations on the core engine.

### 12.4 WebAssembly as Primary Distribution

The WebAssembly port is scoped for demo and preview use only.
It shall not be treated as a primary distribution channel.
Features that would require significant WebAssembly-specific implementation
(e.g., persistent local file system access, background audio on mobile browsers)
are out of scope.

---

## Appendix A: NovelML Tag Requirements

This appendix specifies the behavioral requirements for every built-in NovelML tag.
Each tag shall be implemented as a `Tag_<name>()` function in Ray and registered via
`Suika.installTag()`. When the VN Engine resolves a tag name to a function and the
function completes its frame work, it shall call `Suika.moveToNextTag()` to advance
the execution pointer, unless the tag is explicitly designed to hold execution across
multiple frames (e.g., `[text]`, `[wait]`, `[click]`).

### A.1 Localization Parameter Resolution

For tags that accept localized parameters (e.g., `text`, `text-<locale>`, `voice`,
`voice-<locale>`), the implementation shall apply the following priority when the
current locale is `<lang>-<region>`:

1. Parameter with suffix `-<lang>-<region>` (e.g., `text-en-gb`)
2. Parameter with suffix `-<lang>` (e.g., `text-en`)
3. Parameter without suffix (e.g., `text`)

The implementation shall support the following locale suffixes:

| Suffix    | Language                          |
|-----------|-----------------------------------|
| `-en`     | English (fallback for all regions)|
| `-en-us`  | English (United States)           |
| `-en-gb`  | English (United Kingdom)          |
| `-en-au`  | English (Australia)               |
| `-en-nz`  | English (New Zealand)             |
| `-fr`     | French (fallback)                 |
| `-fr-fr`  | French (France)                   |
| `-fr-ca`  | French (Canada)                   |
| `-es`     | Spanish (fallback)                |
| `-es-la`  | Spanish (Latin America)           |
| `-de`     | German                            |
| `-it`     | Italian                           |
| `-ru`     | Russian                           |
| `-el`     | Greek                             |
| `-zh`     | Chinese (Simplified)              |
| `-zh-tw`  | Chinese (Traditional, Taiwan)     |
| `-ja`     | Japanese                          |

There shall be no automatic fallback from Traditional Chinese (`-zh-tw`) to
Simplified Chinese (`-zh`).

### A.2 Tag Specifications

#### `[anime]` — Run Animation

The `[anime]` tag shall load and execute an animation definition file.

| Parameter     | Default   | Type    | Requirement                                           |
|---------------|-----------|---------|-------------------------------------------------------|
| `file`        | —         | String  | Required unless `stop="true"`. Path to anime file.    |
| `async`       | `false`   | Boolean | If `false`, execution shall block until completion.   |
| `register`    | —         | String  | Required to start or stop a named async animation.    |
| `stop`        | `false`   | Boolean | If `true`, shall stop the named registered animation. |
| `showsysbtn`  | `true`    | Boolean | Controls system button visibility (sync only).        |
| `showmsgbox`  | `true`    | Boolean | Controls message box visibility (sync only).          |
| `shownamebox` | `true`    | Boolean | Controls name box visibility (sync only).             |

When `async="true"`, the tag shall return immediately and the animation shall run
in the background. The `register` parameter shall be required to later stop the
animation via `stop="true"`.

---

#### `[bg]` — Change Background

The `[bg]` tag shall change the background layer with an optional transition effect.

| Parameter  | Default    | Type    | Requirement                                              |
|------------|------------|---------|----------------------------------------------------------|
| `file`     | —          | String  | Required. Set to `none` to clear the background.         |
| `time`     | `0`        | Float   | Transition duration in seconds.                          |
| `clear`    | `false`    | Boolean | If `true`, shall hide all character layers.              |
| `fade`     | `normal`   | String  | Transition method: `normal`, `rule`, or `melt`.          |
| `rule`     | —          | String  | Required when `fade` is `rule` or `melt`.                |
| `x`        | `0`        | Integer | X-axis offset of the background.                        |
| `y`        | `0`        | Integer | Y-axis offset of the background.                        |
| `scale-x`  | `1.0`      | Float   | X-axis scaling factor.                                   |
| `scale-y`  | `1.0`      | Float   | Y-axis scaling factor.                                   |
| `center-x` | `0`        | Integer | Rotation pivot X.                                        |
| `center-y` | `0`        | Integer | Rotation pivot Y.                                        |
| `rotate`   | `0`        | Float   | Rotation in degrees (clockwise positive).                |
| `alpha`    | `255`      | Integer | Opacity: 0 (transparent) to 255 (opaque).               |

Numeric parameters shall support an `r` prefix to denote a relative offset from
the current value (e.g., `x="r50"` moves 50 pixels right of the current position).

---

#### `[bgm]` — Play Background Music

The `[bgm]` tag shall start or stop background music playback.

| Parameter | Default | Type    | Requirement                                            |
|-----------|---------|---------|--------------------------------------------------------|
| `file`    | —       | String  | Required. Set to `none` to stop playback.              |
| `once`    | `false` | Boolean | If `true`, the track shall not loop.                   |

The implementation shall accept Ogg Vorbis files at 44,100 Hz sampling rate.
By default, BGM shall loop indefinitely until stopped or replaced.

---

#### `[callmacro]` — Call Macro

The `[callmacro]` tag shall invoke a named macro defined by `[defmacro]`.

| Parameter  | Default | Type   | Requirement                                                |
|------------|---------|--------|------------------------------------------------------------|
| `name`     | —       | String | Required. Must match a `[defmacro]` name.                  |
| `file`     | —       | String | Optional. File containing the macro; defaults to current.  |
| `arg1–10`  | —       | String | Arguments accessible inside the macro as `${arg1}`–`${arg10}`. |

After the macro completes or a `[returnmacro]` is reached, execution shall return
to the tag immediately following `[callmacro]`.

---

#### `[ch]` — Character Display

The `[ch]` tag shall load, replace, or hide character images on named layer slots.

**Layer slots** (each accepts a filename or `none`): `bg`, `back`, `left`,
`left-center`, `center`, `right-center`, `right`, `face`.

**Per-slot suffix parameters** (e.g., `center-x`, `left-a`):

| Suffix      | Default | Requirement                                     |
|-------------|---------|------------------------------------------------|
| `-x`        | `0`     | X position; supports `r` prefix for relative.  |
| `-y`        | `0`     | Y position; supports `r` prefix for relative.  |
| `-a`        | `255`   | Opacity 0–255.                                  |
| `-scale-x`  | `1.0`   | X scaling; supports `r` prefix.                 |
| `-scale-y`  | `1.0`   | Y scaling; supports `r` prefix.                 |
| `-center-x` | `0`     | Rotation pivot X.                               |
| `-center-y` | `0`     | Rotation pivot Y.                               |
| `-rotate`   | `0`     | Rotation in degrees; supports `r` prefix.       |
| `-dim`      | `false` | If `true`, renders layer at 50% brightness.     |

**Transition parameters:**

| Parameter | Default    | Requirement                                        |
|-----------|------------|----------------------------------------------------|
| `time`    | `0`        | Transition duration in seconds; applies to all slots. |
| `fade`    | `normal`   | `normal`, `rule`, or `melt`.                       |
| `rule`    | —          | Required when `fade` is `rule` or `melt`.          |

---

#### `[chapter]` — Set Chapter Name

The `[chapter]` tag shall update the current chapter name stored in game state.

| Parameter | Default | Requirement                                  |
|-----------|---------|----------------------------------------------|
| `name`    | —       | Required. The chapter name string.           |

The chapter name shall be included in save slot metadata and shall be readable
via `Suika.getChapterName()`.

---

#### `[choose]` — Display Selection Options

The `[choose]` tag shall display up to 8 choice buttons and store the player's
selection in a named variable.

| Parameter         | Default | Requirement                                               |
|-------------------|---------|-----------------------------------------------------------|
| `text1`–`text8`   | —       | Display text for each option. At least one required.      |
| `text<N>-<locale>`| —       | Localized display text; resolved via §A.1.                |
| `name`            | —       | Required. Variable name to receive the selected value.    |
| `value1`–`value8` | —       | Value stored when the corresponding option is selected.   |
| `time`            | `0`     | If >0, auto-selects after this many seconds (timer mode). |

When `value<N>` is absent for a selected option, the implementation shall store
the corresponding `text<N>` in the variable instead.

---

#### `[click]` — Wait for Click

The `[click]` tag shall pause execution until the player clicks the mouse button
or presses a recognized advance key. It takes no parameters.

---

#### `[config]` — Set Configuration Value

The `[config]` tag shall update a named configuration key at runtime.

| Parameter | Requirement                                              |
|-----------|----------------------------------------------------------|
| `name`    | Required. Must match a valid key in `config.ini`.        |
| `value`   | Required. New value string.                              |

The effect shall be identical to calling `Suika.setConfig({key:..., value:...})`.

---

#### `[defmacro]` — Define Macro

The `[defmacro]` tag shall mark the start of a macro definition block.
All tags between `[defmacro]` and the corresponding `[endmacro]` shall be
stored and not executed at definition time.

| Parameter | Requirement                                      |
|-----------|--------------------------------------------------|
| `name`    | Required. Unique identifier for the macro.       |

---

#### `[else]` — Default Conditional Branch

The `[else]` tag shall mark the fallback branch of an `[if]` block. It takes
no parameters. It shall be executed only when no preceding `[if]` or `[elseif]`
condition in the same block evaluated to true.

---

#### `[elseif]` — Additional Conditional Branch

The `[elseif]` tag shall evaluate a condition within an `[if]` block.

| Parameter | Requirement                                      |
|-----------|--------------------------------------------------|
| `lhs`     | Required. Left-hand side of comparison.          |
| `op`      | Required. Comparison operator (see `[if]`).      |
| `rhs`     | Required. Right-hand side of comparison.         |

---

#### `[endif]` — End Conditional Block

The `[endif]` tag shall mark the end of an `[if]` conditional block. It takes
no parameters. Every `[if]` shall have exactly one matching `[endif]`.

---

#### `[endmacro]` — End Macro Definition

The `[endmacro]` tag shall mark the end of a `[defmacro]` block. It takes no
parameters. Execution control shall return to the call site after `[endmacro]`
is reached during macro execution.

---

#### `[goto]` — Jump to Label

The `[goto]` tag shall unconditionally transfer execution to a named label within
the current script file.

| Parameter | Requirement                                      |
|-----------|--------------------------------------------------|
| `name`    | Required. Must match a `[label]` name.           |

---

#### `[gui]` — Show GUI

The `[gui]` tag shall load and display a GUI definition file. Execution shall
block until the GUI completes.

| Parameter | Requirement                                              |
|-----------|----------------------------------------------------------|
| `file`    | Required. Path to the `.gui` definition file.            |

---

#### `[if]` — Conditional Branch

The `[if]` tag shall evaluate a condition and execute the enclosed block only
if the condition is true.

| Parameter | Requirement                                      |
|-----------|--------------------------------------------------|
| `lhs`     | Required. Left-hand side value or variable ref.  |
| `op`      | Required. One of: `===`, `==`, `>`, `>=`, `<`, `<=`. |
| `rhs`     | Required. Right-hand side value or variable ref. |

Operators `===` shall perform string comparison; all other operators shall
perform numeric comparison after parsing both sides as floating-point numbers.
Variable references of the form `${name}` shall be expanded before comparison.

---

#### `[label]` — Define Label

The `[label]` tag shall define a named jump target.

| Parameter | Requirement                                      |
|-----------|--------------------------------------------------|
| `name`    | Required. Unique within the current script file. |
| `desc`    | Optional. Comment string; no effect.             |

---

#### `[layer]` — Direct Layer Manipulation

The `[layer]` tag shall directly set properties of a named stage layer.

| Parameter  | Default | Requirement                                        |
|------------|---------|----------------------------------------------------|
| `name`     | —       | Required. Layer name (see full list in §A.3).      |
| `file`     | —       | If provided, loads the image; `none` clears it.    |
| `x`        | `0`     | X position.                                        |
| `y`        | `0`     | Y position.                                        |
| `alpha`    | `255`   | Opacity 0–255.                                     |
| `scale-x`  | `1.0`   | X scaling factor.                                  |
| `scale-y`  | `1.0`   | Y scaling factor.                                  |
| `center-x` | `0`     | Rotation pivot X.                                  |
| `center-y` | `0`     | Rotation pivot Y.                                  |
| `rotate`   | `0.0`   | Rotation in degrees.                               |

Layers marked "(Invisible to `[layer]`)" in §A.3 shall not be accessible
via this tag.

---

#### `[load]` — Load Script File

The `[load]` tag shall terminate execution of the current script and begin
executing a new NovelML file.

| Parameter | Requirement                                              |
|-----------|----------------------------------------------------------|
| `file`    | Required. Path to the target `.novel` file.              |
| `label`   | Optional. Label to jump to within the new file.          |

All variables shall persist across `[load]` transitions. Any tags placed after
`[load]` in the original file shall not be executed.

---

#### `[move]` — Animate Layer

The `[move]` tag shall animate one or more layer properties over a specified duration.

| Parameter        | Default    | Requirement                                          |
|------------------|------------|------------------------------------------------------|
| `name`           | —          | Required. Target layer (see slots list in `[ch]`).   |
| `time`           | —          | Required. Duration in seconds.                       |
| `async`          | `false`    | If `true`, returns immediately without blocking.     |
| `accel`          | `normal`   | Acceleration curve: `normal`, `accel`, `deaccel`, `smoothstep`, `invsmoothstep`. |
| `<slot>-<suffix>`| —          | Per-layer property targets; supports `r` prefix for relative. |

Animatable per-slot suffixes: `-x`, `-y`, `-a`, `-scale-x`, `-scale-y`,
`-center-x`, `-center-y`, `-rotate`, `-dim`.

---

#### `[pencil]` — Draw Text on Layer

The `[pencil]` tag shall render text directly onto a stage layer's image surface.

| Parameter       | Default     | Requirement                             |
|-----------------|-------------|-----------------------------------------|
| `text`          | —           | Required. Text string to draw.          |
| `layer`         | `text1`     | Target layer name.                      |
| `font-type`     | `0`         | Font slot index (0–3).                  |
| `font-size`     | `16`        | Font size in points.                    |
| `color`         | `#000000`   | Text color in `#RRGGBB` format.         |
| `outline-width` | `0`         | Outline pixel width.                    |
| `outline-color` | `#ffffff`   | Outline color.                          |
| `line-margin`   | —           | Extra space between lines.              |
| `char-margin`   | `0`         | Extra space between characters.         |
| `x`             | `0`         | Drawing area X position.                |
| `y`             | `0`         | Drawing area Y position.                |
| `width`         | —           | Drawing area width.                     |
| `height`        | —           | Drawing area height.                    |

---

#### `[returnmacro]` — Return from Macro

The `[returnmacro]` tag shall immediately exit the currently executing macro and
return control to the tag following the invoking `[callmacro]`. It takes no
parameters.

---

#### `[se]` — Play Sound Effect

The `[se]` tag shall play a sound effect on the SE audio track.

| Parameter | Default | Requirement                                         |
|-----------|---------|-----------------------------------------------------|
| `file`    | —       | Required. Set to `none` to stop SE playback.        |
| `loop`    | `false` | If `true`, the sound effect shall loop continuously. |

The implementation shall accept Ogg Vorbis files at 44,100 Hz. A looped SE
shall be restored when a save slot is loaded.

---

#### `[set]` — Set Variable

The `[set]` tag shall assign a value to a named variable.

| Parameter | Default | Requirement                                               |
|-----------|---------|-----------------------------------------------------------|
| `name`    | —       | Required. Variable name.                                  |
| `value`   | —       | Value to assign directly.                                 |
| `value1`  | —       | Left operand for arithmetic assignment.                   |
| `op`      | —       | Operator: `+`, `-`, `*`, `/`, `//` (int div), `%`.       |
| `value2`  | —       | Right operand for arithmetic assignment.                  |
| `global`  | `false` | If `true`, the variable shall be marked global (saved in global save). |

All variables shall be stored internally as strings. Arithmetic operators shall
parse operands as floating-point numbers before computing.

---

#### `[skip]` — Set Skip Status

The `[skip]` tag shall enable or disable the script skip mode.

| Parameter | Requirement                                       |
|-----------|---------------------------------------------------|
| `enable`  | Required. `true` enables skip; `false` disables.  |

When skip is disabled, the engine shall ignore all player skip inputs.

---

#### `[text]` — Display Text

The `[text]` tag shall display a message in the message box and optionally show
a speaker name. It shall block execution until the player advances (click or key).

| Parameter        | Default | Requirement                                              |
|------------------|---------|----------------------------------------------------------|
| `text`           | —       | Message content.                                         |
| `text-<locale>`  | —       | Localized message; resolved via §A.1.                    |
| `voice`          | —       | Voice file to play with the message.                     |
| `voice-<locale>` | —       | Localized voice file; resolved via §A.1 voice rules.     |
| `name`           | —       | Speaker name shown in the name box.                      |
| `action`         | —       | Control action (see below).                              |
| `space`          | —       | In NVL mode, prepend this string before the message.     |

**`action` values:**

| Value   | Behavior                                    |
|---------|---------------------------------------------|
| `clear` | Clear the message box content.              |
| `new`   | Clear the message box and make it visible.  |
| `show`  | Show the message box without changing text. |
| `hide`  | Hide the message box.                       |
| `inline`| Append text without advancing to next line. |

**Voice resolution** when locale is `<lang>-<region>`:
1. `voice-<lang>-<region>` parameter
2. `voice/<lang>-<region>/` + `voice` parameter
3. `voice-<lang>` parameter
4. `voice/<lang>/` + `voice` parameter
5. `voice` parameter

---

#### `[video]` — Play Video

The `[video]` tag shall play a full-motion video file.

| Parameter    | Default | Requirement                                             |
|--------------|---------|---------------------------------------------------------|
| `file`       | —       | Required. Path to video file (`.mp4` H.264+AAC format). |
| `skippable`  | `false` | If `true`, the player may skip by clicking.             |

Video files shall not be packed into `assets.arc` and shall be distributed as
loose files. Execution shall block until the video ends or is skipped.

---

#### `[volume]` — Set Audio Volume

The `[volume]` tag shall set the volume of a named audio track.

| Parameter       | Requirement                                           |
|-----------------|-------------------------------------------------------|
| `track`         | Required. One of `bgm`, `se`, `voice`.                |
| `volume` / `vol`| Required. Float 0.0 (silent) to 1.0 (maximum).       |
| `time`          | Optional. Fade duration in seconds; 0 = immediate.    |

Volume changes with `time > 0` shall be applied asynchronously; the tag shall
return immediately.

---

#### `[wait]` — Wait for Time

The `[wait]` tag shall pause execution for a specified duration without requiring
player input.

| Parameter     | Requirement                                         |
|---------------|-----------------------------------------------------|
| `time`        | Required. Duration in seconds (decimal supported).  |
| `hidemsgbox`  | Optional. If `true`, force-hide the message box.    |
| `hidenamebox` | Optional. If `true`, force-hide the name box.       |

---

### A.3 Layer Name Reference

The following layer names shall be valid targets for the `[layer]` tag unless
marked as invisible.

| Layer Name  | Description                           | Accessible via `[layer]` |
|-------------|---------------------------------------|--------------------------|
| `bg`        | Background Image                      | Yes                      |
| `bg2`       | Background Image 2                    | Yes                      |
| `efb1`–`efb4` | Back Effect layers 1–4              | Yes                      |
| `chb`       | Center-Back Character                 | Yes                      |
| `chl`       | Left Character                        | Yes                      |
| `chlc`      | Left-Center Character                 | Yes                      |
| `chr`       | Right Character                       | Yes                      |
| `chrc`      | Right-Center Character                | Yes                      |
| `chc`       | Center Character                      | Yes                      |
| `eff1`–`eff4` | Front Effect layers 1–4             | Yes                      |
| `chf`       | Face Character                        | Yes                      |
| `text1`–`text8` | Text Layers 1–8                   | Yes                      |
| `msgbox`    | Message Box                           | No (invisible)           |
| `namebox`   | Name Box                              | No (invisible)           |
| `click`     | Click Animation                       | No (invisible)           |
| `gui1`–`gui32` | GUI Button Layers 1–32             | No (invisible)           |

Eye (`-eye`), Lip (`-lip`), and Fade-Out (`-fo`) sub-layers exist for each
character layer (e.g., `chc-eye`, `chc-lip`, `chc-fo`) and are managed by the
animation subsystem; they shall not be targets of `[layer]`.

---

## Appendix B: Ray VN API Requirements (`Suika.*`)

This appendix specifies the requirements for all `Suika.*` API functions exposed
to Ray scripts. Unless otherwise noted, each function takes a single dictionary
argument and returns the type indicated. Functions with "No parameters" accept an
empty dictionary `{}`.

All `Suika.*` API functions shall be callable from within `Tag_*()` plugin
functions and from `main.ray` lifecycle callbacks (`setup`, `start`, `update`,
`render`). The implementation shall register each function via
`Suika.installAPI()` or equivalent internal mechanism.

### B.1 Fundamental

| Function              | Parameters           | Returns  | Requirement                                                |
|-----------------------|----------------------|----------|------------------------------------------------------------|
| `Suika.loadPlugin()`  | `name` (String, direct) | —     | Shall load the plugin Ray file at `system/plugin/<name>/<name>.ray` and execute `plugin_init_<name>()`. The sole `Suika.*` function that accepts a non-dictionary argument. |
| `Suika.installAPI()`  | `name` (String), `func` (Function) | — | Shall register a Ray function as a callable API with the given name. |
| `Suika.installTag()`  | `name` (String), `func` (Function) | — | Shall register a Ray function as a NovelML tag handler invoked as `Tag_<name>()`. |

### B.2 Configuration

| Function                    | Parameters       | Returns  | Requirement                                                   |
|-----------------------------|------------------|----------|---------------------------------------------------------------|
| `Suika.setConfig()`         | `key`, `value`   | —        | Shall update the named config key at runtime.                 |
| `Suika.getConfigCount()`    | —                | Integer  | Shall return the total number of config keys.                 |
| `Suika.getConfigKey()`      | `index`          | String   | Shall return the config key at the given index.               |
| `Suika.isGlobalSaveConfig()`| `key`            | Boolean  | Shall return `true` if the key is saved to global save.       |
| `Suika.isLocalSaveConfig()` | `key`            | Boolean  | Shall return `true` if the key is saved to local save.        |
| `Suika.getConfigType()`     | `key`            | String   | Shall return `"s"`, `"b"`, `"i"`, or `"f"`.                  |
| `Suika.getStringConfig()`   | `key`            | String   | Shall return the config value as a string.                    |
| `Suika.getBoolConfig()`     | `key`            | Boolean  | Shall return the config value as a boolean.                   |
| `Suika.getIntConfig()`      | `key`            | Integer  | Shall return the config value as an integer.                  |
| `Suika.getFloatConfig()`    | `key`            | Float    | Shall return the config value as a float.                     |
| `Suika.getConfigAsString()` | `key`            | String   | Shall return any config value type as a string.               |
| `Suika.compareLocale()`     | `locale`         | Boolean  | Shall return `true` if the given locale matches the current runtime locale. |

### B.3 Input

| Function                        | Parameters | Returns | Requirement                                                   |
|---------------------------------|------------|---------|---------------------------------------------------------------|
| `Suika.getMousePosX()`          | —          | Integer | Shall return the current mouse X coordinate.                  |
| `Suika.getMousePosY()`          | —          | Integer | Shall return the current mouse Y coordinate.                  |
| `Suika.isMouseLeftPressed()`    | —          | Boolean | Shall return `true` while the left button is held down.       |
| `Suika.isMouseRightPressed()`   | —          | Boolean | Shall return `true` while the right button is held down.      |
| `Suika.isMouseLeftClicked()`    | —          | Boolean | Shall return `true` for the frame in which a left-click completed. |
| `Suika.isMouseRightClicked()`   | —          | Boolean | Shall return `true` for the frame in which a right-click completed. |
| `Suika.isMouseDragging()`       | —          | Boolean | Shall return `true` while the mouse is moving with a button held. |
| `Suika.isReturnKeyPressed()`    | —          | Boolean | Shall return `true` while Return/Enter is held.               |
| `Suika.isSpaceKeyPressed()`     | —          | Boolean | Shall return `true` while Space is held.                      |
| `Suika.isEscapeKeyPressed()`    | —          | Boolean | Shall return `true` while Escape is held.                     |
| `Suika.isUpKeyPressed()`        | —          | Boolean | Shall return `true` while Up arrow is held.                   |
| `Suika.isDownKeyPressed()`      | —          | Boolean | Shall return `true` while Down arrow is held.                 |
| `Suika.isLeftKeyPressed()`      | —          | Boolean | Shall return `true` while Left arrow is held.                 |
| `Suika.isRightKeyPressed()`     | —          | Boolean | Shall return `true` while Right arrow is held.                |
| `Suika.isPageUpKeyPressed()`    | —          | Boolean | Shall return `true` while Page Up is held.                    |
| `Suika.isPageDownKeyPressed()`  | —          | Boolean | Shall return `true` while Page Down is held.                  |
| `Suika.isControlKeyPressed()`   | —          | Boolean | Shall return `true` while Control is held.                    |
| `Suika.isSKeyPressed()`         | —          | Boolean | Shall return `true` while S key is held.                      |
| `Suika.isLKeyPressed()`         | —          | Boolean | Shall return `true` while L key is held.                      |
| `Suika.isHKeyPressed()`         | —          | Boolean | Shall return `true` while H key is held.                      |
| `Suika.isTouchCanceled()`       | —          | Boolean | Shall return `true` when a touch event was canceled.          |
| `Suika.isSwiped()`              | —          | Boolean | Shall return `true` when a swipe gesture was detected.        |
| `Suika.clearInputState()`       | —          | —       | Shall suppress further input processing for the current frame. |

### B.4 Game State

| Function                          | Parameters              | Returns  | Requirement                                                  |
|-----------------------------------|-------------------------|----------|--------------------------------------------------------------|
| `Suika.startCommandRepetition()`  | —                       | —        | Shall begin a multi-frame tag execution mode.                |
| `Suika.stopCommandRepetition()`   | —                       | —        | Shall end multi-frame tag execution mode.                    |
| `Suika.isInCommandRepetition()`   | —                       | Boolean  | Shall return `true` while in multi-frame mode.               |
| `Suika.setMessageActive()`        | —                       | —        | Shall set the message display state to active.               |
| `Suika.clearMessageActive()`      | —                       | —        | Shall reset the message display state.                       |
| `Suika.isMessageActive()`         | —                       | Boolean  | Shall return the current message display state.              |
| `Suika.startAutoMode()`           | —                       | —        | Shall activate automatic advance mode.                       |
| `Suika.stopAutoMode()`            | —                       | —        | Shall deactivate automatic advance mode.                     |
| `Suika.isAutoMode()`              | —                       | Boolean  | Shall return `true` while auto mode is active.               |
| `Suika.startSkipMode()`           | —                       | —        | Shall activate skip mode.                                    |
| `Suika.stopSkipMode()`            | —                       | —        | Shall deactivate skip mode.                                  |
| `Suika.isSkipMode()`              | —                       | Boolean  | Shall return `true` while skip mode is active.               |
| `Suika.setSaveLoad()`             | `enable` (Boolean)      | —        | Shall enable or disable save/load operations.                |
| `Suika.isSaveLoadEnabled()`       | —                       | Boolean  | Shall return the current save/load enable state.             |
| `Suika.setNonInterruptible()`     | `enable` (Boolean)      | —        | Shall enable or disable non-interruptible mode.              |
| `Suika.isNonInterruptible()`      | —                       | Boolean  | Shall return the current non-interruptible state.            |
| `Suika.setPenPosition()`          | `x`, `y` (Integer)      | —        | Shall set the text drawing pen position.                     |
| `Suika.getPenPositionX()`         | —                       | Integer  | Shall return the pen X position.                             |
| `Suika.getPenPositionY()`         | —                       | Integer  | Shall return the pen Y position.                             |
| `Suika.pushForCall()`             | `file`, `index`         | Boolean  | Shall push a return point onto the call stack.               |
| `Suika.popForReturn()`            | —                       | Dict     | Shall pop the call stack; returns `{file, index}`.           |
| `Suika.readCallStack()`           | `sp` (Integer)          | Dict     | Shall read call stack entry at `sp`; returns `{file, index}`.|
| `Suika.writeCallStack()`          | `sp`, `file`, `index`   | —        | Shall overwrite call stack entry at `sp`.                    |
| `Suika.setCallArgument()`         | `index`, `value`        | Boolean  | Shall set calling argument at the given index.               |
| `Suika.getCallArgument()`         | `index`                 | String   | Shall return calling argument at the given index.            |
| `Suika.isPageMode()`              | —                       | Boolean  | Shall return `true` when NVL page mode is enabled.           |
| `Suika.appendBufferedMessage()`   | `message`               | —        | Shall append to the NVL page buffer.                         |
| `Suika.getBufferedMessage()`      | —                       | String   | Shall return the NVL page buffer contents.                   |
| `Suika.clearBufferedMessage()`    | —                       | —        | Shall clear the NVL page buffer.                             |
| `Suika.resetPageLine()`           | —                       | —        | Shall reset the NVL line counter.                            |
| `Suika.incPageLine()`             | —                       | —        | Shall increment the NVL line counter.                        |
| `Suika.isPageTop()`               | —                       | Boolean  | Shall return `true` if at the first line of a page.          |
| `Suika.registerBGVoice()`         | `file`                  | —        | Shall register a background voice file.                      |
| `Suika.getBVoice()`               | —                       | String   | Shall return the registered background voice filename.       |
| `Suika.setBGVoicePlaying()`       | `isPlaying` (Boolean)   | —        | Shall update the background voice playing state.             |
| `Suika.isBGVoicePlaying()`        | —                       | Boolean  | Shall return the background voice playing state.             |
| `Suika.setChapterName()`          | `name`                  | —        | Shall update the current chapter name.                       |
| `Suika.getChapterName()`          | —                       | String   | Shall return the current chapter name.                       |
| `Suika.setLastMessage()`          | `message`, `isAppend`   | —        | Shall set or append the last displayed message.              |
| `Suika.getLastMessage()`          | —                       | String   | Shall return the last displayed message.                     |
| `Suika.setTextSpeed()`            | `speed` (Float)         | —        | Shall set the character display speed.                       |
| `Suika.getTextSpeed()`            | —                       | Float    | Shall return the current text speed.                         |
| `Suika.setAutoSpeed()`            | `speed` (Float)         | —        | Shall set the auto-advance speed.                            |
| `Suika.getAutoSpeed()`            | —                       | Float    | Shall return the current auto-advance speed.                 |
| `Suika.markLastEnglishTagIndex()` | —                       | —        | Shall record the current tag index as the last English tag.  |
| `Suika.getLastEnglishTagIndex()`  | —                       | Integer  | Shall return the recorded English tag index.                 |
| `Suika.clearLastEnglishTagIndex()`| —                       | —        | Shall clear the recorded English tag index.                  |
| `Suika.getLastTagName()`          | —                       | String   | Shall return the name of the last executed tag.              |

### B.5 Image

| Function                        | Parameters                          | Returns | Requirement                                                  |
|---------------------------------|-------------------------------------|---------|--------------------------------------------------------------|
| `Suika.createImageFromFile()`   | `file`                              | Object  | Shall load an image from the asset path; returns null on failure. |
| `Suika.createImage()`           | `width`, `height`                   | Object  | Shall create a blank RGBA image of the given dimensions.     |
| `Suika.getImageWidth()`         | `img`                               | Integer | Shall return the width in pixels.                            |
| `Suika.getImageHeight()`        | `img` or `image`                    | Integer | Shall return the height in pixels.                           |
| `Suika.getImagePixels()`        | `img`                               | Object  | Shall return a packed pixel buffer for direct manipulation.  |
| `Suika.updateImagePixels()`     | `img`                               | —       | Shall upload the pixel buffer to the GPU texture; must be called after pixel edits. |
| `Suika.makePixel()`             | `r`, `g`, `b`, `a` (Integer 0–255)  | Integer | Shall pack RGBA components into a single pixel value.        |
| `Suika.destroyImage()`          | `image`                             | —       | Shall free the image object and its GPU texture.             |
| `Suika.drawImage()`             | `dstImage`, `dstLeft`, `dstTop`, `srcImage`, `dstWidth`, `dstHeight`, `srcLeft`, `srcTop`, `alpha`, `blend` | — | Shall blit the source image region onto the destination. |
| `Suika.drawImage3D()`           | `dstImage`, `x1`–`x4`, `y1`–`y4`, `srcImage`, `srcLeft`, `srcTop`, `srcWidth`, `srcHeight`, `alpha`, `blend` | — | Shall blit with four-point quad transformation. |
| `Suika.makeColor()`             | `r`, `g`, `b`, `a`                  | Integer | Shall return a packed RGBA color value.                      |
| `Suika.fillImageRect()`         | `image`, `left`, `top`, `width`, `height`, `color` | — | Shall fill a rectangle with the given packed color. |

Blend type constants: `Suika.BLEND_COPY`, `Suika.BLEND_ALPHA`, `Suika.BLEND_ADD`,
`Suika.BLEND_SUB`, `Suika.BLEND_DIM`, `Suika.BLEND_GLYPH`, `Suika.BLEND_EMOJI`.

### B.6 Stage

| Function                          | Parameters                    | Returns  | Requirement                                                  |
|-----------------------------------|-------------------------------|----------|--------------------------------------------------------------|
| `Suika.reloadStageImages()`       | —                             | Boolean  | Shall reload all stage layer images from config.             |
| `Suika.reloadStagePositions()`    | —                             | —        | Shall reload stage layer positions from config.              |
| `Suika.getLayerX()`               | `layer` (Integer)             | Integer  | Shall return the layer's current X position.                 |
| `Suika.getLayerY()`               | `layer` (Integer)             | Integer  | Shall return the layer's current Y position.                 |
| `Suika.setLayerPosition()`        | `layer`, `x`, `y`            | —        | Shall set the layer's X and Y position.                      |
| `Suika.getLayerScaleX()`          | `layer`                       | Float    | Shall return the layer's X scaling factor.                   |
| `Suika.getLayerScaleY()`          | `layer`                       | Float    | Shall return the layer's Y scaling factor.                   |
| `Suika.setLayerScale()`           | `layer`, `scale_x`, `scale_y` | —        | Shall set the layer's scaling factors.                       |
| `Suika.getLayerRotate()`          | `layer`                       | Float    | Shall return the layer's rotation in radians.                |
| `Suika.setLayerRotate()`          | `layer`, `rot`                | —        | Shall set the layer's rotation in radians.                   |
| `Suika.getLayerDim()`             | `layer`                       | Boolean  | Shall return the layer's dim state.                          |
| `Suika.setLayerDim()`             | `layer`, `dim`                | —        | Shall set the layer's dim state.                             |
| `Suika.getLayerAlpha()`           | `layer`                       | Integer  | Shall return the layer's alpha (0–255).                      |
| `Suika.setLayerAlpha()`           | `layer`, `alpha`              | —        | Shall set the layer's alpha value.                           |
| `Suika.setLayerBlend()`           | `layer`, `blend`              | —        | Shall set the layer's blend mode.                            |
| `Suika.setLayerFileName()`        | `layer`, `file_name`          | Boolean  | Shall load an image file onto the specified layer.           |
| `Suika.setLayerFrame()`           | `layer`, `frame`              | —        | Shall set the animation frame index for eye/lip sub-layers.  |
| `Suika.getLayerText()`            | `index`                       | String   | Shall return the text content of a text layer.               |
| `Suika.setLayerText()`            | `index`, `text`               | —        | Shall set the text content of a text layer.                  |
| `Suika.getLayerImage()`           | `layer`                       | Object   | Shall return the image object currently on the layer.        |
| `Suika.setLayerImage()`           | `layer`, `image`              | —        | Shall assign an image object to the layer.                   |
| `Suika.clearStageBasic()`         | —                             | —        | Shall clear all basic (non-UI) stage layers.                 |
| `Suika.clearStage()`              | —                             | —        | Shall reset all stage layers to their initial state.         |
| `Suika.chposToLayer()`            | `chpos`                       | Integer  | Shall return the layer index for a character position.       |
| `Suika.chposToEyeLayer()`         | `chpos`                       | Integer  | Shall return the eye sub-layer index for a character position. |
| `Suika.chposToLipLayer()`         | `chpos`                       | Integer  | Shall return the lip sub-layer index for a character position. |
| `Suika.layerToChpos()`            | `layer`                       | Integer  | Shall return the character position for a layer index.       |
| `Suika.renderStage()`             | —                             | —        | Shall composite all stage layers to the screen.              |
| `Suika.drawStageToThumb()`        | —                             | —        | Shall render the stage to the thumbnail image buffer.        |
| `Suika.getThumbImage()`           | —                             | Object   | Shall return the thumbnail image object.                     |
| `Suika.getFadeMethod()`           | —                             | Integer  | Shall return the current fade method constant.               |
| `Suika.getAccelMethod()`          | —                             | Integer  | Shall return the current acceleration method constant.       |
| `Suika.startFade()`               | `desc`, `method`, `time`, `ruleImage` | Boolean | Shall begin a transition effect.                  |
| `Suika.isFadeRunning()`           | —                             | Boolean  | Shall return `true` while a transition is in progress.       |
| `Suika.finishFade()`              | —                             | —        | Shall immediately complete the current transition.           |
| `Suika.getShakeOffset()`          | —                             | Dict     | Shall return `{x, y}` shake offsets.                         |
| `Suika.setShakeOffset()`          | `x`, `y`                      | —        | Shall set the screen shake offset.                           |
| `Suika.setChNameMapping()`        | `chpos`, `chNameIndex`        | —        | Shall map a character position to a character name index.    |
| `Suika.setChTalking()`            | `chpos`                       | —        | Shall set the currently speaking character position.         |
| `Suika.getTalkingChpos()`         | —                             | Integer  | Shall return the position of the speaking character.         |
| `Suika.updateChDimByTalkingCh()`  | —                             | —        | Shall auto-dim non-speaking characters.                      |
| `Suika.forceChDim()`              | `chpos`, `dim`                | —        | Shall force a specific character's dim state.                |
| `Suika.getChDim()`                | `chpos`                       | Boolean  | Shall return a character's current dim state.                |
| `Suika.fillMessageBox()`          | —                             | —        | Shall fill the message box with its background image.        |
| `Suika.showMessageBox()`          | `show` (Boolean)              | —        | Shall show or hide the message box.                          |
| `Suika.getMessageBoxRect()`       | —                             | Dict     | Shall return `{x, y, w, h}` of the message box.             |
| `Suika.fillNameBox()`             | —                             | —        | Shall fill the name box with its background image.           |
| `Suika.showNameBox()`             | `show` (Boolean)              | —        | Shall show or hide the name box.                             |
| `Suika.getNameBoxRect()`          | —                             | Dict     | Shall return `{x, y, w, h}` of the name box.                |
| `Suika.showChoosebox()`           | `index`, `showIdle`, `showHover` | —    | Shall show or hide a choice box at the given index (0–7).    |
| `Suika.fillChooseBoxIdleImage()`  | `index`                       | —        | Shall fill the idle-state layer of the given choose box.     |
| `Suika.fillChooseBoxHoverImage()` | `index`                       | —        | Shall fill the hover-state layer of the given choose box.    |
| `Suika.getChooseBoxRect()`        | —                             | Dict     | Shall return `{x, y, w, h}` of a choice box.                |
| `Suika.showAutoModeBanner()`      | `show` (Boolean)              | —        | Shall show or hide the auto mode indicator.                  |
| `Suika.showSkipModeBanner()`      | `show` (Boolean)              | —        | Shall show or hide the skip mode indicator.                  |
| `Suika.renderImage()`             | `dstLeft`, `dstTop`, `image`, `srcLeft`, `srcTop`, `srcWidth`, `srcHeight`, `alpha`, `blend` | — | Shall render an image directly to the screen. |
| `Suika.renderImage3d()`           | `x1–x4`, `y1–y4`, `tex`, `srcLeft`, `srcTop`, `srcWidth`, `srcHeight`, `alpha` | — | Shall render a quad-transformed image to the screen. |
| `Suika.setClickPosition()`        | `x`, `y`                      | —        | Shall set the position of the click animation.               |
| `Suika.showClick()`               | `show` (Boolean)              | —        | Shall show or hide the click animation.                      |
| `Suika.setClickIndex()`           | `index`                       | —        | Shall set the click animation frame index.                   |
| `Suika.getClickRect()`            | —                             | Dict     | Shall return `{x, y, w, h}` of the click animation area.    |
| `Suika.getSysBtnIdleImage()`      | —                             | Object   | Shall return the system button idle image.                   |
| `Suika.getSysBtnHoverImage()`     | —                             | Object   | Shall return the system button hover image.                  |
| `Suika.startKirakira()`           | —                             | —        | Shall initialize the Kirakira sparkle effect.                |
| `Suika.renderKirakira()`          | —                             | —        | Shall render one frame of the Kirakira effect.               |

### B.7 Audio Mixer

| Function                       | Parameters                    | Returns  | Requirement                                                  |
|--------------------------------|-------------------------------|----------|--------------------------------------------------------------|
| `Suika.setMixerInputFile()`    | `track`, `file`, `isLooped`   | Boolean  | Shall play the audio file on the named track (`bgm`, `se`, `voice`, `sys`). |
| `Suika.setMixerVolume()`       | `track`, `vol`, `span`        | —        | Shall set the track volume with optional fade duration.      |
| `Suika.getMixerVolume()`       | `track`                       | Float    | Shall return the current track volume.                       |
| `Suika.setMasterVolume()`      | `volume`                      | —        | Shall set the master volume affecting all tracks.            |
| `Suika.getMasterVolume()`      | —                             | Float    | Shall return the current master volume.                      |
| `Suika.setMixerGlobalVolume()` | `track`, `vol`                | —        | Shall set the persistent global volume for a track.          |
| `Suika.getMixerGlobalVolume()` | `track`                       | Float    | Shall return the persistent global volume for a track.       |
| `Suika.setCharacterVolume()`   | `index`, `vol`                | —        | Shall set the voice volume for a character by name index.    |
| `Suika.getCharacterVolume()`   | `ch_index`                    | Float    | Shall return the voice volume for a character.               |
| `Suika.isMixerSoundFinished()` | `track` (Integer index)       | Boolean  | Shall return `true` when playback on the track has ended.    |
| `Suika.getTrackFileName()`     | `track` (Integer index)       | String   | Shall return the filename currently loaded on the track.     |
| `Suika.applyCharacterVolume()` | `ch` (Integer)                | —        | Shall apply the character's volume setting to the voice track. |

### B.8 System Button

| Function                      | Parameters              | Returns | Requirement                                                    |
|-------------------------------|-------------------------|---------|----------------------------------------------------------------|
| `Suika.enableSysBtn()`        | `isEnabled` (Boolean)   | —       | Shall enable or disable the system button.                     |
| `Suika.isSysBtnEnabled()`     | —                       | Boolean | Shall return the system button enable state.                   |
| `Suika.updateSysBtnState()`   | —                       | —       | Shall update mouse-over tracking for the system button.        |
| `Suika.isSysBtnPointed()`     | —                       | Boolean | Shall return `true` when the cursor is over the system button. |
| `Suika.isSysBtnClicked()`     | —                       | Boolean | Shall return `true` when the system button was clicked.        |

### B.9 Text Rendering

| Function                       | Parameters                                  | Returns  | Requirement                                                  |
|--------------------------------|---------------------------------------------|----------|--------------------------------------------------------------|
| `Suika.drawTextOnLayer()`      | `layer`, `fontType`, `fontSize`, `color`, `outlineWidth`, `outlineColor`, `lineMargin`, `charMargin`, `x`, `y`, `width`, `height`, `text` | — | Shall render text onto the specified stage layer. |
| `Suika.getStringWidth()`       | `fontType`, `fontSize`, `text`              | Integer  | Shall return the pixel width of the rendered string.         |
| `Suika.getStringHeight()`      | `fontType`, `fontSize`, `text`              | Integer  | Shall return the pixel height of the rendered string.        |
| `Suika.drawGlyph()`            | `img`, `font_type`, `font_size`, `base_font_size`, `outline_size`, `x`, `y`, `color`, `outline_color`, `codepoint`, `is_dim` | Boolean | Shall draw a single UTF-32 glyph onto an image. |
| `Suika.createDrawMsg()`        | (see §B.9 detail)                           | Object   | Shall create a complex text rendering context.               |
| `Suika.destroyDrawMsg()`       | `context`                                   | —        | Shall free the text rendering context.                       |
| `Suika.countDrawMsgChars()`    | `context`                                   | Integer  | Shall return the remaining character count (excluding escapes). |
| `Suika.drawMessage()`          | `context`, `maxChars`                       | Integer  | Shall render up to `maxChars` characters; returns count drawn. |
| `Suika.getDrawMsgPenPosition()`| `context`                                   | Dict     | Shall return the current pen `{x, y}` from the drawing context. |
| `Suika.isEscapeSequenceChar()` | `c`                                         | Boolean  | Shall return `true` if the character is part of an inline escape sequence. |
| `Suika.expandStringWithVariable()` | (variable name/string)                 | String   | Shall expand `${varname}` references in the input string.    |

`Suika.createDrawMsg()` accepts parameters: `image`, `text`, `fontType`, `fontSize`,
`baseFontSize`, `rubySize`, `outlineSize`, `penX`, `penY`, `areaWidth`, `areaHeight`,
`leftMargin`, `rightMargin`, `topMargin`, `bottomMargin`, `lineMargin`, `charMargin`,
`color`, `outlineColor`, `bgColor`, `fillBg`, `dim`, `ignoreLF`, `ignoreFont`,
`ignoreOutline`, `ignoreColor`, `ignoreSize`, `ignorePosition`, `ignoreRuby`,
`ignoreWait`, `inlineWaitHook`, `tategaki`.

### B.10 Tag Execution

| Function                        | Parameters       | Returns  | Requirement                                                    |
|---------------------------------|------------------|----------|----------------------------------------------------------------|
| `Suika.moveToTagFile()`         | `file`           | Boolean  | Shall load and begin executing the specified script file.      |
| `Suika.getTagCount()`           | —                | Integer  | Shall return the total number of tags in the current file.     |
| `Suika.moveToTagIndex()`        | `index`          | Boolean  | Shall move the execution pointer to the given tag index.       |
| `Suika.moveToNextTag()`         | —                | Boolean  | Shall advance the execution pointer to the next tag.           |
| `Suika.moveToLabelTag()`        | `name`           | Boolean  | Shall jump to the named label tag.                             |
| `Suika.moveToMacroTag()`        | `name`           | Boolean  | Shall jump to the named macro definition.                      |
| `Suika.moveToElseTag()`         | —                | Boolean  | Shall jump to the corresponding `[else]`, `[elseif]`, or `[endif]`. |
| `Suika.moveToEndIfTag()`        | —                | Boolean  | Shall jump to the corresponding `[endif]`.                     |
| `Suika.moveToEndMacroTag()`     | —                | Boolean  | Shall jump to the corresponding `[endmacro]`.                  |
| `Suika.getTagFileName()`        | —                | String   | Shall return the current script file name.                     |
| `Suika.getTagIndex()`           | —                | Integer  | Shall return the current tag index.                            |
| `Suika.getTagLine()`            | —                | Integer  | Shall return the source line number of the current tag.        |
| `Suika.getTagBlockDepth()`      | —                | Integer  | Shall return the nesting block depth of the current tag.       |
| `Suika.getTagName()`            | —                | String   | Shall return the tag name of the current tag.                  |
| `Suika.getTagPropertyCount()`   | —                | Integer  | Shall return the number of properties on the current tag.      |
| `Suika.getTagPropertyName()`    | `index`          | String   | Shall return the property name at the given index.             |
| `Suika.getTagPropertyValue()`   | `index`          | String   | Shall return the property value at the given index.            |
| `Suika.getTagArgBool()`         | `name`, `omissible`, `defVal` | Boolean | Shall return a boolean tag argument with optional default. |
| `Suika.getTagArgInt()`          | `name`, `omissible`, `defVal` | Integer | Shall return an integer tag argument with optional default. |
| `Suika.getTagArgFloat()`        | `name`, `omissible`, `defVal` | Float   | Shall return a float tag argument with optional default.   |
| `Suika.getTagArgString()`       | `name`, `omissible`, `defVal` | String  | Shall return a string tag argument with optional default.  |
| `Suika.evaluateTag()`           | —                | Boolean  | Shall expand `${varname}` references in all current tag property values. |
| `Suika.pushTagStackIf()`        | —                | —        | Shall push the current position onto the `if`-block stack.     |
| `Suika.popTagStackIf()`         | —                | —        | Shall pop the `if`-block stack.                                |
| `Suika.pushTagStackWhile()`     | —                | Boolean  | Shall push the current position onto the `while`-loop stack.   |
| `Suika.popTagStackWhile()`      | —                | Boolean  | Shall pop the `while`-loop stack.                              |
| `Suika.pushTagStackFor()`       | —                | Boolean  | Shall push the current position onto the `for`-loop stack.     |
| `Suika.popTagStackFor()`        | —                | Boolean  | Shall pop the `for`-loop stack.                                |

### B.11 Animation

| Function                           | Parameters                 | Returns  | Requirement                                                  |
|------------------------------------|----------------------------|----------|--------------------------------------------------------------|
| `Suika.loadAnimeFromFile()`        | `file`, `reg_name`         | Array    | Shall load an animation file and register it under `reg_name`. Returns boolean array per layer. |
| `Suika.newAnimeSequence()`         | `layer`                    | Boolean  | Shall begin describing a new programmatic animation sequence for the layer. |
| `Suika.addAnimeSequencePropertyF()`| `key`, `val` (Float)       | Boolean  | Shall add a float target property to the current sequence.   |
| `Suika.addAnimeSequencePropertyI()`| `key`, `val` (Integer)     | Boolean  | Shall add an integer target property to the current sequence.|
| `Suika.startLayerAnime()`          | `layer`                    | Boolean  | Shall start the registered sequence on the given layer.      |
| `Suika.isAnimeRunning()`           | —                          | Boolean  | Shall return `true` if any layer animation is active.        |
| `Suika.isAnimeFinishedForLayer()`  | `layer`                    | Boolean  | Shall return `true` when the given layer's animation has completed. |
| `Suika.updateAnimeFrame()`         | —                          | —        | Shall advance all active animation sequences by one frame.   |
| `Suika.loadEyeImageIfExists()`     | `chpos`, `file`            | Boolean  | Shall load an eye-patch image for the given character position. |
| `Suika.reloadEyeAnime()`           | `chpos`                    | Boolean  | Shall restart the eye-blink animation for the given character. |
| `Suika.runLipAnime()`              | `chpos`, `text`            | —        | Shall start lip-sync animation synchronized to the message text. |
| `Suika.stopLipAnime()`             | `chpos`                    | —        | Shall stop lip-sync animation for the given character.       |
| `Suika.clearLayerAnimeSequence()`  | `layer`                    | —        | Shall clear all animation sequences for the given layer.     |
| `Suika.clearAllAnimeSequence()`    | —                          | —        | Shall clear animation sequences for all layers.              |

### B.12 Variables

| Function                          | Parameters              | Returns  | Requirement                                                   |
|-----------------------------------|-------------------------|----------|---------------------------------------------------------------|
| `Suika.setVariableInt()`          | `name`, `value`         | Boolean  | Shall set the named variable to an integer value.             |
| `Suika.setVariableFloat()`        | `name`, `value`         | Boolean  | Shall set the named variable to a float value.                |
| `Suika.setVariableString()`       | `name`, `value`         | Boolean  | Shall set the named variable to a string value.               |
| `Suika.getVariableInt()`          | `name`                  | Integer  | Shall return the variable value as an integer.                |
| `Suika.getVariableFloat()`        | `name`                  | Float    | Shall return the variable value as a float.                   |
| `Suika.getVariableString()`       | `name`                  | String   | Shall return the variable value as a string.                  |
| `Suika.unsetVariable()`           | `name`                  | —        | Shall delete the named variable.                              |
| `Suika.unsetLocalVariables()`     | —                       | —        | Shall delete all local (non-global) variables.                |
| `Suika.makeVariableGlobal()`      | `name`, `is_global`     | Boolean  | Shall set or unset the global flag for the named variable.    |
| `Suika.isGlobalVariable()`        | `name`                  | Boolean  | Shall return `true` if the variable is marked global.         |
| `Suika.getVariableCount()`        | —                       | Integer  | Shall return the total number of variables currently set.     |
| `Suika.getVariableName()`         | `index`                 | String   | Shall return the variable name at the given index.            |
| `Suika.checkVariableExists()`     | `name`                  | Boolean  | Shall return `true` if the named variable exists.             |

### B.13 Save and Load

| Function                        | Parameters   | Returns  | Requirement                                                   |
|---------------------------------|--------------|----------|---------------------------------------------------------------|
| `Suika.executeSaveGlobal()`     | —            | Boolean  | Shall persist all global-classified config and variable values. |
| `Suika.executeLoadGlobal()`     | —            | Boolean  | Shall restore global-classified values from the global save file. |
| `Suika.executeSaveLocal()`      | `index`      | Boolean  | Shall save the full game state (script position, local variables, stage, audio) to the specified slot. |
| `Suika.executeLoadLocal()`      | `index`      | Boolean  | Shall restore game state from the specified slot.             |
| `Suika.checkSaveExists()`       | `index`      | Boolean  | Shall return `true` if a save slot contains data.             |
| `Suika.deleteLocalSave()`       | `index`      | —        | Shall delete the specified save slot.                         |
| `Suika.deleteGlobalSave()`      | —            | —        | Shall delete the global save file.                            |
| `Suika.checkRightAfterLoad()`   | —            | Boolean  | Shall return `true` for the single frame immediately after a load operation. |
| `Suika.getSaveTimestamp()`      | `index`      | Integer  | Shall return the Unix timestamp of when the slot was saved.   |
| `Suika.getLatestSaveIndex()`    | —            | Integer  | Shall return the index of the most recently modified slot.    |
| `Suika.getSaveChapterName()`    | `index`      | String   | Shall return the chapter name stored in the slot.             |
| `Suika.getSaveLastMessage()`    | `index`      | String   | Shall return the last message stored in the slot.             |
| `Suika.getSaveThumbnail()`      | `index`      | Object   | Shall return the thumbnail image object for the slot.         |
| `Suika.writeSaveData()`         | `key`, `data`| Boolean  | Shall write raw string data to a key-based save entry.        |
| `Suika.readSaveData()`          | `key`        | String   | Shall read raw string data from a key-based save entry.       |

### B.14 History (Backlog)

| Function                    | Parameters                                              | Returns  | Requirement                                           |
|-----------------------------|---------------------------------------------------------|----------|-------------------------------------------------------|
| `Suika.clearHistory()`      | —                                                       | —        | Shall remove all backlog entries.                     |
| `Suika.addHistory()`        | `name`, `msg`, `voice`, `bodyColor`, `bodyOutlineColor`, `nameColor`, `nameOutlineColor` | Boolean | Shall append an entry to the backlog. |
| `Suika.getHistoryCount()`   | —                                                       | Integer  | Shall return the total backlog entry count.           |
| `Suika.getHistoryName()`    | `index`                                                 | String   | Shall return the speaker name at the given index.     |
| `Suika.getHistoryMessage()` | `index`                                                 | String   | Shall return the message text at the given index.     |
| `Suika.getHistoryVoice()`   | `index`                                                 | String   | Shall return the voice file path at the given index.  |

### B.15 Seen Flags

| Function                  | Parameters       | Returns  | Requirement                                                    |
|---------------------------|------------------|----------|----------------------------------------------------------------|
| `Suika.loadSeen()`        | —                | Boolean  | Shall load the seen-flags file for the current script.         |
| `Suika.saveSeen()`        | —                | Boolean  | Shall persist the seen-flags file for the current script.      |
| `Suika.getSeenFlags()`    | —                | Integer  | Shall return the seen-flag bitmask for the current tag.        |
| `Suika.setSeenFlags()`    | `flag`           | —        | Shall write the seen-flag bitmask for the current tag.         |

For a `[text]` tag, bit 0 shall be 1 if the message has been read before. For a
`[choose]` tag, each bit shall indicate that the corresponding option has been
selected in a prior playthrough.

### B.16 GUI

| Function                        | Parameters              | Returns  | Requirement                                                   |
|---------------------------------|-------------------------|----------|---------------------------------------------------------------|
| `Suika.loadGUIFile()`           | `file`, `isSys`         | Boolean  | Shall load a GUI definition file. `isSys=true` marks it as a system GUI that returns to the interrupted tag. |
| `Suika.startGUI()`              | —                       | —        | Shall begin execution of the loaded GUI.                      |
| `Suika.stopGUI()`               | —                       | —        | Shall terminate the active GUI.                               |
| `Suika.isGUIRunning()`          | —                       | Boolean  | Shall return `true` while a GUI is active.                    |
| `Suika.isGUIFinished()`         | —                       | Boolean  | Shall return `true` when the GUI has completed.               |
| `Suika.getGUIResultLabel()`     | —                       | String   | Shall return the label string of the button that closed the GUI. |
| `Suika.isGUIResultTitle()`      | —                       | Boolean  | Shall return `true` if the GUI was dismissed via a title-return action. |
| `Suika.checkIfSavedInGUI()`     | —                       | Boolean  | Shall return `true` if a save was performed during GUI.       |
| `Suika.checkIfLoadedInGUI()`    | —                       | Boolean  | Shall return `true` if a load was performed during GUI.       |
| `Suika.checkRightAfterSysGUI()` | —                       | Boolean  | Shall return `true` for the frame immediately after a system GUI exits. |

### B.17 HAL and System

| Function                       | Parameters           | Returns  | Requirement                                                   |
|--------------------------------|----------------------|----------|---------------------------------------------------------------|
| `Suika.getMillisec()`          | —                    | Integer  | Shall return elapsed milliseconds since the time origin.      |
| `Suika.checkFileExists()`      | `file`               | Boolean  | Shall return `true` if the file exists in the asset path.     |
| `Suika.readFileContent()`      | `file`               | String   | Shall return the full UTF-8 text content of the file.         |
| `Suika.playVideo()`            | `file`, `is_skippable` | Boolean | Shall start video playback.                                  |
| `Suika.stopVideo()`            | —                    | —        | Shall stop video playback.                                    |
| `Suika.isVideoPlaying()`       | —                    | Boolean  | Shall return `true` while a video is playing.                 |
| `Suika.isFullScreenSupported()`| —                    | Boolean  | Shall return `true` on platforms that support full screen.    |
| `Suika.enterFullScreenMode()`  | —                    | —        | Shall transition the window to full-screen mode.              |
| `Suika.logInfo()`              | `msg`                | —        | Shall write an informational message to the debug log.        |
| `Suika.logWarn()`              | `msg`                | —        | Shall write a warning message to the debug log.               |
| `Suika.logError()`             | `msg`                | —        | Shall write an error message to the debug log.                |
| `Suika.speakText()`            | `msg`                | —        | Shall invoke the platform TTS engine with the given text.     |
| `Suika.getVmInt()`             | (implementation-defined) | Integer | Shall read an integer value from the NoctLang VM state.    |
| `Suika.setVmInt()`             | (implementation-defined) | —       | Shall write an integer value to the NoctLang VM state.      |
| `Suika.callVmFunction()`       | (implementation-defined) | —       | Shall invoke a function by name within the NoctLang VM.     |

### B.18 Constants

The implementation shall expose the following constant groups as read-only
properties of the `Suika` namespace:

- **Paths**: `Suika.PATH_START_TAG`, `Suika.PATH_CONFIG`, `Suika.PATH_SYSMENU_GUI`,
  `Suika.PATH_SAVE_GUI`, `Suika.PATH_LOAD_GUI`, `Suika.PATH_HISTORY_GUI`,
  `Suika.PATH_CONFIG_GUI`
- **Layer indices**: `Suika.LAYER_BG`, `Suika.LAYER_BG_FO`, `Suika.LAYER_BG2`,
  `Suika.LAYER_EFB1`–`EFB4`, `Suika.LAYER_CHB`, `Suika.LAYER_CHL`,
  `Suika.LAYER_CHLC`, `Suika.LAYER_CHR`, `Suika.LAYER_CHRC`, `Suika.LAYER_CHC`,
  `Suika.LAYER_EFF1`–`EFF4`, `Suika.LAYER_CHF`, and their `_EYE`, `_LIP`, `_FO`
  variants; `Suika.LAYER_MSGBOX`, `Suika.LAYER_NAMEBOX`,
  `Suika.LAYER_CHOOSE1_IDLE`–`CHOOSE8_HOVER`, `Suika.LAYER_CLICK`,
  `Suika.LAYER_AUTO`, `Suika.LAYER_SKIP`, `Suika.LAYER_TEXT1`–`TEXT8`,
  `Suika.LAYER_GUI_BTN1`–`GUI_BTN32`
- **Layer counts**: `Suika.STAGE_LAYERS`, `Suika.TEXT_LAYERS`,
  `Suika.EFFECT_LAYERS`, `Suika.BUTTON_LAYERS`, `Suika.CHOOSEBOX_COUNT`,
  `Suika.CLICK_FRAMES`, `Suika.CH_BASIC_LAYERS`, `Suika.CH_ALL_LAYERS`,
  `Suika.KIRAKIRA_FRAMES`
- **Character positions**: `Suika.CH_BACK`, `Suika.CH_LEFT`,
  `Suika.CH_LEFT_CENTER`, `Suika.CH_RIGHT`, `Suika.CH_RIGHT_CENTER`,
  `Suika.CH_CENTER`, `Suika.CH_FACE`
- **Fade methods**: `Suika.FADE_INVALID`, `Suika.FADE_NORMAL`,
  `Suika.FADE_RULE`, `Suika.FADE_MELT`
- **Blend types**: `Suika.BLEND_COPY`, `Suika.BLEND_ALPHA`, `Suika.BLEND_ADD`,
  `Suika.BLEND_SUB`, `Suika.BLEND_DIM`, `Suika.BLEND_GLYPH`, `Suika.BLEND_EMOJI`
- **Fade descriptors**: `Suika.FADE_DESC_BG`, `Suika.FADE_DESC_CHB`,
  `Suika.FADE_DESC_CHL`, `Suika.FADE_DESC_CHLC`, `Suika.FADE_DESC_CHR`,
  `Suika.FADE_DESC_CHRC`, `Suika.FADE_DESC_CHC`, `Suika.FADE_DESC_CHF`,
  `Suika.FADE_DESC_COUNT`
- **Mixer tracks**: `Suika.MIXER_TRACKS`, `Suika.TRACK_BGM`,
  `Suika.TRACK_VOICE`, `Suika.TRACK_SE`, `Suika.TRACK_SYS`
- **Animation**: `Suika.REG_ANIME_COUNT`, `Suika.ANIME_ACCEL_INVALID`,
  `Suika.ANIME_ACCEL_UNIFORM`, `Suika.ANIME_ACCEL_ACCEL`,
  `Suika.ANIME_ACCEL_DEACCEL`, `Suika.ANIME_ACCEL_SMOOTHSTEP`,
  `Suika.ANIME_ACCEL_INVSMOOTHSTEP`
- **Font**: `Suika.FONT_SELECT1`–`FONT_SELECT4`, `Suika.FONT_COUNT`,
  `Suika.EMOJI_COUNT`
- **Call stack**: `Suika.CALL_STACK_MAX`, `Suika.CALL_ARGS`
- **Character voice**: `Suika.CH_VOL_SLOTS`, `Suika.CH_VOL_SLOT_DEFAULT`
- **Character map**: `Suika.CHARACTER_MAP_COUNT`
- **Save**: `Suika.ALL_SAVE_SLOTS`, `Suika.NORMAL_SAVE_SLOTS`,
  `Suika.QUICK_SAVE_INDEX`

---

## Appendix C: Ray 2D API Requirements (`Engine.*`)

This appendix specifies the requirements for all `Engine.*` API functions and
properties exposed to Ray scripts by the Playfield Engine layer. These APIs are
available in both the VN Engine context (via `main.ray`) and in standalone
Playfield-layer applications.

The `main.ray` script shall implement four lifecycle entry points that the
Playfield Engine shall call in the following order and frequency:

| Function   | Called when                                     |
|------------|-------------------------------------------------|
| `setup()`  | Once, before the window is created. Shall return a dictionary with `width`, `height`, and `title`. |
| `start()`  | Once, after the window is created and rendering is ready. |
| `update()` | Every frame, before rendering.                  |
| `render()` | Every frame, after `update()`.                  |

### C.1 Debug

| Function  | Parameters     | Requirement                                          |
|-----------|----------------|------------------------------------------------------|
| `print()` | value (direct) | Shall print a string or dump an object to the debug log. Takes a single non-dictionary argument. |

### C.2 Time

| Property / Function  | Returns  | Requirement                                              |
|----------------------|----------|----------------------------------------------------------|
| `Engine.millisec`    | Integer  | Shall return elapsed milliseconds since the application started. Read-only property. |
| `Engine.getDate()`   | Dict     | Shall return `{year, month, day, hour, minute, second}` reflecting the current local wall-clock time. |

### C.3 Input Properties

All input properties are read-only Boolean or Integer values refreshed each frame.

**Mouse:**

| Property                     | Type    | Requirement                                           |
|------------------------------|---------|-------------------------------------------------------|
| `Engine.mousePosX`           | Integer | Shall return the mouse cursor X position.             |
| `Engine.mousePosY`           | Integer | Shall return the mouse cursor Y position.             |
| `Engine.isMouseLeftPressed`  | Boolean | Shall be `true` while the left mouse button is held.  |
| `Engine.isMouseRightPressed` | Boolean | Shall be `true` while the right mouse button is held. |

**Keyboard** — the following Boolean properties shall each be `true` while the
corresponding key is held: `Engine.isEscapeKeyPressed`, `Engine.isReturnKeyPressed`,
`Engine.isSpaceKeyPressed`, `Engine.isTabKeyPressed`, `Engine.isBackspaceKeyPressed`,
`Engine.isDeleteKeyPressed`, `Engine.isHomeKeyPressed`, `Engine.isEndKeyPressed`,
`Engine.isPageupKeyPressed`, `Engine.isPagedownKeyPressed`,
`Engine.isShiftKeyPressed`, `Engine.isControlKeyPressed`, `Engine.isAltKeyPressed`,
`Engine.isUpKeyPressed`, `Engine.isDownKeyPressed`, `Engine.isRightKeyPressed`,
`Engine.isLeftKeyPressed`, `Engine.isAKeyPressed`–`Engine.isZKeyPressed` (A–Z),
`Engine.is0KeyPressed`–`Engine.is9KeyPressed` (0–9),
`Engine.isF1KeyPressed`–`Engine.isF12KeyPressed` (F1–F12).

**Gamepad:**

| Property                    | Type    | Requirement                                               |
|-----------------------------|---------|-----------------------------------------------------------|
| `Engine.isGamepadUpPressed`    | Boolean | Shall be `true` while the gamepad D-pad Up is pressed.  |
| `Engine.isGamepadDownPressed`  | Boolean | Shall be `true` while the gamepad D-pad Down is pressed. |
| `Engine.isGamepadLeftPressed`  | Boolean | Shall be `true` while the gamepad D-pad Left is pressed. |
| `Engine.isGamepadRightPressed` | Boolean | Shall be `true` while the gamepad D-pad Right is pressed. |
| `Engine.isGamepadAPressed`     | Boolean | Shall be `true` while gamepad button A is pressed.      |
| `Engine.isGamepadBPressed`     | Boolean | Shall be `true` while gamepad button B is pressed.      |
| `Engine.isGamepadXPressed`     | Boolean | Shall be `true` while gamepad button X is pressed.      |
| `Engine.isGamepadYPressed`     | Boolean | Shall be `true` while gamepad button Y is pressed.      |
| `Engine.isGamepadLPressed`     | Boolean | Shall be `true` while the gamepad L shoulder is pressed.|
| `Engine.isGamepadRPressed`     | Boolean | Shall be `true` while the gamepad R shoulder is pressed.|
| `Engine.gamepadAnalogX1`       | Integer | Shall return left analog stick X in range −32768 to 32767. |
| `Engine.gamepadAnalogY1`       | Integer | Shall return left analog stick Y in range −32768 to 32767. |
| `Engine.gamepadAnalogX2`       | Integer | Shall return right analog stick X in range −32768 to 32767. |
| `Engine.gamepadAnalogY2`       | Integer | Shall return right analog stick Y in range −32768 to 32767. |
| `Engine.gamepadAnalogL`        | Integer | Shall return the L trigger in range −32768 to 32767.    |
| `Engine.gamepadAnalogR`        | Integer | Shall return the R trigger in range −32768 to 32767.    |

### C.4 Rendering

| Function                      | Parameters                                                     | Returns | Requirement                                                  |
|-------------------------------|----------------------------------------------------------------|---------|--------------------------------------------------------------|
| `Engine.createColorTexture()` | `width`, `height`, `r`, `g`, `b`, `a`                         | Object  | Shall create and return a solid-color RGBA texture.          |
| `Engine.loadTexture()`        | `file`                                                         | Object  | Shall load a texture from the asset path. The returned object shall expose `.width` and `.height` integer properties. |
| `Engine.destroyTexture()`     | `texture`                                                      | —       | Shall free the texture and its GPU resources.                |
| `Engine.renderTexture()`      | `dstLeft`, `dstTop`, `dstWidth`, `dstHeight`, `texture`, `srcLeft`, `srcTop`, `srcWidth`, `srcHeight`, `alpha` | — | Shall render a source rectangle of a texture to a destination rectangle on screen. |
| `Engine.draw()`               | `texture`, `x`, `y`                                            | —       | Shall render the entire texture at the given screen position. Convenience wrapper around `Engine.renderTexture()`. |
| `Engine.renderTexture3D()`    | `x1`, `y1`, `x2`, `y2`, `x3`, `y3`, `x4`, `y4`, `texture`, `srcLeft`, `srcTop`, `srcWidth`, `srcHeight`, `alpha` | — | Shall render a texture mapped to a four-point quad on screen. |
| `Engine.loadFont()`           | `slot`, `file`                                                 | —       | Shall load a TrueType font file into the specified font slot (0–3). |
| `Engine.createTextTexture()`  | `slot`, `text`, `size`, `r`, `g`, `b`, `a`                    | Object  | Shall create and return a texture with the rendered text string. |

### C.5 Sound

| Function                   | Parameters               | Returns | Requirement                                                   |
|----------------------------|--------------------------|---------|---------------------------------------------------------------|
| `Engine.playSound()`       | `stream`, `file`         | —       | Shall start one-shot playback of an audio file on the given track (0–3). |
| `Engine.playSoundLoop()`   | `stream`, `file`         | —       | Shall start looping playback of an audio file on the given track. |
| `Engine.stopSound()`       | `stream`                 | —       | Shall stop playback on the given track.                       |
| `Engine.setSoundVolume()`  | `stream`, `volume`       | —       | Shall set the volume on the given track (0.0–1.0). Pass `stream=-1` to set master volume. |

### C.6 Save Data

| Function                   | Parameters      | Returns | Requirement                                                   |
|----------------------------|-----------------|---------|---------------------------------------------------------------|
| `Engine.writeSaveData()`   | `key`, `value`  | —       | Shall persist the value (integer, float, array, or dictionary) associated with `key`. Shall fail if the data exceeds the platform-defined size limit. |
| `Engine.readSaveData()`    | `key`           | Object  | Shall return the value associated with `key`. Shall fail if the key does not exist. |
| `Engine.checkSaveData()`   | `key`           | Boolean | Shall return `true` if a saved entry for `key` exists.        |
