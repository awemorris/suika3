クイックスタート
================

下記のステップに従って、最初のゲームを実行してください。

## Playfield の取得


**Windows の場合:**

* [Web サイト](https://noctvm.io/playfield/) から `Windows 11 64ビット ZIP` をダウンロードします
* [Web サイト](https://noctvm.io/playfield/) から `サンプルゲーム ZIP` をダウンロードします
* `サンプルゲーム ZIP` をお好きなフォルダに展開します
* `Windows 11 64-bit ZIP` を展開し、`playfield.exe` をゲームフォルダにコピーします (例: "samples/shoot2")
* `playfield.exe` をダブルクリックして、ゲームを実行します

**macOS の場合:**

* [Web サイト](https://noctvm.io/playfield/) から `macOS Arm64/Intel DMG` をダウンロードします
* [Web サイト](https://noctvm.io/playfield/) から `サンプルゲーム ZIP` をダウンロードします
* `サンプルゲーム ZIP` をお好きなフォルダに展開します
* `DMG` を展開し、`Playfield` アプリをゲームフォルダにコピーします (例: "samples/shoot2")
* `Playfield` アプリをダブルクリックして、ゲームを実行します

**Linux の場合:**

* [Web サイト](https://noctvm.io/playfield/) から `Playfield-x86_64.AppImage` をダウンロードします
* [Web サイト](https://noctvm.io/playfield/) から `サンプルゲーム ZIP` をダウンロードします
* `サンプルゲーム ZIP` をお好きなフォルダに展開します
* `AppImage` を ゲームフォルダにコピーします (例: "samples/shoot2")
* 必要であれば、AppImage ファイルを実行可能にします (`chmod +x Playfield-x86_64.AppImage`)
* AppImage をダブルクリックして、ゲームを実行します

## 最初のゲームの作成

最小のゲームを作るには:

1. お好きなフォルダを作成します
2. お好きなテキストエディタをそのフォルダで開きます
3. `main.pf` というファイルを作成します
4. 下記の内容をそのファイルにペーストして保存します

```
// ウィンドウ作成時に呼ばれます
func setup() {
    // ウィンドウの設定を返します
    return {
        width: 1280,
        height: 720,
        title: My First Game
    };
}

// ゲーム開始時に一度だけ呼ばれます
func start() {
    // 100x100 の白いテクスチャを作成します
    tex = Engine.createColorTexture({
        width: 100,
        height: 100,
        r: 255, g: 255, b: 255, a: 255
    });
}

// 毎フレーム、レンダリングの前に呼ばれます
func update() {
    posX = Engine.mousePosX;
    posY = Engine.mousePosY;
}

// 毎フレーム、レンダリングのために呼ばれます
func render() {
    Engine.draw({ texture: tex, x: posX, y: posY });
}
```

## ゲームの実行

Playfield Engine は下記の２通りの方法でゲームをロードできます:

* `main.pf` ファイルを含むフォルダ
* `assets.arc` ファイル (すべてのゲームアセットが入っている)

`assets.arc` ファイルを作成するには、[ゲームの配布](distribute.md) を見てください。
