NVLモード リファレンス
====================

Suika3は全画面小説スタイル（NVLモード）に対応しています。

## 概要

```
# 新しいページ
[text action="clear"]

# ブロック形式1 (現在のところ VS Code 拡張機能ではハイライトされません)
[text text=
  """
  これはNVLモードのテストです。
  NVLモードはフルスクリーンのメッセージボックスを持ちます。
  デフォルトでは、各テキストタグは改行を行います。
  """]

# ブロック形式2 (VS Code 拡張機能でハイライトされます)
[text text="""
  これはNVLモードのテストです。
  NVLモードはフルスクリーンのメッセージボックスを持ちます。
  デフォルトでは、各テキストタグは改行を行います。
"""]

# インラインの段落継続
[text action="inline" text=" 段落を続行するには、inline アクションを使用してください。"]

# 新しいページ
[text action="clear"]
[text text="メッセージボックスを明示的にクリアしてください。"]
[text text="これが Suika3 でNVLがページモードと呼ばれる理由です！"]
```

## ADV/NVL切り替え

`start.novel` の末尾に2つのマクロを追加してください：

```
#
# NVLモードを開始するマクロ
#
[defmacro name="nvl-mode"]
   [text action="hide"]
   [wait time="0.3"] # メッセージボックスが非表示になるのを待ちます
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
# ADVモードに戻すマクロ
#
[defmacro name="adv-mode"]
   [text action="hide"]
   [wait time="0.3"] # メッセージボックスが非表示になるのを待ちます
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

次に `nvl-mode` マクロを呼び出して、NVLモードに切り替えます。

```
[callmacro name="nvl-mode"]
```

ADVモード（通常のメッセージモード）に戻したい場合は、`adv-mode` マクロを呼び出してください：

```
[callmacro name="adv-mode"]
```
