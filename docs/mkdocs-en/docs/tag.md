Suika3 Tag Reference
====================

## Index

| Tag Name          | Description                                                        |
| :---              | :---                                                               |
| **[anime]**       | Loads and runs an animation file.                                  |
| **[bg]**          | Changes the background image with a fading effect.                 |
| **[bgm]**         | Plays a background music file (Ogg Vorbis format).                 |
| **[callmacro]**   | Calls a defined macro.                                             |
| **[ch]**          | Shows or hides characters with detailed layer parameters.          |
| **[chapter]**     | Sets a chapter name.                                               |
| **[choose]**      | Displays options and stores the selection or jumps to a label.     |
| **[click]**       | Waits for a user click.                                            |
| **[config]**      | Sets a configuration value for the game system.                    |
| **[defmacro]**    | Starts a macro definition.                                         |
| **[else]**        | Part of the if/elseif branch for when no conditions are met.       |
| **[elseif]**      | Specifies an additional condition in a branch.                     |
| **[endif]**       | Ends a conditional branch.                                         |
| **[endmacro]**    | Ends a macro definition.                                           |
| **[goto]**        | Jumps to a specified label tag.                                    |
| **[gui]**         | Shows a GUI from a specified file.                                 |
| **[if]**          | Branches the process based on a specified condition.               |
| **[label]**       | Defines a label for jump targets.                                  |
| **[layer]**       | Loads/unloads images or sets parameters for specific layers.       |
| **[load]**        | Loads a NovelML file and can jump to a specific label.             |
| **[move]**        | Animates character layers over a specified time.                   |
| **[returnmacro]** | Returns from a macro execution.                                    |
| **[se]**          | Plays a sound effect file (Ogg Vorbis format).                     |
| **[set]**         | Sets a variable value (all variables are treated as text).         |
| **[skip]**        | Enables or disables the skip status.                               |
| **[text]**        | Displays text in the message box, optionally with a name.          |
| **[video]**       | Plays a video file (supports skippable settings).                  |
| **[volume]**      | Sets the sound volume for BGM, SE, or Voice tracks.                |
| **[wait]**        | Waits for a specified number of seconds.                           |

---

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

---

## ch

This tag shows or hides characters.
All arguments are omissible.

Basic Usage:
```
[ch center="chara001.png" time="1.0"]
[ch right="chara002.png" time="1.0"]
[ch left="chara003.png" time="1.0"]
[ch
  time="1.0"
  bg="bg-file.png"
  bg-x="0"
  bg-y="0"
  bg-a="255"
  bg-sx="1.0"
  bg-sy="1.0"
  bg-cx="0"
  bg-cy="0"
  bg-rotate="0.0"
  back="back-file.png"
  back-x="0"
  back-y="0"
  back-a="255"
  back-sx="1.0"
  back-sy="1.0"
  back-cx="0"
  back-cy="0"
  back-rotate="0.0"
  back-dim="false"
  left="left-file.png"
  left-x="0"
  left-y="0"
  left-a="255"
  left-sx="1.0"
  left-sy="1.0"
  left-cx="0"
  left-cy="0"
  left-rotate="0.0"
  left-dim="false"
  left-center="left-center-file.png"
  left-center-x="0"
  left-center-y="0"
  left-center-a="255"
  left-center-sx="1.0"
  left-center-sy="1.0"
  left-center-cx="0"
  left-center-cy="0"
  left-center-rotate="0.0"
  left-center-dim="false"
  right="right-file.png"
  right-x="0"
  right-y="0"
  right-a="255"
  right-sx="1.0"
  right-sy="1.0"
  right-cx="0"
  right-cy="0"
  right-rotate="0.0"
  right-dim="false"
  right-center="right-center-file.png"
  right-center-x="0"
  right-center-y="0"
  right-center-a="255"
  right-center-sx="1.0"
  right-center-sy="1.0"
  right-center-cx="0"
  right-center-cy="0"
  right-center-rotate="0.0"
  right-center-dim="false"
  center="center-file.png"
  center-x="0"
  center-y="0"
  center-a="255"
  center-sx="1.0"
  center-sy="1.0"
  center-cx="0"
  center-cy="0"
  center-rotate="0.0"
  center-dim="false"
  face="face-file.png"
  face-x="0"
  face-y="0"
  face-a="255"
  face-sx="1.0"
  face-sy="1.0"
  face-cx="0"
  face-cy="0"
  face-rotate="0.0"
  face-dim="false"
]
```

### Layer Name Arguments

The following arguments specify file names to load.
`none` will unload the image.

* `bg`
* `back`
* `left`
* `left-center`
* `right`
* `right-center`
* `center`
* `face`

### `*-x` arguments

The following arguments specify X positions.

* `bg-x`
* `back-x`
* `left-x`
* `left-center-x`
* `right-x`
* `right-center-x`
* `center-x`
* `face-x`

Values such as `r100` or `r-100` means `+100` or `-100` to the current value.

### `*-y` arguments

The following arguments specify Y positions.

* `bg-y`
* `back-y`
* `left-y`
* `left-center-y`
* `right-y`
* `right-center-y`
* `center-y`
* `face-y`

Values such as `r100` or `r-100` means `+100` or `-100` to the current value.

### `*-a` arguments

The following arguments specify Y positions.

* `bg-a`
* `back-a`
* `left-a`
* `left-center-a`
* `right-a`
* `right-center-a`
* `center-a`
* `face-a`

Values such as `r100` or `r-100` means `+100` or `-100` to the current value.

### `*-sx` arguments

The following arguments specify X scaling factor.

* `bg-sx`
* `back-sx`
* `left-sx`
* `left-center-sx`
* `right-sx`
* `right-center-sx`
* `center-sx`
* `face-sx`

Values such as `r1.0` or `r-1.0` means `+1.0` or `-1.0` to the current value.

### `*-sy` arguments

The following arguments specify Y scaling factor.

* `bg-sy`
* `back-sy`
* `left-sy`
* `left-center-sy`
* `right-sy`
* `right-center-sy`
* `center-sy`
* `face-sy`

Values such as `r1.0` or `r-1.0` means `+1.0` or `-1.0` to the current value.

### `*-cx` arguments

The following arguments specify X center for rotation.

* `bg-cx`
* `back-cx`
* `left-cx`
* `left-center-cx`
* `right-cx`
* `right-center-cx`
* `center-cx`
* `face-cx`

Values such as `r100` or `r-100` means `+100` or `-100` to the current value.

### `*-cy` arguments

The following arguments specify Y center for rotation.

* `bg-cy`
* `back-cy`
* `left-cy`
* `left-center-cy`
* `right-cy`
* `right-center-cy`
* `center-cy`
* `face-cy`

Values such as `r100` or `r-100` means `+100` or `-100` to the current value.

### `*-rotate` arguments

The following arguments specify rotations.

* `bg-rotate`
* `back-rotate`
* `left-rotate`
* `left-center-rotate`
* `right-rotate`
* `right-center-rotate`
* `center-rotate`
* `face-rotate`

Values such as `r30` or `r-30` means `+30` or `-30` to the current value.

---

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

### `text1` to `text8` arguments

These arguments specify the texts of the options.

### `label1` to `label8` arguments

These arguments specify the label to jump to when an option is chosen.

---

## click

This tag waits for a click


---

## gui

This tag shows a GUI.

Basic Usage:
```
[gui file="mygui.txt"]
```

### `file` argument

This argument specifies a file name to load.

---

## label

This tag defines a label for jump targets.

Basic Usage:
```
[label name="MyLabel"]
```

### `name` argument

This argument specifies the label name.

---

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

---

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

--

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

---

## goto

This tag jumps to a label tag.

Basic Usage:
```
[goto name="LabelName"]
```

### `name` argument

This argument specifies the name of the destination label.

---

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

---

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

---

## wait

This tag waits for specified seconds.

Basic Usage:
```
[wait time="1.0"]
```

### `time` argument

This argument specifies seconds to wait.

---

## bgm

This tag plays a background music file.

Basic Usage:
```
[bgm file="bgm01.ogg"]
```

### `file` argument

This argument specifies the file name to play.
The file must be `Ogg Vorbis` format with 44100Hz sampling rate.

---

## se

This tag plays a sound effecf file.

Basic Usage:
```
[se file="se01.ogg"]
```

### `file` argument

This argument specifies the file name to play.
The file must be `Ogg Vorbis` format with 44100Hz sampling rate.

---

## volume

This tag sets a sound volume for a track with a fading.

Basic Usage:
```
[volume track="bgm" volume="1.0"]
[volume track="se" volume="1.0"]
[volume track="voice" volume="1.0"]
```

### `track` argument

This argument specifies the track to set volume.

|Track Name   |Description                   |
|-------------|------------------------------|
|bgm          |Background Music              |
|se           |Sound Effect and System Sound |
|voice        |Voice                         |

---

## chapter

This tag sets a chapter name.

Basic Usage:
```
[chapter name="Chapter 01"]
```

### `name` argument

This argument specifies the chapter name.

---

## skip

This tag sets a skip enable status.

Basic Usage:
```
[skip enable="true"]
[skip enable="false"]
```

### `enable` argument

This argument specifies whether skip is enabled or not.

---

## config

This tag sets a config value.

Basic Usage:
```
[config name="msgbox.x" value="100"]
```

### `name` argument

This argument specifies the config name.

### `value` argument

This argument specifies the config value.

---

## layer

This tag loads an image, unload an image, or sets parameters for a layer.

Basic Usage:
```
[layer
  name="bg"
  x="100"
  y="100"
  alpha="255"
  scale-x="1.0"
  scale-y="1.0"
  center-x="0"
  center-y="0"
  rotate="0.0"
]
```

### `name` argument

This argument specifies the layer.

|Layer Name       |Description                            |
|-----------------|---------------------------------------|
|bg               |Background Image                       |
|bg2              |Background Image 2                     |
|efb1             |Back Effect 1                          |
|efb2             |Back Effect 2                          |
|efb3             |Back Effect 3                          |
|efb4             |Back Effect 4                          |
|chb              |Center-Back Character                  |
|chb-eye          |Center-Back Character's Eyes           |
|chb-lip          |Center-Back Character's Lips           |
|chb-fo           |Fading-Out Center-Back Character       |
|chl              |Left Character                         |
|chl-eye          |Left Character's Eyes                  |
|chl-lip          |Left Character's Lips                  |
|chl-fo           |Fading-Out Left Character              |
|chlc             |Left-Center Character                  |
|chlc-eye         |Left-Center Character's Eyes           |
|chlc-lip         |Left-Center Character's Lips           |
|chlc-fo          |Fading-Out Left-Center Character       |
|chr              |Right Character                        |
|chr-eye          |Right Character's Eyes                 |
|chr-lip          |Right Character's Lips                 |
|chr-fo           |Fading-Out Right Character             |
|chrc             |Right-Center Character                 |
|chrc-eye         |Right-Center Character's Eyes          |
|chrc-lip         |Right-Center Character's Lips          |
|chrc-fo          |Fading-Out Right-Center Character      |
|chc              |Center Character                       |
|chc-eye          |Center Character's Eyes                |
|chc-lip          |Center Character's Lips                |
|chc-fo           |Fading-Out Center Character            |
|eff1             |Front Effect 1                         |
|eff2             |Front Effect 2                         |
|eff3             |Front Effect 3                         |
|eff4             |Front Effect 4                         |
|chf              |Face Character                         |
|chf-eye          |Face Character's Eyes                  |
|chf-lip          |Face Character's Lips                  |
|chf-fo           |Fading-Out Face Character              |
|text1            |Text Layer 1                           |
|text2            |Text Layer 2                           |
|text3            |Text Layer 3                           |
|text4            |Text Layer 4                           |
|text5            |Text Layer 5                           |
|text6            |Text Layer 6                           |
|text7            |Text Layer 7                           |
|text8            |Text Layer 8                           |

### `x` argument

This argument specifies the layer X position.

### `y` argument

This argument specifies the layer Y position.

### `alpha` argument

This argument specifies the layer alpha value.

### `scale-x` argument

This argument specifies the layer's X scaling factor.

### `scale-y` argument

This argument specifies the layer's Y scaling factor.

### `center-x` argument

This argument specifies the layer's X center for rotation.

### `center-y` argument

This argument specifies the layer's Y center for rotation.

### `rotate` argument

This argument specifies the layer's rotation in degrees.

---

## move

This tag animates a character.

Basic Usage:
```
[move
  time="2.0"
  name="chc"
  x="100"
  y="100"
  alpha="255"
  scale-x="1.0"
  scale-y="1.0"
  center-x="0"
  center-y="0"
  rotate="0.0"
  dim="false"
]
```

### `time` argument

This argument specifies the animation time in seconds.

### `name` argument

This argument specifies the layer.

|Layer Name       |Description                            |
|-----------------|---------------------------------------|
|bg               |Background Image                       |
|bg2              |Background Image 2                     |
|chb              |Center-Back Character                  |
|chl              |Left Character                         |
|chlc             |Left-Center Character                  |
|chr              |Right Character                        |
|chrc             |Right-Center Character                 |
|chc              |Center Character                       |
|chf              |Face Character                         |

### `x` argument

This argument specifies the layer X position.

`r100` means +100px relative to the current position.

`r-100` means -100px relative to the current position.

### `y` argument

This argument specifies the layer Y position.

`r100` means +100px relative to the current position.

`r-100` means -100px relative to the current position.

### `alpha` argument

This argument specifies the layer alpha value.

`r100` means +100 relative to the current position.

`r-100` means -100 relative to the current position.

### `scale-x` argument

This argument specifies the layer's X scaling factor.

`r0.5` means +0.5 relative to the current position.

`r-0.5` means -0.5 relative to the current position.

### `scale-y` argument

This argument specifies the layer's Y scaling factor.

`r0.5` means +0.5 relative to the current position.

`r-0.5` means -0.5 relative to the current position.

### `center-x` argument

This argument specifies the layer's X center for rotation.

`r100` means +100 relative to the current position.

`r-100` means -100 relative to the current position.

### `center-y` argument

This argument specifies the layer's Y center for rotation.

`r100` means +100 relative to the current position.

`r-100` means -100 relative to the current position.

### `rotate` argument

This argument specifies the layer's rotation in degrees.

`r30` means +30deg relative to the current position.

`r-30` means -30deg relative to the current position.

---

## anime

This tag loads an anime file and run it.

Basic Usage:
```
[anime
  file="anime-file.txt"
  async="false"
  showsysbtn="false"
  showmsgbox="false"
  shownamebox="false"
]
[anime
  file="anime-file.txt"
  async="true"
  register="LoopAnime1"
]
[anime
  stop="true"
  register="LoopAnime1"
]
```

### `file` argument

This argument specifies a file name to load.

### `async` argument

This argument specifies whether anime asynchronous or not.
For synchronous animes, this tag waits for completion of the anime.
For asynchronous animes, this tag doesn't wait for its completion.

### `showsysbtn` argument

This argument specifies whether to show the system button or not.
This argument is only valid for synchronous animes.

### `showmsgbox` argument

This argument specifies whether to show the message box or not.
This argument is only valid for synchronous animes.

### `shownamebox` argument

This argument specifies whether to show the name box or not.
This argument is only valid for synchronous animes.

### `register` argument

This arguments specified the anime instance name to control it later.

### `stop` arguments

This arguments specifies the stop of an anime specified by a registered name.

---

## defmacro

This tag starts a macro definition.

Basic Usage:
```
[defmacro name="MyMacro1"]
    [text text="Some messages."]
[endmacro]
```

---

## callmacro

This tag calls a macro.

Basic Usage:
```
[callmacro name="MyMacro1"]
```

---

## returnmacro

This tag returns from a macro.

Basic Usage:
```
[defmacro name="MyMacro1"]
    [if lhs="${var1}" op="==" rhs="1"]
       [returnmacro]
    [endif]
    # ...
[endmacro]
```
---

## video

This tag plays a video.

Basic Usage:
```
[video file="myvideo.mp4" skippable="false"]
```

### `file` arguments

This arguments specifies a file name to play.

### `skippable` arguments

This arguments specifies whether the video is skippable by a click or not.
