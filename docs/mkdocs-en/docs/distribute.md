Distributing Games
==================

Once your game is ready, you can bundle it into a single package for easy distribution.

## Creating assets.arc

### On Windows

You can use the packaging tool included with Playfield Engine:

1. Go to the `misc/windows/` folder.
2. Drag and drop all your game files (such as main.pf, images, sounds, etc.) onto `playfield-pack.exe`.
3. The tool will create an `assets.arc` file in the same folder.

This file contains all of your game’s scripts and assets in one archive.

### On macOS

Use `misc/macos/playfield-pack` instead:

1. Open the terminal.
2. Go to `misc/macos/`. (`cd misc/macos`)
3. Run `xattr -c playfield-pack` to clear the macOS's quarantine flag.
4. Run `./playfield-pack <files>` to create a package.

### On Linux

Use `misc/linux/playfield-pack` instead:

1. Open the terminal.
2. Go to `misc/linux/`. (`cd misc/macos`)
3. Run `./playfield-pack <files>` to create a package.

## Files to Distribute

To distribute your game, you only need the following two files:

* `playfield.exe`
* `assets.arc`

Place them in the same folder. Your players can then run the game by simply double-clicking playfield.exe.

## Changing the Icon

### For Windows

Use a third-party tool to replace the application icon in playfield.exe.

Or alternatively:
1. Download the Playfield Engine source code.
2. Replace `resources/windows/icon.ico` with your own icon.
3. Rebuild the app.

### For macOS

1. Download the Playfield Engine source code.
2. Replace `resources/macos/icon512.png` with your own icon image.
3. Rebuild the app.

### For Linux

1. Download the Playfield Engine source code.
2. Replace `external/StratoHAL/src/icon.c`, with your own icon data.
  * The file must contain a valid XPM image text.
3. Rebuild the app.
