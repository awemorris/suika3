Distributing Games
==================

Once your game is ready, deploy it to your target platform.

The table below summarizes how assets are packaged on each platform:

| Platform         | Asset packaging                                      |
|------------------|------------------------------------------------------|
| Windows          | `assets.arc`                                         |
| macOS            | `assets.arc`                                         |
| Linux            | `assets.arc`                                         |
| WebAssembly      | `assets.arc`                                         |
| iOS              | `assets.arc` (embedded in app)                       |
| Android          | Loose files copied into `app/src/main/assets/`       |
| OpenHarmony      | Loose files copied into `resources/rawfile/`         |
| Unity (consoles) | Loose files copied into `Assets/StreamingAssets/`    |

---

## Creating assets.arc

Most platforms require your game files to be bundled into a single `assets.arc` archive.
The `suika3-pack` tool is included in the SDK for each platform.

> **Note:** Video files cannot be packed into `assets.arc` and must be distributed as separate files alongside it.

**Windows**

1. Go to `SDK/windows/`.
2. Copy `suika3-pack.exe` into your game folder.
3. Drag and drop your game files (e.g., `main.ray`, `images/`, `system/`) onto `suika3-pack.exe`.
4. `assets.arc` will be created in the same folder.

**macOS**

1. Open a terminal and go to `SDK/macos/`.
2. Run `xattr -c suika3-pack` to clear the macOS quarantine flag.
3. Run `./suika3-pack <files>` to create `assets.arc`.

**Linux**

1. Open a terminal and go to `SDK/linux/`.
2. Run `./suika3-pack <files>` to create `assets.arc`.

---

## Windows

Place the following two files in the same folder and distribute them together:

| File          | Description          |
|---------------|----------------------|
| `suika3.exe`  | The game executable. |
| `assets.arc`  | The packaged assets. |

Players launch the game by double-clicking `suika3.exe`.

---

## macOS

### Simple distribution

`SDK/macos/` includes `Suika3.dmg`, a disk image containing the `Suika3` app bundle.

1. Distribute `Suika3.dmg` and `assets.arc` together.
2. Players open the DMG, copy `Suika3` out of it, place it in the same folder as `assets.arc`, and double-click to launch.

### Full distribution (custom icon, embedded assets)

To embed assets inside the app bundle or change the app icon, use the Xcode project at `SDK/macos/project/`.

1. Replace `SDK/macos/project/Resources/assets.arc` with your packaged `assets.arc`.
2. Open `SDK/macos/project/` in Xcode.
3. Customize the app icon and other settings as needed.
4. Build the project to produce a self-contained `Suika3.app`.

---

## Linux

Suika3 depends on GStreamer, so **Flatpak is the recommended distribution method on Linux**
regardless of whether you use the pre-built binary or build from source.
Bundling via Flatpak ensures players do not need to install the correct GStreamer version themselves.

For **SteamOS**, follow Valve's recommended packaging method for Steam Deck instead.

### Source build

Build Suika3 from source (see [build.md](build.md)) to obtain the `suika3` binary,
then package it as a Flatpak for distribution.

### Flatpak

A pre-built Flatpak release is available. Install and configure it as follows:

```
flatpak install --user -y Suika3-x86_64.flatpak
flatpak override --user --filesystem=host vn.suika3.engine
flatpak run vn.suika3.engine .
```

The `flatpak override` step grants the app access to host filesystem paths.
Without it, the engine cannot read game files outside the sandbox.
The final argument (`.` in the example) is the path to the folder containing your game files
(`assets.arc` or loose asset files).

When distributing a Flatpak-based game, wrap these commands in a launcher script so players
do not need to run them manually.

---

## WebAssembly (Wasm)

Deploy `SDK/wasm/index.html` and `assets.arc` to the same folder on a web server.
Players launch the game in their browser by opening `index.html`.

### Local testing

**Windows**

Place `SDK/wasm/suika3-web.exe` in the same folder as `index.html` and `assets.arc`, then run it.
It starts a local web server and opens the game in your default browser automatically.

**macOS and Linux**

Run the following command in the folder containing `index.html` and `assets.arc`:

```
python3 -m http.server
```

Then open `http://localhost:8000` in your browser.

---

## iOS

Create `assets.arc` first using `suika3-pack` (see [Creating assets.arc](#creating-assetsarc) above).

Two workflows are available.

### Method A: Xcode

1. Replace `SDK/ios/Resources/assets.arc` with your packaged `assets.arc`.
2. Open `SDK/ios/` in Xcode.
3. Build and run the project on a device or in the iOS Simulator.

### Method B: VS Code Task (macOS only)

1. Replace `SDK/ios/Resources/assets.arc` with your packaged `assets.arc`.
2. Open the Command Palette and select **Tasks: Run Task → Build iOS IPA**.
3. The IPA is built and transferred to a USB-connected iPhone automatically.

> **Note:** Before using Method B, register your iPhone as a development device in Xcode at least once.

---

## Android

Game files are copied directly into the app project as loose files — `assets.arc` is not used.

Two workflows are available.

### Method A: Android Studio

1. Copy your game files (e.g., `main.ray`, `images/`, `system/`) into `SDK/android/app/src/main/assets/`.
2. Open `SDK/android/` in Android Studio.
3. Build and run the project.

### Method B: VS Code Task

1. Open the Command Palette and select **Tasks: Run Task → Build Android APK**.
2. OpenJDK and the Android SDK are downloaded automatically if not already present.
3. The APK is built and transferred to a USB-connected Android device.

---

## OpenHarmony (HarmonyOS NEXT)

Game files are copied directly into the app project as loose files — `assets.arc` is not used.

1. Copy your game files (e.g., `main.ray`, `images/`, `system/`) into `SDK/openharmony/entry/src/main/resources/rawfile/`.
2. Open `SDK/openharmony/` in DevEco Studio.
3. Build and run the project.

---

## Unity (Console Platforms)

The Unity project at `SDK/unity/` enables export to console platforms supported by Unity.
Game files are copied directly into the project as loose files — `assets.arc` is not used.

1. Copy your game files (e.g., `main.ray`, `images/`, `system/`) into `SDK/unity/Assets/StreamingAssets/`.
2. Open `SDK/unity/` in the Unity Editor.
3. In **Player Settings**, enable **Allow unsafe code**.
4. In **Player Settings**, set the audio sampling rate to **44100**.
5. Double-click and load the **MainScene** scene.
6. Assign the `StratoScript` game object to the `StratoScript.cs` script.
7. Click **Play** to preview the game in the editor.
8. Use Unity's build tools to export to your target console platform.
