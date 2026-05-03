Suika3：入门指南
=============================

欢迎来到 Suika3！这份指南会用几个简单步骤，带你快速完成第一个
视觉小说专案的起步。

## 1. 安装

先把引擎跑起来，让你看看它的魅力。

### Windows

- **下载与解压缩**
    - 下载 [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip) 并解压缩到你喜欢的资料夹。
- **启动**
    - 开启该资料夹并执行 `suika3.exe`，就能启动范例游戏！

### macOS

- **下载与解压缩**
    - 下载 Suika3-full.zip 并解压缩到你喜欢的资料夹。
- **挂载磁碟映像档**
    - 进入 `SDK/macos/` 并开启 `Suika3.dmg`。
- **设定 App Bundle**
    - 将 DMG 里的 `Suika3` app 复制到与 `suika3.exe`（以及资料夹）相同的位置。
    - 注意：App bundle 必须与你的游戏资料放在同一层，才能正常运作。
- **启动**
    - 双击 `Suika3` app 即可启动范例游戏！

### Linux

- **下载与解压缩**
    - 下载 Suika3-full.zip 并解压缩到你喜欢的目录。
- **安装 Flatpak 套件**
    - 进入 `SDK/linux/` 并开启 `Suika3.flatpak`（或执行 `flatpak install --user Suika3.flatpak`）。
    - 这会把 `.novel` 和 `.ray` 档案关联到 Suika3 引擎。
- **启动**
    - 开启解压后的资料夹，然后双击 `start.novel` 来启动范例游戏！

## 2. Visual Studio Code 整合

VSCode 整合支援 Windows、macOS 与 Linux！

另外也可使用 [NovelML-Helper](https://github.com/lalalll-lalalll/NovelML-Helper) 取得语法高亮。

- 用 `Visual Studio Code` 开启解压后的资料夹。
- 点开命令面板。
- 点选 `Run Task`。
- 从以下专案中选择：
    - `Suika3: Run`（或 `Ctrl+Shift+B`）
    - `Suika3: Create a package`
    - `Suika3: Build Android APK`
    - `Suika3: Build iOS IPA`
- 如果发生错误，请点选 `PROBLEMS`。

## 3. 自订你的故事（`start.novel`）

接著，让游戏说出你想要的内容。

- **开启：**
    - 在专案资料夹中找到 `start.novel`，并用你喜欢的文字编辑器开启它。
- **编辑：**
    - 在档案开头加入以下标签：
    ```
    [text text="Hello, world! This is my first game."]
    ```
- **测试：**
    - 储存档案并再次执行 Suika3。
    - 你应该会在画面上看到新的讯息！

## 4. 自订画面（main.ray）

你可以很轻松地调整游戏视窗的外观与感受。

- **找到：**
    - 在编辑器中开启 `main.ray`。
- **修改：**
    - 找到 `func setup()` 区段。
    - 你可以在这里调整解析度与视窗标题：
    ```
    // 视窗开启时呼叫。
    func setup() {
        return {
            width:      1280,            // 游戏画面宽度
            height:     720,             // 游戏画面高度
            title:      "My First Game", // 游戏标题
            fullscreen: false            // 设为 true 即可全萤幕
        };
    }
    ```

## 5. 内部运作（进阶提示）

你的 `main.ray` 档案下半部包含引擎的核心逻辑。除非你要做进阶客制，
否则最好先维持这些函式原样：

- `func start()`：
    - 游戏启动时只会呼叫一次。
- `func update()`：
    - 每一帧都会执行，用来处理游戏逻辑。
- `func render()`：
    - 在更新完成后，把画面上的所有内容绘制出来。

```
// 游戏开始前呼叫。
func start() {
    // 在这里载入外挂。
    // Suika.loadPlugin("testplugin");

    // 请勿删除以下这行。
    Suika.start();
}

// 每一帧绘制前呼叫。
func update() {
    // 请勿删除以下这行。
    Suika.update();
}

// 每一帧绘制时呼叫。
func render() {
    // 请勿删除以下这行。
    Suika.render();
}
```

> [!TIPS]
> 这些函式是驱动 Suika3 的 `Playfield Engine` 核心机制。
> `Suika.start()`、`Suika.update()` 与 `Suika.render()` 必须保留，
> 游戏才能正常运作。
