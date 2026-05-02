NVL 模式參考
==================

Suika3 支援全螢幕小說風格，也就是所謂的 NVL 模式。

## 概觀

```
# New page.
[text action="clear"]

# Block style 1. (this is not highlighted by the VS Code extension for now)
[text text=
  """
  Hello, this is NVL mode test.
  NVL mode has a fullscreen-styled message box.
  By default, each text tag will do a line feed.
  """]

# Block style 2. (this is highlighted by the VS Code extension)
[text text="""
  Hello, this is NVL mode test.
  NVL mode has a fullscreen-styled message box.
  By default, each text tag will do a line feed.
"""]

# Inline paragraph continuation.
[text action="inline" text=" To continue a paragraph, use the inline action."]

# New page.
[text action="clear"]
[text text="Please clear the message box explicitly."]
[text text="This is why NVL is called Page Mode in Suika3!"]
```

## 切換

在 `start.novel` 的結尾加入兩個巨集：
```
#
# 啟動 NVL 模式的巨集。
#
[defmacro name="nvl-mode"]
   [text action="hide"]
   [wait time="0.3"] # 等待訊息視窗隱藏。
   [config name="game.novel" value="true"]
   [config name="msgbox.image" value="system/message/msgbox-nvl.png"]
   [config name="msgbox.x" value="0"]
   [config name="msgbox.y" value="0"]
   [config name="msgbox.margin.line" value="60"]
   [config name="namebox.enable" value="false"]
   [config name="choose.box1.idle" value="system/choose/nvl.png"]
   [config name="choose.box1.hover" value="system/choose/nvl.png"]
   [config name="choose.box1.idle_anime" value="system/choose/idle-nvl.anime"]
   [config name="choose.box1.hover_anime" value="system/choose/hover-nvl.anime"]
   [config name="choose.box2.idle" value="system/choose/nvl.png"]
   [config name="choose.box2.hover" value="system/choose/nvl.png"]
   [config name="choose.box2.idle_anime" value="system/choose/idle-nvl.anime"]
   [config name="choose.box2.hover_anime" value="system/choose/hover-nvl.anime"]
   [config name="choose.box3.idle" value="system/choose/nvl.png"]
   [config name="choose.box3.hover" value="system/choose/nvl.png"]
   [config name="choose.box3.idle_anime" value="system/choose/idle-nvl.anime"]
   [config name="choose.box3.hover_anime" value="system/choose/hover-nvl.anime"]
   [config name="click.move" value="true"]
   [text action="clear"]
[endmacro]

#
# 切回 ADV 模式的巨集。
#
[defmacro name="adv-mode"]
   [text action="hide"]
   [wait time="0.3"] # 等待訊息視窗隱藏。
   [config name="game.novel" value="false"]
   [config name="msgbox.image" value="system/message/msgbox.png"]
   [config name="msgbox.x" value="0"]
   [config name="msgbox.y" value="520"]
   [config name="msgbox.margin.line" value="40"]
   [config name="namebox.enable" value="true"]
   [config name="choose.box1.idle" value="system/choose/idle.png"]
   [config name="choose.box1.hover" value="system/choose/hover.png"]
   [config name="choose.box1.idle_anime" value="system/choose/idle.anime"]
   [config name="choose.box1.hover_anime" value="system/choose/hover.anime"]
   [config name="choose.box2.idle" value="system/choose/idle.png"]
   [config name="choose.box2.hover" value="system/choose/hover.png"]
   [config name="choose.box2.idle_anime" value="system/choose/idle.anime"]
   [config name="choose.box2.hover_anime" value="system/choose/hover.anime"]
   [config name="choose.box3.idle" value="system/choose/idle.png"]
   [config name="choose.box3.hover" value="system/choose/hover.png"]
   [config name="choose.box3.idle_anime" value="system/choose/idle.anime"]
   [config name="choose.box3.hover_anime" value="system/choose/hover.anime"]
   [config name="click.move" value="false"]
[endmacro]
```

接著呼叫 `nvl-mode` 巨集，就會切換到 NVL 模式。
```
[callmacro name="nvl-mode"]
```

如果你想切回 ADV 模式（一般訊息模式），就呼叫 `adv-mode` 巨集：
```
[callmacro name="adv-mode"]
```
