GUI
===

## 简介

GUI 是 Suika3 的 UI/UX 建置功能。

在 Suika3 中，按钮会定义在专门的 GUI 模式里，并以同步方式运作，也就是说，呼叫一个 GUI 会导致按钮点选或取消。

像是在文字标签执行时同时显示按钮这类非同步回呼，则是刻意避免的，因为这对初学者来说可能不太容易理解和管理。

GUI 档案包含一串按钮定义。每个按钮都包含行为型别、闲置与滑入图片，以及额外属性。按钮会对应到动画图层，而当按钮状态改变时，也可以触发动画档。

---

## GUI 范例

```
# 全域设定区段。
global {
    fadein:       0.2;            # 淡入时间（秒）。
    fadeout:      0.2;            # 淡出时间（秒）。
}

# 区块描述一个按钮。
# 区块名称可以随意命名，不会影响任何事。
button1 {
    # 图层 ID（`1`-`32`）
    # `1` 表示 GUI 图层最上层，`32` 表示最下层。
    id: 1;
 
    # 行为型别（`goto` 代表点选时跳到某个标签。）
    type: goto;

    # 要前往的标签。
    label: button1_clicked;

    # 位置
    x: 39;
    y: 99;

    # `width:` 与 `height:` 可省略，因为可以从图片大小推算。

    # 图片
    image-idle:  gui/item-idle.png;    # 按钮未被指到时显示。
    image-hover: gui/item-hover.png;   # 按钮被指到时显示。
    image-press: gui/item-press.png;   # 按钮被按下时显示。

    # 动画
    anime-idle:  gui/item-idle.anime;   # 按钮状态变成 `idle` 时执行。
    anime-hover: gui/item-hover.anime;  # 按钮状态变成 `hover` 时执行。
    anime-press: gui/item-press.anime;  # 按钮被按下时执行。

    # 注意 `press` 不是独立状态，而是 `hover` 状态的附加旗标。
    # 当 `hover` 动画执行时，`idle` 动画会被取消。
    # 而当 `idle` 动画执行时，`hover` 动画会被取消。
    # 但是 `press` 动画不会取消任何东西。
}
```

在 `global` 区段中，你可以指定 GUI 的选项。

其他区段则会被解读为按钮定义。
这里的 `button1` 会在位置 `(39, 99)` 建立一个按钮。
如果按钮被点选，就会发生称为 `goto` 的跳转。

---

## 按钮型别

| 说明                           | 意义                                               |
|--------------------------------|----------------------------------------------------|
| type: noaction;                | 不可点选的图片。                                   |
| type: goto;                    | 点选时跳到某个标签。                               |
| type: gui;                     | 点选时跳到某个 GUI。                               |
| type: mastervol;               | 显示为主音量滑杆。                                 |
| type: bgmvol;                  | 显示为 BGM 音量滑杆。                              |
| type: sevol;                   | 显示为 SE 音量滑杆。                               |
| type: voicevol;                | 显示为语音音量滑杆。                               |
| type: charactervol;            | 显示为角色音量滑杆。                               |
| type: textspeed;               | 显示为文字速度滑杆。                               |
| type: autospeed;               | 显示为自动模式速度滑杆。                           |
| type: preview;                 | 显示为文字预览区域。                               |
| type: fullscreen;              | 显示为全萤幕模式按钮。                             |
| type: window;                  | 显示为视窗模式按钮。                               |
| type: default;                 | 按下时重设设定。                                   |
| type: savepage;                | 显示为储存/读取页面按钮。                          |
| type: save;                    | 显示为储存槽。                                     |
| type: load;                    | 显示为读取槽。                                     |
| type: auto;                    | 显示为自动模式按钮。                               |
| type: skip;                    | 显示为跳过模式按钮。                               |
| type: title;                   | 显示为回到标题按钮。                               |
| type: history;                 | 显示为历史纪录按钮。                               |
| type: historyscroll;           | 显示为垂直历史纪录卷轴。                           |
| type: historyscroll-horizontal;| 显示为水平历史纪录卷轴。                           |
| type: cancel;                  | 显示为取消按钮。                                   |
| type: namevar;                 | 显示为预览名称变数值的区域。                       |
| type: char;                    | 显示为输入字元的按钮。                             |
| type: language                 | 点选时切换语言。                                   |

### `noaction`

不可点选的图片。

```
background {
    type: noaction;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `goto`

可点选的按钮。点选时，标签执行会跳到 `label:` 引数指定的标签。

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

可点选的按钮。点选时，GUI 执行会接续到 `file:` 引数指定的新 GUI。

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

用来设定主音量的滑杆。

```
slider1 {
    type: mastervol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑杆基底条
    image-hover: hover.png;  # 滑杆基底条（高亮）
    image-knob:  knob.png;   # 滑杆旋钮
}
```

### `bgmvol`

用来设定 BGM 音量的滑杆。

这个滑杆会把音量储存在全域存档中。
它和 `[volume]` 标签设定的音量不同，后者会储存在本地存档中。

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑杆基底条
    image-hover: hover.png;  # 滑杆基底条（高亮）
    image-knob:  knob.png;   # 滑杆旋钮
}
```

### `sevol`

用来设定 SE 音量的滑杆。

这个滑杆会把音量储存在全域存档中。
它和 `[volume]` 标签设定的音量不同，后者会储存在本地存档中。

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑杆基底条
    image-hover: hover.png;  # 滑杆基底条（高亮）
    image-knob:  knob.png;   # 滑杆旋钮
}
```

### `voicevol`

用来设定 SE 音量的滑杆。

这个滑杆会把音量储存在全域存档中。
它和 `[volume]` 标签设定的音量不同，后者会储存在本地存档中。

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑杆基底条
    image-hover: hover.png;  # 滑杆基底条（高亮）
    image-knob:  knob.png;   # 滑杆旋钮
}
```

### `charactervol`

用来设定每个角色个别音量的滑杆。

角色索引由 `index:` 引数传入。
index 0 代表未定义角色，1-32 则代表已定义角色。

```
slider1 {
    type: charactervol;
    index: 1;  # 角色索引
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑杆基底条
    image-hover: hover.png;  # 滑杆基底条（高亮）
    image-knob:  knob.png;   # 滑杆旋钮
}
```

### `textspeed`

用来设定文字速度的滑杆。

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑杆基底条
    image-hover: hover.png;  # 滑杆基底条（高亮）
    image-knob:  knob.png;   # 滑杆旋钮
}
```

### `autospeed`

用来设定自动模式速度的滑杆。

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # 滑杆基底条
    image-hover: hover.png;  # 滑杆基底条（高亮）
    image-knob:  knob.png;   # 滑杆旋钮
}
```

### `preview`

用来预览字型、语言与速度的文字区域。

```
preview1 {
    type: preview;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `fullscreen`

可点选的按钮。
点选时，引擎会进入全萤幕模式。

```
fullscreen1 {
    type: fullscreen;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # 全萤幕模式时使用。
}
```

### `window`

可点选的按钮。
点选时，引擎会进入视窗模式。

```
window1 {
    type: window;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # 视窗模式时使用。
}
```

### `default`

可点选的按钮。
点选时，会重设所有设定。

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

可点选的按钮。
点选时，会切换储存画面的页面。

```
savepage1 {
    type: savepage;
    index: 0; # 页面索引（0-）
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-active:  active.png;  # 页面启用时使用。
}
```

### `save`

可点选的按钮。
点选时，会执行储存。

```
save1 {
    type: save;
    index: 0; # 页面中的索引。实际储存槽 = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/save/save-item-idle.png;
    image-hover: system/save/save-item-hover.png;

    index-x:   10;   # 编号
    index-y:   0;
    date-x:    60;   # 日期
    date-y:    0;
    thumb-x:   27;   # 缩图
    thumb-y:   77;
    chapter-x: 260;  # 章节标题
    chapter-y: 48;
    msg-x:     260;  # 最后讯息
    msg-y:     96;
}
```

### `load`

可点选的按钮。
点选时，会执行读取。

```
load1 {
    type: load;
    index: 0; # 页面中的索引。实际储存槽 = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/load/load-item-idle.png;
    image-hover: system/load/load-item-hover.png;

    index-x:   10;   # 编号
    index-y:   0;
    date-x:    60;   # 日期
    date-y:    0;
    thumb-x:   27;   # 缩图
    thumb-y:   77;
    chapter-x: 260;  # 章节标题
    chapter-y: 48;
    msg-x:     260;  # 最后讯息
    msg-y:     96;
}
```

### `auto`

可点选的按钮。
点选时，会启动自动模式。

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

可点选的按钮。
点选时，会启动跳过模式。

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

可点选的按钮。
点选时，引擎会回到指定的 NovelML 档案。

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

可点选的按钮。
它会显示一则讯息历史专案。

```
history {
    type: history;
    index: 0; # 页面中的索引
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;

    name-x: 20;   # 名称
    name-y: 10;
    text-x:  20;  # 有名称时的文字
    text-y:  50;
    msg-x:  20;   # 无名称时的讯息
    msg-y:  10;
}
```

### `historyscroll`

用于历史纪录画面的垂直卷轴。

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

用于直排历史纪录画面的水平卷轴。

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

可点选的按钮。
点选时，GUI 会被取消。

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

用来显示变数值的文字区域。
它用于名称编辑。

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

用来把字元附加到变数上的按钮。
它用于名称编辑。

```
char1 {
    type: char;
    variable: variable_name; # 变数名称。
    msg: A;                  # 要附加的文字。
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `language`

用来切换语言的按钮。

```
language_english1 {
    type: language;
    langu: en;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png; # 英文启用时使用。
}
```

---

## 常见按钮选项

| 说明                  | 意义                                                        |
|-----------------------|-------------------------------------------------------------|
| type:                 | 按钮的型别。                                                |
| x:                    | 按钮的 X 座标。                                             |
| y:                    | 按钮的 Y 座标。                                             |
| width:                | 按钮宽度。（预设会设成 idle 图片宽度）                      |
| height:               | 按钮高度。                                                  |
| pointse:              | 按钮被指到时播放的音效。                                    |
| clickse:              | 按钮被点选时播放的音效。                                    |

### pointse:

`pointse: btn-change.ogg;` 表示滑鼠游标进入按钮时会播放这个音效档。

### clickse:

`clickse: click.ogg;` 表示按钮被点选时会播放这个音效档。
