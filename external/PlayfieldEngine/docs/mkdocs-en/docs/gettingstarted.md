Getting Started
===============

Follow the steps below to run your first game.

## Get Playfield Engine

**On Windows:**
* Download the latest `Full` zip.
* Copy the `playfield.exe` app into a game folder. (e.g., "samples/shoot2")
* Double-click `playfield.exe` to run the sample game.

**On macOS:**

* Download the latest `Full` zip.
* Open `macos/Playfield.dmg`.
* Copy the `Playfield` app from the DMG into a game folder. (e.g., "samples/shoot2")
* Double-click the `Playfield` app to run the sample game.

**On Linux:**

* Download the latest `Full` zip.
* Place the `linux/Playfield-x86_64.AppImage` file to a game folder. (e.g., "samples/shoot2")
* Make the AppImage executable if necessary (`chmod +x Playfield-x86_64.AppImage`).
* Double-click the AppImage to run the sample game.

## Making a First Game

To create a minimal game:

1. Make a new folder anywhere you like.
2. Open your favorite text editor in that folder.
    - On Windows, you can use `Playfield Editor` if you don't have a favorite editor.
3. Create a new file called `main.ray`.
4. Copy the following code into to the file an save it.

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

## Running Games

Playfield Engine can load a game in two ways:

* From a folder that contains a `main.ray` file.
* From an `assets.arc` file that bundles all game assets.

To create an `assets.arc` file, see [Distributing Games](distribute.md).
