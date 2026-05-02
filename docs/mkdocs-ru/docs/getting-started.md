Suika3: Getting Started Guide
=============================

Welcome to Suika3! This guide will help you jump-start your very first
visual novel project in just a few easy steps.

## 1. Installation

Let's get the engine running so you can see the magic happen!

### Windows

- **Download & Extract**
    - Download [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip) and extract it to your preferred folder.
- **Launch**
    - Open the folder and run `suika3.exe` to start to the sample game!

### macOS

- **Download & Extract**
    - Download Suika3-full.zip and extract it to your preferred folder.
- **Mount the Disk Image**
    - Navigate to `SDK/macos/` and open `Suika3.dmg`.
- **Setup the App Bundle**
    - Copy the `Suika3` app from the DMG into the same folder where `suika3.exe` (and the data folder) is located.
    - Note: The app bundle must reside alongside your game data to function correctly.
- **Launch**
    - Double-click the `Suika3` app to start the sample game!

### Linux

- **Download & Extract**
    - Download Suika3-full.zip and extract it to your preferred directory.
- **Install the Flatpak Package**
    - Navigate to `SDK/linux/` and open `Suika3.flatpak` (or run `flatpak install --user Suika3.flatpak`).
    - This associates `.novel` and `.ray` files with the Suika3 engine.
- **Launch**
    - Open the extracted folder, then double-click `start.novel` to launch the sample game!

## 2. Visual Studio Code Integration

VSCode Integration is available on Windows, macOS, and Linux!

Also, [NovelML-Helper](https://github.com/lalalll-lalalll/NovelML-Helper) is available for syntax highlighting.

- Open the extracted folder by `Visual Studio Code`.
- Click the command palette.
- Click `Run Task`.
- Choose from:
    - `Suika3: Run` (or `Ctrl+Shift+B`)
    - `Suika3: Create a package`
    - `Suika3: Build Android APK`
    - `Suika3: Build iOS IPA`
- Click `PROBLEMS` if an error occurred.

## 3. Personalize Your Story (`start.novel`)

Now, let's make the game say exactly what you want.

- **Open:**
    - Find the `start.novel` file in your project folder and open it with your favorite text editor.
- **Edit:**
    - Add the following tag at the beginning of the file:
    ```
    [text text="Hello, world! This is my first game."]
    ```
- **Test:**
    - Save the file and run Suika3 again.
    - You should see your new message on the screen!

## 4. Customize the Screen (main.ray)

You can easily change the look and feel of your game window.

- **Locate:**
    - Open the `main.ray` file in your editor.
- **Modify:**
    - Look for the `func setup()` section.
    - You can change the resolution and the title of your window here:
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

## 5. Under the Hood (Advanced Tips)

The bottom part of your `main.ray` file contains the core engine
logic. It's best to leave these functions as they are unless you are
doing advanced customization:

- `func start()`:
    - This is called once when your game launches.
- `func update()`:
    - This runs every single frame to handle game logic.
- `func render()`:
    - This draws everything on the screen after the update is done.

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

> [!TIPS]
> These functions are the core mechanism of the `Playfield Engine` that
> powers Suika3. Suika.start(), Suika.update(), and Suika.render() must
> remain in place for the game to function properly.
