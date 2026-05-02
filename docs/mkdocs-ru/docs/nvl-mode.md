NVL Mode Reference
==================

Suika3 support the full screen novel style, so called NVL mode.

## Overview

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

## Switch

Add two macros at the end of your `start.novel`:
```
#
# Macro to start NVL mode.
#
[defmacro name="nvl-mode"]
   [text action="hide"]
   [wait time="0.3"] # Wait for the message box to hide.
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
# Macro to go back ADV mode.
#
[defmacro name="adv-mode"]
   [text action="hide"]
   [wait time="0.3"] # Wait for the message box to hide.
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

Then call the `nvl-mode` macro to switch to NVL mode.
```
[callmacro name="nvl-mode"]
```

If you want to switch back to ADV mode (normal message mode), call the `adv-mode` macro:
```
[callmacro name="adv-mode"]
```
