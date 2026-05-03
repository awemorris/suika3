Suika3：快速入门指南
=============================

欢迎使用 Suika3！本指南将帮助你通过几个简单的步骤，快速启动你的第一个视觉小说项目。

## 1. 安装

让我们先让引擎跑起来，亲眼见证奇迹的发生！

### Windows

- **下载与解压**
    - 下载 [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip) 并解压到你偏好的文件夹。
- **启动**
    - 打开文件夹并运行 `suika3.exe`，即可开始游玩示例游戏！

### macOS

- **下载与解压**
    - 下载 Suika3-full.zip 并解压到你偏好的文件夹。
- **挂载磁盘镜像**
    - 进入 `SDK/macos/` 目录并打开 `Suika3.dmg`。
- **设置 App 包**
    - 将 DMG 中的 `Suika3` 应用程序复制到与 `suika3.exe`（以及数据文件夹）相同的文件夹中。
    - 注意：App 包必须与你的游戏数据位于同一目录下才能正常工作。
- **启动**
    - 双击 `Suika3` 应用来启动示例游戏！

### Linux

- **下载与解压**
    - 下载 Suika3-full.zip 并解压到你偏好的目录。
- **安装 Flatpak 包**
    - 进入 `SDK/linux/` 目录并打开 `Suika3.flatpak`（或运行 `flatpak install --user Suika3.flatpak`）。
    - 这将把 `.novel` 和 `.ray` 文件与 Suika3 引擎关联起来。
- **启动**
    - 打开解压后的文件夹，然后双击 `start.novel` 来启动示例游戏！

## 2. Visual Studio Code 集成

VSCode 集成支持 Windows、macOS 和 Linux！

此外，[NovelML-Helper](https://github.com/lalalll-lalalll/NovelML-Helper)可用于语法高亮。

- 使用 `Visual Studio Code` 打开解压后的文件夹。
- 点击命令面板（Command Palette）。
- 点击 `运行任务 (Run Task)`。
- 选择以下任一项：
    - `Suika3: Run` (or `Ctrl+Shift+B`)
    - `Suika3: Create a package`
    - `Suika3: Build Android APK`
    - `Suika3: Build iOS IPA`
- 如果发生错误，请点击 `PROBLEMS`（问题）面板查看。

## 3. 个性化你的故事 (`start.novel`)

现在，让我们来修改游戏，让它说出你想要的内容。

- **打开：**
    - 在项目文件夹中找到 `start.novel` 文件，并用你喜欢的文本编辑器打开。
- **编辑：**
    - Add the following tag at the beginning of the file:
    ```
    [text text="Hello, world!This is my first game."]
    ```
- **测试：**
    - 保存文件并重新运行 Suika3。
    - 你应该能在屏幕上看到你的新消息！

## 4. 自定义屏幕 (main.ray)

你可以轻松地更改游戏窗口的外观和感觉。

- **定位：**
    - 在编辑器中打开 `main.ray` 文件。
- **修改：**
    - 找到 `func setup()` 部分。
    - 你可以在这里更改分辨率和窗口标题：
    ```
    // Called when the window is opened.
    func setup() {
        return {
            width:      1280,            // 游戏的宽度
            height:     720,             // 游戏的高度
            title:      "My First Game", // 游戏的标题
            fullscreen: false            // 设置为 true 以全屏模式运行
        };
    }
    ```

## 5. 引擎核心机制（进阶提示）

`main.ray` 文件的底部包含核心引擎逻辑。除非你要进行高级定制，否则最好保持这些函数不变：

- `func start() {`
    - 游戏启动时调用一次。
- `func update()`:
    - 每一帧渲染前运行，用于处理游戏逻辑。
- `func render()`:
    - 在更新完成后，每一帧绘制画面时调用。

```
// 在游戏开始前被调用。
func start() {
    // Load plugins here.
    // Suika.loadPlugin("testplugin");
    // 不要删除下面这行。
    Suika.start();
}
// Called before a frame rendering.
func update() {
    // 不要删除下面这行。
    Suika.update();
}
// Called every frame rendering.
func render() {
    // 不要删除下面这行。
    Suika.render();
}
```

> [!提示]
> 这些函数是驱动 Suika3 的 `Playfield 引擎` 的核心机制。为了保证游戏正常运行，`Suika.start()`、`Suika.update()` 和 `Suika.render()` 必须保留在原位。
