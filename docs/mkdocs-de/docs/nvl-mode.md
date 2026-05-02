NVL-Modus-Referenz
==================

Suika3 unterstützt den bildschirmfüllenden Novel-Stil, den sogenannten NVL-Modus.

## Überblick

```
# Neue Seite.
[text action="clear"]

# Blockstil 1. (wird von der VS-Code-Erweiterung derzeit noch nicht hervorgehoben)
[text text=
  """
  Hallo, dies ist ein Test des NVL-Modus.
  Der NVL-Modus hat ein bildschirmfüllendes Textfenster.
  Standardmäßig erzeugt jeder Text-Tag einen Zeilenumbruch.
  """]

# Blockstil 2. (wird von der VS-Code-Erweiterung hervorgehoben)
[text text="""
  Hallo, dies ist ein Test des NVL-Modus.
  Der NVL-Modus hat ein bildschirmfüllendes Textfenster.
  Standardmäßig erzeugt jeder Text-Tag einen Zeilenumbruch.
"""]

# Inline-Fortsetzung eines Absatzes.
[text action="inline" text=" Um einen Absatz fortzusetzen, verwende die Inline-Aktion."]

# Neue Seite.
[text action="clear"]
[text text="Bitte leere das Textfenster ausdrücklich."]
[text text="Darum wird NVL in Suika3 als Page Mode bezeichnet!"]
```

## Umschalten

Füge am Ende deiner `start.novel` zwei Makros hinzu:
```
#
# Makro zum Starten des NVL-Modus.
#
[defmacro name="nvl-mode"]
   [text action="hide"]
   [wait time="0.3"] # Auf das Ausblenden des Textfensters warten.
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
# Makro, um in den ADV-Modus zurückzukehren.
#
[defmacro name="adv-mode"]
   [text action="hide"]
   [wait time="0.3"] # Auf das Ausblenden des Textfensters warten.
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

Rufe dann das Makro `nvl-mode` auf, um in den NVL-Modus zu wechseln.
```
[callmacro name="nvl-mode"]
```

Wenn du zurück in den ADV-Modus (normaler Nachrichtenmodus) wechseln möchtest, rufe das Makro `adv-mode` auf:
```
[callmacro name="adv-mode"]
```
