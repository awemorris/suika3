Config Reference
================

`config.ini` is the main configuration file for your Suika3 game.
It controls everything from window titles and fonts to UI layout, sound volumes, and character behavior.

Lines starting with `#` are comments. To disable an optional setting, prefix it with `#`.

---

## Game Settings

| Name                       | Type       | Description                                                   |
|----------------------------|------------|---------------------------------------------------------------|
| game.novel                 | Boolean    | Enable NVL (full-screen novel) mode. (`true`/`false`)         |
| game.locale                | String     | Force the language over the system locale setting.            |
| game.title                 | String     | Game title (default, used as fallback).                       |
| game.title.en              | String     | Localized game title in English.                              |
| game.title.fr              | String     | Localized game title in French.                               |
| game.title.es              | String     | Localized game title in Spanish.                              |
| game.title.de              | String     | Localized game title in German.                               |
| game.title.it              | String     | Localized game title in Italian.                              |
| game.title.el              | String     | Localized game title in Greek.                                |
| game.title.ru              | String     | Localized game title in Russian.                              |
| game.title.zh_cn           | String     | Localized game title in Simplified Chinese.                   |
| game.title.zh_tw           | String     | Localized game title in Traditional Chinese.                  |
| game.title.ja              | String     | Localized game title in Japanese.                             |

### game.locale

| Value     | Description                        |
|-----------|------------------------------------|
| (empty)   | Use the system locale setting.     |
| `en`      | Fix to English.                    |
| `en-us`   | Fix to English (US).               |
| `en-gb`   | Fix to English (UK).               |
| `en-au`   | Fix to English (Australia).        |
| `en-nz`   | Fix to English (New Zealand).      |
| `fr`      | Fix to French.                     |
| `fr-fr`   | Fix to French (France).            |
| `fr-ca`   | Fix to French (Canada).            |
| `es`      | Fix to Spanish.                    |
| `es-es`   | Fix to Spanish (Spain).            |
| `es-la`   | Fix to Spanish (Latin America).    |
| `de`      | Fix to German.                     |
| `it`      | Fix to Italian.                    |
| `el`      | Fix to Greek.                      |
| `ru`      | Fix to Russian.                    |
| `zh-cn`   | Fix to Simplified Chinese.         |
| `zh-tw`   | Fix to Traditional Chinese.        |
| `ja`      | Fix to Japanese.                   |

---

## Font File Settings

Up to four TrueType fonts can be loaded. Font 1 is the default.
In messages, you can switch fonts inline with `\f{N}` (e.g., `\f{2}`).

| Name       | Type   | Description                        |
|------------|--------|------------------------------------|
| font.ttf1  | String | Path to font 1 (default font).     |
| font.ttf2  | String | Path to font 2 (optional).         |
| font.ttf3  | String | Path to font 3 (optional).         |
| font.ttf4  | String | Path to font 4 (optional).         |

Example:
```
font.ttf1=system/font/rounded-l-mplus-1c-bold.ttf
#font.ttf2=
```

---

## Message Box Settings

The message box displays dialogue text during the game.

### Layout

| Name                  | Type    | Description                                              |
|-----------------------|---------|----------------------------------------------------------|
| msgbox.image          | String  | Path to the message box image.                           |
| msgbox.anime.hide     | String  | Animation file played when the message box hides.        |
| msgbox.anime.show     | String  | Animation file played when the message box appears.      |
| msgbox.x              | Integer | X position of the message box in pixels.                 |
| msgbox.y              | Integer | Y position of the message box in pixels.                 |
| msgbox.margin.left    | Integer | Left margin for text inside the message box.             |
| msgbox.margin.top     | Integer | Top margin for text inside the message box.              |
| msgbox.margin.right   | Integer | Right margin for text inside the message box.            |
| msgbox.margin.bottom  | Integer | Bottom margin for text inside the message box.           |
| msgbox.margin.line    | Integer | Line height in pixels (includes character height).       |
| msgbox.margin.char    | Integer | Additional horizontal spacing between characters.        |

### Font

| Name                       | Type    | Description                                           |
|----------------------------|---------|-------------------------------------------------------|
| msgbox.font.select         | Integer | Font to use (1–4).                                   |
| msgbox.font.size           | Integer | Font size in pixels.                                  |
| msgbox.font.r/g/b          | Integer | Font color (0–255 each).                              |
| msgbox.font.outline.width  | Integer | Outline width in pixels (0 to disable).               |
| msgbox.font.outline.r/g/b  | Integer | Outline color (0–255 each).                           |
| msgbox.font.ruby           | Integer | Ruby (furigana) font size in pixels.                  |
| msgbox.font.tategaki       | Boolean | Enable vertical (tategaki) writing.                   |

### Character Background Fill

Draws a colored fill behind each character glyph.

| Name              | Type    | Description                                 |
|-------------------|---------|---------------------------------------------|
| msgbox.fill.enable | Boolean | Enable character background fill.           |
| msgbox.fill.r/g/b  | Integer | Fill color (0–255 each).                    |

### Dimming (NVL Mode)

In NVL mode, previously displayed paragraphs can be dimmed to emphasize the current line.

| Name                        | Type    | Description                                        |
|-----------------------------|---------|----------------------------------------------------|
| msgbox.dim.enable           | Boolean | Enable dimming of previous paragraphs.             |
| msgbox.dim.r/g/b            | Integer | Dim font color (0–255 each).                       |
| msgbox.dim.outline.width    | Integer | Dim outline width in pixels.                       |
| msgbox.dim.outline.r/g/b   | Integer | Dim outline color (0–255 each).                    |

### Read Messages

Already-seen messages can be displayed in a different color.

| Name                         | Type    | Description                                       |
|------------------------------|---------|---------------------------------------------------|
| msgbox.seen.enable           | Boolean | Enable different color for seen messages.         |
| msgbox.seen.r/g/b            | Integer | Seen message font color (0–255 each).             |
| msgbox.seen.outline.width    | Integer | Seen message outline width in pixels.             |
| msgbox.seen.outline.r/g/b   | Integer | Seen message outline color (0–255 each).          |

### Other

| Name                  | Type    | Description                                              |
|-----------------------|---------|----------------------------------------------------------|
| msgbox.skip_unseen    | Boolean | Allow skip mode to advance through unseen messages.      |

---

## Name Box Settings

The name box displays the speaker's name above the message box.
Disable it (`namebox.enable=false`) for full-screen NVL-style games.

### Layout

| Name                   | Type    | Description                                             |
|------------------------|---------|---------------------------------------------------------|
| namebox.enable         | Boolean | Enable the name box.                                    |
| namebox.image          | String  | Path to the name box image.                             |
| namebox.anime.hide     | String  | Animation file played when the name box hides.          |
| namebox.anime.show     | String  | Animation file played when the name box appears.        |
| namebox.x              | Integer | X position of the name box in pixels.                   |
| namebox.y              | Integer | Y position of the name box in pixels.                   |
| namebox.margin.top     | Integer | Top margin for text inside the name box.                |
| namebox.margin.left    | Integer | Left margin for text inside the name box.               |
| namebox.centering      | Boolean | Center the name text horizontally inside the name box.  |

### Font

| Name                        | Type    | Description                                          |
|-----------------------------|---------|------------------------------------------------------|
| namebox.font.select         | Integer | Font to use (1–4).                                   |
| namebox.font.size           | Integer | Font size in pixels.                                 |
| namebox.font.r/g/b          | Integer | Font color (0–255 each).                             |
| namebox.font.outline.width  | Integer | Outline width in pixels (0 to disable).              |
| namebox.font.outline.r/g/b  | Integer | Outline color (0–255 each).                          |
| namebox.font.ruby           | Integer | Ruby font size in pixels.                            |
| namebox.font.tategaki       | Boolean | Enable vertical (tategaki) writing.                  |

---

## Click Animation Settings

A small animation shown at the bottom of the screen to prompt the player to click.

| Name              | Type    | Description                                                   |
|-------------------|---------|---------------------------------------------------------------|
| click.x           | Integer | X position of the click animation.                           |
| click.y           | Integer | Y position of the click animation.                           |
| click.interval    | Float   | Duration of each animation frame cycle in seconds.           |
| click.image1–16   | String  | Image paths for each frame (at least 1 required).            |
| click.move        | Boolean | If `true`, the animation moves with the text cursor position. |

---

## Choose Box Settings

Up to 8 choice boxes can be displayed by `[choose]`. Each box has its own position and images.

### Font

| Name                           | Type    | Description                                         |
|--------------------------------|---------|-----------------------------------------------------|
| choose.font.select             | Integer | Font to use (1–4).                                  |
| choose.font.size               | Integer | Font size in pixels.                                |
| choose.font.idle.r/g/b         | Integer | Font color when not hovered (0–255 each).           |
| choose.font.idle.outline.width | Integer | Outline width when not hovered.                     |
| choose.font.idle.outline.r/g/b | Integer | Outline color when not hovered.                     |
| choose.font.hover.r/g/b        | Integer | Font color when hovered (0–255 each).               |
| choose.font.hover.outline.width| Integer | Outline width when hovered.                         |
| choose.font.hover.outline.r/g/b| Integer | Outline color when hovered.                         |
| choose.font.ruby               | Integer | Ruby font size in pixels.                           |
| choose.font.tategaki           | Boolean | Enable vertical (tategaki) writing.                 |

### Sound Effects

| Name              | Type   | Description                                        |
|-------------------|--------|----------------------------------------------------|
| choose.change_se  | String | Sound effect played when moving between choices.   |
| choose.click_se   | String | Sound effect played when a choice is selected.     |

### Box Layout (1–8)

Each box (1–8) shares the same set of keys. Replace `N` with the box number.

| Name                    | Type    | Description                                          |
|-------------------------|---------|------------------------------------------------------|
| choose.boxN.idle        | String  | Image path for the idle (non-hovered) state.         |
| choose.boxN.hover       | String  | Image path for the hovered state.                    |
| choose.boxN.x           | Integer | X position in pixels.                               |
| choose.boxN.y           | Integer | Y position in pixels.                               |
| choose.boxN.margin.top  | Integer | Top margin for the label text inside the box.        |
| choose.boxN.idle_anime  | String  | Animation file for the idle state.                   |
| choose.boxN.hover_anime | String  | Animation file for the hovered state.                |

---

## Save Data Settings

| Name               | Type    | Description                                             |
|--------------------|---------|---------------------------------------------------------|
| save.thumb.width   | Integer | Width of save slot thumbnail in pixels.                 |
| save.thumb.height  | Integer | Height of save slot thumbnail in pixels.                |
| save.new_image     | String  | Path to a "New" badge image shown on unused save slots. |

---

## System Button Settings

The system button (SysBtn) is a hamburger-style menu button, typically in the top-left corner.
It appears on mouse movement and auto-hides after inactivity. See also: [sysmenu.md](sysmenu.md).

| Name                  | Type    | Description                                                 |
|-----------------------|---------|-------------------------------------------------------------|
| sysbtn.enable         | Boolean | Enable the system button. Set to `false` for kiosk/demo.   |
| sysbtn.idle           | String  | Image path for the idle state.                              |
| sysbtn.hover          | String  | Image path for the hovered state.                           |
| sysbtn.anime.out      | String  | Animation while button is hidden.                           |
| sysbtn.anime.fadein   | String  | Animation while button is appearing.                        |
| sysbtn.anime.appear   | String  | Animation while button is visible but not hovered.          |
| sysbtn.anime.hover    | String  | Animation while button is hovered.                          |
| sysbtn.anime.fadeout  | String  | Animation while button is disappearing.                     |
| sysbtn.x              | Integer | X position in pixels.                                       |
| sysbtn.y              | Integer | Y position in pixels.                                       |
| sysbtn.width          | Integer | Width of the hit area in pixels.                            |
| sysbtn.height         | Integer | Height of the hit area in pixels.                           |
| sysbtn.enter_se       | String  | Sound effect when the cursor enters the button.             |
| sysbtn.leave_se       | String  | Sound effect when the cursor leaves the button.             |
| sysbtn.click_se       | String  | Sound effect when the button is clicked.                    |

---

## Auto Mode Settings

Auto mode advances text automatically without player input.

| Name                  | Type    | Description                                                 |
|-----------------------|---------|-------------------------------------------------------------|
| automode.image        | String  | Path to the auto mode banner image.                         |
| automode.anime.hide   | String  | Animation played when the banner hides.                     |
| automode.anime.show   | String  | Animation played when the banner appears.                   |
| automode.x            | Integer | X position of the banner in pixels.                         |
| automode.y            | Integer | Y position of the banner in pixels.                         |
| automode.enter_se     | String  | Sound effect when entering auto mode.                       |
| automode.leave_se     | String  | Sound effect when leaving auto mode.                        |

---

## Skip Mode Settings

Skip mode rapidly advances through already-seen text.

| Name                  | Type    | Description                                                 |
|-----------------------|---------|-------------------------------------------------------------|
| skipmode.image        | String  | Path to the skip mode banner image.                         |
| skipmode.anime.hide   | String  | Animation played when the banner hides.                     |
| skipmode.anime.show   | String  | Animation played when the banner appears.                   |
| skipmode.x            | Integer | X position of the banner in pixels.                         |
| skipmode.y            | Integer | Y position of the banner in pixels.                         |
| skipmode.enter_se     | String  | Sound effect when entering skip mode.                       |
| skipmode.leave_se     | String  | Sound effect when leaving skip mode.                        |

---

## GUI Settings

These settings control the appearance of built-in GUI screens (save/load and history).

### Save/Load Screen

Font settings for the text elements within each save slot item.

| Prefix                    | Description                                |
|---------------------------|--------------------------------------------|
| `gui.save.index.font.*`   | Slot index number (e.g., "1", "2", …).    |
| `gui.save.date.font.*`    | Save date and time.                        |
| `gui.save.chapter.font.*` | Chapter name at the time of saving.        |
| `gui.save.msg.font.*`     | Preview of the message text.               |

Each prefix supports the following suffixes:

| Suffix              | Type    | Description                                 |
|---------------------|---------|---------------------------------------------|
| `.select`           | Integer | Font to use (1–4).                          |
| `.size`             | Integer | Font size in pixels.                        |
| `.r` / `.g` / `.b`  | Integer | Font color (0–255 each).                    |
| `.outline.width`    | Integer | Outline width in pixels.                    |
| `.outline.r/g/b`    | Integer | Outline color (0–255 each).                 |
| `.ruby`             | Integer | Ruby font size in pixels.                   |
| `.tategaki`         | Boolean | Enable vertical writing.                    |
| `.margin.char`      | Integer | Horizontal character spacing in pixels.     |
| `.margin.line`      | Integer | Line height in pixels (msg only).           |
| `.multiline`        | Boolean | Allow multi-line display (msg only).        |

### History Screen

| Prefix                      | Description                               |
|-----------------------------|-------------------------------------------|
| `gui.history.name.font.*`   | Speaker name column.                      |
| `gui.history.text.font.*`   | Dialogue text column.                     |

Each prefix supports the same font suffixes as above, plus:

| Name                          | Type    | Description                                               |
|-------------------------------|---------|-----------------------------------------------------------|
| gui.history.quote.name_separator | String | Separator between name and text (default: `\n`).       |
| gui.history.quote.start       | String  | Opening quotation mark for dialogue text.                 |
| gui.history.quote.end         | String  | Closing quotation mark for dialogue text.                 |
| gui.history.hide_last         | Boolean | Hide the most recent (current) history entry.             |

### Text Preview

The text preview item shows the current message in a GUI overlay.

| Name                          | Type    | Description                        |
|-------------------------------|---------|------------------------------------|
| `gui.preview.font.*`          | —       | Same font suffixes as above.       |

---

## Sound Settings

Initial volume levels. Players can adjust these in-game via the system menu.

| Name                    | Type  | Description                                                          |
|-------------------------|-------|----------------------------------------------------------------------|
| sound.vol.bgm           | Float | BGM volume (0.0–1.0).                                               |
| sound.vol.voice         | Float | Voice volume (0.0–1.0).                                             |
| sound.vol.se            | Float | Sound effect volume (0.0–1.0).                                      |
| sound.vol.per_character | Float | Per-character volume multiplier for voice (0.0–1.0).                |

---

## Character Settings

### Names

Up to 32 characters can be registered. Their names are used for lip sync, auto focus, and localization.
Replace `N` with a number from 1 to 32.

| Name                    | Type   | Description                                      |
|-------------------------|--------|--------------------------------------------------|
| character.nameN         | String | Default name (used as fallback).                 |
| character.nameN.en      | String | English name.                                    |
| character.nameN.zh-cn   | String | Simplified Chinese name.                         |
| character.nameN.zh-tw   | String | Traditional Chinese name.                        |
| character.nameN.ja      | String | Japanese name.                                   |

### Image Folders

Specifies which image folder belongs to which character, enabling lip sync and auto focus.

| Name                  | Type   | Description                                              |
|-----------------------|--------|----------------------------------------------------------|
| character.folderN     | String | Path to the character's image folder (e.g., `ch/midori/`). |

### Eye Blinking

See also: [eye-blink.md](eye-blink.md).

| Name                         | Type  | Description                                           |
|------------------------------|-------|-------------------------------------------------------|
| character.eyeblink.interval  | Float | Average interval between blinks in seconds.           |
| character.eyeblink.frame     | Float | Duration of each blink frame in seconds.              |

### Lip Sync

See also: [lip-sync.md](lip-sync.md).

| Name                       | Type    | Description                                           |
|----------------------------|---------|-------------------------------------------------------|
| character.lipsync.frame    | Float   | Duration of each lip sync frame in seconds.           |
| character.lipsync.chars    | Integer | Number of characters spoken per lip sync cycle.       |

---

## Auto Focus Settings

> **Note:** Auto focus is not yet functional as of RC1.

Automatically adjusts character brightness to highlight the current speaker.

| Name                        | Type    | Description                                                     |
|-----------------------------|---------|-----------------------------------------------------------------|
| autofocus.on_text_name      | Boolean | Focus the speaker and dim others when `[text]` has a name.      |
| autofocus.on_text_no_name   | Boolean | Dim all characters when `[text]` has no name.                   |
| autofocus.on_ch             | Boolean | Dim non-speakers when `[ch]` is used.                           |
| autofocus.on_choose         | Boolean | Dim all characters when `[choose]` is shown.                    |

---

## Stage Settings

Controls the default margins applied when placing character sprites on the stage.

| Name                    | Type    | Description                                           |
|-------------------------|---------|-------------------------------------------------------|
| stage.ch_margin.bottom  | Integer | Bottom margin for character sprites in pixels.        |
| stage.ch_margin.left    | Integer | Left margin for character sprites in pixels.          |
| stage.ch_margin.right   | Integer | Right margin for character sprites in pixels.         |

---

## Kira Kira Effect Settings

A click-effect animation that plays at the cursor position when the player clicks.

| Name               | Type    | Description                                              |
|--------------------|---------|----------------------------------------------------------|
| kirakira.enable    | Boolean | Enable the Kira Kira effect.                             |
| kirakira.add_blend | Boolean | Use additive blending (brighter glow effect).            |
| kirakira.frame     | Float   | Duration of each animation frame in seconds.             |
| kirakira.image1–16 | String  | Image paths for each frame of the effect.                |

---

## Emoji Settings

Up to 32 emoji images can be registered and used inline in messages with `\e{name}`.

| Name            | Type   | Description                          |
|-----------------|--------|--------------------------------------|
| emoji.nameN     | String | Identifier used in `\e{name}` tags.  |
| emoji.imageN    | String | Path to the emoji image file.        |

Example:
```
emoji.name1=heart
emoji.image1=system/emoji/heart.png
```

---

## Text-To-Speech Settings

| Name        | Type    | Description              |
|-------------|---------|--------------------------|
| tts.enable  | Boolean | Enable text-to-speech.   |

---

## Release Mode Settings

In release mode, save data is written to the OS's user data directory (e.g., AppData on Windows)
instead of the game folder. Enable this for distributed builds.

| Name                  | Type    | Description                            |
|-----------------------|---------|----------------------------------------|
| release_mode.enable   | Boolean | Enable release (install app) mode.     |
