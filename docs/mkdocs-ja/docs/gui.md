GUI
===

## はじめに

GUI は Suika3 の UI/UX 作成機能です。

Suika3 では、ボタンは専用の GUI モードで定義され、同期的に動作します。つまり、GUI を呼び出すと、ボタンのクリックまたはキャンセルのいずれかの結果になります。

テキストタグの実行中にボタンを表示するような非同期コールバックは、初心者のプログラマーにとって理解や管理が難しくなりやすいため、意図的に避けています。

GUI ファイルには、ボタン定義の一覧を記述します。各ボタンには、動作タイプ、通常時とホバー時の画像、追加のプロパティが含まれます。ボタンはアニメーションレイヤーに対応付けられ、ボタンの状態が変化したときにアニメーションファイルを実行できます。

---

## GUI のサンプル

```
# グローバル設定セクション。
global {
    fadein:       0.2;            # フェードイン時間（秒）。
    fadeout:      0.2;            # フェードアウト時間（秒）。
}

# ブロックはボタンを表します。
# ブロック名は自由に付けることができ、動作には影響しません。
button1 {
    # レイヤー ID（`1`-`32`）
    # GUI レイヤーでは、`1` が最前面、`32` が最背面を表します。
    id: 1;
 
    # 動作タイプ（`goto` はクリック時にラベルへ移動することを意味します）
    type: goto;

    # 移動先のラベル。
    label: button1_clicked;

    # 位置
    x: 39;
    y: 99;

    # `width:` と `height:` は画像サイズから推測できるため省略できます。

    # 画像
    image-idle:  gui/item-idle.png;    # ボタンがポイントされていないときに表示されます。
    image-hover: gui/item-hover.png;   # ボタンがポイントされているときに表示されます。
    image-press: gui/item-press.png;   # ボタンが押されているときに表示されます。

    # アニメーション
    anime-idle:  gui/item-idle.anime;   # ボタンの状態が `idle` に変化したときに実行されます。
    anime-hover: gui/item-hover.anime;  # ボタンの状態が `hover` に変化したときに実行されます。
    anime-press: gui/item-press.anime;  # ボタンが押されたときに実行されます。

    # `press` は独立した状態ではなく、`hover` 状態に付くフラグです。
    # `hover` アニメーションが実行されると、`idle` アニメーションはキャンセルされます。
    # また、`idle` アニメーションが実行されると、`hover` アニメーションはキャンセルされます。
    # ただし、`press` アニメーションは何もキャンセルしません。
}
```

`global` セクションでは、GUI のオプションを指定できます。

その他のセクションはボタン定義として解釈されます。ここでは、`button1` が位置 `(39, 99)` にボタンを作成します。ボタンがクリックされると、`goto` と呼ばれるジャンプが発生します。

---

## ボタンタイプ

| 記述                            | 意味                                                |
|---------------------------------|-----------------------------------------------------|
| type: noaction;                 | クリックできない画像。                              |
| type: goto;                     | クリック時にラベルへジャンプします。                |
| type: gui;                      | クリック時に GUI へジャンプします。                 |
| type: mastervol;                | マスター音量スライダーとして表示されます。          |
| type: bgmvol;                   | BGM 音量スライダーとして表示されます。              |
| type: sevol;                    | SE 音量スライダーとして表示されます。               |
| type: voicevol;                 | ボイス音量スライダーとして表示されます。            |
| type: charactervol;             | キャラクター音量スライダーとして表示されます。      |
| type: textspeed;                | テキスト速度スライダーとして表示されます。          |
| type: autospeed;                | オートモード速度スライダーとして表示されます。      |
| type: preview;                  | テキストプレビュー領域として表示されます。          |
| type: fullscreen;               | フルスクリーンモードボタンとして表示されます。      |
| type: window;                   | ウィンドウモードボタンとして表示されます。          |
| type: default;                  | 押すと設定をリセットします。                        |
| type: savepage;                 | セーブ/ロードページボタンとして表示されます。       |
| type: save;                     | セーブスロットとして表示されます。                  |
| type: load;                     | ロードスロットとして表示されます。                  |
| type: auto;                     | オートモードボタンとして表示されます。              |
| type: skip;                     | スキップモードボタンとして表示されます。            |
| type: title;                    | タイトルへ戻るボタンとして表示されます。            |
| type: history;                  | 履歴ボタンとして表示されます。                      |
| type: historyscroll;            | 縦方向の履歴スクロールスライダーとして表示されます。|
| type: historyscroll-horizontal; | 横方向の履歴スクロールスライダーとして表示されます。|
| type: cancel;                   | キャンセルボタンとして表示されます。                |
| type: namevar;                  | 名前変数の値をプレビューする領域として表示されます。|
| type: char;                     | 文字を入力するボタンとして表示されます。            |
| type: language                  | クリック時に言語を変更します。                      |

### `noaction`

クリックできない画像です。

```
background {
    type: noaction;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `goto`

クリック可能なボタンです。クリックされると、タグの実行は `label:` パラメータで指定されたラベルへジャンプします。

```
button1 {
    type: goto;
    label: label_clicked_button1;
    x: 0;
    y: 0;
    image-idle:  idle.png;
    image-hover: hover.png;
}
```

### `gui`

クリック可能なボタンです。クリックされると、GUI の実行は `file:` パラメータで指定された新しい GUI へ連鎖します。

```
button1 {
    type: gui;
    label: gui2.gui;
    x: 0;
    y: 0;
    image-idle:  idle.png;
    image-hover: hover.png;
}
```

### `mastervol`

マスター音量を設定するスライダーです。

```
slider1 {
    type: mastervol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # スライダーのベースバー
    image-hover: hover.png;  # スライダーのベースバー（明るい状態）
    image-knob:  knob.png;   # スライダーのつまみ
}
```

### `bgmvol`

BGM 音量を設定するスライダーです。

このスライダーは、グローバルセーブデータファイルに保存される音量を設定します。これは `[volume]` タグで設定され、ローカルセーブデータに保存される音量とは異なります。

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # スライダーのベースバー
    image-hover: hover.png;  # スライダーのベースバー（明るい状態）
    image-knob:  knob.png;   # スライダーのつまみ
}
```

### `sevol`

SE 音量を設定するスライダーです。

このスライダーは、グローバルセーブデータファイルに保存される音量を設定します。これは `[volume]` タグで設定され、ローカルセーブデータに保存される音量とは異なります。

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # スライダーのベースバー
    image-hover: hover.png;  # スライダーのベースバー（明るい状態）
    image-knob:  knob.png;   # スライダーのつまみ
}
```

### `voicevol`

ボイス音量を設定するスライダーです。

このスライダーは、グローバルセーブデータファイルに保存される音量を設定します。これは `[volume]` タグで設定され、ローカルセーブデータに保存される音量とは異なります。

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # スライダーのベースバー
    image-hover: hover.png;  # スライダーのベースバー（明るい状態）
    image-knob:  knob.png;   # スライダーのつまみ
}
```

### `charactervol`

キャラクターごとの音量を設定するスライダーです。

キャラクターのインデックスは `index:` パラメータで渡します。インデックス 0 は未定義のキャラクター用で、1-32 は定義済みキャラクター用です。

```
slider1 {
    type: charactervol;
    index: 1;  # キャラクターインデックス
    x: 0;
    y: 0;
    image-idle:  idle.png;   # スライダーのベースバー
    image-hover: hover.png;  # スライダーのベースバー（明るい状態）
    image-knob:  knob.png;   # スライダーのつまみ
}
```

### `textspeed`

テキスト速度を設定するスライダーです。

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # スライダーのベースバー
    image-hover: hover.png;  # スライダーのベースバー（明るい状態）
    image-knob:  knob.png;   # スライダーのつまみ
}
```

### `autospeed`

オートモード速度を設定するスライダーです。

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # スライダーのベースバー
    image-hover: hover.png;  # スライダーのベースバー（明るい状態）
    image-knob:  knob.png;   # スライダーのつまみ
}
```

### `preview`

フォント、言語、速度をプレビューするテキスト領域です。

```
preview1 {
    type: preview;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `fullscreen`

クリック可能なボタンです。クリックされると、エンジンはフルスクリーンモードに入ります。

```
fullscreen1 {
    type: fullscreen;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # フルスクリーンモード中に使用します。
}
```

### `window`

クリック可能なボタンです。クリックされると、エンジンはウィンドウモードに入ります。

```
window1 {
    type: window;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # ウィンドウモード中に使用します。
}
```

### `default`

クリック可能なボタンです。クリックされると、すべての設定をリセットします。

```
reset1 {
    type: default;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `savepage`

クリック可能なボタンです。クリックされると、セーブ画面のページを切り替えます。

```
savepage1 {
    type: savepage;
    index: 0; # ページインデックス（0 から）
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-active:  active.png;  # ページがアクティブなときに使用します。
}
```

### `save`

クリック可能なボタンです。クリックされると、セーブを実行します。

```
save1 {
    type: save;
    index: 0; # ページ内のインデックス。実際のセーブスロット = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/save/save-item-idle.png;
    image-hover: system/save/save-item-hover.png;

    index-x:   10;   # 番号
    index-y:   0;
    date-x:    60;   # 日付
    date-y:    0;
    thumb-x:   27;   # サムネイル
    thumb-y:   77;
    chapter-x: 260;  # チャプタータイトル
    chapter-y: 48;
    msg-x:     260;  # 最後のメッセージ
    msg-y:     96;
}
```

### `load`

クリック可能なボタンです。クリックされると、ロードを実行します。

```
load1 {
    type: load;
    index: 0; # ページ内のインデックス。実際のセーブスロット = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/load/load-item-idle.png;
    image-hover: system/load/load-item-hover.png;

    index-x:   10;   # 番号
    index-y:   0;
    date-x:    60;   # 日付
    date-y:    0;
    thumb-x:   27;   # サムネイル
    thumb-y:   77;
    chapter-x: 260;  # チャプタータイトル
    chapter-y: 48;
    msg-x:     260;  # 最後のメッセージ
    msg-y:     96;
}
```

### `auto`

クリック可能なボタンです。クリックされると、オートモードを開始します。

```
auto1 {
    type: auto;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `skip`

クリック可能なボタンです。クリックされると、スキップモードを開始します。

```
skip1 {
    type: skip;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `title`

クリック可能なボタンです。クリックされると、エンジンは指定された NovelML ファイルへ戻ります。

```
title1 {
    type: title;
    file: title.novel;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `history`

クリック可能なボタンです。メッセージ履歴の項目を表示します。

```
history {
    type: history;
    index: 0; # ページ内のインデックス
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;

    name-x: 20;   # 名前
    name-y: 10;
    text-x:  20;  # テキスト（名前がある場合）
    text-y:  50;
    msg-x:  20;   # メッセージ（名前がない場合）
    msg-y:  10;
}
```

### `historyscroll`

履歴画面用の縦方向スクロールバーです。

```
scroll1 {
    type: historyscroll;
    x: 1200;
    y: 40;

    image-idle:  system/history/history-bar-idle.png;
    image-hover: system/history/history-bar-hover.png;
    image-knob:  system/history/history-bar-knob.png;
}
```

### `historyscroll-horizontal`

縦書きの履歴画面用の横方向スクロールバーです。

```
scroll1 {
    type: historyscroll-horizontal;
    x: 40;
    y: 650;

    image-idle:  system/history/history-bar-idle.png;
    image-hover: system/history/history-bar-hover.png;
    image-knob:  system/history/history-bar-knob.png;
}
```

### `cancel`

クリック可能なボタンです。クリックされると、GUI はキャンセルされます。

```
cancel1 {
    type: cancel;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `var`

変数の値を表示するテキスト領域です。名前編集に使用します。

```
var1 {
    type: var;
    variable: variable_name;
    x: 0;
    y: 0;
    image-idle:    idle.png;
}
```

### `char`

変数に文字を追加するボタンです。名前編集に使用します。

```
char1 {
    type: char;
    variable: variable_name; # 変数名。
    msg: A;                  # 追加するテキスト。
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `language`

言語を切り替えるボタンです。

```
language_english1 {
    type: language;
    langu: en;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png; # 英語がアクティブなときに使用します。
}
```

---

## 共通のボタンオプション

| 記述                  | 意味                                                        |
|-----------------------|-------------------------------------------------------------|
| type:                 | ボタンのタイプ。                                            |
| x:                    | ボタンの X 位置。                                           |
| y:                    | ボタンの Y 位置。                                           |
| width:                | ボタンの幅。（デフォルトでは通常時画像の幅に設定されます）  |
| height:               | ボタンの高さ。                                              |
| pointse:              | ボタンがポイントされたときの効果音。                        |
| clickse:              | ボタンがクリックされたときの効果音。                        |

### pointse:

`pointse: btn-change.ogg;` は、マウスカーソルがボタンに入ったときに効果音ファイルを再生することを示します。

### clickse:

`clickse: click.ogg;` は、ボタンがクリックされたときに効果音ファイルを再生することを示します。
