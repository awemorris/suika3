Suika3 设计文档
=============

## 分层组件模型

Suika3 并非采用单体设计，而是由小型库和组件分离构成，整体呈现出一种层层堆叠的层级结构。

```
+----------------------------------------+
| VN 引擎 (Suika3)		| ---> 运行 `.novel`
+----------------------------------------+
| 2D 游戏引擎 (Playfield 引擎)	| ---> 运行 `.ray`
+----------------------------------------+
| 脚本语言 (NoctLang)	| ---> 即时编译/提前编译 基础设施
+----------------------------------------+
| 硬件抽象层 (StratoHAL)	| ---> 用于可移植性
+----------------------------------------+
```

这种结构在 Suika3 中被称为“分层组件模型”。

- 每一层仅实现一个功能。
- 每一层向上一层提供公开的 C 语言 API。
- 每一层只能使用下一层提供的公开 C 语言 API。

这种方法的优势在于，像 C++ 这样的面向对象语言中的类必须考虑多对多的依赖关系，从而带来固有的复杂性；而分层组件模型只需要考虑与上一层和下一层的关系，这使得设计、实现、修改和扩展变得更加简单。
一个全栈游戏引擎是复杂且庞大的软件，但通过简单组件的一对一关系进行组装，整体构建变得更加容易，从而提高了可移植性。

可移植性之所以得到提高，是因为底层是一个吸收操作系统差异的“硬件抽象层”，而其上的各层在设计和实现时无需考虑操作系统的差异。

## 启动与执行序列

<img src="sequence.svg">

- StratoHAL: 调用 `main()`。
- StratoHAL: 挂载包文件。
- StratoHAL: 调用 `Playfield 引擎` 中的 `hal_callback_on_event_setup()` 以确定屏幕尺寸。
    - Playfield Engine: `hal_callback_on_event_setup()` 被调用。
    - Playfield Engine: 加载 `main.ray`。
    - Playfield Engine: 调用 `main.ray` 中的 `setup()`。
        - Suika3 Game: `setup()` 被调用。
        - Suika3 Game: 返回屏幕尺寸。
    - Playfield Engine: 返回屏幕尺寸。
- StratoHAL: 现在已知晓屏幕尺寸。
- StratoHAL: 创建窗口。
- StratoHAL: 设置音频。
- StratoHAL: 调用 `Playfield 引擎` 中的 `hal_callback_on_event_start()`。
    - Playfield Engine: `hal_callback_on_event_start()` 被调用。
    - Playfield Engine: 调用 `main.ray` 中的 `start()`。
        - Suika3 Game: `start()` 被调用。
        - Suika3 Game: 调用 `Suika.start()`。
            - Suika3 Internal: `Suika.start()` 被调用。
            - Suika3 Internal: 加载 `config.ini`。
            - Suika3 Internal: 加载 `start.novel`。
            - Suika3 Internal: 返回。
        - Suika3 Game: 返回。
    - Playfield Engine: 返回。
- StratoHAL: 游戏循环:
    - StratoHAL: 调用 `Playfield 引擎` 中的 `on_hal_callback_update()`。
        - Playfield Engine: `on_hal_callback_update()` 被调用。
    - Playfield Engine: 调用 `main.ray` 中的 `update()`。
            - Suika3 Game: `update()` 被调用。
            - Suika3 Game: 调用 `Suika.update()`。
                - Suika3 Internal: `Suika.update()` 被调用。
                - Suika3 Internal: 获取当前标签及其参数。
                - Suika3 Internal: 调用对应当前标签的 `Tag_*()`。
                    - Tag_*: 运行一帧。
                    - Tag_*: 如果完成则移动到下一个标签。
                    - Tag_*: 返回。
            - Suika3 Game: 返回。
    - Playfield Engine: 调用 `main.ray` 中的 `render()`。
            - Suika3: `render()` 被调用。
            - Suika3: 调用 `Suika.render()`。
                - Suika3 Internal: 渲染舞台层。
                - Suika3 Internal: 返回。
            - User Game: 返回。
    - StratoHAL: 完成渲染。
    - StratoHAL: 回到循环开始。
