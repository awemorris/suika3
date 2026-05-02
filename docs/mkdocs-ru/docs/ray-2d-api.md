Низкоуровневый API Ray
=================

`Low Level API (Engine.*)` предназначен для гибкого создания 2D-игр.

Каждая функция API принимает один параметр.
Параметр должен быть словарем, а аргументы должны храниться в виде пар «ключ-значение».

## Скелет

```
// Не определяйте переменные вне функций, потому что это синтаксическая ошибка.

// Вызывается при создании окна.
func setup() {
    // Возвращает конфигурацию окна.
    return {
        width: 1280,
        height: 720,
        title: "My First Game"
    };
}

// Вызывается один раз при запуске игры.
func start() {
    // Здесь следует определять глобальные переменные.
    posX = 0;
    posY = 0;

    // Создает белую текстуру 100x100.
    tex = Engine.createColorTexture({
        width: 100,
        height: 100,
        r: 255, g: 255, b: 255, a: 255
    });
}

// Вызывается каждый кадр перед отрисовкой.
func update() {
    posX = Engine.mousePosX;
    posY = Engine.mousePosY;
}

// Вызывается каждый кадр для отрисовки графики.
func render() {
    Engine.draw({ texture: tex, x: posX, y: posY });
}
```

## Отладка

### print()

Этот API выводит строку или дамп объекта.
Принимает только несловарный аргумент.

```
func dumpEnemies() {
    if (Engine.isGamepadXPressed) {
        print("[Текущие враги]");
        print(enemies);
    }
}
```

## Время

### Абсолютное время

|Переменная                 |Описание                                   |
|---------------------------|-------------------------------------------|
|Engine.millisec            |Время в миллисекундах.                     |

```
func update() {
    // Получает секунды, прошедшие с прошлого вызова.
    var curTime = Engine.millisec;
    var dt = (curTime - lastTime) * 0.001;
    lastTime = curTime;

    updateCharacters(dt);
}
```

### Engine.getDate()

Возвращает словарь даты.

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

## Ввод

Это переменные, а не функции.

|Переменная                     |Описание                                   |
|------------------------------|-------------------------------------------|
|Engine.mousePosX              |Положение мыши по X.                       |
|Engine.mousePosY              |Положение мыши по Y.                       |
|Engine.isMouseLeftPressed     |Состояние левой кнопки мыши.               |
|Engine.isMouseRightPressed    |Состояние правой кнопки мыши.              |
|Engine.isEscapeKeyPressed     |Состояние клавиши Escape.                  |
|Engine.isReturnKeyPressed     |Состояние клавиши Return.                  |
|Engine.isSpaceKeyPressed      |Состояние клавиши Space.                   |
|Engine.isTabKeyPressed        |Состояние клавиши Tab.                     |
|Engine.isBackspaceKeyPressed  |Состояние клавиши Backspace.               |
|Engine.isDeleteKeyPressed     |Состояние клавиши Delete.                  |
|Engine.isHomeKeyPressed       |Состояние клавиши Home.                    |
|Engine.isEndKeyPressed        |Состояние клавиши End.                     |
|Engine.isPageupKeyPressed     |Состояние клавиши PageUp.                  |
|Engine.isPagedownKeyPressed   |Состояние клавиши PageDown.                |
|Engine.isShiftKeyPressed      |Состояние клавиши Shift.                   |
|Engine.isControlKeyPressed    |Состояние клавиши Control.                 |
|Engine.isAltKeyPressed        |Состояние клавиши Alt.                     |
|Engine.isUpKeyPressed         |Состояние стрелки вверх.                   |
|Engine.isDownKeyPressed       |Состояние стрелки вниз.                    |
|Engine.isRightKeyPressed      |Состояние стрелки вправо.                  |
|Engine.isLeftKeyPressed       |Состояние стрелки влево.                   |
|Engine.isAKeyPressed          |Состояние клавиши 'A'.                     |
|Engine.isBKeyPressed          |Состояние клавиши 'B'.                     |
|Engine.isCKeyPressed          |Состояние клавиши 'C'.                     |
|Engine.isDKeyPressed          |Состояние клавиши 'D'.                     |
|Engine.isEKeyPressed          |Состояние клавиши 'E'.                     |
|Engine.isFKeyPressed          |Состояние клавиши 'F'.                     |
|Engine.isGKeyPressed          |Состояние клавиши 'G'.                     |
|Engine.isHKeyPressed          |Состояние клавиши 'H'.                     |
|Engine.isIKeyPressed          |Состояние клавиши 'I'.                     |
|Engine.isJKeyPressed          |Состояние клавиши 'J'.                     |
|Engine.isKKeyPressed          |Состояние клавиши 'K'.                     |
|Engine.isLKeyPressed          |Состояние клавиши 'L'.                     |
|Engine.isMKeyPressed          |Состояние клавиши 'M'.                     |
|Engine.isNKeyPressed          |Состояние клавиши 'N'.                     |
|Engine.isOKeyPressed          |Состояние клавиши 'O'.                     |
|Engine.isPKeyPressed          |Состояние клавиши 'P'.                     |
|Engine.isQKeyPressed          |Состояние клавиши 'Q'.                     |
|Engine.isRKeyPressed          |Состояние клавиши 'R'.                     |
|Engine.isSKeyPressed          |Состояние клавиши 'S'.                     |
|Engine.isTKeyPressed          |Состояние клавиши 'T'.                     |
|Engine.isUKeyPressed          |Состояние клавиши 'U'.                     |
|Engine.isVKeyPressed          |Состояние клавиши 'V'.                     |
|Engine.isWKeyPressed          |Состояние клавиши 'W'.                     |
|Engine.isXKeyPressed          |Состояние клавиши 'X'.                     |
|Engine.isYKeyPressed          |Состояние клавиши 'Y'.                     |
|Engine.isZKeyPressed          |Состояние клавиши 'Z'.                     |
|Engine.is1KeyPressed          |Состояние клавиши '1'.                     |
|Engine.is2KeyPressed          |Состояние клавиши '2'.                     |
|Engine.is3KeyPressed          |Состояние клавиши '3'.                     |
|Engine.is4KeyPressed          |Состояние клавиши '4'.                     |
|Engine.is5KeyPressed          |Состояние клавиши '5'.                     |
|Engine.is6KeyPressed          |Состояние клавиши '6'.                     |
|Engine.is7KeyPressed          |Состояние клавиши '7'.                     |
|Engine.is8KeyPressed          |Состояние клавиши '8'.                     |
|Engine.is9KeyPressed          |Состояние клавиши '9'.                     |
|Engine.is0KeyPressed          |Состояние клавиши '0'.                     |
|Engine.isF1KeyPressed         |Состояние клавиши F1.                      |
|Engine.isF2KeyPressed         |Состояние клавиши F2.                      |
|Engine.isF3KeyPressed         |Состояние клавиши F3.                      |
|Engine.isF4KeyPressed         |Состояние клавиши F4.                      |
|Engine.isF5KeyPressed         |Состояние клавиши F5.                      |
|Engine.isF6KeyPressed         |Состояние клавиши F6.                      |
|Engine.isF7KeyPressed         |Состояние клавиши F7.                      |
|Engine.isF8KeyPressed         |Состояние клавиши F8.                      |
|Engine.isF9KeyPressed         |Состояние клавиши F9.                      |
|Engine.isF10KeyPressed        |Состояние клавиши F10.                     |
|Engine.isF11KeyPressed        |Состояние клавиши F11.                     |
|Engine.isF12KeyPressed        |Состояние клавиши F12.                     |
|Engine.isGamepadUpPressed     |Состояние стрелки вверх на геймпаде.       |
|Engine.isGamepadDownPressed   |Состояние стрелки вниз на геймпаде.        |
|Engine.isGamepadLeftPressed   |Состояние стрелки влево на геймпаде.       |
|Engine.isGamepadRightPressed  |Состояние стрелки вправо на геймпаде.      |
|Engine.isGamepadAPressed      |Состояние кнопки A на геймпаде.            |
|Engine.isGamepadBPressed      |Состояние кнопки B на геймпаде.            |
|Engine.isGamepadXPressed      |Состояние кнопки X на геймпаде.            |
|Engine.isGamepadYPressed      |Состояние кнопки Y на геймпаде.            |
|Engine.isGamepadLPressed      |Состояние кнопки L на геймпаде.            |
|Engine.isGamepadRPressed      |Состояние кнопки R на геймпаде.            |
|Engine.gamepadAnalogX1        |Аналоговый вход геймпада 1 по X (-32768, 32767)|
|Engine.gamepadAnalogY1        |Аналоговый вход геймпада 1 по Y (-32768, 32767)|
|Engine.gamepadAnalogX2        |Аналоговый вход геймпада 2 по X (-32768, 32767)|
|Engine.gamepadAnalogY2        |Аналоговый вход геймпада 2 по Y (-32768, 32767)|
|Engine.gamepadAnalogL         |Аналоговый вход L на геймпаде (-32768, 32767)|
|Engine.gamepadAnalogR         |Аналоговый вход R на геймпаде (-32768, 32767)|

```
func update() {
    if (Engine.isMouseLeftPressed) {
        player.x = player.x + 100;
    }
}
```

## Отрисовка

### Engine.createColorTexture()

Этот API создает текстуру с указанным цветом и возвращает текстуру.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|width               |Ширина текстуры.                                              |
|height              |Высота текстуры.                                              |
|r                   |Красный компонент.                                            |
|g                   |Зеленый компонент.                                            |
|b                   |Синий компонент.                                              |
|a                   |Альфа-компонент.                                              |

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

Этот API загружает текстуру из папки `assets` и возвращает текстуру.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|file                |Имя файла для загрузки.                                       |

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

Этот API уничтожает текстуру.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|texture             |Текстура.                                                     |

```
func destroyPlayerTexture() {
    Engine.loadTexture({
        texture: playerTex
    });
}
```

### Engine.renderTexture()

Этот API отрисовывает текстуру на экран.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|dstLeft             |Координата X на экране.                                       |
|dstTop              |Координата Y на экране.                                       |
|dstWidth            |Ширина на экране.                                             |
|dstHeight           |Высота на экране.                                             |
|texture             |Текстура.                                                     |
|srcLeft             |X левого верхнего угла текстуры.                              |
|srcTop              |Y левого верхнего угла текстуры.                              |
|srcWidth            |Ширина прямоугольника текстуры.                               |
|srcHeight           |Высота прямоугольника текстуры.                               |
|alpha               |Значение альфы (0-255).                                       |

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

Этот API отрисовывает текстуру на экран (упрощенная версия `Engine.renderTexture()`.)

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|texture             |Текстура.                                                     |
|x                   |Координата X на экране.                                       |
|y                   |Координата Y на экране.                                       |

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

Этот API отрисовывает текстуру на экран.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|x1                  |Координата X1 на экране.                                      |
|y1                  |Координата Y1 на экране.                                      |
|x2                  |Координата X2 на экране.                                      |
|y2                  |Координата Y2 на экране.                                      |
|x3                  |Координата X3 на экране.                                      |
|y3                  |Координата Y3 на экране.                                      |
|x4                  |Координата X4 на экране.                                      |
|y4                  |Координата Y4 на экране.                                      |
|texture             |Текстура.                                                     |
|srcLeft             |X левого верхнего угла текстуры.                              |
|srcTop              |Y левого верхнего угла текстуры.                              |
|srcWidth            |Ширина прямоугольника текстуры.                               |
|srcHeight           |Высота прямоугольника текстуры.                               |
|alpha               |Значение альфы (0-255).                                       |

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

Этот API загружает файл шрифта из папки `assets` в указанный слот шрифта.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|slot                |Индекс слота шрифта. (0-3)                                    |
|file                |Имя файла для загрузки.                                       |

```
func loadNotoSansFont() {
    Engine.loadFont({ slot: 0, file: "NotoSans.ttf" });
}
```

### Engine.createTextTexture()

Этот API создает текстуру и рисует на ней текст.
     	 	 
|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|slot                |Индекс слота шрифта. (0-3)                                    |
|text                |Текст для рисования.                                          |
|size                |Размер шрифта.                                                |
|r                   |Красный компонент.                                            |
|g                   |Зеленый компонент.                                            |
|b                   |Синий компонент.                                              |
|a                   |Альфа-компонент.                                              |

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

## Звук

### Engine.playSound()

Этот API начинает воспроизведение звукового файла из папки `assets` на указанной звуковой дорожке.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|stream              |Индекс дорожки. (0-3)                                         |
|file                |Файл для воспроизведения.                                     |

```
func playJumpSound() {
    Engine.playSound({ stream: 0, file: "jump.ogg" });
}
```

### Engine.playSoundLoop()

Этот API начинает воспроизведение звукового файла из папки `assets` на указанной звуковой дорожке.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|stream              |Индекс дорожки. (0-3)                                         |
|file                |Файл для воспроизведения.                                     |

```
func playBGM() {
    Engine.playSoundLoop({ stream: 0, file: "jump.ogg" });
}
```

### Engine.stopSound()

Этот API останавливает воспроизведение звука на указанной звуковой дорожке.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|stream              |Индекс дорожки. (0-3)                                         |

```
func playJumpSound() {
    Engine.stopSound({ stream: 0 });
}
```

### Engine.setSoundVolume()

Этот API задает громкость звука на указанной звуковой дорожке.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|stream              |Индекс дорожки. (0-3, -1 для master)                          |
|volume              |Значение громкости. (0-1.0)                                   |

```
func playJumpSound() {
    Engine.setSoundVolume({
        stream: 0,
        volume: 1.0
    });
}
```

## Данные сохранения

### Engine.writeSaveData()

Этот API записывает значение сохранения данных, соответствующее строковому ключу.
Если значение сохранения слишком велико, этот API завершится с ошибкой.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|key                 |Строка ключа.                                                 |
|value               |Значение. (целое число, число с плавающей точкой, массив или словарь) |

### Engine.readSaveData()

Этот API читает значение сохранения данных, соответствующее строковому ключу.
Возвращаемое значение будет объектом, представляющим значение сохранения данных.
Этот API завершится с ошибкой, если указанный ключ недоступен.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|key                 |Строка ключа.                                                 |

### Engine.checkSaveData()

Этот API проверяет, существует ли сохранение данных для строкового ключа.
Возвращаемое значение - логическое.

|Имя аргумента       |Описание                                                      |
|--------------------|--------------------------------------------------------------|
|key                 |Строка ключа.                                                 |
