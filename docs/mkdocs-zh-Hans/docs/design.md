Suika3 设计
=============

## 分层元件模型

Suika3 并不是单体式设计，而是拆分成多个小型函式库与元件，整体呈现层层堆叠的阶层结构。

- 每一层只实作一项功能。
- 每一层都会向上一层提供公开的 C 语言 API。
- 每一层只能使用下一层提供的公开 C 语言 API。

在 Suika3 里，这种结构称为「分层元件模型」。

这种做法的优点是，像 C++ 这类物件导向语言中的类别，往往必须面对多对多的相依关系，内建复杂度很高；而分层元件模型只需要考虑上一层与下一层的关系，因此设计、实作、修改与扩充都更单纯。
完整的游戏引擎本来就是复杂又庞大的软体，但如果透过简单元件的一对一关系来组装，整体就更容易建构，也能提升可携性。

可携性提升的原因在于，最底层是吸收作业系统差异的「硬体抽象层」，而其上的各层在设计与实作时就不必再顾虑作业系统差异。

```
+-----------------------------------+
| VN Engine (Suika3)                |
+-----------------------------------+
| 2D Game Engine (Playfield Engine) |
+-----------------------------------+
| Scripting Language (NoctLang)     |
+-----------------------------------+
| Hardware Abstraction Layer        |
| (StratoHAL)                       |
+-----------------------------------+
```

## Boot and Execution Sequence

<img src="sequence.svg">

- StratoHAL: `main()` is called.
- StratoHAL: Mount the package file.
- StratoHAL: Calls `hal_callback_on_event_setup()` in `Playfield Engine` tp determine the screen size.
    - Playfield Engine: `hal_callback_on_event_setup()` is called.
    - Playfield Engine: Loads `main.ray`.
    - Playfield Engine: Calls `setup()` in the `main.ray`.
        - Suika3 Game: `setup()` is called.
        - Suika3 Game: Returns the screen size.
    - Playfield Engine: Returns the screen size.
- StratoHAL: Now knows the screen size.
- StratoHAL: Creates the window.
- StratoHAL: Sets up audio.
- StratoHAL: Calls `hal_callback_on_event_start()` in `Playfield Engine`.
    - Playfield Engine: `hal_callback_on_event_start()` is called.
    - Playfield Engine: Calls `start()` in the `main.ray`.
        - Suika3 Game: `start()` is called.
        - Suika3 Game: Calls `Suika.start()`.
            - Suika3 Internal: `Suika.start()` is called.
            - Suika3 Internal: Load `config.ini`.
            - Suika3 Internal: Load `start.novel`.
            - Suika3 Internal: Returns.
        - Suika3 Game: Returns.
    - Playfield Engine: Returns.
- StratoHAL: Game Loop:
    - StratoHAL: Calls `on_hal_callback_update()` in `Playfield Engine`.
        - Playfield Engine: `on_hal_callback_update()` is called.
		- Playfield Engine: Calls `update()` in the `main.ray`.
            - Suika3 Game: `update()` is called.
            - Suika3 Game: Calls `Suika.update()`.
                - Suika3 Internal: `Suika.update()` is called.
                - Suika3 Internal: Get the current tag and its parameters.
                - Suika3 Internal: Calls `Tag_*()` correnponding to the current tag.
                    - Tag_*: Run a frame.
                    - Tag_*: Moves to the next tag if finished.
                    - Tag_*: Returns.
            - Suika3 Game: Returns.
		- Playfield Engine: Calls `render()` in the `main.ray`.
            - Suika3: `render()` is called.
            - Suika3: Calls `Suika.render()`.
                - Suika3 Internal: Renders the stage layers.
                - Suika3 Internal: Returns.
            - User Game: Returns.
    - StratoHAL: Finish rendering.
    - StratoHAL: Go back to the beginning of the loop.
