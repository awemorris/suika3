Distributing Games
==================

Once your game is ready, you can bundle it into a single package for easy distribution.

## Creating assets.arc

### On Windows

You can use the packaging tool included with Playfield Engine:

1. Go to the `SDK/windows/` folder.
2. Find the `suika3-pack.exe` file.
3. Copy the `suika3-pack.exe` file to the game folder.
4. Drag and drop all your game files (such as `main.ray`, `images`, `system`, etc.) onto `suika3-pack.exe`.
5. The tool will create an `assets.arc` file in the same folder.

This file contains all of your game’s scripts and assets in one archive.

### On macOS

Use `SDK/macos/playfield-pack` instead:

1. Open the terminal.
2. Go to `SDK/macos/`. (`cd SDK/macos`)
3. Run `xattr -c suika3-pack` to clear the macOS's quarantine flag.
4. Run `./suika3-pack <files>` to create a package.

### On Linux

Use `SDK/linux/suika3-pack` instead:

1. Open the terminal.
2. Go to `SDK/linux/`. (`cd SDK/macos`)
3. Run `./suika3-pack <files>` to create a package.

## Files to Distribute

To distribute your game, you only need the following two files:

* `suika3.exe`
* `assets.arc`

Place them in the same folder. Your players can then run the game by simply double-clicking playfield.exe.

## Videos

Please note that video files cannot be packed into the `assets.arc`
file, and are required to be distributed as normal files.
