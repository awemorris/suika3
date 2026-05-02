API de bajo nivel de rayos
=================

El `Low Level API (Engine.*)` está diseñado para la creación versátil de juegos 2D.

Cada función API toma un parámetro.
El parámetro debe ser un diccionario y los argumentos deben almacenarse como pares clave-valor.

## esqueleto

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

## Depurar

### print()

Esta API imprime una cadena o vuelca un objeto.
Sólo se necesita un argumento que no esté en el diccionario.

```
func dumpEnemies() {
    if (Engine.isGamepadXPressed) {
        print("[Current emenies]");
        print(enemies);
    }
}
```

## Tiempo

### Tiempo absoluto

|Variable |Description |
|----------------------------|-------------------------------------------|
|Engine.millisec |Tiempo en milisegundos.                          |

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

Devuelve un diccionario de fechas.

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

## Aporte

Estas son variables, no funciones.

|Variable |Description |
|--------------------------------|-------------------------------------------|
|Engine.mousePosX |Posición del ratón X. |
|Engine.mousePosY |Posición del ratón Y. |
|Engine.isMouseLeftPressed |Estado del botón izquierdo del ratón.                   |
|Engine.isMouseRightPressed |Estado del botón derecho del ratón.                  |
|Engine.isEscapeKeyPressed |Estado de la clave de escape.                          |
|Engine.isReturnKeyPressed |Regresar estado de clave.                          |
|Engine.isSpaceKeyPressed |Estado de la clave de espacio.                           |
|Engine.isTabKeyPressed |Estado de la tecla Tab.                             |
|Engine.isBackspaceKeyPressed |Estado de la tecla de retroceso.                       |
|Engine.isDeleteKeyPressed |Eliminar estado de clave.                          |
|Engine.isHomeKeyPressed |Estado de la clave de inicio.                            |
|Engine.isEndKeyPressed |Fin del estado de la clave.                             |
|Engine.isPageupKeyPressed |Estado de la clave Re Pág.                          |
|Engine.isPagedownKeyPressed |Estado de la clave PageDown.                        |
|Engine.isShiftKeyPressed |Estado de la tecla Shift.                           |
|Engine.isControlKeyPressed |Estado de la clave de control.                         |
|Engine.isAltKeyPressed |Estado de la tecla Alt.                             |
|Engine.isUpKeyPressed |Estado de la tecla de flecha hacia arriba.                        |
|Engine.isDownKeyPressed |Estado de la tecla de flecha hacia abajo.                      |
|Engine.isRightKeyPressed |Estado de la tecla de flecha derecha.                     |
|Engine.isLeftKeyPressed |Estado de la tecla de flecha izquierda.                      |
|Engine.isAKeyPressed |Estado de clave 'A'.                             |
|Engine.isBKeyPressed |Estado de clave 'B'.                             |
|Engine.isCKeyPressed |Estado de clave 'C'.                             |
|Engine.isDKeyPressed |Estado de clave 'D'.                             |
|Engine.isEKeyPressed |Estado de clave 'E'.                             |
|Engine.isFKeyPressed |Estado de clave 'F'.                             |
|Engine.isGKeyPressed |Estado de clave 'G'.                             |
|Engine.isHKeyPressed |Estado de clave 'H'.                             |
|Engine.isIKeyPressed |Estado clave 'I'.                             |
|Engine.isJKeyPressed |Estado de clave 'J'.                             |
|Engine.isKKeyPressed |Estado de clave 'K'.                             |
|Engine.isLKeyPressed |Estado de clave 'L'.                             |
|Engine.isMKeyPressed |Estado de clave 'M'.                             |
|Engine.isNKeyPressed |Estado de clave 'N'.                             |
|Engine.isOKeyPressed |Estado de clave 'O'.                             |
|Engine.isPKeyPressed |Estado de la clave 'P'.                             |
|Engine.isQKeyPressed |Estado de la clave 'Q'.                             |
|Engine.isRKeyPressed |Estado de la clave 'R'.                             |
|Engine.isSKeyPressed |Estado de clave 'S'.                             |
|Engine.isTKeyPressed |Estado de clave 'T'.                             |
|Engine.isUKeyPressed |Estado de clave 'U'.                             |
|Engine.isVKeyPressed |Estado de clave 'V'.                             |
|Engine.isWKeyPressed |Estado de clave 'W'.                             |
|Engine.isXKeyPressed |Estado de clave 'X'.                             |
|Engine.isYKeyPressed |Estado de clave 'Y'.                             |
|Engine.isZKeyPressed |Estado de clave 'Z'.                             |
|Engine.is1KeyPressed |Estado clave '1'.                             |
|Engine.is2KeyPressed |Estado clave '2'.                             |
|Engine.is3KeyPressed |Estado clave '3'.                             |
|Engine.is4KeyPressed |Estado clave '4'.                             |
|Engine.is5KeyPressed |Estado clave '5'.                             |
|Engine.is6KeyPressed |Estado clave '6'.                             |
|Engine.is7KeyPressed |Estado clave '7'.                             |
|Engine.is8KeyPressed |Estado de clave '8'.                             |
|Engine.is9KeyPressed |Estado clave '9'.                             |
|Engine.is0KeyPressed |Estado de clave '0'.                             |
|Engine.isF1KeyPressed |Estado de la tecla F1.                              |
|Engine.isF2KeyPressed |Estado de la tecla F2.                              |
|Engine.isF3KeyPressed |Estado de la tecla F3.                              |
|Engine.isF4KeyPressed |Estado de la tecla F4.                              |
|Engine.isF5KeyPressed |Estado de la clave F5.                              |
|Engine.isF6KeyPressed |Estado de la clave F6.                              |
|Engine.isF7KeyPressed |Estado de la tecla F7.                              |
|Engine.isF8KeyPressed |Estado de la tecla F8.                              |
|Engine.isF9KeyPressed |Estado de la clave F9.                              |
|Engine.isF10KeyPressed |Estado de la tecla F10.                             |
|Engine.isF11KeyPressed |Estado de la clave F11.                             |
|Engine.isF12KeyPressed |Estado de la clave F12.                             |
|Engine.isGamepadUpPressed |Estado de flecha hacia arriba del gamepad.                    |
|Engine.isGamepadDownPressed |Estado de flecha hacia abajo del gamepad.                  |
|Engine.isGamepadLeftPressed |Estado de la flecha izquierda del gamepad.                  |
|Engine.isGamepadRightPressed |Estado de la flecha derecha del gamepad.                 |
|Engine.isGamepadAPressed |Gamepad Estado del botón A.                    |
|Engine.isGamepadBPressed |Estado del botón B del gamepad.                    |
|Engine.isGamepadXPressed |Estado del botón X del gamepad.                    |
|Engine.isGamepadYPressed |Estado del botón Y del gamepad.                    |
|Engine.isGamepadLPressed |Estado del botón L del gamepad.                    |
|Engine.isGamepadRPressed |Estado del botón R del gamepad.                    |
|Engine.gamepadAnalogX1 |Gamepad analógico 1 X (-32768, 32767) |
|Engine.gamepadAnalogY1 |Gamepad analógico 1 Y (-32768, 32767) |
|Engine.gamepadAnalogX2 |Gamepad analógico 2 X (-32768, 32767) |
|Engine.gamepadAnalogY2 |Gamepad analógico 2 Y (-32768, 32767) |
|Engine.gamepadAnalogL |Gamepad analógico L (-32768, 32767) |
|Engine.gamepadAnalogR |Gamepad analógico R (-32768, 32767) |

```
func update() {
    if (Engine.isMouseLeftPressed) {
        player.x = player.x + 100;
    }
}
```

## Representación

### Engine.createColorTexture()

Esta API crea una textura con el color especificado y devuelve una textura.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|width |Ancho de textura.                                                |
|height |Altura de textura.                                               |
|r |Color rojo.                                                    |
|g |Color verde.                                                  |
|b |Color azul.                                                   |
|a |Color alfa.                                                  |

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

Esta API carga una textura a partir de activos y devuelve una textura.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|file |Nombre del archivo a cargar.                                            |

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

Esta API destruye una textura.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|texture |Texture. |

```
func destroyPlayerTexture() {
    Engine.loadTexture({
        texture: playerTex
    });
}
```

### Engine.renderTexture()

Esta API representa una textura en la pantalla.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|dstLeft |Coordenada de pantalla X. |
|dstTop |Coordenada de pantalla Y. |
|dstWidth |Ancho en pantalla.                                              |
|dstHeight |Altura en pantalla.                                             |
|texture |Texture. |
|srcLeft |Textura arriba izquierda X. |
|srcTop |Textura arriba izquierda Y. |
|srcWidth |Ancho del rectángulo de textura.                                      |
|srcHeight |Altura del rectángulo de textura.                                     |
|alpha |Valor alfa (0-255) |

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

Esta API representa una textura en la pantalla (una versión más simple de `Engine.renderTexture()`).

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|texture |Texture. |
|x |Coordenada de pantalla X. |
|y |Coordenada de pantalla Y. |

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

Esta API representa una textura en la pantalla.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|x1 |Coordenada de pantalla X1.                                         |
|y1 |Coordenada de pantalla Y1.                                         |
|x2 |Coordenada de pantalla X2.                                         |
|y2 |Coordenada de pantalla Y2.                                         |
|x3 |Coordenada de pantalla X3.                                         |
|y3 |Coordenada de pantalla Y3.                                         |
|x4 |Coordenada de pantalla X4.                                         |
|y4 |Coordenada de pantalla Y4.                                         |
|texture |Texture. |
|srcLeft |Textura arriba izquierda X. |
|srcTop |Textura arriba izquierda Y. |
|srcWidth |Ancho del rectángulo de textura.                                      |
|srcHeight |Altura del rectángulo de textura.                                     |
|alpha |Valor alfa (0-255) |

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

Esta API carga un archivo de fuente de recurso en una ranura de fuente especificada.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|slot |Índice de ranura de fuente. (0-3) |
|file |Nombre del archivo a cargar.                                            |

```
func loadNotoSansFont() {
    Engine.loadFont({ slot: 0, file: "NotoSans.ttf" });
}
```

### Engine.createTextTexture()

Esta API crea una textura y dibuja un texto sobre ella.
     	 	 
|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|slot |Índice de ranura de fuente. (0-3) |
|text |Texto a dibujar.                                                 |
|size |Tamaño de fuente.                                                    |
|r |Color rojo.                                                    |
|g |Color verde.                                                  |
|b |Color azul.                                                   |
|a |Color alfa.                                                  |

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

## Sonido

### Engine.playSound()

Esta API comienza a reproducir un archivo de recursos de sonido en una pista de sonido específica.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|stream |Índice de pistas. (0-3) |
|file |Archivo para reproducir.                                                 |

```
func playJumpSound() {
    Engine.playSound({ stream: 0, file: "jump.ogg" });
}
```

### Engine.playSoundLoop()

Esta API comienza a reproducir un archivo de recursos de sonido en una pista de sonido específica.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|stream |Índice de pistas. (0-3) |
|file |Archivo para reproducir.                                                 |

```
func playBGM() {
    Engine.playSoundLoop({ stream: 0, file: "jump.ogg" });
}
```

### Engine.stopSound()

Esta API detiene la reproducción de sonido en una pista de sonido específica.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|stream |Índice de pistas. (0-3) |

```
func playJumpSound() {
    Engine.stopSound({ stream: 0 });
}
```

### Engine.setSoundVolume()

Esta API establece un volumen de sonido en una pista de sonido específica.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|stream |Índice de pistas. (0-3, -1 para maestro) |
|volume |Valor de volumen. (0-1,0) |

```
func playJumpSound() {
    Engine.setSoundVolume({
        stream: 0,
        volume: 1.0
    });
}
```

## Guardar datos

### Engine.writeSaveData()

Esta API escribe un valor de datos guardados que corresponde a una cadena de clave.
Si el valor de los datos guardados es demasiado grande, esta API fallará.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|key |Cadena de claves.                                                   |
|value |Valor. (entero, coma flotante, matriz o diccionario) |

### Engine.readSaveData()

Esta API lee el valor de los datos guardados que corresponde a una cadena de clave.
El valor de retorno será un objeto que representa un valor de datos guardados.
Esta API fallará cuando la clave especificada no esté disponible.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|key |Cadena de claves.                                                   |

### Engine.checkSaveData()

Esta API comprueba si los datos guardados existen para una cadena de clave o no.
El valor de retorno es booleano.

|Nombre del argumento |Description |
|--------------------|--------------------------------------------------------------|
|key |Cadena de claves.                                                   |
