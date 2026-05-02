Справочник по режиму NVL
========================

Suika3 поддерживает полноэкранный стиль новеллы, так называемый режим NVL.

## Обзор

```
# Новая страница.
[text action="clear"]

# Блочный стиль 1. (пока не подсвечивается расширением VS Code)
[text text=
  """
  Привет, это тест режима NVL.
  В режиме NVL окно сообщений занимает весь экран.
  По умолчанию каждый тег text добавляет перевод строки.
  """]

# Блочный стиль 2. (подсвечивается расширением VS Code)
[text text="""
  Привет, это тест режима NVL.
  В режиме NVL окно сообщений занимает весь экран.
  По умолчанию каждый тег text добавляет перевод строки.
"""]

# Продолжение абзаца в той же строке.
[text action="inline" text=" Чтобы продолжить абзац, используйте действие inline."]

# Новая страница.
[text action="clear"]
[text text="Явно очищайте окно сообщений."]
[text text="Именно поэтому NVL в Suika3 называется Page Mode."]
```

## Переключение

Добавьте два макроса в конец вашего `start.novel`:
```
#
# Макрос для запуска режима NVL.
#
[defmacro name="nvl-mode"]
   [text action="hide"]
   [wait time="0.3"] # Дождаться скрытия окна сообщений.
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
# Макрос для возврата в режим ADV.
#
[defmacro name="adv-mode"]
   [text action="hide"]
   [wait time="0.3"] # Дождаться скрытия окна сообщений.
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

Затем вызовите макрос `nvl-mode`, чтобы переключиться в режим NVL.
```
[callmacro name="nvl-mode"]
```

Если нужно вернуться в режим ADV (обычный режим сообщений), вызовите макрос `adv-mode`:
```
[callmacro name="adv-mode"]
```
