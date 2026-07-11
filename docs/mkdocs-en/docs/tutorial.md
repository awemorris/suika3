Suika3 Tutorial
===============

## Introduction

Suika3 is a modern, portable visual novel engine designed for professional-grade VN development.
If you have built games with other VN engines before, you will find many familiar concepts here —
but Suika3 adds a layered architecture, a built-in scripting language (Ray), and first-class support
for mobile and web platforms.

This tutorial walks you through building a short visual novel from scratch, adding features
step by step across each chapter. By the end, you will have a working game that runs on PC,
mobile, and the web.

**What we are building**

Throughout this tutorial, we create a short VN called *A Sunny Afternoon* — a two-character
slice-of-life story with branching dialogue, animations, and a custom UI. Every chapter extends
the same project, so each step builds on the last.

**Assumed knowledge**

- You have made a visual novel with some engine before (Ren'Py, TyranoBuilder, KiriKiri, etc.)
- You are comfortable editing plain text files
- No programming experience is required for Chapters 1–8; Chapter 9 introduces scripting

**Environment**

Suika3 runs on Windows, macOS, Linux, iOS, Android, WebAssembly, and more.
This tutorial uses Windows in its examples, but all steps work the same on macOS and Linux
unless noted otherwise.

---

## Chapter 1: Installation and First Launch

### Windows

- **Download & Extract**
    - Download [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip).
    - Extract the archive to a folder of your choice, such as `C:\dev\suika3\`.
- **Launch**
    - Open the folder and run `suika3-win64.exe` to start to the sample game!

### macOS

- **Download & Extract**
    - Download [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip).
    - Extract the archive to a folder of your choice, such as desktop.
- **Mount the Disk Image**
    - Navigate to `SDK/macos/` and open `Suika3.dmg`.
- **Setup the App Bundle**
    - Copy the `Suika3` app from the DMG into the same folder where `suika3-win64.exe` (and the data folder) is located.
    - Note: The app bundle must reside alongside your game data to function correctly.
- **Launch**
    - Double-click the `Suika3` app to start the sample game!

### Linux

- **Download & Extract**
    - Download [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip).
    - Extract the archive to a folder of your choice, such as desktop.
- **Install the Flatpak Package**
    - Navigate to `SDK/linux/` and open `Suika3.flatpak` (or run `flatpak install --user Suika3.flatpak`).
    - This associates `.novel` and `.ray` files with the Suika3 engine.
- **Launch**
    - Open the extracted folder, then double-click `start.novel` to launch the sample game!

### Running the Sample Game

Inside the extracted folder, run `suika3-win64.exe` (Windows) or `Suika3.app` (macOS).
The sample game launches immediately. Play through it to get a feel for what Suika3 can do.

### Folder Structure

After extraction, the folder looks like this:

```
suika3-win64.exe    ← the engine executable
start.novel         ← the entry point script (NovelML)
main.ray            ← the entry point script (Ray)
assets/             ← your game's images, audio, and other files
system/             ← engine system files (UI images, fonts, etc.)
config.ini          ← game configuration
SDK/                ← platform SDKs for distribution
```

The two most important files for your game are:

- **`start.novel`** — where your story begins. Written in NovelML, Suika3's tag-based scripting language.
- **`main.ray`** — the Ray script that runs on startup. You will not need to edit this until Chapter 9.

For now, open `start.novel` in a text editor and take a look at how it is structured.

For full installation instructions, see [getting-started.md](getting-started.md).

---

## Chapter 2: Writing Your First Scenario

### What is NovelML?

NovelML is a list of **tags** — commands that tell the engine what to do.
Tags run one by one from the top of the file to the bottom.
Each tag is written inside square brackets: `[tagname parameter="value"]`.

If you have used KiriKiri or similar engines, this will feel familiar.

### Creating start.novel

Replace the contents of `start.novel` with the following:

```
[text name="Midori" text="Good afternoon! What a lovely day."]
[click]
[text name="Midori" text="I wonder if Xiaoling is home..."]
[click]
```

Run `suika3.exe`. You should see Midori's dialogue appear in the message box.
Click to advance through each line.

### The [text] Tag

The `[text]` tag displays a line of dialogue.

```
[text name="Midori" text="Hello!"]
```

| Parameter | Description                                      |
|-----------|--------------------------------------------------|
| `name`    | The speaker's name, shown in the name box.       |
| `text`    | The dialogue text displayed in the message box.  |

Omit `name` for narration with no speaker:

```
[text text="The sun hung low over the quiet street."]
```

### The [click] Tag

`[click]` pauses execution and waits for the player to click (or tap).
Always place a `[click]` after `[text]` unless you want the next tag to run immediately.

### Labels and Jumps

Use `[label]` to mark a position in the script, and `[goto]` to jump to it.

```
[label name="start"]
[text name="Midori" text="Let's take a walk!"]
[click]
[goto label="start"]
```

This loops the line indefinitely — not very useful here, but essential for menus and loops.

### Splitting into Multiple Files

As your game grows, you will want to split it into multiple files.
Use `[load]` to jump to another NovelML file:

```
[load file="scene2.novel"]
```

You can also jump to a specific label inside that file:

```
[load file="scene2.novel" label="afternoon"]
```

Create a `scene2.novel` file and add a few more lines of dialogue.
Then update `start.novel` to load it at the end:

```
[text name="Midori" text="Good afternoon! What a lovely day."]
[click]
[text name="Midori" text="I wonder if Xiaoling is home..."]
[click]
[load file="scene2.novel"]
```

For the full NovelML syntax reference, see [novelml-syntax.md](novelml-syntax.md).
For a complete list of tags, see [novelml-tags.md](novelml-tags.md).

---

## Chapter 3: Backgrounds, Characters, and BGM

### Showing a Background

The `[bg]` tag changes the background image with a crossfade effect.

```
[bg file="assets/bg/park.png" time="0.5"]
```

| Parameter | Description                                      |
|-----------|--------------------------------------------------|
| `file`    | Path to the background image.                    |
| `time`    | Crossfade duration in seconds (default: `0.5`).  |

Place your background images in `assets/bg/`. For our sample game, add a park scene image.

```
[bg file="assets/bg/park.png" time="0.5"]
[text name="Midori" text="The park is so peaceful today."]
[click]
```

### Showing Characters

The `[ch]` tag places a character sprite on screen.
Suika3 uses a **layer system**: each character occupies a named layer (`left`, `center`, `right`, etc.),
and you can show, hide, or replace characters independently.

```
[ch left="assets/ch/midori/normal.png" time="0.3"]
```

| Parameter | Description                                           |
|-----------|-------------------------------------------------------|
| `layer`   | Which layer to place the character on.                |
| `file`    | Path to the character image.                          |
| `time`    | Fade-in duration in seconds.                          |

To hide a character, omit `file` (or set it to empty):

```
[ch left="none" time="0.3"]
```

Add Midori and Xiaoling to the park scene:

```
[bg file="assets/bg/park.png" time="0.5"]
[ch left="assets/ch/midori/normal.png" time="0.3"]
[ch right="assets/ch/xiaoling/normal.png" time="0.3"]
[text name="Midori" text="Oh, Xiaoling! I was just thinking of you."]
[click]
[text name="Xiaoling" text="What a coincidence. I was heading to the café."]
[click]
```

### Playing BGM and Sound Effects

`[bgm]` starts a background music track (Ogg Vorbis format). It loops by default.

```
[bgm file="assets/bgm/afternoon.ogg" volume="0.8"]
```

`[se]` plays a one-shot sound effect:

```
[se file="assets/se/chime.ogg"]
```

To stop the BGM:

```
[bgm]
```

### Adjusting Message Display in config.ini

The appearance of the message box and name box is controlled by `config.ini`.
Open it and look for the following sections:

- **`font.ttf1`** — the default font. Replace with your preferred TrueType font.
- **`msgbox.*`** — position, margins, font size, and color for the dialogue text.
- **`namebox.*`** — position and font for the speaker name.

For example, to increase the message font size:

```
msgbox.font.size=40
```

See [config.md](config.md) for the full reference.

---

## Chapter 4: Choices and Branching

### Showing Choices

The `[choose]` tag presents the player with options and jumps to a label based on their selection.

```
[choose
    text1="Go to the café"     label1="cafe"
    text2="Stay in the park"   label2="park"
]
```

When the player picks an option, execution jumps to the corresponding `[label]`.

```
[label name="cafe"]
[text name="Midori" text="Great idea! Let's go."]
[click]
[goto label="end"]

[label name="park"]
[text name="Midori" text="Actually, let's stay a little longer."]
[click]

[label name="end"]
```

You can define up to 8 choices. Add `text3`, `label3`, and so on for more options.

### Branching with Variables

To remember a choice and use it later, store the result in a variable using `[if]`.

First, set a variable with the `[choose]` tag's `var` parameter:

```
[choose
    text1="Agree"     label1="agreed"
    text2="Decline"   label2="declined"
    var="response"
    val1="agree"
    val2="decline"
]
```

Then check it later:

```
[if cond="response == 'agree'"]
[text name="Xiaoling" text="I knew you'd say yes!"]
[click]
[elseif cond="response == 'decline'"]
[text name="Xiaoling" text="Oh, that's a shame."]
[click]
[endif]
```

Variables persist across file loads, so you can store choices in one scene and act on them in another.

### Adjusting Choice Box Appearance in config.ini

The layout and style of choice boxes are configured in `config.ini` under `choose.box*`.
Each of the 8 boxes has its own position and images:

```
choose.box1.x=200
choose.box1.y=300
choose.box1.idle=system/choose/idle.png
choose.box1.hover=system/choose/hover.png
```

Font colors for idle and hovered states are set separately:

```
choose.font.idle.r=255
choose.font.idle.g=255
choose.font.idle.b=255

choose.font.hover.r=255
choose.font.hover.g=200
choose.font.hover.b=100
```

See [config.md](config.md) for the full list of `choose.*` settings.

---

## Chapter 5: NVL Mode (Full-Screen Novel Style)

### ADV vs NVL

Suika3 supports two presentation styles:

- **ADV mode** (default): a message box overlays the bottom of the screen, one line at a time.
- **NVL mode**: text fills the entire screen, accumulating line by line like a prose novel.

You can use both styles in the same game and switch between them at any point.

### Enabling NVL Mode

To switch the entire game to NVL mode by default, set the following in `config.ini`:

```
game.novel=true
```

Alternatively, keep `game.novel=false` and switch to NVL mode at specific points in your script
by hiding the name box and adjusting the message box to full-screen dimensions.

### Writing NVL-style Text

In NVL mode, each `[text]` tag appends a new line to the screen.
Use `action="clear"` to wipe the screen and start a new page:

```
[text action="clear"]
[text text="The letter arrived on a Tuesday."]
[click]
[text text="She read it twice before setting it down."]
[click]
```

To continue a paragraph without a line break, use `action="inline"`:

```
[text text="It was a short letter."]
[text action="inline" text=" But it said everything."]
[click]
```

### Mixing ADV and NVL

You can transition between styles mid-game. To switch to NVL for a flashback, for example:

```
[config key="game.novel" value="true"]
[text action="clear"]
[text text="Three years earlier..."]
[click]
```

And switch back to ADV afterwards:

```
[config key="game.novel" value="false"]
```

### NVL-related config.ini Settings

| Key                  | Description                                                   |
|----------------------|---------------------------------------------------------------|
| `game.novel`         | Set to `true` to enable NVL mode globally.                    |
| `namebox.enable`     | Set to `false` to hide the name box in NVL mode.             |
| `msgbox.dim.enable`  | Dim previously displayed paragraphs to highlight the current line. |
| `msgbox.dim.r/g/b`   | Color of the dimmed text.                                     |

See [nvl-mode.md](nvl-mode.md) and [config.md](config.md) for details.

---

## Chapter 6: Adding Animations

### The Layer Model

Suika3 renders your scene as a stack of **layers**. Each layer holds one image and can be
moved, scaled, rotated, and faded independently. The built-in layers include:

| Layer name | Contents                        |
|------------|---------------------------------|
| `bg`       | Background image                |
| `left`     | Left character sprite           |
| `center`   | Center character sprite         |
| `right`    | Right character sprite          |
| `msg`      | Message box                     |
| `name`     | Name box                        |

Animations target one or more layers and describe how their properties change over time.

### Writing an Anime File

An anime file describes a sequence of layer transforms.
Each block defines one animation segment:

```
slide_in {
    layer: left;

    start: 0.0;
    end:   0.5;

    from-x: r-100;
    to-x:   r0;

    from-a: 0;
    to-a:   255;
}
```

This slides the `left` layer in from 100 pixels to the left while fading it in, over 0.5 seconds.

- `r0` means "the layer's current position" (relative origin).
- `r-100` means "100 pixels to the left of the current position".
- `from-a` and `to-a` control alpha (transparency), from 0 (invisible) to 255 (fully visible).

Save this as `assets/anime/slide_in.anime`.

### Playing an Animation

Use the `[anime]` tag to play an anime file:

```
[anime file="assets/anime/slide_in.anime"]
```

By default, `[anime]` waits for the animation to finish before moving on.

### Moving Characters with [move]

For simple character movements, `[move]` is a convenient shorthand
that does not require a separate anime file:

```
[move time="0.5" left-x="r50"]
```

This slides the `left` layer 50 pixels to the right over 0.5 seconds.

### Putting It Together

Let's animate Xiaoling entering from the right side:

```
[bg file="assets/bg/park.png" time="0.5"]
[ch left="assets/ch/midori/normal.png" time="0.3"]
[anime file="assets/anime/slide_in_right.anime"]
[text name="Midori" text="Oh! You startled me."]
[click]
```

For the full anime syntax reference, see [anime.md](anime.md).

---

## Chapter 7: Customizing the UI (GUI)

### What is a GUI File?

Suika3's GUI system lets you build interactive screens — title menus, save/load screens,
settings panels — using a dedicated `.gui` file format.

A GUI file defines a list of **buttons**. Each button has:
- idle and hover images
- a behavior type (e.g., save, load, quit, jump to label)
- optional animations for state changes

Buttons are synchronous: when you call `[gui]`, the engine waits until the player clicks
a button or cancels before continuing.

### A Simple Menu

Create `assets/gui/title.gui`:

```
global {
    fadein:  0.2;
    fadeout: 0.2;
}

button {
    type:       label;
    label:      start;
    idle:       assets/gui/btn_start_idle.png;
    hover:      assets/gui/btn_start_hover.png;
    x:          540;
    y:          400;
}

button {
    type:       quit;
    idle:       assets/gui/btn_quit_idle.png;
    hover:      assets/gui/btn_quit_hover.png;
    x:          540;
    y:          500;
}
```

The first button jumps to a `[label name="start"]` in the current script when clicked.
The second button exits the game.

### Showing the GUI

```
[bg file="assets/bg/title.png"]
[gui file="assets/gui/title.gui"]

[label name="start"]
[load file="scene1.novel"]
```

When `[gui]` runs, the menu appears. After the player clicks a button, execution continues
from the line after `[gui]`.

### Button Animations

Buttons can trigger anime files when their state changes:

```
button {
    type:           label;
    label:          start;
    idle:           assets/gui/btn_start_idle.png;
    hover:          assets/gui/btn_start_hover.png;
    x:              540;
    y:              400;
    idle_anime:     assets/gui/btn_idle.anime;
    hover_anime:    assets/gui/btn_hover.anime;
}
```

### System UI in config.ini

Several system UI elements are configured in `config.ini`:

| Section          | Controls                                              |
|------------------|-------------------------------------------------------|
| `sysbtn.*`       | The hamburger menu button (position, images, sounds). |
| `automode.*`     | The auto-advance mode banner.                         |
| `skipmode.*`     | The skip mode banner.                                 |
| `gui.save.*`     | Font and layout of save/load slot items.              |
| `gui.history.*`  | Font and layout of the dialogue history screen.       |

To disable the system button (useful for kiosk or demo builds):

```
sysbtn.enable=false
```

See [gui.md](gui.md), [sysmenu.md](sysmenu.md), and [config.md](config.md) for details.

---

## Chapter 8: Bringing Characters to Life

### Eye Blinking

Suika3 can automatically animate characters blinking.
The blink frames are stored as a separate image alongside the character sprite.

**File layout:**

```
assets/ch/midori/normal.png          ← main character image
assets/ch/midori/eye/normal.png      ← blink frames
```

The blink image contains multiple frames laid out horizontally, left to right.
Each frame is the same size as the main character image, showing only the difference
(the closed-eye area). The edges must be blurred so they blend smoothly.

**config.ini settings:**

```
character.eyeblink.interval=4.0    # average seconds between blinks
character.eyeblink.frame=0.05      # duration of each blink frame in seconds
```

The interval is randomized slightly, and occasional double-blinks happen automatically.

See [eye-blink.md](eye-blink.md) for details on preparing the blink image.

### Lip Sync

Lip sync works the same way as eye blinking, but for mouth movement during voiced lines.

**File layout:**

```
assets/ch/midori/normal.png          ← main character image
assets/ch/midori/lip/normal.png      ← lip sync frames
```

**config.ini settings:**

```
character.lipsync.frame=0.04       # duration of each lip frame in seconds
character.lipsync.chars=14         # characters spoken per lip sync cycle
```

### Registering Characters in config.ini

For lip sync and auto focus to work, register each character's name and image folder:

```
character.name1=Midori
character.name1.en=Midori
character.name1.ja=みどり
character.folder1=assets/ch/midori/

character.name2=Xiaoling
character.name2.en=Xiaoling
character.folder2=assets/ch/xiaoling/
```

When the engine sees `[text name="Midori" ...]`, it looks up `character.name*` to identify
which character is speaking and activates the corresponding lip sync.

Up to 32 characters can be registered. See [lip-sync.md](lip-sync.md) and [config.md](config.md).

---

## Chapter 9: Extending the Engine with Ray

### What is Ray?

Ray is Suika3's built-in scripting language, based on NoctLang.
It can do everything you would expect from a general-purpose language:
variables, loops, conditionals, functions, dictionaries, and more.

More importantly, Ray gives you direct access to the engine's internals through two APIs:

- **`Suika.*`** — the high-level VN API (layers, images, config, input, etc.)
- **`Engine.*`** — the low-level 2D rendering API

The main entry point is `main.ray`, which runs on startup before `start.novel`.

### Defining a Custom Tag

The most common use of Ray in a VN project is adding custom NovelML tags.
Define a function named `Tag_<tagname>()` and it becomes available as `[tagname]` in NovelML.

In `main.ray`:

```
func Tag_flash(params) {
    // Flash the screen white for 0.1 seconds.
    Suika.setLayerImage({
        layer: Suika.LAYER_BG,
        image: "assets/effect/white.png"
    });
    Suika.wait({time: 0.1});
    Suika.setLayerImage({
        layer: Suika.LAYER_BG,
        image: ""
    });

    Suika.moveToNextTag();
    return true;
}
```

Use it in `start.novel`:

```
[flash]
[text name="Midori" text="What was that?!"]
[click]
```

Every custom tag function must call `Suika.moveToNextTag()` when it is done
and return `true` to signal success.

### Working with Layers and Images

Layers hold images. You can retrieve the image on any layer and manipulate it directly.

```
func Tag_tint_bg(params) {
    // Get the image currently on the background layer.
    var img = Suika.getLayerImage({layer: Suika.LAYER_BG});

    // Get the raw pixel buffer (array of ARGB uint32 values).
    var pixels = Suika.getImagePixels({image: img});

    // Apply a red tint.
    var w = Suika.getImageWidth({image: img});
    var h = Suika.getImageHeight({image: img});
    for (y in 0..h) {
        for (x in 0..w) {
            var p = pixels[y * w + x];
            var a = (p >> 24) & 0xff;
            var r = (p >> 16) & 0xff;
            var g = (p >>  8) & 0xff;
            var b =  p        & 0xff;
            g = g / 2;
            b = b / 2;
            pixels[y * w + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

    // Upload the modified pixels back to the GPU.
    Suika.updateImagePixels({image: img});

    Suika.moveToNextTag();
    return true;
}
```

### Organizing as a Plugin

For tags you want to reuse across projects, package them as a plugin.

Create `system/plugin/myplugin/myplugin.ray`:

```
func plugin_init_myplugin() {
    // Called once when the plugin is loaded.
}

func Tag_flash(params) {
    // ...same as above...
}
```

Load it from `main.ray`:

```
Suika.loadPlugin({name: "myplugin"});
```

After loading, `[flash]` is available in all NovelML files.

For the Ray language reference, see [ray-syntax.md](ray-syntax.md).
For the full VN API, see [ray-vn-api.md](ray-vn-api.md).
For more code examples, see [ray-samples.md](ray-samples.md).
For the plugin system, see [plugin.md](plugin.md).

---

## Chapter 10: Distributing for PC

### Step 1: Enable Release Mode

Before packaging, set the following in `config.ini` to store save data
in the OS user data directory instead of the game folder:

```
release_mode.enable=true
```

This is required for Windows Store and macOS App Store submissions,
and is good practice for any distributed build.

### Step 2: Create assets.arc

Bundle all your game files into a single archive using `suika3-pack`.

**Windows:**

1. Copy `SDK/windows/suika3-pack.exe` into your game folder.
2. Drag and drop your game files (`start.novel`, `main.ray`, `assets/`, `system/`, `config.ini`) onto `suika3-pack.exe`.
3. `assets.arc` is created in the same folder.

**macOS:**

```
cd SDK/macos
xattr -c suika3-pack
./suika3-pack start.novel main.ray assets/ system/ config.ini
```

**Linux:**

```
cd SDK/linux
./suika3-pack start.novel main.ray assets/ system/ config.ini
```

> **Note:** Video files cannot be packed into `assets.arc`. Distribute them as separate files alongside it.

### Step 3: Distribute

**Windows**

Give players `suika3.exe` and `assets.arc` in the same folder.
They launch the game by double-clicking `suika3.exe`.

**macOS — Simple**

Distribute `SDK/macos/Suika3.dmg` and `assets.arc` together.
Players open the DMG, copy `Suika3` out of it, place it next to `assets.arc`, and launch.

**macOS — Full (custom icon, embedded assets)**

1. Replace `SDK/macos/project/Resources/assets.arc` with your `assets.arc`.
2. Open `SDK/macos/project/` in Xcode, set your app icon and bundle ID.
3. Build to produce a self-contained `Suika3.app`.

**Linux**

Suika3 depends on GStreamer, so **Flatpak is the recommended distribution method**.
This ensures players do not need to install a matching GStreamer version manually.

```
flatpak install --user -y Suika3-x86_64.flatpak
flatpak override --user --filesystem=host vn.suika3.engine
flatpak run vn.suika3.engine .
```

The last argument (`.`) is the path to the folder containing your game files.
Wrap these commands in a launcher script for distribution so players do not run them manually.

For SteamOS, follow Valve's recommended packaging method for Steam Deck.

See [distribute.md](distribute.md) for the complete reference.

---

## Chapter 11: Distributing for Mobile

### iOS

Create `assets.arc` first (see Chapter 10).

**Method A: Xcode**

1. Replace `SDK/ios/Resources/assets.arc` with your `assets.arc`.
2. Open `SDK/ios/` in Xcode.
3. Set your bundle ID, app icon, and signing certificate.
4. Build and run on a device or in the iOS Simulator.

**Method B: VS Code Task (macOS only)**

1. Replace `SDK/ios/Resources/assets.arc` with your `assets.arc`.
2. Open the Command Palette and select **Tasks: Run Task → Build iOS IPA**.
3. The IPA is built and transferred to a USB-connected iPhone automatically.

> **Note:** Before using Method B, register your iPhone as a development device in Xcode at least once.

### Android

Android does not use `assets.arc`. Copy your game files directly into the project.

**Method A: Android Studio**

1. Copy your game files into `SDK/android/app/src/main/assets/`.
2. Open `SDK/android/` in Android Studio.
3. Build and run the project.

**Method B: VS Code Task**

1. Open the Command Palette and select **Tasks: Run Task → Build Android APK**.
2. OpenJDK and the Android SDK are downloaded automatically if not already present.
3. The APK is built and transferred to a USB-connected Android device.

### OpenHarmony (HarmonyOS NEXT)

OpenHarmony also uses loose files rather than `assets.arc`.

1. Copy your game files into `SDK/openharmony/entry/src/main/resources/rawfile/`.
2. Open `SDK/openharmony/` in DevEco Studio.
3. Build and run the project.

See [distribute.md](distribute.md) for the complete reference.

---

## Chapter 12: Publishing on the Web (WebAssembly)

### Deploying to a Web Server

Upload the following two files to the same folder on any static web server:

| File                       | Description             |
|----------------------------|-------------------------|
| `SDK/wasm/index.html`      | The game's web page.    |
| `assets.arc`               | Your packaged assets.   |

Players open `index.html` in their browser to launch the game.
No plugins or installation required.

### Local Testing

Before uploading, test the build locally.

**Windows**

Copy `SDK/wasm/suika3-web.exe` into the same folder as `index.html` and `assets.arc`, then run it.
It starts a local web server and opens the game in your default browser automatically.

**macOS and Linux**

Run the following in the folder containing `index.html` and `assets.arc`:

```
python3 -m http.server
```

Then open `http://localhost:8000` in your browser.

> **Note:** Opening `index.html` directly from the filesystem (without a web server) will not work
> due to browser security restrictions on local file access.

See [distribute.md](distribute.md) for the complete reference.

---

## Chapter 13: Localization (i18n)

Suika3 has built-in support for multiple languages. The engine selects the correct text
automatically based on the player's OS locale — no extra code required.
You can also let players switch languages in-game at runtime.

### How Locale Selection Works

Every `[text]` and `[choose]` tag accepts locale-suffixed parameters alongside the default text.
When the tag runs, the engine picks the best match for the current locale using this priority:

1. The most specific locale variant (e.g., `text-en-gb`)
2. The language fallback (e.g., `text-en`)
3. The unsuffixed default (`text`)

This means you can provide as many or as few translations as you need,
and the engine always falls back gracefully.

### Localizing Dialogue with [text]

Add locale suffixes to the `text` parameter:

```
[text
    name="Midori"
    text="Good afternoon!"
    text-ja="こんにちは！"
    text-fr="Bonne après-midi !"
    text-zh-cn="下午好！"
]
[click]
```

When a Japanese player runs this, they see `こんにちは！`.
A French player sees `Bonne après-midi !`.
Everyone else sees `Good afternoon!`.

The `name` parameter is not localized here — use `character.name*` in `config.ini` to provide
localized speaker names (see Chapter 8 and [config.md](config.md)).

### Localizing Choices with [choose]

Each choice option supports locale suffixes on its `text` parameter:

```
[choose
    text1="Go to the café"    text1-ja="カフェに行く"    text1-fr="Aller au café"    label1="cafe"
    text2="Stay in the park"  text2-ja="公園に残る"      text2-fr="Rester au parc"   label2="park"
]
```

The selected label is the same regardless of language — only the displayed text changes.

### Supported Locale Suffixes

| Suffix      | Language                          |
|-------------|-----------------------------------|
| `-en`       | English (fallback for all regions)|
| `-en-us`    | English (US)                      |
| `-en-gb`    | English (UK)                      |
| `-en-au`    | English (Australia)               |
| `-en-nz`    | English (New Zealand)             |
| `-fr`       | French (fallback)                 |
| `-fr-fr`    | French (France)                   |
| `-fr-ca`    | French (Canada)                   |
| `-es`       | Spanish (fallback)                |
| `-es-la`    | Spanish (Latin America)           |
| `-de`       | German                            |
| `-it`       | Italian                           |
| `-ru`       | Russian                           |
| `-el`       | Greek                             |
| `-zh-cn`    | Simplified Chinese                |
| `-zh-tw`    | Traditional Chinese (Taiwan)      |
| `-ja`       | Japanese                          |

For a full list including planned future languages, see [novelml-tags.md](novelml-tags.md).

### Localizing Voice Files

Voice files can also be localized. Different dubs play automatically per locale:

```
[text
    name="Midori"
    text="Good afternoon!"
    text-ja="こんにちは！"
    voice="voice/midori/greeting.ogg"
    voice-ja="voice/midori/greeting-ja.ogg"
]
```

### Switching Language at Runtime

To force a specific language regardless of the OS locale,
write `game.locale` in `config.ini` (see [config.md](config.md)):

```
game.locale=ja
```

You can also change this at runtime from a NovelML script using `[config]`:

```
[config key="game.locale" value="ja"]
```

After this tag runs, all subsequent `[text]` and `[choose]` tags will use Japanese text.
This makes it easy to implement an in-game language selection screen.

### Language Switching Buttons in a GUI

The GUI system has a dedicated `language` button type that switches the locale when clicked
and shows an active state image when that language is currently selected.

Add one button per supported language to your settings GUI file:

```
EN {
    id:           7;
    type:         language;
    lang:         en-us;
    x:            1050;
    y:            20;
    image-idle:   system/config/config-lang-en-idle.png;
    image-hover:  system/config/config-lang-en-hover.png;
    image-active: system/config/config-lang-en-active.png;
}

JA {
    id:           8;
    type:         language;
    lang:         ja;
    x:            1150;
    y:            20;
    image-idle:   system/config/config-lang-ja-idle.png;
    image-hover:  system/config/config-lang-ja-hover.png;
    image-active: system/config/config-lang-ja-active.png;
}
```

- **`lang`** — the locale to activate when clicked (matches `game.locale` values).
- **`image-active`** — shown instead of `image-idle` when this language is currently active,
  giving the player a clear indication of the selected language.

The language change takes effect immediately — no restart or reload is needed.
The engine saves the selected locale so it persists across sessions.

For the full GUI button reference, see [gui.md](gui.md).

---

## Appendix

### Common Tags Quick Reference

| Tag          | What it does                                              |
|--------------|-----------------------------------------------------------|
| `[text]`     | Display dialogue or narration.                            |
| `[click]`    | Wait for a player click.                                  |
| `[bg]`       | Change the background image.                              |
| `[ch]`       | Show or hide a character sprite.                          |
| `[bgm]`      | Play or stop background music.                            |
| `[se]`       | Play a sound effect.                                      |
| `[anime]`    | Play an animation file.                                   |
| `[move]`     | Move a layer over time.                                   |
| `[choose]`   | Show a choice menu.                                       |
| `[if]`       | Begin a conditional branch.                               |
| `[elseif]`   | Add a condition to a branch.                              |
| `[else]`     | Fallback branch when no condition matches.                |
| `[endif]`    | End a conditional branch.                                 |
| `[label]`    | Mark a position for jumps.                                |
| `[goto]`     | Jump to a label.                                          |
| `[load]`     | Load another NovelML file.                                |
| `[gui]`      | Show a GUI screen.                                        |
| `[chapter]`  | Set the current chapter name (used in save slots).        |
| `[config]`   | Change a config value at runtime.                         |

### Next Steps

Once you have finished this tutorial, explore the following topics:

- **AOT compilation** — compile your Ray scripts to native code for iOS performance compliance.
  See [aot.md](aot.md).
- **Console distribution via Unity** — use the Unity project at `SDK/unity/` to target
  console platforms. See [distribute.md](distribute.md).
- **Building from source** — if you want to modify the engine itself or build for Linux.
  See [build.md](build.md).
- **Engine architecture** — understand how the layers fit together.
  See [design.md](design.md) and [srs.md](srs.md).
