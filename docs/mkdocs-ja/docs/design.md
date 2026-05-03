Suika3 の設計
============

## レイヤーコンポーネントモデル

Suika3 は一枚岩の設計ではなく、小さなライブラリ群としてパーツに分離しており、全体としてはそれらを階層的に積み重ねた構成になっています。

- それぞれの階層は、１つの機能のみを実現します。
- それぞれの階層は、自分より１つ上の階層に対して、C言語の公開APIを提供します。
- それぞれの階層は、自分より１つ下の階層が提供する、C言語の公開APIのみを利用することができます。

このような構成を、Suika3では「レイヤーコンポーネントモデル」と呼んでいます。

このアプローチが優れている点は、C++のようなオブジェクト指向言語のクラスが、多対多の依存関係を考慮しなければならないという複雑さを抱える一方で、レイヤーコンポーネントモデルでは自分より１つ下と１つ上の層との関係のみを考慮すればよいため、設計・実装・改変・拡張がシンプルになることです。
フルスタックのゲームエンジンは複雑で大きなソフトウェアですが、シンプルなコンポーネントの一対一の関係でまとめあげることで、それを全体として作りやすくして、結果として移植性も高まる、というのがこの手法の趣旨です。

移植性が高まる理由は、１番下のレイヤーがOSの違いを吸収する「ハードウェア抽象化レイヤー」と呼ばれる層になっていて、それよりも上の層はOSの違いを一切気にせず設計・実装が行えるからです。


```
+-----------------------------------+
| VN対応エンジン (Suika3)                |
+-----------------------------------+
| 2Dゲームエンジン (Playfield Engine)  |
+-----------------------------------+
| スクリプト言語 (NoctLang)              |
+-----------------------------------+
| ハードウェア抽象化レイヤ (StratoHAL)   |
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
