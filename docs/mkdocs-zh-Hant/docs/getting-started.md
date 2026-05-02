Suika3：入門指南
=============================

歡迎來到 Suika3！這份指南會用幾個簡單步驟，帶你快速完成第一個
視覺小說專案的起步。

## 1. 安裝

先把引擎跑起來，讓你看看它的魅力。

### Windows

- **下載與解壓縮**
    - 下載 [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip) 並解壓縮到你喜歡的資料夾。
- **啟動**
    - 開啟該資料夾並執行 `suika3.exe`，就能啟動範例遊戲！

### macOS

- **下載與解壓縮**
    - 下載 Suika3-full.zip 並解壓縮到你喜歡的資料夾。
- **掛載磁碟映像檔**
    - 進入 `SDK/macos/` 並開啟 `Suika3.dmg`。
- **設定 App Bundle**
    - 將 DMG 裡的 `Suika3` app 複製到與 `suika3.exe`（以及資料夾）相同的位置。
    - 注意：App bundle 必須與你的遊戲資料放在同一層，才能正常運作。
- **啟動**
    - 雙擊 `Suika3` app 即可啟動範例遊戲！

### Linux

- **下載與解壓縮**
    - 下載 Suika3-full.zip 並解壓縮到你喜歡的目錄。
- **安裝 Flatpak 套件**
    - 進入 `SDK/linux/` 並開啟 `Suika3.flatpak`（或執行 `flatpak install --user Suika3.flatpak`）。
    - 這會把 `.novel` 和 `.ray` 檔案關聯到 Suika3 引擎。
- **啟動**
    - 開啟解壓後的資料夾，然後雙擊 `start.novel` 來啟動範例遊戲！

## 2. Visual Studio Code 整合

VSCode 整合支援 Windows、macOS 與 Linux！

另外也可使用 [NovelML-Helper](https://github.com/lalalll-lalalll/NovelML-Helper) 取得語法高亮。

- 用 `Visual Studio Code` 開啟解壓後的資料夾。
- 點開命令面板。
- 點選 `Run Task`。
- 從以下專案中選擇：
    - `Suika3: Run`（或 `Ctrl+Shift+B`）
    - `Suika3: Create a package`
    - `Suika3: Build Android APK`
    - `Suika3: Build iOS IPA`
- 如果發生錯誤，請點選 `PROBLEMS`。

## 3. 自訂你的故事（`start.novel`）

接著，讓遊戲說出你想要的內容。

- **開啟：**
    - 在專案資料夾中找到 `start.novel`，並用你喜歡的文字編輯器開啟它。
- **編輯：**
    - 在檔案開頭加入以下標籤：
    ```
    [text text="Hello, world! This is my first game."]
    ```
- **測試：**
    - 儲存檔案並再次執行 Suika3。
    - 你應該會在畫面上看到新的訊息！

## 4. 自訂畫面（main.ray）

你可以很輕鬆地調整遊戲視窗的外觀與感受。

- **找到：**
    - 在編輯器中開啟 `main.ray`。
- **修改：**
    - 找到 `func setup()` 區段。
    - 你可以在這裡調整解析度與視窗標題：
    ```
    // 視窗開啟時呼叫。
    func setup() {
        return {
            width:      1280,            // 遊戲畫面寬度
            height:     720,             // 遊戲畫面高度
            title:      "My First Game", // 遊戲標題
            fullscreen: false            // 設為 true 即可全螢幕
        };
    }
    ```

## 5. 內部運作（進階提示）

你的 `main.ray` 檔案下半部包含引擎的核心邏輯。除非你要做進階客制，
否則最好先維持這些函式原樣：

- `func start()`：
    - 遊戲啟動時只會呼叫一次。
- `func update()`：
    - 每一幀都會執行，用來處理遊戲邏輯。
- `func render()`：
    - 在更新完成後，把畫面上的所有內容繪製出來。

```
// 遊戲開始前呼叫。
func start() {
    // 在這裡載入外掛。
    // Suika.loadPlugin("testplugin");

    // 請勿刪除以下這行。
    Suika.start();
}

// 每一幀繪製前呼叫。
func update() {
    // 請勿刪除以下這行。
    Suika.update();
}

// 每一幀繪製時呼叫。
func render() {
    // 請勿刪除以下這行。
    Suika.render();
}
```

> [!TIPS]
> 這些函式是驅動 Suika3 的 `Playfield Engine` 核心機制。
> `Suika.start()`、`Suika.update()` 與 `Suika.render()` 必須保留，
> 遊戲才能正常運作。
