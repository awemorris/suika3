Ray 低レベル API
=================

`Low Level API (Engine.*)` は、汎用的な 2D ゲーム制作向けに設計されています。

すべての API 関数は 1 つの引数を取ります。
引数は辞書でなければならず、各引数値はキーと値のペアとして格納する必要があります。

## 骨組み

```
// 関数の外で変数を定義しないでください。構文エラーになります。

// ウィンドウが作成されたときに呼び出されます。
func setup() {
    // ウィンドウ設定を返します。
    return {
        width: 1280,
        height: 720,
        title: "My First Game"
    };
}

// ゲーム開始時に一度だけ呼び出されます。
func start() {
    // グローバル変数はここで定義してください。
    posX = 0;
    posY = 0;

    // 白い 100x100 のテクスチャを作成します。
    tex = Engine.createColorTexture({
        width: 100,
        height: 100,
        r: 255, g: 255, b: 255, a: 255
    });
}

// 描画前に毎フレーム呼び出されます。
func update() {
    posX = Engine.mousePosX;
    posY = Engine.mousePosY;
}

// グラフィックを描画するために毎フレーム呼び出されます。
func render() {
    Engine.draw({ texture: tex, x: posX, y: posY });
}
```

## デバッグ

### print()

この API は文字列を表示するか、オブジェクトをダンプします。
辞書ではない引数だけを取ります。

```
func dumpEnemies() {
    if (Engine.isGamepadXPressed) {
        print("[現在の敵]");
        print(enemies);
    }
}
```

## 時間

### 絶対時間

|変数                        |説明                                       |
|----------------------------|-------------------------------------------|
|Engine.millisec             |ミリ秒単位の時刻。                        |

```
func update() {
    // 前回の呼び出しからの経過秒数を取得します。
    var curTime = Engine.millisec;
    var dt = (curTime - lastTime) * 0.001;
    lastTime = curTime;

    updateCharacters(dt);
}
```

### Engine.getDate()

日付の辞書を返します。

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

## 入力

これらは関数ではなく変数です。

|変数                            |説明                                       |
|--------------------------------|-------------------------------------------|
|Engine.mousePosX                |マウス位置 X。                            |
|Engine.mousePosY                |マウス位置 Y。                            |
|Engine.isMouseLeftPressed       |マウス左ボタンの状態。                    |
|Engine.isMouseRightPressed      |マウス右ボタンの状態。                    |
|Engine.isEscapeKeyPressed       |Escape キーの状態。                       |
|Engine.isReturnKeyPressed       |Return キーの状態。                       |
|Engine.isSpaceKeyPressed        |Space キーの状態。                        |
|Engine.isTabKeyPressed          |Tab キーの状態。                          |
|Engine.isBackspaceKeyPressed    |Backspace キーの状態。                    |
|Engine.isDeleteKeyPressed       |Delete キーの状態。                       |
|Engine.isHomeKeyPressed         |Home キーの状態。                         |
|Engine.isEndKeyPressed          |End キーの状態。                          |
|Engine.isPageupKeyPressed       |PageUp キーの状態。                       |
|Engine.isPagedownKeyPressed     |PageDown キーの状態。                     |
|Engine.isShiftKeyPressed        |Shift キーの状態。                        |
|Engine.isControlKeyPressed      |Control キーの状態。                      |
|Engine.isAltKeyPressed          |Alt キーの状態。                          |
|Engine.isUpKeyPressed           |上矢印キーの状態。                        |
|Engine.isDownKeyPressed         |下矢印キーの状態。                        |
|Engine.isRightKeyPressed        |右矢印キーの状態。                        |
|Engine.isLeftKeyPressed         |左矢印キーの状態。                        |
|Engine.isAKeyPressed            |'A' キーの状態。                          |
|Engine.isBKeyPressed            |'B' キーの状態。                          |
|Engine.isCKeyPressed            |'C' キーの状態。                          |
|Engine.isDKeyPressed            |'D' キーの状態。                          |
|Engine.isEKeyPressed            |'E' キーの状態。                          |
|Engine.isFKeyPressed            |'F' キーの状態。                          |
|Engine.isGKeyPressed            |'G' キーの状態。                          |
|Engine.isHKeyPressed            |'H' キーの状態。                          |
|Engine.isIKeyPressed            |'I' キーの状態。                          |
|Engine.isJKeyPressed            |'J' キーの状態。                          |
|Engine.isKKeyPressed            |'K' キーの状態。                          |
|Engine.isLKeyPressed            |'L' キーの状態。                          |
|Engine.isMKeyPressed            |'M' キーの状態。                          |
|Engine.isNKeyPressed            |'N' キーの状態。                          |
|Engine.isOKeyPressed            |'O' キーの状態。                          |
|Engine.isPKeyPressed            |'P' キーの状態。                          |
|Engine.isQKeyPressed            |'Q' キーの状態。                          |
|Engine.isRKeyPressed            |'R' キーの状態。                          |
|Engine.isSKeyPressed            |'S' キーの状態。                          |
|Engine.isTKeyPressed            |'T' キーの状態。                          |
|Engine.isUKeyPressed            |'U' キーの状態。                          |
|Engine.isVKeyPressed            |'V' キーの状態。                          |
|Engine.isWKeyPressed            |'W' キーの状態。                          |
|Engine.isXKeyPressed            |'X' キーの状態。                          |
|Engine.isYKeyPressed            |'Y' キーの状態。                          |
|Engine.isZKeyPressed            |'Z' キーの状態。                          |
|Engine.is1KeyPressed            |'1' キーの状態。                          |
|Engine.is2KeyPressed            |'2' キーの状態。                          |
|Engine.is3KeyPressed            |'3' キーの状態。                          |
|Engine.is4KeyPressed            |'4' キーの状態。                          |
|Engine.is5KeyPressed            |'5' キーの状態。                          |
|Engine.is6KeyPressed            |'6' キーの状態。                          |
|Engine.is7KeyPressed            |'7' キーの状態。                          |
|Engine.is8KeyPressed            |'8' キーの状態。                          |
|Engine.is9KeyPressed            |'9' キーの状態。                          |
|Engine.is0KeyPressed            |'0' キーの状態。                          |
|Engine.isF1KeyPressed           |F1 キーの状態。                           |
|Engine.isF2KeyPressed           |F2 キーの状態。                           |
|Engine.isF3KeyPressed           |F3 キーの状態。                           |
|Engine.isF4KeyPressed           |F4 キーの状態。                           |
|Engine.isF5KeyPressed           |F5 キーの状態。                           |
|Engine.isF6KeyPressed           |F6 キーの状態。                           |
|Engine.isF7KeyPressed           |F7 キーの状態。                           |
|Engine.isF8KeyPressed           |F8 キーの状態。                           |
|Engine.isF9KeyPressed           |F9 キーの状態。                           |
|Engine.isF10KeyPressed          |F10 キーの状態。                          |
|Engine.isF11KeyPressed          |F11 キーの状態。                          |
|Engine.isF12KeyPressed          |F12 キーの状態。                          |
|Engine.isGamepadUpPressed       |ゲームパッド上方向の状態。                |
|Engine.isGamepadDownPressed     |ゲームパッド下方向の状態。                |
|Engine.isGamepadLeftPressed     |ゲームパッド左方向の状態。                |
|Engine.isGamepadRightPressed    |ゲームパッド右方向の状態。                |
|Engine.isGamepadAPressed        |ゲームパッド A ボタンの状態。             |
|Engine.isGamepadBPressed        |ゲームパッド B ボタンの状態。             |
|Engine.isGamepadXPressed        |ゲームパッド X ボタンの状態。             |
|Engine.isGamepadYPressed        |ゲームパッド Y ボタンの状態。             |
|Engine.isGamepadLPressed        |ゲームパッド L ボタンの状態。             |
|Engine.isGamepadRPressed        |ゲームパッド R ボタンの状態。             |
|Engine.gamepadAnalogX1          |ゲームパッドアナログ 1 X (-32768, 32767)。|
|Engine.gamepadAnalogY1          |ゲームパッドアナログ 1 Y (-32768, 32767)。|
|Engine.gamepadAnalogX2          |ゲームパッドアナログ 2 X (-32768, 32767)。|
|Engine.gamepadAnalogY2          |ゲームパッドアナログ 2 Y (-32768, 32767)。|
|Engine.gamepadAnalogL           |ゲームパッドアナログ L (-32768, 32767)。  |
|Engine.gamepadAnalogR           |ゲームパッドアナログ R (-32768, 32767)。  |

```
func update() {
    if (Engine.isMouseLeftPressed) {
        player.x = player.x + 100;
    }
}
```

## 描画

### Engine.createColorTexture()

この API は指定された色のテクスチャを作成し、テクスチャを返します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|width               |テクスチャの幅。                                             |
|height              |テクスチャの高さ。                                           |
|r                   |赤色。                                                       |
|g                   |緑色。                                                       |
|b                   |青色。                                                       |
|a                   |アルファ値。                                                 |

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

この API はアセットからテクスチャを読み込み、テクスチャを返します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|file                |読み込むファイル名。                                         |

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

この API はテクスチャを破棄します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|texture             |テクスチャ。                                                 |

```
func destroyPlayerTexture() {
    Engine.loadTexture({
        texture: playerTex
    });
}
```

### Engine.renderTexture()

この API はテクスチャを画面に描画します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|dstLeft             |画面座標 X。                                                 |
|dstTop              |画面座標 Y。                                                 |
|dstWidth            |画面上の幅。                                                 |
|dstHeight           |画面上の高さ。                                               |
|texture             |テクスチャ。                                                 |
|srcLeft             |テクスチャ左上 X。                                           |
|srcTop              |テクスチャ左上 Y。                                           |
|srcWidth            |テクスチャ矩形の幅。                                         |
|srcHeight           |テクスチャ矩形の高さ。                                       |
|alpha               |アルファ値 (0-255)。                                         |

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

この API はテクスチャを画面に描画します (`Engine.renderTexture()` の簡易版です)。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|texture             |テクスチャ。                                                 |
|x                   |画面座標 X。                                                 |
|y                   |画面座標 Y。                                                 |

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

この API はテクスチャを画面に描画します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|x1                  |画面座標 X1。                                                |
|y1                  |画面座標 Y1。                                                |
|x2                  |画面座標 X2。                                                |
|y2                  |画面座標 Y2。                                                |
|x3                  |画面座標 X3。                                                |
|y3                  |画面座標 Y3。                                                |
|x4                  |画面座標 X4。                                                |
|y4                  |画面座標 Y4。                                                |
|texture             |テクスチャ。                                                 |
|srcLeft             |テクスチャ左上 X。                                           |
|srcTop              |テクスチャ左上 Y。                                           |
|srcWidth            |テクスチャ矩形の幅。                                         |
|srcHeight           |テクスチャ矩形の高さ。                                       |
|alpha               |アルファ値 (0-255)。                                         |

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

この API はアセットのフォントファイルを指定されたフォントスロットに読み込みます。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|slot                |フォントスロットのインデックス (0-3)。                       |
|file                |読み込むファイル名。                                         |

```
func loadNotoSansFont() {
    Engine.loadFont({ slot: 0, file: "NotoSans.ttf" });
}
```

### Engine.createTextTexture()

この API はテクスチャを作成し、その上にテキストを描画します。
     	 	 
|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|slot                |フォントスロットのインデックス (0-3)。                       |
|text                |描画するテキスト。                                           |
|size                |フォントサイズ。                                             |
|r                   |赤色。                                                       |
|g                   |緑色。                                                       |
|b                   |青色。                                                       |
|a                   |アルファ値。                                                 |

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

## サウンド

### Engine.playSound()

この API は指定されたサウンドトラックでサウンドアセットファイルの再生を開始します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|stream              |トラックのインデックス (0-3)。                               |
|file                |再生するファイル。                                           |

```
func playJumpSound() {
    Engine.playSound({ stream: 0, file: "jump.ogg" });
}
```

### Engine.playSoundLoop()

この API は指定されたサウンドトラックでサウンドアセットファイルのループ再生を開始します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|stream              |トラックのインデックス (0-3)。                               |
|file                |再生するファイル。                                           |

```
func playBGM() {
    Engine.playSoundLoop({ stream: 0, file: "jump.ogg" });
}
```

### Engine.stopSound()

この API は指定されたサウンドトラックのサウンド再生を停止します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|stream              |トラックのインデックス (0-3)。                               |

```
func playJumpSound() {
    Engine.stopSound({ stream: 0 });
}
```

### Engine.setSoundVolume()

この API は指定されたサウンドトラックの音量を設定します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|stream              |トラックのインデックス (0-3、-1 はマスター)。                |
|volume              |音量値 (0-1.0)。                                             |

```
func playJumpSound() {
    Engine.setSoundVolume({
        stream: 0,
        volume: 1.0
    });
}
```

## セーブデータ

### Engine.writeSaveData()

この API はキー文字列に対応するセーブデータ値を書き込みます。
セーブデータ値が大きすぎる場合、この API は失敗します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|key                 |キー文字列。                                                 |
|value               |値 (整数、浮動小数点数、配列、または辞書)。                  |

### Engine.readSaveData()

この API はキー文字列に対応するセーブデータ値を読み込みます。
戻り値はセーブデータ値を表すオブジェクトです。
指定されたキーが利用できない場合、この API は失敗します。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|key                 |キー文字列。                                                 |

### Engine.checkSaveData()

この API はキー文字列に対応するセーブデータが存在するかどうかを確認します。
戻り値は真偽値です。

|引数名              |説明                                                          |
|--------------------|--------------------------------------------------------------|
|key                 |キー文字列。                                                 |
