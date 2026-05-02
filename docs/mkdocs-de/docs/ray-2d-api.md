Ray Low Level API
=================

Der `Low Level API (Engine.*)` ist für die vielseitige Erstellung von 2D-Spielen konzipiert.

Jede API-Funktion benötigt einen Parameter. Der Parameter muss ein Wörterbuch sein und Argumente müssen als Schlüssel-Wert-Paare gespeichert werden.

## Skelton

```
// Definieren Sie keine Variablen außerhalb von Funktionen, da es sich um einen Syntaxfehler handelt.

// Wird aufgerufen, wenn das Fenster erstellt wird.
func setup() {
    // Gibt die Fensterkonfiguration zurück.
    return {
        width: 1280,
        height: 720,
        title: "My First Game"
    };
}

// Wird einmal aufgerufen, wenn das Spiel beginnt.
func start() {
    // Hier sollten globale Variablen definiert werden.
    posX = 0;
    posY = 0;

    // Erstellen Sie eine weiße 100x100-Textur.
    tex = Engine.createColorTexture({
        width: 100,
        height: 100,
        r: 255, g: 255, b: 255, a: 255
    });
}

// Jeder Frame wird vor dem Rendern aufgerufen.
func update() {
    posX = Engine.mousePosX;
    posY = Engine.mousePosY;
}

// Jeder Frame wird aufgerufen, um Grafiken zu rendern.
func render() {
    Engine.draw({ texture: tex, x: posX, y: posY });
}
```

## Debuggen

### drucken()

Diese API gibt eine Zeichenfolge aus oder gibt ein Objekt aus. Akzeptiert nur ein Nicht-Wörterbuch-Argument.

```
func dumpEnemies() {
    if (Engine.isGamepadXPressed) {
        print("[Current emenies]");
        print(enemies);
    }
}
```

## Zeit

### Absolute Zeit

| Variable | Beschreibung |
|----------------------------|-------------------------------------------|
| Engine.millisec | Zeit in Millisekunden. |

```
func update() {
    // Rufen Sie die Sekunden seit dem letzten Anruf ab.
    var curTime = Engine.millisec;
    var dt = (curTime - lastTime) * 0.001;
    lastTime = curTime;

    updateCharacters(dt);
}
```

### Engine.getDate()

Gibt ein Datumswörterbuch zurück.

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

## Eingang

Dies sind Variablen, keine Funktionen.

| Variable | Beschreibung |
|--------------------------------|-------------------------------------------|
| Engine.mousePosX | Mausposition X. |
| Engine.mousePosY | Mausposition Y. |
| Engine.isMouseLeftPressed | Zustand der linken Maustaste. |
| Engine.isMouseRightPressed | Zustand der rechten Maustaste. |
| Engine.isEscapeKeyPressed | Zustand der Escape-Taste. |
| Engine.isReturnKeyPressed | Gibt den Schlüsselstatus zurück. |
| Engine.isSpaceKeyPressed | Status der Leertaste. |
| Engine.isTabKeyPressed | Status der Tabulatortaste. |
| Engine.isBackspaceKeyPressed | Status der Rücktaste. |
| Engine.isDeleteKeyPressed | Schlüsselstatus löschen. |
| Engine.isHomeKeyPressed | Zustand des Home-Schlüssels. |
| Engine.isEndKeyPressed | Schlüsselzustand beenden. |
| Engine.isPageupKeyPressed | Zustand des PageUp-Schlüssels. |
| Engine.isPagedownKeyPressed | Status des PageDown-Schlüssels. |
| Engine.isShiftKeyPressed | Status der Umschalttaste. |
| Engine.isControlKeyPressed | Status des Steuerschlüssels. |
| Engine.isAltKeyPressed | Status der Alt-Taste. |
| Engine.isUpKeyPressed | Status der Aufwärtspfeiltaste. |
| Engine.isDownKeyPressed | Status der Abwärtspfeiltaste. |
| Engine.isRightKeyPressed | Status der rechten Pfeiltaste. |
| Engine.isLeftKeyPressed | Zustand der linken Pfeiltaste. |
| Engine.isAKeyPressed | Schlüsselzustand „A“. |
| Engine.isBKeyPressed | Schlüsselzustand „B“. |
| Engine.isCKeyPressed | Zustand der C-Taste. |
| Engine.isDKeyPressed | 'D'-Schlüsselzustand. |
| Engine.isEKeyPressed | 'E'-Schlüsselzustand. |
| Engine.isFKeyPressed | „F“-Tastenzustand. |
| Engine.isGKeyPressed | 'G'-Schlüsselzustand. |
| Engine.isHKeyPressed | 'H'-Schlüsselzustand. |
| Engine.isIKeyPressed | Schlüsselzustand „I“. |
| Engine.isJKeyPressed | 'J'-Schlüsselzustand. |
| Engine.isKKeyPressed | Schlüsselzustand „K“. |
| Engine.isLKeyPressed | 'L'-Schlüsselzustand. |
| Engine.isMKeyPressed | 'M'-Schlüsselzustand. |
| Engine.isNKeyPressed | 'N'-Schlüsselzustand. |
| Engine.isOKeyPressed | „O“-Schlüsselzustand. |
| Engine.isPKeyPressed | 'P'-Tastenzustand. |
| Engine.isQKeyPressed | Status der „Q“-Taste. |
| Engine.isRKeyPressed | 'R'-Tastenzustand. |
| Engine.isSKeyPressed | 'S'-Schlüsselzustand. |
| Engine.isTKeyPressed | 'T'-Schlüsselzustand. |
| Engine.isUKeyPressed | 'U'-Schlüsselzustand. |
| Engine.isVKeyPressed | 'V'-Schlüsselzustand. |
| Engine.isWKeyPressed | 'W'-Schlüsselzustand. |
| Engine.isXKeyPressed | 'X'-Schlüsselzustand. |
| Engine.isYKeyPressed | 'Y'-Schlüsselzustand. |
| Engine.isZKeyPressed | Schlüsselzustand „Z“. |
| Engine.is1KeyPressed | Schlüsselzustand „1“. |
| Engine.is2KeyPressed | Schlüsselzustand „2“. |
| Engine.is3KeyPressed | Schlüsselzustand „3“. |
| Engine.is4KeyPressed | Schlüsselzustand „4“. |
| Engine.is5KeyPressed | Schlüsselzustand „5“. |
| Engine.is6KeyPressed | Schlüsselzustand „6“. |
| Engine.is7KeyPressed | Schlüsselzustand „7“. |
| Engine.is8KeyPressed | Schlüsselzustand „8“. |
| Engine.is9KeyPressed | Schlüsselzustand „9“. |
| Engine.is0KeyPressed | Schlüsselstatus „0“. |
| Engine.isF1KeyPressed | Status der F1-Taste. |
| Engine.isF2KeyPressed | Status der F2-Taste. |
| Engine.isF3KeyPressed | Status der F3-Taste. |
| Engine.isF4KeyPressed | Status der F4-Taste. |
| Engine.isF5KeyPressed | Status der F5-Taste. |
| Engine.isF6KeyPressed | Status der F6-Taste. |
| Engine.isF7KeyPressed | Status der F7-Taste. |
| Engine.isF8KeyPressed | Status der F8-Taste. |
| Engine.isF9KeyPressed | Status der F9-Taste. |
| Engine.isF10KeyPressed | Status der F10-Taste. |
| Engine.isF11KeyPressed | Status der F11-Taste. |
| Engine.isF12KeyPressed | Status der F12-Taste. |
| Engine.isGamepadUpPressed | Status des Aufwärtspfeils des Gamepads. |
| Engine.isGamepadDownPressed | Status des Gamepad-Pfeils nach unten. |
| Engine.isGamepadLeftPressed | Status des linken Pfeils des Gamepads. |
| Engine.isGamepadRightPressed | Status des rechten Pfeils des Gamepads. |
| Engine.isGamepadAPressed | Zustand der Gamepad-A-Taste. |
| Engine.isGamepadBPressed | Zustand der Gamepad-B-Taste. |
| Engine.isGamepadXPressed | Status der Gamepad X-Taste. |
| Engine.isGamepadYPressed | Status der Y-Taste des Gamepads. |
| Engine.isGamepadLPressed | Status der L-Taste des Gamepads. |
| Engine.isGamepadRPressed | Status der R-Taste des Gamepads. |
| Engine.gamepadAnalogX1 | Gamepad analog 1 X (-32768, 32767) |
| Engine.gamepadAnalogY1 | Gamepad analog 1 J (-32768, 32767) |
| Engine.gamepadAnalogX2 | Gamepad analog 2 X (-32768, 32767) |
| Engine.gamepadAnalogY2 | Gamepad analog 2 Y (-32768, 32767) |
| Engine.gamepadAnalogL | Gamepad analog L (-32768, 32767) |
| Engine.gamepadAnalogR | Gamepad analog R (-32768, 32767) |

```
func update() {
    if (Engine.isMouseLeftPressed) {
        player.x = player.x + 100;
    }
}
```

## Rendern

### Engine.createColorTexture()

Diese API erstellt eine Textur mit der angegebenen Farbe und gibt eine Textur zurück.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Breite | Texturbreite. |
| Höhe | Texturhöhe. |
| R | Rote Farbe. |
| G | Grüne Farbe. |
| B | Blaue Farbe. |
| A | Alpha-Farbe. |

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

Diese API lädt eine Textur aus Assets und gibt eine Textur zurück.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Datei | Zu ladender Dateiname. |

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

Diese API zerstört eine Textur.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Textur | Textur. |

```
func destroyPlayerTexture() {
    Engine.loadTexture({
        texture: playerTex
    });
}
```

### Engine.renderTexture()

Diese API rendert eine Textur auf dem Bildschirm.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| dstLeft | Bildschirmkoordinate X. |
| dstTop | Bildschirmkoordinate Y. |
| dstWidth | Breite im Bildschirm. |
| dstHeight | Höhe im Bildschirm. |
| Textur | Textur. |
| srcLeft | Textur oben links X. |
| srcTop | Textur oben links Y. |
| srcWidth | Breite des Texturrechtecks. |
| srcHeight | Höhe des Texturrechtecks. |
| Alpha | Alphawert (0-255) |

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

Diese API rendert eine Textur auf dem Bildschirm (eine einfachere Version von `__TECH0__`).

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Textur | Textur. |
| X | Bildschirmkoordinate X. |
| j | Bildschirmkoordinate Y. |

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

Diese API rendert eine Textur auf dem Bildschirm.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| x1 | Bildschirmkoordinate X1. |
| y1 | Bildschirmkoordinate Y1. |
| x2 | Bildschirmkoordinate X2. |
| y2 | Bildschirmkoordinate Y2. |
| x3 | Bildschirmkoordinate X3. |
| y3 | Bildschirmkoordinate Y3. |
| x4 | Bildschirmkoordinate X4. |
| y4 | Bildschirmkoordinate Y4. |
| Textur | Textur. |
| srcLeft | Textur oben links X. |
| srcTop | Textur oben links Y. |
| srcWidth | Breite des Texturrechtecks. |
| srcHeight | Höhe des Texturrechtecks. |
| Alpha | Alphawert (0-255) |

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

Diese API lädt eine Asset-Schriftartdatei in einen angegebenen Schriftarten-Slot.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Slot | Schriftarten-Slot-Index. (0-3) |
| Datei | Zu ladender Dateiname. |

```
func loadNotoSansFont() {
    Engine.loadFont({ slot: 0, file: "NotoSans.ttf" });
}
```

### Engine.createTextTexture()

Diese API erstellt eine Textur und zeichnet einen Text darauf.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Slot | Schriftarten-Slot-Index. (0-3) |
| Text | Text zum Zeichnen. |
| Größe | Schriftgröße. |
| R | Rote Farbe. |
| G | Grüne Farbe. |
| B | Blaue Farbe. |
| A | Alpha-Farbe. |

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

## Klang

### Engine.playSound()

Diese API startet die Wiedergabe einer Sound-Asset-Datei auf einer angegebenen Soundspur.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Strom | Titelindex. (0-3) |
| Datei | Datei zum Abspielen. |

```
func playJumpSound() {
    Engine.playSound({ stream: 0, file: "jump.ogg" });
}
```

### Engine.playSoundLoop()

Diese API startet die Wiedergabe einer Sound-Asset-Datei auf einer angegebenen Soundspur.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Strom | Titelindex. (0-3) |
| Datei | Datei zum Abspielen. |

```
func playBGM() {
    Engine.playSoundLoop({ stream: 0, file: "jump.ogg" });
}
```

### Engine.stopSound()

Diese API stoppt die Tonwiedergabe auf einer bestimmten Tonspur.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Strom | Titelindex. (0-3) |

```
func playJumpSound() {
    Engine.stopSound({ stream: 0 });
}
```

### Engine.setSoundVolume()

Diese API legt die Lautstärke für eine bestimmte Tonspur fest.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Strom | Titelindex. (0-3, -1 für Master) |
| Volumen | Lautstärkewert. (0-1.0) |

```
func playJumpSound() {
    Engine.setSoundVolume({
        stream: 0,
        volume: 1.0
    });
}
```

## Daten speichern

### Engine.writeSaveData()

Diese API schreibt einen Speicherdatenwert, der einer Schlüsselzeichenfolge entspricht. Wenn der Speicherdatenwert zu groß ist, schlägt diese API fehl.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Schlüssel | Schlüsselzeichenfolge. |
| Wert | Wert. (Ganzzahl, Gleitkomma, Array oder Wörterbuch) |

### Engine.readSaveData()

Diese API liest den Speicherdatenwert, der einer Schlüsselzeichenfolge entspricht. Der Rückgabewert ist ein Objekt, das einen gespeicherten Datenwert darstellt. Diese API schlägt fehl, wenn der angegebene Schlüssel nicht verfügbar ist.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Schlüssel | Schlüsselzeichenfolge. |

### Engine.checkSaveData()

Diese API prüft, ob die Speicherdaten für eine Schlüsselzeichenfolge vorhanden sind oder nicht. Der Rückgabewert ist ein boolescher Wert.

| Argumentname | Beschreibung |
|--------------------|--------------------------------------------------------------|
| Schlüssel | Schlüsselzeichenfolge. |
