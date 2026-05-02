Suika3: はじめに
=================

Suika3 へようこそ！このガイドでは簡単な手順で初めてのビジュアルノベル制作をすぐに始められるようにします。

## 1. インストール

まずはエンジンを起動して、実際に動くところを見てみましょう！

### Windows

- **ダウンロードと展開**
    - [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip) をダウンロードし、任意のフォルダーに展開します。
- **起動**
    - フォルダーを開き、`suika3.exe` を実行してサンプルゲームを起動します。

### macOS

- **ダウンロードと展開**
    - Suika3-SDK-Full.zip をダウンロードし、任意のフォルダーに展開します。
- **ディスクイメージをマウント**
    - `SDK/macos/` に移動し、`Suika3.dmg` を開きます。
- **アプリバンドルの設定**
    - DMG 内の `Suika3` アプリを、`suika3.exe`（および data フォルダー）があるフォルダーと同じ場所にコピーします。
    - 注意: 正しく動作させるには、アプリバンドルをゲームデータと同じ場所に置く必要があります。
- **起動**
    - `Suika3` アプリをダブルクリックしてサンプルゲームを起動します。

### Linux

- **ダウンロードと展開**
    - Suika3-SDK-Full.zip をダウンロードし、任意のディレクトリに展開します。
- **Flatpak パッケージのインストール**
    - `SDK/linux/` に移動して `Suika3.flatpak` を開きます。（または `flatpak install --user Suika3.flatpak` を実行します。）
    - これにより、`.novel` ファイルと `.ray` ファイルが Suika3 エンジンに関連付けられます。
- **起動**
    - 展開したフォルダーを開き、`start.novel` をダブルクリックしてサンプルゲームを起動します。

## 2. Visual Studio Code 連携

VSCode 連携は Windows、macOS、Linux で利用できます。

また、構文ハイライトには [NovelML-Helper](https://github.com/lalalll-lalalll/NovelML-Helper) を利用できます。

- 展開したフォルダーを `Visual Studio Code` で開きます。
- コマンドパレットをクリックします。
- `Run Task` をクリックします。
- 次の中から選択します。
    - `Suika3: Run`（または `Ctrl+Shift+B`）
    - `Suika3: Create a package`
    - `Suika3: Build Android APK`
    - `Suika3: Build iOS IPA`
- エラーが発生した場合は `PROBLEMS` をクリックします。

## 3. ストーリーを自分好みにする（`start.novel`）

次に、ゲームに表示したいメッセージを設定してみましょう。

- **開く:**
    - プロジェクトフォルダー内の `start.novel` ファイルを見つけ、お好みのテキストエディターで開きます。
- **編集:**
    - ファイルの先頭に次のタグを追加します。
    ```
    [text text="こんにちは、世界！これは私の最初のゲームです。"]
    ```
- **テスト:**
    - ファイルを保存し、Suika3 をもう一度実行します。
    - 画面に新しいメッセージが表示されるはずです。

## 4. 画面をカスタマイズする（main.ray）

ゲームウィンドウの見た目や動作は簡単に変更できます。

- **場所を確認:**
    - エディターで `main.ray` ファイルを開きます。
- **変更:**
    - `func setup()` セクションを探します。
    - ここでウィンドウの解像度やタイトルを変更できます。
    ```
    // ウィンドウが開かれたときに呼び出されます。
    func setup() {
        return {
            width:      1280,            // ゲームの幅
            height:     720,             // ゲームの高さ
            title:      "私の最初のゲーム", // ゲームのタイトル
            fullscreen: false            // フルスクリーンモードにする場合は true
        };
    }
    ```

## 5. 内部の仕組み（上級者向けのヒント）

`main.ray` ファイルの下部には、エンジンの中核となるロジックが含まれています。
高度なカスタマイズを行う場合を除き、これらの関数はそのままにしておくことをおすすめします。

- `func start()`:
    - ゲームの起動時に一度だけ呼び出されます。
- `func update()`:
    - ゲームロジックを処理するために、毎フレーム実行されます。
- `func render()`:
    - 更新処理が終わったあと、画面上のすべてを描画します。

```
// ゲームが開始される前に呼び出されます。
func start() {
    // ここでプラグインを読み込みます。
    // Suika.loadPlugin("testplugin");

    // 次の行は削除しないでください。
    Suika.start();
}

// フレーム描画の前に呼び出されます。
func update() {
    // 次の行は削除しないでください。
    Suika.update();
}

// 毎フレームの描画時に呼び出されます。
func render() {
    // 次の行は削除しないでください。
    Suika.render();
}
```

> [!TIPS]
> これらの関数は、Suika3 を動かしている `Playfield Engine` の中核となる仕組みです。
> ゲームを正しく動作させるには、Suika.start()、Suika.update()、Suika.render() を
> そのまま残しておく必要があります。
