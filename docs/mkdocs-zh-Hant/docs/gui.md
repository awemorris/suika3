GUI
===

## 簡介

GUI 是 Suika3 的 UI/UX 建置功能。

在 Suika3 中，按鈕會定義在專門的 GUI 模式裡，並以同步方式運作，也就是說，呼叫一個 GUI 會導致按鈕點選或取消。

像是在文字標籤執行時同時顯示按鈕這類非同步回呼，則是刻意避免的，因為這對初學者來說可能不太容易理解和管理。

GUI 檔案包含一串按鈕定義。每個按鈕都包含行為型別、閒置與滑入圖片，以及額外屬性。按鈕會對應到動畫圖層，而當按鈕狀態改變時，也可以觸發動畫檔。

---

## GUI 範例

```
# 全域設定區段。
global {
    fadein:       0.2;            # 淡入時間（秒）。
    fadeout:      0.2;            # 淡出時間（秒）。
}

# 區塊描述一個按鈕。
# 區塊名稱可以隨意命名，不會影響任何事。
button1 {
    # 圖層 ID（`1`-`32`）
    # `1` 表示 GUI 圖層最上層，`32` 表示最下層。
    id: 1;
 
    # 行為型別（`goto` 代表點選時跳到某個標籤。）
    type: goto;

    # 要前往的標籤。
    label: button1_clicked;

    # 位置
    x: 39;
    y: 99;

    # `width:` 與 `height:` 可省略，因為可以從圖片大小推算。

    # 圖片
    image-idle:  gui/item-idle.png;    # 按鈕未被指到時顯示。
    image-hover: gui/item-hover.png;   # 按鈕被指到時顯示。
    image-press: gui/item-press.png;   # 按鈕被按下時顯示。

    # 動畫
    anime-idle:  gui/item-idle.anime;   # 按鈕狀態變成 `idle` 時執行。
    anime-hover: gui/item-hover.anime;  # 按鈕狀態變成 `hover` 時執行。
    anime-press: gui/item-press.anime;  # 按鈕被按下時執行。

    # 注意 `press` 不是獨立狀態，而是 `hover` 狀態的附加旗標。
    # 當 `hover` 動畫執行時，`idle` 動畫會被取消。
    # 而當 `idle` 動畫執行時，`hover` 動畫會被取消。
    # 但是 `press` 動畫不會取消任何東西。
}
```

在 `global` 區段中，你可以指定 GUI 的選項。

其他區段則會被解讀為按鈕定義。
這裡的 `button1` 會在位置 `(39, 99)` 建立一個按鈕。
如果按鈕被點選，就會發生稱為 `goto` 的跳轉。

---

## 按鈕型別

| 說明                           | 意義                                               |
|--------------------------------|----------------------------------------------------|
| type: noaction;                | 不可點選的圖片。                                   |
| type: goto;                    | 點選時跳到某個標籤。                               |
| type: gui;                     | 點選時跳到某個 GUI。                               |
| type: mastervol;               | 顯示為主音量滑桿。                                 |
| type: bgmvol;                  | 顯示為 BGM 音量滑桿。                              |
| type: sevol;                   | 顯示為 SE 音量滑桿。                               |
| type: voicevol;                | 顯示為語音音量滑桿。                               |
| type: charactervol;            | 顯示為角色音量滑桿。                               |
| type: textspeed;               | 顯示為文字速度滑桿。                               |
| type: autospeed;               | 顯示為自動模式速度滑桿。                           |
| type: preview;                 | 顯示為文字預覽區域。                               |
| type: fullscreen;              | 顯示為全螢幕模式按鈕。                             |
| type: window;                  | 顯示為視窗模式按鈕。                               |
| type: default;                 | 按下時重設設定。                                   |
| type: savepage;                | 顯示為儲存/讀取頁面按鈕。                          |
| type: save;                    | 顯示為儲存槽。                                     |
| type: load;                    | 顯示為讀取槽。                                     |
| type: auto;                    | 顯示為自動模式按鈕。                               |
| type: skip;                    | 顯示為跳過模式按鈕。                               |
| type: title;                   | 顯示為回到標題按鈕。                               |
| type: history;                 | 顯示為歷史紀錄按鈕。                               |
| type: historyscroll;           | 顯示為垂直歷史紀錄卷軸。                           |
| type: historyscroll-horizontal;| 顯示為水平歷史紀錄卷軸。                           |
| type: cancel;                  | 顯示為取消按鈕。                                   |
| type: namevar;                 | 顯示為預覽名稱變數值的區域。                       |
| type: char;                    | 顯示為輸入字元的按鈕。                             |
| type: language                 | 點選時切換語言。                                   |

### `noaction`

不可點選的圖片。

```
background {
    type: noaction;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `goto`

可點選的按鈕。點選時，標籤執行會跳到 `label:` 引數指定的標籤。

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

可點選的按鈕。點選時，GUI 執行會接續到 `file:` 引數指定的新 GUI。

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

用來設定主音量的滑桿。

```
slider1 {
    type: mastervol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑桿基底條
    image-hover: hover.png;  # 滑桿基底條（高亮）
    image-knob:  knob.png;   # 滑桿旋鈕
}
```

### `bgmvol`

用來設定 BGM 音量的滑桿。

這個滑桿會把音量儲存在全域存檔中。
它和 `[volume]` 標籤設定的音量不同，後者會儲存在本地存檔中。

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑桿基底條
    image-hover: hover.png;  # 滑桿基底條（高亮）
    image-knob:  knob.png;   # 滑桿旋鈕
}
```

### `sevol`

用來設定 SE 音量的滑桿。

這個滑桿會把音量儲存在全域存檔中。
它和 `[volume]` 標籤設定的音量不同，後者會儲存在本地存檔中。

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑桿基底條
    image-hover: hover.png;  # 滑桿基底條（高亮）
    image-knob:  knob.png;   # 滑桿旋鈕
}
```

### `voicevol`

用來設定 SE 音量的滑桿。

這個滑桿會把音量儲存在全域存檔中。
它和 `[volume]` 標籤設定的音量不同，後者會儲存在本地存檔中。

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑桿基底條
    image-hover: hover.png;  # 滑桿基底條（高亮）
    image-knob:  knob.png;   # 滑桿旋鈕
}
```

### `charactervol`

用來設定每個角色個別音量的滑桿。

角色索引由 `index:` 引數傳入。
index 0 代表未定義角色，1-32 則代表已定義角色。

```
slider1 {
    type: charactervol;
    index: 1;  # 角色索引
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑桿基底條
    image-hover: hover.png;  # 滑桿基底條（高亮）
    image-knob:  knob.png;   # 滑桿旋鈕
}
```

### `textspeed`

用來設定文字速度的滑桿。

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑桿基底條
    image-hover: hover.png;  # 滑桿基底條（高亮）
    image-knob:  knob.png;   # 滑桿旋鈕
}
```

### `autospeed`

用來設定自動模式速度的滑桿。

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑桿基底條
    image-hover: hover.png;  # 滑桿基底條（高亮）
    image-knob:  knob.png;   # 滑桿旋鈕
}
```

### `preview`

用來預覽字型、語言與速度的文字區域。

```
preview1 {
    type: preview;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `fullscreen`

可點選的按鈕。
點選時，引擎會進入全螢幕模式。

```
fullscreen1 {
    type: fullscreen;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # 全螢幕模式時使用。
}
```

### `window`

可點選的按鈕。
點選時，引擎會進入視窗模式。

```
window1 {
    type: window;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # 視窗模式時使用。
}
```

### `default`

可點選的按鈕。
點選時，會重設所有設定。

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

可點選的按鈕。
點選時，會切換儲存畫面的頁面。

```
savepage1 {
    type: savepage;
    index: 0; # 頁面索引（0-）
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-active:  active.png;  # 頁面啟用時使用。
}
```

### `save`

可點選的按鈕。
點選時，會執行儲存。

```
save1 {
    type: save;
    index: 0; # 頁面中的索引。實際儲存槽 = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/save/save-item-idle.png;
    image-hover: system/save/save-item-hover.png;

    index-x:   10;   # 編號
    index-y:   0;
    date-x:    60;   # 日期
    date-y:    0;
    thumb-x:   27;   # 縮圖
    thumb-y:   77;
    chapter-x: 260;  # 章節標題
    chapter-y: 48;
    msg-x:     260;  # 最後訊息
    msg-y:     96;
}
```

### `load`

可點選的按鈕。
點選時，會執行讀取。

```
load1 {
    type: load;
    index: 0; # 頁面中的索引。實際儲存槽 = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/load/load-item-idle.png;
    image-hover: system/load/load-item-hover.png;

    index-x:   10;   # 編號
    index-y:   0;
    date-x:    60;   # 日期
    date-y:    0;
    thumb-x:   27;   # 縮圖
    thumb-y:   77;
    chapter-x: 260;  # 章節標題
    chapter-y: 48;
    msg-x:     260;  # 最後訊息
    msg-y:     96;
}
```

### `auto`

可點選的按鈕。
點選時，會啟動自動模式。

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

可點選的按鈕。
點選時，會啟動跳過模式。

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

可點選的按鈕。
點選時，引擎會回到指定的 NovelML 檔案。

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

可點選的按鈕。
它會顯示一則訊息歷史專案。

```
history {
    type: history;
    index: 0; # 頁面中的索引
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;

    name-x: 20;   # 名稱
    name-y: 10;
    text-x:  20;  # 有名稱時的文字
    text-y:  50;
    msg-x:  20;   # 無名稱時的訊息
    msg-y:  10;
}
```

### `historyscroll`

用於歷史紀錄畫面的垂直卷軸。

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

用於直排歷史紀錄畫面的水平卷軸。

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

可點選的按鈕。
點選時，GUI 會被取消。

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

用來顯示變數值的文字區域。
它用於名稱編輯。

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

用來把字元附加到變數上的按鈕。
它用於名稱編輯。

```
char1 {
    type: char;
    variable: variable_name; # 變數名稱。
    msg: A;                  # 要附加的文字。
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `language`

用來切換語言的按鈕。

```
language_english1 {
    type: language;
    langu: en;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png; # 英文啟用時使用。
}
```

---

## 常見按鈕選項

| 說明                  | 意義                                                        |
|-----------------------|-------------------------------------------------------------|
| type:                 | 按鈕的型別。                                                |
| x:                    | 按鈕的 X 座標。                                             |
| y:                    | 按鈕的 Y 座標。                                             |
| width:                | 按鈕寬度。（預設會設成 idle 圖片寬度）                      |
| height:               | 按鈕高度。                                                  |
| pointse:              | 按鈕被指到時播放的音效。                                    |
| clickse:              | 按鈕被點選時播放的音效。                                    |

### pointse:

`pointse: btn-change.ogg;` 表示滑鼠遊標進入按鈕時會播放這個音效檔。

### clickse:

`clickse: click.ogg;` 表示按鈕被點選時會播放這個音效檔。
