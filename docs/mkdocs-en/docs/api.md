Playfield Script API
====================

Every API function takes one parameter. The parameter must be a dictionary and
arguments must be stored as key-and-value pairs.

## Skelton

```
// Do not define variables outside functions because it's a syntax error.

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
    // Global variables should be defined here.
    posX = 0;
    posY = 0;

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

## Debug

### print()

This API prints a string or dumps an object.
Only this API takes a non-dictionary argument.

```
func dumpEnemies() {
    if (Engine.isGamepadXPressed) {
        print("[Current emenies]");
        print(enemies);
    }
}
```

## Time

### Absolute Time

|Variable                    |Description                                |
|----------------------------|-------------------------------------------|
|Engine.millisec             |Time in millisec.                          |

```
func update() {
    // Get the seconds since the last call.
    var curTime = Engine.millisec;
    var dt = (curTime - lastTime) * 0.001;
    lastTime = curTime;

    updateCharacters(dt);
}
```

### Engine.getDate()

Returns a date dictionary.

```
func frame() {
    var date = Engine.getDate({});

    var year  = date.year;
    var month = date.month;
    var day   = date.day;
    var hour  = date.hour;
    var min   = date.minute;
    var sec   = date.second;
}
```

## Input

These are variables, not functions.

|Variable                        |Description                                |
|--------------------------------|-------------------------------------------|
|Engine.mousePosX                |Mouse position X.                          |
|Engine.mousePosY                |Mouse position Y.                          |
|Engine.isMouseLeftPressed       |Left mouse button state.                   |
|Engine.isMouseRightPressed      |Right mouse button state.                  |
|Engine.isEscapeKeyPressed       |Escape key state.                          |
|Engine.isReturnKeyPressed       |Return key state.                          |
|Engine.isSpaceKeyPressed        |Space key state.                           |
|Engine.isTabKeyPressed          |Tab key state.                             |
|Engine.isBackspaceKeyPressed    |Backspace key state.                       |
|Engine.isDeleteKeyPressed       |Delete key state.                          |
|Engine.isHomeKeyPressed         |Home key state.                            |
|Engine.isEndKeyPressed          |End key state.                             |
|Engine.isPageupKeyPressed       |PageUp key state.                          |
|Engine.isPagedownKeyPressed     |PageDown key state.                        |
|Engine.isShiftKeyPressed        |Shift key state.                           |
|Engine.isControlKeyPressed      |Control key state.                         |
|Engine.isAltKeyPressed          |Alt key state.                             |
|Engine.isUpKeyPressed           |Up arrow key state.                        |
|Engine.isDownKeyPressed         |Down arrow key state.                      |
|Engine.isRightKeyPressed        |Right arrow key state.                     |
|Engine.isLeftKeyPressed         |Left arrow key state.                      |
|Engine.isAKeyPressed            |'A' key state.                             |
|Engine.isBKeyPressed            |'B' key state.                             |
|Engine.isCKeyPressed            |'C' key state.                             |
|Engine.isDKeyPressed            |'D' key state.                             |
|Engine.isEKeyPressed            |'E' key state.                             |
|Engine.isFKeyPressed            |'F' key state.                             |
|Engine.isGKeyPressed            |'G' key state.                             |
|Engine.isHKeyPressed            |'H' key state.                             |
|Engine.isIKeyPressed            |'I' key state.                             |
|Engine.isJKeyPressed            |'J' key state.                             |
|Engine.isKKeyPressed            |'K' key state.                             |
|Engine.isLKeyPressed            |'L' key state.                             |
|Engine.isMKeyPressed            |'M' key state.                             |
|Engine.isNKeyPressed            |'N' key state.                             |
|Engine.isOKeyPressed            |'O' key state.                             |
|Engine.isPKeyPressed            |'P' key state.                             |
|Engine.isQKeyPressed            |'Q' key state.                             |
|Engine.isRKeyPressed            |'R' key state.                             |
|Engine.isSKeyPressed            |'S' key state.                             |
|Engine.isTKeyPressed            |'T' key state.                             |
|Engine.isUKeyPressed            |'U' key state.                             |
|Engine.isVKeyPressed            |'V' key state.                             |
|Engine.isWKeyPressed            |'W' key state.                             |
|Engine.isXKeyPressed            |'X' key state.                             |
|Engine.isYKeyPressed            |'Y' key state.                             |
|Engine.isZKeyPressed            |'Z' key state.                             |
|Engine.is1KeyPressed            |'1' key state.                             |
|Engine.is2KeyPressed            |'2' key state.                             |
|Engine.is3KeyPressed            |'3' key state.                             |
|Engine.is4KeyPressed            |'4' key state.                             |
|Engine.is5KeyPressed            |'5' key state.                             |
|Engine.is6KeyPressed            |'6' key state.                             |
|Engine.is7KeyPressed            |'7' key state.                             |
|Engine.is8KeyPressed            |'8' key state.                             |
|Engine.is9KeyPressed            |'9' key state.                             |
|Engine.is0KeyPressed            |'0' key state.                             |
|Engine.isF1KeyPressed           |F1 key state.                              |
|Engine.isF2KeyPressed           |F2 key state.                              |
|Engine.isF3KeyPressed           |F3 key state.                              |
|Engine.isF4KeyPressed           |F4 key state.                              |
|Engine.isF5KeyPressed           |F5 key state.                              |
|Engine.isF6KeyPressed           |F6 key state.                              |
|Engine.isF7KeyPressed           |F7 key state.                              |
|Engine.isF8KeyPressed           |F8 key state.                              |
|Engine.isF9KeyPressed           |F9 key state.                              |
|Engine.isF10KeyPressed          |F10 key state.                             |
|Engine.isF11KeyPressed          |F11 key state.                             |
|Engine.isF12KeyPressed          |F12 key state.                             |
|Engine.isGamepadUpPressed       |Gamepad up arrow state.                    |
|Engine.isGamepadDownPressed     |Gamepad down arrow state.                  |
|Engine.isGamepadLeftPressed     |Gamepad left arrow state.                  |
|Engine.isGamepadRightPressed    |Gamepad right arrow state.                 |
|Engine.isGamepadAPressed        |Gamepad A button state.                    |
|Engine.isGamepadBPressed        |Gamepad B button state.                    |
|Engine.isGamepadXPressed        |Gamepad X button state.                    |
|Engine.isGamepadYPressed        |Gamepad Y button state.                    |
|Engine.isGamepadLPressed        |Gamepad L button state.                    |
|Engine.isGamepadRPressed        |Gamepad R button state.                    |
|Engine.gamepadAnalogX1          |Gamepad analog 1 X (-32768, 32767)         |
|Engine.gamepadAnalogY1          |Gamepad analog 1 Y (-32768, 32767)         |
|Engine.gamepadAnalogX2          |Gamepad analog 2 X (-32768, 32767)         |
|Engine.gamepadAnalogY2          |Gamepad analog 2 Y (-32768, 32767)         |
|Engine.gamepadAnalogL           |Gamepad analog L (-32768, 32767)           |
|Engine.gamepadAnalogR           |Gamepad analog R (-32768, 32767)           |

```
func update() {
    if (Engine.isMouseLeftPressed) {
        player.x = player.x + 100;
    }
}
```

## Rendering

### Engine.createColorTexture()

This API creates a texture with the specified color, and returns a texture.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|width               |Texture width.                                                |
|height              |Texture height.                                               |
|r                   |Red color.                                                    |
|g                   |Green color.                                                  |
|b                   |Blue color.                                                   |
|a                   |Alpha color.                                                  |

```
func createBlockTexture() {
    blockTex = Engine.createColorTexture({
                   width:  16,
                   height: 16,
                   r:      255,
                   g:      255,
                   b:      255,
                   a:      255
               });
}
```

### Engine.loadTexture()

This API loads a texture from assets, and returns a texture.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|file                |File name to load.                                            |

```
func loadPlayerTexture() {
   playerTex = Engine.loadTexture({
                   file: "player.png"
               });

   var width = playerTex.width;
   var height = playerTex.height;
}
```

### Engine.destroyTexture()

This API destroys a texture.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|texture             |Texture.                                                      |

```
func destroyPlayerTexture() {
    Engine.loadTexture({
        texture: playerTex
    });
}
```

### Engine.renderTexture()

This API renders a texture to the screen.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|dstLeft             |Screen coordinate X.                                          |
|dstTop              |Screen coordinate Y.                                          |
|dstWidth            |Width in screen.                                              |
|dstHeight           |Height in screen.                                             |
|texture             |Texture.                                                      |
|srcLeft             |Texture top left X.                                           |
|srcTop              |Texture top left Y.                                           |
|srcWidth            |Texture rectangle width.                                      |
|srcHeight           |Texture rectangle height.                                     |
|alpha               |Alpha value (0-255)                                           |

```
func renderPlayer() {
    Engine.renderTexture({
        dstLeft:   playerPos.x,
        dstTop:    playerPos.y,
        dstWidth:  playerTex.width,
	dstHeight: playerTex.height,
        texture:   playerTex,
        srcLeft:   0,
        srcTop:    0,
        srcWidth:  playerTex.width,
        srcHeight: playerTex.height,
        alpha:     255
    });
}
```

### Engine.draw()

This API renders a texture to the screen, a simple version of `Engine.renderTexture()`.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|texture             |Texture.                                                      |
|x                   |Screen coordinate X.                                          |
|y                   |Screen coordinate Y.                                          |

```
func renderPlayer() {
    Engine.draw({
        texture: playerTex,
        x:       playerPos.x,
        y:       playerPos.y
    });
}
```

### Engine.renderTexture3D()

This API renders a texture to the screen.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|x1                  |Screen coordinate X1.                                         |
|y1                  |Screen coordinate Y1.                                         |
|x2                  |Screen coordinate X2.                                         |
|y2                  |Screen coordinate Y2.                                         |
|x3                  |Screen coordinate X3.                                         |
|y3                  |Screen coordinate Y3.                                         |
|x4                  |Screen coordinate X4.                                         |
|y4                  |Screen coordinate Y4.                                         |
|texture             |Texture.                                                      |
|srcLeft             |Texture top left X.                                           |
|srcTop              |Texture top left Y.                                           |
|srcWidth            |Texture rectangle width.                                      |
|srcHeight           |Texture rectangle height.                                     |
|alpha               |Alpha value (0-255)                                           |

```
func renderPlayer() {
    Engine.renderTexture({
        dstLeft:   playerPos.x,
        dstTop:    playerPos.y,
        dstWidth:  playerTex.width,
	dstHeight: playerTex.height,
        texture:   playerTex,
        srcLeft:   0,
        srcTop:    0,
        srcWidth:  playerTex.width,
        srcHeight: playerTex.height,
        alpha:     255
    });
}
```

### Engine.loadFont()

This API loads an asset font file to a specified font slot.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|slot                |Font slot index. (0-3)                                        |
|file                |File name to load.                                            |

```
func loadNotoSansFont() {
    Engine.loadFont({ slot: 0, file: "NotoSans.ttf" });
}
```

### Engine.createTextTexture()

This API creates a texture and draws a text on it.
     	 	 
|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|slot                |Font slot index. (0-3)                                        |
|text                |Text to draw.                                                 |
|size                |Font size.                                                    |
|r                   |Red color.                                                    |
|g                   |Green color.                                                  |
|b                   |Blue color.                                                   |
|a                   |Alpha color.                                                  |

```
func createScoreTexture() {
    scoreTex = Engine.createTextTexture({
                   slot: 0,
                   text: "Score: " + score,
                   size: 32,
                   r:    255,
                   g:    255,
                   b:    255,
                   a:    255
               });
}
```

## Sound

### Engine.playSound()

This API starts playing a sound asset file on a specified sound track.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|stream              |Track index. (0-3)                                            |
|file                |File to play.                                                 |

```
func playJumpSound() {
    Engine.playSound({ stream: 0, file: "jump.ogg" });
}
```

### Engine.stopSound()

This API stops a sound playback on a specified sound track.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|stream              |Track index. (0-3)                                            |

```
func playJumpSound() {
    Engine.stopSound({ stream: 0 });
}
```

### Engine.setSoundVolume()

This API sets a sound volume on a specified sound track.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|stream              |Track index. (0-3, -1 for master)                             |
|volume              |Volume value. (0-1.0)                                         |

```
func playJumpSound() {
    Engine.setSoundVolume({
        stream: 0,
        volume: 1.0
    });
}
```

## Save Data

### Engine.writeSaveData()

This API writes a save data value that corresponds to a key string.
If the save data value is too large, this API will fail.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|key                 |Key string.                                                   |
|value               |Value. (integer, floating point, array, or dictionary)        |

### Engine.readSaveData()

This API reads save data value that corresponds to a key string.
The return value will be an object that represents a save data value.
This API will fail when the specified key is not available.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|key                 |Key string.                                                   |

### Engine.checkSaveData()

This API checks whether the save data exist for a key string or not.
The return value is a boolean.

|Argument Name       |Description                                                   |
|--------------------|--------------------------------------------------------------|
|key                 |Key string.                                                   |
