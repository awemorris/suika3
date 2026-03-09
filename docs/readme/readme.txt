Playfield
=========

 * Load this file into ChatGPT.
 * Try entering: "Create a game in Playfield Script."

About
-----

Playfield is a lightweight game scripting runtime.  With scripts written in
Playfield Script, it can render graphics and play sounds across multiple
platforms.

Whether you're a beginner or a seasoned power user, you can create games on
your PC — just like coding in classic BASIC, but with modern features.

Files
-----

  playfield.exe           ... Main Executable (64-bit)
  playfield-editor.exe    ... Text Editor For Convenience
  main.pf                 ... Sample main script
  readme.txt              ... This document

Installation
------------

Just extract the ZIP file to any folder of your choice.

Usage
-----

Double-click "playfield.exe".  If a "main.pf" file exists in the same folder,
it will run automatically.

Please use your favorite text editor instead of "playfield-editor.exe" as it
is just for your convenience.

License / Copyright Notice
--------------------------

zlib License
(C) 2025 Awe Morris

Contact
-------

Email: Awe Morris <awe@noctvm.io>
Project Page: https://noctvm.io/playfield/

I'd love to hear from you, feel free to reach out.

==============================================================================

Playfield Script Syntax
=======================

`Playfield` uses `Noct` as a scripting language.

## Assignments

Variables in Noct are dynamically typed and don't require explicit
declaration. The assignment operator (`=`) is used to create and
assign values to variables.

As shown in the example below, Noct supports various data types
including integers, floating-point numbers, and strings. Variables can
be reassigned to different types at any time during execution.

```
func main() {
    var a = 123;
    print(a);

    var b = 1.0;
    print(b);

    var c = "string";
    print(c);
}
```

## Global Variables

Global variables can be defined in functions, and cannot be defined
outside functions.

```
func main() {
    globalVariable = 123;
    print(globalVariable);
}
```

## Local Variables

Using the `var` keyword allows you to declare a variable as
local. Without `var` declaration, assigning to a variable may create a
global variable.

```
func main() {
    var a = 123;
    print(a);
}
```

## Array

Arrays are ordered collections of values, accessed by index. Arrays
support iteration through the `for` loop construct, allowing you to
iterate through each value directly.

```
func main() {
    var array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Arrays can hold values of different types simultaneously, reflecting
the dynamic typing system.

```
func main() {
    var array = [123, "string"];
}
```

The language provides a built-in function `push()` to add elements to
the end of an array.  Also, `pop()` removes the final element.

```
func main() {
    var array = []
    array->push(0);
    array->push(1);
    array->push(2);

    var last = array->pop();

    print("Length = " + array.length);
}
```

## Dictionary

Dictionaries store key-value pairs, similar to hash maps or objects in
other languages. They are defined using curly braces with key-value
pairs separated by colons. Dictionaries support iteration where both
the key and value can be accessed simultaneously.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print("key = " + key);
        print("value = " + value);
    }

    print("Length = " + dict.length);
}
```

Dictionaries may be constructed in a single step way. An assignment
can be an array style which uses `[]`, or an object style which uses
`.`.

```
func main() {
    var dict = {};
    dict["key1"] = "value1";
    dict.key2 = "value2";
}
```

The built-in function `remove()` allows for the deletion of entries by
key.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    remove(dict, "key1");
}
```

## For-loop

The for-loop construct provides a concise syntax for iterating through
sequences such as ranges, arrays, and dictionaries.

The range syntax (using the `..` operator) creates an iterator that
generates values from the start to one less than the end value.

```
func main() {
    for (i in 0..10) {
        print(i);
    }
}
```

For-loops can also iterate directly over arrays and other collection
types.

Arrays can be iterated by the for-value syntax.

```
func main() {
    array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Dictionaries can be iterated by the for-key-value syntax.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

## While Loops

The while-loop provides a traditional iteration mechanism that
continues execution as long as a specified condition remains
true. Unlike for-loops which are designed for iterating over
collections, while-loops are more flexible and can be used for
implementing various algorithms where the number of iterations isn't
known in advance. The example shows a basic counter implementation
incrementing from 0 to 9.

```
func main() {
    var i = 0;
    while (i < 10) {
        print(i);
        i = i + 1;
    }
}
```

## If and Else Blocks

Control flows allow for conditional executions based on evaluated
expressions. The if-else construct follows a familiar syntax where
conditions are evaluated in sequence.

```
func main() {
    var a = readint();
    if (a == 0) {
        print("0");
    } else if (a == 1) {
        print("1");
    } else {
        print("other");
    }
}
```

## Lambda Functions

Functions are first-class objects in the language. Anonymous
functions, also known as `lambda` expressions, allow you to create
functions without names.

```
func main() {
    var f = (a, b) => { return a + b; }
    print(f(1, 2));
}
```

Lambda functions are simply translated to named functions in the
compilation process. Therefore, they can't capture variables declared
in outer functions.

## Increment/Decrement (+=, -=, ++, --)

```
func main() {
    var a = 123;
    a += 321;
    a++;

    var b = 123;
    b -= 321;
    b--;
}
```

`++` and `--` are supported only as standalone statements (`a++;`, `b--;`).
Using them inside expressions is disallowed to avoid complex side-effects.

## OOP in Noct

The object-oriented model in Noct is a lightweight variation of prototype-based OOP.

- Classes are simply dictionary templates
- Inheritance and instantiation are realized by dictionary merging
- There is no prototype chain, and modifying a class does not affect existing instances

This design treats dictionaries as first-class objects, and the author refers to it as Dictionary-based OOP (D-OOP).

```
func main() {
    // The base class definition. (A class is just a dictionary.)
    Animal = class {
        name: "Animal",
        cry: (this) => {
        }
    };

    // The subclass definition. (Just a dictionary merging.)
    Cat = extend Animal {
        name: "Cat",
        voice: "meow",
        cry: (this) => {
            print(this.name + " cries like " + this.voice);
        }
    };

    // Instantiation. (Just a dictionary merging.)
    var myCat = new Cat {
        voice: "neee"
    };

    // This-call uses -> () syntax. (Equal to myCat.cry(myCat))
    myCat->cry();
}
```

## Intrinsics

### int()

```
var i = int(1.23);
```

### float()

```
var f = float(123);
```

### newArray()

```
var array = newArray(10);
```

### push()

```
var array = [1, 2, 3];
array->push(4);
```

### pop()

```
var array = [1, 2, 3];
var last = array->pop();
```

### resize()

```
var array = [1, 2, 3];
array->resize(2);
```

### substring()

```
var s1 = "ABCDEFG";
var s2 = s1.substring(1, -1); // from the char 1, to the end "BCDEFG"
}
```

### charAt()

```
var s = "ABC";
for (i in 0 .. s.length) {
   var c = s->charAt(i);
   print(c);
}
```

### sin(), cos(), tan()

```
var s = sin(x);
var c = cos(x);
var t = tan(x);
```

### abs()

```
var a = abs(x);
```

### random()

```
var r = random(); // 0 .. 1.0
```

### fast_gc(), full_gc(), compact_gc()

```
// Do young GC.
fast_gc();

// Do young + old GC.
full_gc();

// Do young + old + compact GC.
compact_gc();
```

### unset()

```
var dic = {key1: "ABC"};
dic->unset("key1");
```

==============================================================================

Playfield Script API
====================

Every API function takes one parameter. The parameter must be a dictionary and
arguments must be stored as key-and-value pairs.

## Skelton

```
func setup() {
    return {
        width:  1920,
        height: 1080,
        title:  "My Game"
    };
}

func start() {
    playerTex = Engine.loadTexture({
                    file: "player.png"
                });
}

func frame() {
    Engine.draw({
        texture: playerTex,
        x:       Engine.mousePosX,
        y:       Engine.mousePosY
    });
}
```

## Debug

### debug()

This API prints a string or dumps an object.
Only this API takes a non-dictionary argument.

```
func dumpEnemies() {
    if (Engine.isGamepadXPressed) {
        debug("[Current emenies]");
        debug(enemies);
    }
}
```

## Time

### Absolute Time

|Variable                    |Description                                |
|----------------------------|-------------------------------------------|
|Engine.millisec             |Time in millisec.                          |

```
func frame() {
    var dt = (Engine.millisec - lastTime) * 0.001;

    update(dt);
    render();
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

### Mouse Positions

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

==============================================================================
All Licenses
==============================================================================

Playfield

(C) 2025 Awe Morris

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

---

zlib

Copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

---

libpng

Copyright (c) 1998-2019 Glenn Randers-Pehrson
Portions copyright (c) 1996 Guy Eric Schalnat, Group 42, Inc.

This copy of the libpng reference library is supplied for research and 
personal use only.  Commercial use is permitted only if you make 
arrangements with the copyright holder.

COPYRIGHT NOTICE, DISCLAIMER, and LICENSE:

Copyright (c) 1995-2019 The PNG Reference Library Authors.
Copyright (c) 1996-1997 Andreas Dilger.
Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.

The authors and distributors specifically disclaim any warranties, 
including, but not limited to, the implied warranties of 
merchantability and fitness for a particular purpose.  
The library is provided "AS IS", and the authors and distributors 
have no obligation to provide maintenance, support, updates, 
enhancements, or modifications.

Permission is hereby granted to use, copy, modify, and distribute 
this source code, or portions hereof, for any purpose, without fee, 
subject to the following restrictions:

1. The origin of this source code must not be misrepresented.
2. Altered versions must be plainly marked as such and must not be 
   misrepresented as being the original source.
3. This Copyright notice may not be removed or altered from any 
   source or altered source distribution.

---

libogg

Copyright (c) 1994-2023 Xiph.Org Foundation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of the Xiph.Org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE FOUNDATION
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

---

libvorbis

Copyright (c) 2002-2023 Xiph.Org Foundation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of the Xiph.Org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE FOUNDATION
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

---

IGJ JPEG 9

This software is copyright (C) 1991-2020, Thomas G. Lane, Guido Vollbeding.

This software is based in part on the work of the Independent JPEG Group.

Permission is hereby granted to use, copy, modify, and distribute this
software (or portions thereof) for any purpose, without fee, subject to these
conditions:

(1) If any part of the source code for this software is distributed, then this
README file must be included, with this copyright and no-warranty notice
unaltered; and any additions, deletions, or changes to the original files
must be clearly indicated in accompanying documentation.

(2) If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the work of
the Independent JPEG Group".

(3) Permission for use of this software is granted only if the user accepts
full responsibility for any undesirable consequences; the authors accept
NO LIABILITY for damages of any kind.

These conditions apply to any software derived from or based on the IJG code,
not just to the unmodified library. If you use our work, you ought to
acknowledge us.

NOTE: The Independent JPEG Group's name should not be used in advertising or
otherwise to promote products derived from this software without prior written
permission.

---

libwebp

Copyright (c) 2010, Google Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

  * Neither the name of Google nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

---

brotli

Copyright (c) 2009, Google Inc.
All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights  
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
copies of the Software, and to permit persons to whom the Software is  
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in  
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN  
THE SOFTWARE.

---

bzip2

This program, "bzip2", the associated library "libbzip2", and all
documentation, are copyright  1996-2019 Julian R Seward.  
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions, and the following disclaimer.

2. The origin of this software must not be misrepresented; you must
   not claim that you wrote the original software. If you use this
   software in a product, an acknowledgment in the product
   documentation would be appreciated but is not required.

3. Altered source versions must be plainly marked as such, and must
   not be misrepresented as being the original software.

4. The name of the author may not be used to endorse or promote
   products derived from this software without specific prior written
   permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

---

FreeType2

                    The FreeType Project LICENSE
                    ----------------------------

                            2006-Jan-27

                    Copyright 1996-2023
              by David Turner, Robert Wilhelm, and Werner Lemberg


Introduction
============

The FreeType Project is distributed in several archive packages;
some of them may contain, in addition to the FreeType font engine,
various tools and contributions which rely on, or relate to, the
FreeType Project.

This license applies to all files found in such packages, and which
do not fall under their own explicit license.  The license affects
thus the FreeType font engine, the test programs, documentation and
makefiles, at the very least.


Legal Terms
===========

0. Definitions
--------------

Throughout this license, the terms `package', `FreeType Project',
and `FreeType archive' refer to the set of files originally
distributed by the authors (David Turner, Robert Wilhelm, and
Werner Lemberg) as the `FreeType Project', be they named as alpha,
beta or final release.

`You' refers to the licensee, or person using the project, where
`using' is a generic term including compiling the project's source
code as well as linking it to form a `program' or `executable'.
This program is referred to as `a program using the FreeType
engine'.

This license applies to all files distributed in the original
FreeType Project, including all source code, binaries and
documentation, unless otherwise stated in the file itself.


1. No Warranty
--------------

THE FREETYPE PROJECT IS PROVIDED `AS IS' WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


2. Redistribution
-----------------

This license grants a worldwide, royalty-free, perpetual and
irrevocable right and license to use, execute, perform, compile,
display, copy, create derivative works of, distribute and sublicense
the FreeType Project (in both source and object code forms) and
derivative works thereof for any purpose; and to authorize others
to exercise some or all of the rights granted herein, subject to the
following conditions:

  o Redistribution of source code must retain this license file
    (`FTL.TXT') unaltered; any additions, deletions or changes to
    the original files must be clearly indicated in accompanying
    documentation.  The copyright notices of the unaltered, original
    files must be preserved in all copies of source files.

  o Redistribution in binary form must provide a disclaimer that
    states that the software is based in part on the work of the
    FreeType Team, in the distribution documentation.  We also
    encourage you to put an URL to the FreeType web page in your
    documentation, though this isn't mandatory.


3. Advertising
--------------

Neither the FreeType authors and contributors nor you shall use the
name of the other in any publicity or advertising without prior
written consent.

We suggest, but do not require, that you acknowledge the use of
FreeType in your documentation, for example by saying

   "Portions of this software are copyright © 2019 The FreeType
    Project (www.freetype.org).  All rights reserved."

Please replace the year with the value from the copyright notice
found in each file.

Legal Terms -- Final Words
==========================

The FreeType Project is copyright (C) 1996-2023 by David Turner,
Robert Wilhelm, and Werner Lemberg.  All rights reserved.
