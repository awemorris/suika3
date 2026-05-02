GUI
===

## Introduction

GUI is Suika3's UI/UX creation feature.

In Suika3, buttons are defined in a dedicated GUI mode and operate in
a synchronous manner, that is, calling a GUI results in a button click or
a cancel.

Asynchronous callbacks such as showing buttons while text tag
execution are intentionally avoided because they can be difficult for
beginner programmers to understand and manage.

A GUI file contains a list of button definitions. Each button includes
a behavior type, idle and hover images, and additional
properties. Buttons are mapped to animation layers, and animation
files can be triggered when a button's state changes.

---

## GUI Sample

```
global {
    fadein:       0.2;            # Fading-in time in seconds.
    fadeout:      0.2;            # Fading-out time in seconds.
}

button1 {
    # Layer ID (1-32)
    id: 1;

    # Behavior Type (Go to a label)
    type: goto;

    # Label to go to.
    label: button1_clicked;   

    # Position
    x: 39;
    y: 99;

    # Images
    image-idle:  gui/item-idle.png;
    image-hover: gui/item-hover.png;
}
```

In the `global` section, you can specify options for the GUI.

Other sections are interpreted as button definitions.
Here, `button1` makes a button at position (39, 99).
If the button is clicked, a `go to` jump will happen.

---

## Button Types

| Description                     | Meaning                                           |
|---------------------------------|---------------------------------------------------|
| type: noaction;                 | Non-clickable image.                              |
| type: goto;                     | Jumps to a label when clicked.                    |
| type: gui;                      | Jumps to a GUI when clicked.                      |
| type: mastervol;                | Shown as a master volume slider.                  |
| type: bgmvol;                   | Shown as a BGM volume slider.                     |
| type: sevol;                    | Shown as an SE volume slider.                     |
| type: voicevol;                 | Shown as an voice volume slider.                  |
| type: charactervol;             | Shown as a character volume slider.               |
| type: textspeed;                | Shown as a text speed slider.                     |
| type: autospeed;                | Shown as an auto mode speed slider.               |
| type: preview;                  | Shown as a text preview area.                     |
| type: fullscreen;               | Shown as a full screen mode button.               |
| type: window;                   | Shown as s windoe mode button.                    |
| type: default;                  | Resets settings when pressed.                     |
| type: savepage;                 | Shown as a save/load page button.                 |
| type: save;                     | Shown as a save slot.                             |
| type: load;                     | Shown as a load slot.                             |
| type: auto;                     | Shown as an auto mode button.                     |
| type: skip;                     | Shown as a skip mode button.                      |
| type: title;                    | Shown as a back-to-title button.                  |
| type: history;                  | Shown as a history button.                        |
| type: historyscroll;            | Shown as a vertical history scroll slider.        |
| type: historyscroll-horizontal; | Shown as a horizontal history scroll slider.      |
| type: cancel;                   | Shown as a cancel button.                         |
| type: namevar;                  | Shown as a area to preview a name variable value. |
| type: char;                     | Shown as a button to input a character.           |
| type: language                  | Change the language when clicked.                 |

### `noaction`

A non-clickable image.

```
background {
    type: noaction;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `goto`

A clickable button. When clicked, tag execution jumps to a label specified by `label:` parameter.

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

A clickable button. When clicked, GUI execution is chained to a new GUI specified by `file:` parameter.

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

A slider to set the master volume.

```
slider1 {
    type: mastervol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `bgmvol`

A slider to set the BGM volume.

This slider sets the volume to be stored in the global save data file.
This is defferent than the volume to be set by the `[volume]` tag and stored in the local save data.

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `sevol`

A slider to set the SE volume.

This slider sets the volume to be stored in the global save data file.
This is defferent than the volume to be set by the `[volume]` tag and stored in the local save data.

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `voicevol`

A slider to set the SE volume.

This slider sets the volume to be stored in the global save data file.
This is defferent than the volume to be set by the `[volume]` tag and stored in the local save data.

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `charactervol`

A slider to set a per-character volume.

Character index is passed by the `index:` parameter.
index 0 is for non-defined character, and 1-32 for defined characters.

```
slider1 {
    type: charactervol;
    index: 1;  # Character Index
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `textspeed`

A slider to set the text speed.

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `autospeed`

A slider to set the auto mode speed.

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `preview`

A text area to preview the font, language, and speed.

```
preview1 {
    type: preview;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `fullscreen`

A clickable button.
When clicked, the engine will enter a full screen mode.

```
fullscreen1 {
    type: fullscreen;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # For when in the full screen mode.
}
```

### `window`

A clickable button.
When clicked, the engine will enter a windowed mode.

```
window1 {
    type: window;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # For when in the windowed mode.
}
```

### `default`

A clickable button.
When clicked, it resets all settings.

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

A clickable button.
When clicked, it switches the save screen page.

```
savepage1 {
    type: savepage;
    index: 0; # Page index (0-)
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-active:  active.png;  # For when the page is active.
}
```

### `save`

A clickable button.
When clicked, it executes a save.

```
save1 {
    type: save;
    index: 0; # Index in apage. actual save slot = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/save/save-item-idle.png;
    image-hover: system/save/save-item-hover.png;

    index-x:   10;   # Number
    index-y:   0;
    date-x:    60;   # Date
    date-y:    0;
    thumb-x:   27;   # Thumbnail
    thumb-y:   77;
    chapter-x: 260;  # Chapter title
    chapter-y: 48;
    msg-x:     260;  # Last message
    msg-y:     96;
}
```

### `load`

A clickable button.
When clicked, it executes a load.

```
load1 {
    type: load;
    index: 0; # Index in apage. actual save slot = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/load/load-item-idle.png;
    image-hover: system/load/load-item-hover.png;

    index-x:   10;   # Number
    index-y:   0;
    date-x:    60;   # Date
    date-y:    0;
    thumb-x:   27;   # Thumbnail
    thumb-y:   77;
    chapter-x: 260;  # Chapter title
    chapter-y: 48;
    msg-x:     260;  # Last message
    msg-y:     96;
}
```

### `auto`

A clickable button.
When clicked, it start Auto Mode.

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

A clickable button.
When clicked, it start Skip Mode.

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

A clickable button.
When clicked, the engine goes back to the specified NovelML file.

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

A clickable button.
It shows a message history item.

```
history {
    type: history;
    index: 0; # Index in a page
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;

    name-x: 20;   # Name
    name-y: 10;
    text-x:  20;  # Text (for when there is a name)
    text-y:  50;
    msg-x:  20;   # Message (for when no name)
    msg-y:  10;
}
```

### `historyscroll`

A vertical scroll bar for history screen.

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

A horizontal scroll bar for vertical writing history screen.

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

A clickable button.
When clicked, the GUI will be canceled.

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

A text area to show a variable value.
It is used for name editing.

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

A button to append a character to a variable.
It is used for name editing.

```
char1 {
    type: char;
    variable: variable_name; # Variable name.
    msg: A;                  # Text to append.
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `language`

A button to switch the language.

```
language_english1 {
    type: language;
    langu: en;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png; # For when English is active.
}
```

---

## Common Button Options

| Description           |Meaning                                                      |
|-----------------------|-------------------------------------------------------------|
| type:                 | Type of the button.                                         |
| x:                    | X position of the button.                                   |
| y:                    | Y position of the button.                                   |
| width:                | Width of the button. (by default, sets to idle image width) |
| height:               | Height of the button.                                       |
| pointse:              | Sound effect for when the button is pointed.                |
| clickse:              | Sound effect for when the button is clicked.                |

### pointse:

`pointse: btn-change.ogg;` indicates a sound effect file is played
back when the mouse cursor enters the button.

### clickse:

`clickse: click.ogg;` indicates a sound effect file is played back
when the button is clicked.
