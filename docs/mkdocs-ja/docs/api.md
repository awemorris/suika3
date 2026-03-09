Playfield Script API
====================

すべての API 関数は 1 つのパラメータを取ります。
そのパラメータは辞書でなければならず、引数はキーと値のペアとして格納されます。

## 骨組み

```
// 関数の外で変数を宣言しないでくささい（文法エラーです）

// ウィンドウが作成されるときに呼ばれます
func setup() {
    // ウィンドウの設定を返します
    return {
        width: 1280,
        height: 720,
        title: "My First Game"
    };
}

// ゲームが開始するときに呼ばれます
func start() {
    // ここでグローバル変数を宣言します
    posX = 0;
    posY = 0;

    // 100x100サイズの白いテクスチャを作ります
    tex = Engine.createColorTexture({
        width: 100,
        height: 100,
        r: 255, g: 255, b: 255, a: 255
    });
}

// フレームごとの描画の前に呼ばれます
func update() {
    posX = Engine.mousePosX;
    posY = Engine.mousePosY;
}

// フレームごとの描画の際に呼ばれます
func render() {
    Engine.draw({ texture: tex, x: posX, y: posY });
}
```

## デバッグ

### print()

この API は文字列を表示するか、オブジェクトをダンプします。
この API だけは、辞書でない引数を取ります。

```
func dumpEnemies() {
    if (Engine.isGamepadXPressed) {
        debug("[Current emenies]");
        debug(enemies);
    }
}
```

## 時間

### 絶対的な時間

|変数                        |説明                                       |
|----------------------------|-------------------------------------------|
|Engine.millisec             |起動からの時間（ミリ秒）                   |

```
func frame() {
    var dt = (Engine.millisec - lastTime) * 0.001;

    update(dt);
    render();
}
```

### Engine.getDate()

日付を表す辞書を返します。

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

これらは変数であり、関数ではありません。

|変数                            |説明                                       |
|--------------------------------|-------------------------------------------|
|Engine.mousePosX                |マウスX座標                                |
|Engine.mousePosY                |マウスY座標                                |
|Engine.isMouseLeftPressed       |左マウスボタン                             |
|Engine.isMouseRightPressed      |右マウスボタン                             |
|Engine.isEscapeKeyPressed       |Escape キーの状態                          |
|Engine.isReturnKeyPressed       |Return キーの状態                          |
|Engine.isSpaceKeyPressed        |Space キーの状態                           |
|Engine.isTabKeyPressed          |Tab キーの状態                             |
|Engine.isBackspaceKeyPressed    |Backspace キーの状態                       |
|Engine.isDeleteKeyPressed       |Delete キーの状態                          |
|Engine.isHomeKeyPressed         |Home キーの状態                            |
|Engine.isEndKeyPressed          |End キーの状態                             |
|Engine.isPageupKeyPressed       |PageUp キーの状態                          |
|Engine.isPagedownKeyPressed     |PageDown キーの状態                        |
|Engine.isShiftKeyPressed        |Shift キーの状態                           |
|Engine.isControlKeyPressed      |Control キーの状態                         |
|Engine.isAltKeyPressed          |Alt キーの状態                             |
|Engine.isUpKeyPressed           |Up arrow キーの状態                        |
|Engine.isDownKeyPressed         |Down arrow キーの状態                      |
|Engine.isRightKeyPressed        |Right arrow キーの状態                     |
|Engine.isLeftKeyPressed         |Left arrow キーの状態                      |
|Engine.isAKeyPressed            |'A' キーの状態                             |
|Engine.isBKeyPressed            |'B' キーの状態                             |
|Engine.isCKeyPressed            |'C' キーの状態                             |
|Engine.isDKeyPressed            |'D' キーの状態                             |
|Engine.isEKeyPressed            |'E' キーの状態                             |
|Engine.isFKeyPressed            |'F' キーの状態                             |
|Engine.isGKeyPressed            |'G' キーの状態                             |
|Engine.isHKeyPressed            |'H' キーの状態                             |
|Engine.isIKeyPressed            |'I' キーの状態                             |
|Engine.isJKeyPressed            |'J' キーの状態                             |
|Engine.isKKeyPressed            |'K' キーの状態                             |
|Engine.isLKeyPressed            |'L' キーの状態                             |
|Engine.isMKeyPressed            |'M' キーの状態                             |
|Engine.isNKeyPressed            |'N' キーの状態                             |
|Engine.isOKeyPressed            |'O' キーの状態                             |
|Engine.isPKeyPressed            |'P' キーの状態                             |
|Engine.isQKeyPressed            |'Q' キーの状態                             |
|Engine.isRKeyPressed            |'R' キーの状態                             |
|Engine.isSKeyPressed            |'S' キーの状態                             |
|Engine.isTKeyPressed            |'T' キーの状態                             |
|Engine.isUKeyPressed            |'U' キーの状態                             |
|Engine.isVKeyPressed            |'V' キーの状態                             |
|Engine.isWKeyPressed            |'W' キーの状態                             |
|Engine.isXKeyPressed            |'X' キーの状態                             |
|Engine.isYKeyPressed            |'Y' キーの状態                             |
|Engine.isZKeyPressed            |'Z' キーの状態                             |
|Engine.is1KeyPressed            |'1' キーの状態                             |
|Engine.is2KeyPressed            |'2' キーの状態                             |
|Engine.is3KeyPressed            |'3' キーの状態                             |
|Engine.is4KeyPressed            |'4' キーの状態                             |
|Engine.is5KeyPressed            |'5' キーの状態                             |
|Engine.is6KeyPressed            |'6' キーの状態                             |
|Engine.is7KeyPressed            |'7' キーの状態                             |
|Engine.is8KeyPressed            |'8' キーの状態                             |
|Engine.is9KeyPressed            |'9' キーの状態                             |
|Engine.is0KeyPressed            |'0' キーの状態                             |
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
|Engine.isGamepadUpPressed       |ゲームパッド上キー                         |
|Engine.isGamepadDownPressed     |ゲームパッド下キー                         |
|Engine.isGamepadLeftPressed     |ゲームパッド左キー                         |
|Engine.isGamepadRightPressed    |ゲームパッド右キー                         |
|Engine.isGamepadAPressed        |ゲームパッドボタン1                        |
|Engine.isGamepadBPressed        |ゲームパッドボタン2                        |
|Engine.isGamepadXPressed        |ゲームパッドボタン3                        |
|Engine.isGamepadYPressed        |ゲームパッドボタン4                        |
|Engine.isGamepadLPressed        |ゲームパッドボタン5                        |
|Engine.isGamepadRPressed        |ゲームパッドボタン6                        |
|Engine.gamepadAnalogX1          |ゲームパッドアナログ1 X (-32768, 32767)    |
|Engine.gamepadAnalogY1          |ゲームパッドアナログ1 Y (-32768, 32767)    |
|Engine.gamepadAnalogX2          |ゲームパッドアナログ1 X (-32768, 32767)    |
|Engine.gamepadAnalogY2          |ゲームパッドアナログ1 Y (-32768, 32767)    |
|Engine.gamepadAnalogL           |ゲームパッドアナログL (-32768, 32767)      |
|Engine.gamepadAnalogR           |ゲームパッドアナログR (-32768, 32767)      |

```
func update() {
    if (Engine.isMouseLeftPressed) {
        player.x = player.x + 100;
    }
}
```

## レンダリング

### Engine.createColorTexture()

この API は指定された色のテクスチャを作成して返します。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|width               |テクスチャの幅                                          |
|height              |テクスチャの高さ                                        |
|r                   |色の R 値 (0-255)                                       |
|g                   |色の G 値 (0-255)                                       |
|b                   |色の B 値 (0-255)                                       |
|a                   |アルファ値 (0-255)                                      |

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

この API はアセットからテクスチャをロードし、テクスチャを返します。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|file                |ロードするファイルの名前                                |

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

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|texture             |テクスチャ                                              |

```
func destroyPlayerTexture() {
    Engine.loadTexture({
        texture: playerTex
    });
}
```

### Engine.renderTexture()

この API はテクスチャをスクリーンに描画します。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|dstLeft             |スクリーン X 座標                                       |
|dstTop              |スクリーン Y 座標                                       |
|dstWidth            |スクリーンでの幅                                        |
|dstHeight           |スクリーンでの高さ                                      |
|texture             |テクスチャ                                              |
|srcLeft             |テクスチャの左上 X 座標                                 |
|srcTop              |テクスチャの左上 Y 座標                                 |
|srcWidth            |テクスチャ矩形の幅                                      |
|srcHeight           |テキスチャ矩形の高さ                                    |
|alpha               |アルファ値 (0-255)                                      |

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

この API はテキスチャをスクリーンに描画します。
`Engine.renderTexture()` のシンプル版です。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|texture             |テクスチャ                                              |
|x                   |スクリーン X 座標                                       |
|y                   |スクリーン Y 座標                                       |

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

この API はテクスチャを 3D 変形してスクリーンに描画します。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|x1                  |スクリーン座標 X1                                       |
|y1                  |スクリーン座標 Y1                                       |
|x2                  |スクリーン座標 X2                                       |
|y2                  |スクリーン座標 Y2                                       |
|x3                  |スクリーン座標 X3                                       |
|y3                  |スクリーン座標 Y3                                       |
|x4                  |スクリーン座標 X4                                       |
|y4                  |スクリーン座標 Y4                                       |
|texture             |テクスチャ                                              |
|srcLeft             |テクスチャ左上 X 座標                                   |
|srcTop              |テクスチャ左上 Y 座標                                   |
|srcWidth            |テクスチャ矩形の幅                                      |
|srcHeight           |テクスチャ矩形の高さ                                    |
|alpha               |アルファ値 (0-255)                                      |

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

この API はフォントファイルをフォントスロットにロードします。

|Argument Name       |Description                                             |
|--------------------|--------------------------------------------------------|
|slot                |フォントスロット番号 (0-3)                              |
|file                |ロードするファイルの名前                                |

```
func loadNotoSansFont() {
    Engine.loadFont({ slot: 0, file: "NotoSans.ttf" });
}
```

### Engine.createTextTexture()

この API はテキストを描画したテクスチャを描画します。
     	 	 
|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|slot                |フォントスロット番号 (0-3)                              |
|text                |描画するテキスト                                        |
|size                |フォントサイズ                                          |
|r                   |色の R 値 (0-255)                                       |
|g                   |色の G 値 (0-255)                                       |
|b                   |色の B 値 (0-255)                                       |
|a                   |アルファ値 (0-255)                                      |

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

この API はサウンドをサウンドトラック上で再生します。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|stream              |トラック番号 (0-3)                                      |
|file                |再生するファイルの名前                                  |

```
func playJumpSound() {
    Engine.playSound({ stream: 0, file: "jump.ogg" });
}
```

### Engine.stopSound()

この API はサウンドトラック上のサウンド再生を停止します。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|stream              |Track index. (0-3)                                      |

```
func playJumpSound() {
    Engine.stopSound({ stream: 0 });
}
```

### Engine.setSoundVolume()

この API はサウンドトラックのボリュームを設定します。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|stream              |トラック番号 (0-3, -1 ならマスター)                     |
|volume              |ボリューム値 (0-1.0)                                    |

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

この API は文字列キーに対応するセーブデータ値を書き込みます。
セーブデータ値が大きすぎる場合、API は失敗します。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|key                 |文字列キー                                              |
|value               |値 (整数, 浮動小数点数, 配列, 辞書)                     |

### Engine.readSaveData()

この API は文字列キーに対応するセーブデータ値を読み込みます。
戻り値はセーブデータ値を表すオブジェクトです。
文字列キーが存在しないとき、この API は失敗します。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|key                 |文字列キー                                              |

### Engine.checkSaveData()

この API はセーブデータの文字列キーが存在するかどうかをチェックします。
戻り値は boolean です。

|引数名              |説明                                                    |
|--------------------|--------------------------------------------------------|
|key                 |文字列キー                                              |
