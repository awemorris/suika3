Suika3 Tag Specification
========================

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

This tag shows a text in the message box.

Basic Usage:
```
[text text="This is a message."]
[text name="Name Here" text="This is a character's line."]
```

### `name` argument

This argument specifies a character name to show.

### `text` argument

This argument specifies a text name to show.


## choose

This tag shows a options. The text of the chosen option will be
assigned to the variable specified by `name`.

Basic Usage:
```
[choose
  name="variable_name"
  text1="Option Text 1"
  text2="Option Text 2"
  text3="Option Text 3"]
```

### `name` argument

This argument specifies a variable name to store the result.

### `text1` to `text8` arguments

These arguments specified texts of options.


## gui

This tag shows a GUI.

Basic Usage:
```
[gui file="my_gui.txt"]
```

### `file` argument

This argument specifies the file name of the GUI to load.


## set

This tag sets a variable.
Note that all variables are text.

Basic Usage:
```
[set name="var1" value="my-value"]
```

### `name` argument

This argument specifies the variable name.

### `value` argument

This argument specifies the variable value.

## goto

This tag jumps to a label tag.

Basic Usage:
```
[goto name="LabelName"]
```

### `name` argument

This argument specifies the name of the destination label.


## label

This tag makes a label with a name.

Basic Usage:
```
[label name="LabelName"]
```

### `name` argument

This argument specifies the name of the label.


## if/elseif/endif

This tag branches by a condition.

Basic Usage:
```
[if lhs="${variable}" op="==" rhs="100"]
   # Do something. (variable == 100)
[elseif lhs="${variable}" op="==" rhs="101"]
   # Do something. (variable == 101)
[else]
   # Do something. (other)
[endif]
```

### `lhs` argument

This argument specified the lhs of the condition.

### `op` argument

This argument specified the operator of the condition.

|Operator  |Description         |
|----------|--------------------|
|==        |Equal               |
|>         |Greater Than        |
|>=        |Greater Or Equal    |
|<         |Less                |
|<=        |Less Or Equal       |

### `rhs` argument

This argument specified the rhs of the condition.


## load

This tag loads a NovelML file.

Basic Usage:
```
[load file="scene01.novel"]
[load file="scene02.novel" label="chapter1"]
```

### `file` argument

This argument specifies the file name to load.

### `label` argument (optional)

This argument specifies the label to jump.


## wait
## bgm
## se
## volume
## chapter
## skip
## config
## layer
## move
## anime
## script
## video
## gosub
## return
## shake
