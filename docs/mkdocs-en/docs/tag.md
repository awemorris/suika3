Tag Specification
=================

## bg

This tag changes the background image with a fading.

Basic Usage:
```
[bg file="background.png" time="1.0"]
```

### `file` argument

This argument specifies the file name of the new background image.
Specifying `none` removes the background image.

### `time` argument

This argument specifies the time of the fading in seconds.

### `method` argument

This argument specifies the fading method.

|Name    |Description                 |
|--------|----------------------------|
|`normal`|Alpha blending. (default)   |
|`rule`  |1-bit universal transition. |
|`melt`  |8-bit universal transition. |

### `rule` argument

This argument specifies the rule image file for the `rule` and `melt` fadings.

### `x` argument

This argument specifies the X offset to show the background image.

### `y` argument

This argument specifies the Y offset to show the background image.


## ch


## choose

This tag shows the options and let user choose one.

Basic Usage:
```
[choose
    text1="This is option1" label1="Label1"
    text2="This is option2" label2="Label2"
    text3="This is option4" label3="Label3"
]
```

## `text1` to `text8` arguments

These arguments specify the texts of the options.

## `label1` to `label8` arguments

These arguments specify the label to jump to when an option is chosen.


## click

This tag waits for a click


## gui

This tag shows a GUI.


## label


## text


