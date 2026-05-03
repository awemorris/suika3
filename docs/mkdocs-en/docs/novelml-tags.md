Suika3 Tag Reference
====================

## Index

| Tag Name                    | Description                                                        |
|-----------------------------|--------------------------------------------------------------------|
| [anime](#anime)             | Loads and runs an animation file.                                  |
| [bg](#bg)                   | Changes the background image with a fading effect.                 |
| [bgm](#bgm)                 | Plays a background music file (Ogg Vorbis format).                 |
| [callmacro](#callmacro)     | Calls a defined macro.                                             |
| [ch](#ch)                   | Shows or hides characters with detailed layer parameters.          |
| [chapter](#chapter)         | Sets a chapter name.                                               |
| [choose](#choose)           | Displays options and stores the selection or jumps to a label.     |
| [click](#click)             | Waits for a user click.                                            |
| [config](#config)           | Sets a configuration value for the game system.                    |
| [defmacro](#defmacro)       | Starts a macro definition.                                         |
| [else](#else)               | Part of the if/elseif branch for when no conditions are met.       |
| [elseif](#elseif)           | Specifies an additional condition in a branch.                     |
| [endif](#endif)             | Ends a conditional branch.                                         |
| [endmacro](#endmacro)       | Ends a macro definition.                                           |
| [goto](#goto)               | Jumps to a specified label tag.                                    |
| [gui](#gui)                 | Shows a GUI from a specified file.                                 |
| [if](#if)                   | Branches the process based on a specified condition.               |
| [label](#label)             | Defines a label for jump targets.                                  |
| [layer](#layer)             | Loads/unloads images or sets parameters for specific layers.       |
| [load](#load)               | Loads a NovelML file and can jump to a specific label.             |
| [move](#move)               | Animates character layers over a specified time.                   |
| [pencil](#pencil)           | Draw a text on a layer.                                            |
| [returnmacro](#returnmacro) | Returns from a macro execution.                                    |
| [se](#se)                   | Plays a sound effect file (Ogg Vorbis format).                     |
| [set](#set)                 | Sets a variable value (all variables are treated as text).         |
| [skip](#skip)               | Enables or disables the skip status.                               |
| [text](#text)               | Displays text in the message box, optionally with a name.          |
| [video](#video)             | Plays a video file (supports skippable settings).                  |
| [volume](#volume)           | Sets the sound volume for BGM, SE, or Voice tracks.                |
| [wait](#wait)               | Waits for a specified number of seconds.                           |

---

## `anime`

Run Animation

The `anime` tag loads and executes an animation definition from a file. 
It allows for complex visual effects, character movements, or looping environmental animations beyond simple transitions.

### Basic Usage

```
# Run a synchronous animation (waits for completion)
[anime file="opening_effect.txt"]

# Run an asynchronous looping animation
[anime file="sparkle.txt" async="true" register="my_loop"]

# Stop a registered asynchronous animation
[anime stop="true" register="my_loop"]
```

### Arguments

| Argument      | Omissible      | Description                                        | Notes                                                             |
|---------------|----------------|----------------------------------------------------|-------------------------------------------------------------------|
| `file`        | Yes            | The filename of the animation definition.          | *Required unless `stop="true"` is used.                           |
| `async`       | Yes (`false`)  | Whether to run the animation asynchronously.       | If `false`, the script waits until the animation finishes.        |
| `register`    | Yes            | A unique name to identify this animation instance. | Required for controlling or stopping async animations later.      |
| `stop`        | Yes (`false`)  | Stops a registered animation if set to `true`.     | Requires the `register` argument.                                 |
| `showsysbtn`  | Yes (`true`)   | Whether to show system buttons during playback.    | Only valid for synchronous animations.                            |
| `showmsgbox`  | Yes (`true`)   | Whether to show the message box during playback.   | Only valid for synchronous animations.                            |
| `shownamebox` | Yes (`true`)   | Whether to show the name box during playback.      | Only valid for synchronous animations.                            |

### Tips

**Synchronous vs. Asynchronous**:
* **Synchronous (`async="false"`)**: Great for cutscenes where you want the player to watch the animation before any text or choices appear.
* **Asynchronous (`async="true"`)**: Perfect for background effects (like falling snow or a flickering light) that should continue while the story progresses.

**Managing Instances**:
* By using the `register` argument, you can label a specific animation.
* This is how you tell the engine exactly which animation to stop when you use `stop="true"`.

**UI Control**:
* Use `showmsgbox="false"` if your animation is meant to take up the full screen and you want the dialogue window to disappear temporarily for a cleaner look.

---

## `bg`

Change Background

The `bg` tag changes the background image with a smooth fading effect.
It's the primary way to set the scene in your visual novel.

### Basic Usage

```
# Transition to background.png over 1.0 second
[bg file="background.png" time="1.0"]

# Fade to a black screen (removes the background)
[bg file="none" time="1.0"]
```

### Arguments

| Argument   | Omissible      | Description                                   | Notes                                                                        |
|------------|----------------|-----------------------------------------------|------------------------------------------------------------------------------|
| `file`     | No             | The filename of the new background image.     | Set to `none` to remove the background.                                      |
| `time`     | Yes (`0`)      | The duration of the fading effect in seconds. | Default is `0.0` (instant change).                                           |
| `method`   | Yes (`normal`) | The fading method/style.                      | Choose from `normal`, `rule`, or `melt`.                                     |
| `rule`     | Yes            | The rule image file for specific transitions. | Required when `method` is set to `rule` or `melt`.                           |
| `x`        | Yes (`0`)      | The X-axis offset for the background image.   | Supports absolute values (e.g., `100`) or relative values (e.g., `r100`).    |
| `y`        | Yes (`0`)      | The Y-axis offset for the background image.   | Supports absolute values (e.g., `100`) or relative values (e.g., `r-100`).   |
| `alpha`    | Yes (`255`)    | The alpha value of the background image.      | `0` to `255`.                                                                |
| `clear`    | Yes (`false`)  | Whether to vanish the characters or not.      | If `true`, all characters will be vanished.                                  |

### Transition Methods (`method`)

You can create different atmospheres by choosing the right transition style:

| Type     | Description                                                                                                                          |
|----------|--------------------------------------------------------------------------------------------------------------------------------------|
| `normal` | Alpha Blending. The default method. Performs a smooth cross-fade between the old and new images.                                     |
| `rule`   | 1-bit Universal Transition. Uses a grayscale "rule" image to determine the switching order.                                          |
| `melt`   | 8-bit Universal Transition. Similar to `rule`, but with soft, blurred edges at the transition boundary, creating a "melting" effect. |

For `rule` and `melt`, the image switches pixel-by-pixel from the darkest to the lightest areas of the rule map.

### Tips

**Relative Positioning**: 
* If you want to nudge the background from its current position, use the `r` prefix.
* For example, `x="r50"` moves the image 50 pixels to the right of its current X coordinate.

**What is a Rule Image?**:
* It's a grayscale image where black areas transition first and white areas transition last.
* By creating custom rule images, you can achieve effects like horizontal wipes, circular reveals, or even more artistic patterns!

---

## `bgm`

Play Background Music

The `bgm` tag plays a background music track. 
Music is an essential tool for setting the mood of your scene, and it will continue to loop automatically until stopped or changed.

### Basic Usage

```
# Start playing a BGM track
[bgm file="field_theme.ogg"]

# Stop the current BGM (use "none")
[bgm file="none"]
```

### Arguments

| Argument | Omissible     | Description                        | Notes                                  |
|----------|---------------|------------------------------------|----------------------------------------|
| `file`   | No            | The filename of the music to play. | Set to `none` to stop the current BGM. |
| `once`   | Yes (`false`) | Don't loop.                        |                                        |

### Tips

**Required Format**:
* For compatibility and performance, Suika3 requires BGM files to be in **Ogg Vorbis** format.
* The sampling rate MUST be **44,100Hz**.

**Looping**:
* Background music is designed to loop by default, so you don't need to worry about the music ending abruptly during a long dialogue scene.

**Smooth Transitions**:
* If you call `[bgm]` while another track is already playing, the engine will typically handle the transition. 
* To adjust the loudness of the music, you'll want to use the `[volume]` tag.

**Stopping Music**:
* When a scene ends or the mood changes to silence, remember to use `[bgm file="none"]` to give the player's ears a rest!

---

## `callmacro`

Call Macro

The `callmacro` tag executes a previously defined macro.
It allows you to trigger a specific sequence of commands, such as character entrances or UI animations, multiple times throughout your script without rewriting the original code.

### Basic Usage

```
# Call a macro named "kaito_entrance"
[callmacro name="kaito_entrance"]

# Call a macro for a specific scene transition
[callmacro name="fade_to_white"]
```

### Arguments

| Argument | Omissible | Description                               | Notes                                              |
|----------|-----------|-------------------------------------------|----------------------------------------------------|
| `name`   | No        | The name of the macro to execute.         | Must match a name defined by a `[defmacro]` tag.   |
| `file`   | Yes       | The file name where the macro is defined. | Omit this to call a macro inside the current file. |

### Tips

**Efficiency**:
* By using `[callmacro]`, you can keep your main story script focused and readable.
* Instead of seeing 10 lines of animation code, you'll just see one clear command.

**Execution Flow**:
* When the engine hits `[callmacro]`, it immediately jumps to the defined macro, runs all the tags inside it, and then automatically returns to the very next line after the `[callmacro]` tag.

**Modular Design**:
* Think of macros as "custom tags" for your game.
* If you decide to change how a character enters a scene, you only need to update the code once in the `[defmacro]` block, and every `[callmacro]` will reflect that change!

---

## `ch`

Character Display

The `ch` tag shows, hides, or updates character images on various layers.
It allows for detailed control over positioning, scaling, and rotations for multiple characters and background at once.

### Basic Usage

```
# Show a character at the center
[ch center="chara001.png" time="1.0"]

# Show multiple characters with specific positions
[ch left="chara002.png" right="chara003.png" time="0.5"]

# Hide a specific character
[ch center="none" time="1.0"]
```

### Arguments

| Argument  | Omissible      | Description                            | Notes                                                 |
|-----------|----------------|----------------------------------------|-------------------------------------------------------|
| `time`    | Yes (`0`)      | Duration of the transition in seconds. | Affects all layer changes within this tag.            |
| `method`  | Yes (`normal`) | The fading method/style.               | `normal`, `rule`, or `melt`.                          |
| `rule`    | Yes            | The rule image file for transitions.   | Required when `method` is `rule` or `melt`.           |

#### Layer File Arguments

Specify a filename to load an image onto a layer. Set to `none` to unload (hide) the image.

| Argument       | Description                               |
|----------------|-------------------------------------------|
| `bg`           | Background layer.                         |
| `back          | Back-Center character.                    |
| `left`         | Left character.                           |
| `left-center`  | Left-Center character.                    |
| `center`       | Center character.                         |
| `right-center` | Right-center character.                   |
| `right`        | Right character.                          |
| `face`         | Face character.                           |

#### Layer Parameter Arguments

Each layer above (e.g., `center`) can be customized using the following suffixes (e.g., `center-x`, `center-rotate`).

| Suffix      | Omissible     | Description                | Notes                                                         |
|-------------|---------------|----------------------------|---------------------------------------------------------------|
| `-x`        | Yes (`0`)     | X position.                | Supports absolute (e.g., `100`) or relative (e.g., `r50`).    |
| `-y`        | Yes (`0`)     | Y position.                | Supports absolute (e.g., `100`) or relative (e.g., `r-50`).   |
| `-a`        | Yes (`255`)   | Alpha value. (opacity)     | `0` (transparent) to `255` (opaque).                          |
| `-scale-x`  | Yes (`1.0`)   | X scaling factor.          | `1.0` is original size. Supports `r` prefix.                  |
| `-scale-y`  | Yes (`1.0`)   | Y scaling factor.          | `1.0` is original size. Supports `r` prefix.                  |
| `-center-x` | Yes (`0`)     | X center for rotation.     | Pivot point for the rotation effect.                          |
| `-center-y` | Yes (`0`)     | Y center for rotation.     | Pivot point for the rotation effect.                          |
| `-rotate`   | Yes (`0`)     | Rotation in degrees.       | Positive for clockwise. Supports `r` prefix.                  |
| `-dim`      | Yes (`false`) | Dimming status.            | If `true`, the layer is rendered 50% darker.                  |

### Tips

**Batch Updates**:
* You can update multiple characters and the background simultaneously in a single `[ch]` tag to ensure they animate together perfectly.

**Relative Transformation**:
* Like the `bg` tag, all numeric parameters support the `r` prefix.
* For example, `center-y="r-50"` will hop the center character 50 pixels upward from its current position.

---

## `chapter`

Set Chapter Name

The `chapter` tag sets the name of the current chapter. 
This name is typically used by the game system to display progress in the save/load menu or on the game screen, helping players keep track of their journey.

### Basic Usage

```
# Set the chapter name at the beginning of a story segment
[chapter name="Chapter 01: The Beginning"]

# Update the chapter name as the story progresses
[chapter name="Intermission: A Quiet Night"]
```

### Arguments

| Argument | Omissible | Description                        | Notes                                                      |
|----------|-----------|------------------------------------|------------------------------------------------------------|
| `name`   | No        | The name of the chapter to be set. | This string will be stored in the game's system variables. |

### Tips

**Save Data Visibility**:
* In many Suika3 configurations, the string you set here is what appears on the "Save" and "Load" slots.
* Choose a name that helps the player remember exactly where they were in the story!

**Consistency**:
* It's a good practice to call the `[chapter]` tag immediately after a `[label]` that starts a new major scene or chapter. 

**Updating Names**:
* You can call `[chapter]` as many times as you like.
* Every time you do, the old chapter name is overwritten by the new one.

---

## `choose`

Display Selection Options

The `choose` tag displays up to 8 interactive buttons for the player. 
It stores the text of the chosen item in a variable.

### Basic Usage

```
# Store selection in a variable
[choose
    text1="Red Pill"
    text2="Green Pill"
    text3="Blue Pill"
    name="user_choice"
    value1="red"
    value2="green"
    value3="blue"
]
```

### Arguments

| Argument         | Omissible | Description                                    | Notes                                              |
|------------------|-----------|------------------------------------------------|--------------------------------------------------- |
| `text1`          | Yes       | The text displayed on each button.             | At least one option are typically required.        |
| `text2`          | Yes       | The text displayed on each button.             | At least one option are typically required.        |
| `text3`          | Yes       | The text displayed on each button.             | At least one option are typically required.        |
| `text4`          | Yes       | The text displayed on each button.             | At least one option are typically required.        |
| `text5`          | Yes       | The text displayed on each button.             | At least one option are typically required.        |
| `text6`          | Yes       | The text displayed on each button.             | At least one option are typically required.        |
| `text7`          | Yes       | The text displayed on each button.             | At least one option are typically required.        |
| `text8`          | Yes       | The text displayed on each button.             | At least one option are typically required.        |
| `text<N>-locale` | Yes       | The text displayed on each button. (localized) | At least one option are typically required.        |
| `name`           | No        | The variable name to store the result.         | Stores the text of the selected option.            |
| `value1`         | Yes       | The value assigned to the result variable.     | At least one option are typically required.        |
| `value2`         | Yes       | The value assigned to the result variable.     | At least one option are typically required.        |
| `value3`         | Yes       | The value assigned to the result variable.     | At least one option are typically required.        |
| `value4`         | Yes       | The value assigned to the result variable.     | At least one option are typically required.        |
| `value5`         | Yes       | The value assigned to the result variable.     | At least one option are typically required.        |
| `value6`         | Yes       | The value assigned to the result variable.     | At least one option are typically required.        |
| `value7`         | Yes       | The value assigned to the result variable.     | At least one option are typically required.        |
| `value8`         | Yes       | The value assigned to the result variable.     | At least one option are typically required.        |
| `time`           | Yes (`0`) | Timer in seconds.                              | If `0`, no timer is enabled.                       |

### Localization

For example, if the user OS environment is set to Japanese, `text1-ja` is preferred instead of `text1`.

| Suffix      | Language                                 |
|-------------|------------------------------------------|
| -en         | English (Fallback)                       |
| -en-us      | English (America)                        |
| -en-gb      | English (British)                        |
| -en-au      | English (Austraria)                      |
| -en-nz      | English (New Zealand)                    |
| -fr         | French (Fallback)                        |
| -fr-fr      | French (France)                          |
| -fr-ca      | French (Canada)                          |
| -es         | Spanish (Spain, Fallback)                |
| -es-la      | Spanish (Latin America)                  |
| -de         | German                                   |
| -it         | Italian                                  |
| -ru         | Russian                                  |
| -el         | Greek                                    |
| -zh         | Chinese (Simplified)                     |
| -zh-tw      | Chinese (Traditional, Taiwan)            |
| -ja         | Japanese                                 |
| (no suffix) | Fallback (developer decides)             |

For English OS locales including all regions, `-en` is used as the
default fallback.  If a more specific variant such as `-en-gb` is
specified in a tag and best matches with the user region, it will be
preferred. The same mechanism is applied to Spanish and French. Note
that there is no fallback from Traditional Chinese to Simplified
Chinese.

For example, if the user locale is `en-AU`, the following priority is applied:
* 1. text1-en-au
* 2. text1-en
* 3. text1

The following are currently not supported but planned to be supported.

| Suffix      | Language                                 |
|-------------|------------------------------------------|
| -ko         | Korean                                   |
| -vi         | Vietnamese                               |
| -id         | Indonesia                                |
| -zh-hk      | Traditional Chinese (Hong Kong)          |
| -pt         | Portuguese (Fallback)                    |
| -pt-br      | Portuguese (Brazil)                      |
| -pl         | Polish                                   |
| -tr         | Turkish                                  |
| -ta         | Tamil                                    |
| -te         | Telugu                                   |
| -kn         | Kannada                                  |
| -si         | Sinhala                                  |
| -ar         | Arabic (RTL)                             |
| -fa         | Persian (RTL)                            |

### Tips

**Branching Logic**:
* You can use the `[if]` tag to check the stored value and create complex branches.

```
[choose
  name="var1"
  text1="Go to school"
  text2="Go to hospital"
  value1="1"
  value2="2"]

[if lhs="${var1}" op="=" rhs="1"]
  # School.
[else]
  # Hospital.
[endif]
```

**Variable Persistence**:
* Since everything is a string, remember that even numbers like "100" are stored as text.
* Suika3's logic tags (like `if`) can handle these strings for comparisons.

---

## `set`

Set Variable

The `set` tag assigns a value to a variable name. 
In Suika3, **all variables are treated as text strings**, but they can be compared numerically in other tags like `[if]`.

### Basic Usage

```
# Assign a simple string to a variable
[set name="player_name" value="Kaito"]

# Set a numeric-like value (stored as a string)
[set name="health" value="100"]

# Clear a variable by setting it to an empty string
[set name="flag_event_01" value=""]

# Add 1 to var1.
[set name="var1" value1="${var1}" op="+" value2="1"]

```

### Arguments

| Argument | Omissible     | Description                                 | Notes                                                               |
|----------|---------------|---------------------------------------------|---------------------------------------------------------------------|
| `name`   | No            | The unique name of the variable.            | Use alphanumeric characters and underscores for best compatibility. |
| `value`  | Yes           | The content to store in the variable.       | Remember: everything is stored as a string!                         |
| `value1` | Yes           | The operand 1 for opcode.                   |                                                                     |
| `value2` | Yes           | The operand 2 for opcode.                   |                                                                     |
| `op`     | Yes           | The opcode. (`+`, `-`, `*`, `/`, `//`, `%`) |                                                                     |
| `global` | Yes (`false`) | Make the flag global.                       | Global variables are for achievement flags e.g., "Saw ED1".         |

### Tips

**String Handling**:
* Since Suika3 treats everything as text, `value="100"` and `value="May"` are handled the same way internally.
* You can reference these variables in other tags (like `text` or `if`) using the `${variable_name}` syntax.

**Flag Management**:
* For game flags (like "has met the hero"), it's a common practice to use `"true"` and `"false"` or `"1"` and `"0"`. 
* Consistency is key! If you start using `"1"`, stick with it so your `[if]` checks don't get confused.

**Variable Naming**:
* Avoid using spaces or special symbols in your variable names. `my_variable` is much safer than `my variable!`.

---

## `click`

Wait for Click

The `click` tag pauses the script execution and waits for the player to click the mouse or press a key.
It is commonly used to create pauses between visual changes or before a major transition.

### Basic Usage

```
# Change the background, then wait for the player to click
[bg file="sunset.png" time="1.0"]
[click]

# After the click, show the character
[ch center="chara01.png" time="1.0"]
```

### Arguments

This tag does not take any arguments.

| Argument | Omissible | Description | Notes           |
|----------|-----------|-------------|-----------------|
| -        | -         | -           | -               |

### Tips

**Timing and Pacing**:
* Use `[click]` when you want to give the player a moment to look at a new background or a specific character expression before the dialogue continues.
* Unlike the `[text]` tag, which waits for a click automatically after displaying a message, `[click]` is used for manual flow control during non-dialogue sequences.

**Visual Feedback**:
* When the script hits a `[click]` tag, the game will remain still. Ensure that any preceding animations (like `[bg]` or `[ch]`) have a `time` set, or the screen might feel static too abruptly.

**For timed waits:**
* Use `[wait]` for timed waits.

---

## `goto`

Jump to Label

The `goto` tag immediately moves the NovelML execution to a specified label. 
It is useful tool for controlling the flow of your story, allowing you to skip sections or loop back to previous parts.

Please note that small branches should be realized by `[if]`.

### Basic Usage

```
# Jump to the beginning of the morning scene
[goto name="morning_start"]

# ... this part of the script will be skipped ...

[label name="morning_start"]
[text text="The sun rises over the horizon."]
```

### Arguments

| Argument | Omissible | Description                       | Notes                                         |
|----------|-----------|-----------------------------------|-----------------------------------------------|
| `name`   | No        | The target label name to jump to. | Must match a name defined by a `[label]` tag. |

### Tips

**Unconditional Jump**:
* Unlike `[if]`, `[goto]` always jumps to the target label as soon as the engine hits the tag.

**Flow Management**:
* Use `[goto]` at the end of a branching path to bring the story back to a "common" route. 
* It's also great for creating loops (like a "Return to Title" sequence) when combined with other logic.

**Across Files?**:
* Remember that `[goto]` typically works within the current script file.
* If you want to jump to a different file entirely, you'll want to look at the `[load]` tag!

---

## `defmacro`

Define Macro

The `defmacro` tag starts the definition of a macro. 
A macro allows you to group multiple tags and commands into a single named block, which can be reused throughout your script using the `[callmacro]` tag.

### Basic Usage

```
# Define a macro for a specific character's entrance
[defmacro name="enter_kaito"]
    [ch center="kaito_smile.png" time="0.5"]
    [text name="Kaito" text="Hey! Did I keep you waiting?"]
[endmacro]

# Later in your script, call it with a single line
[callmacro name="enter_kaito"]
```

### Arguments

| Argument | Omissible | Description                     | Notes                                             |
|----------|-----------|---------------------------------|---------------------------------------------------|
| `name`   | No        | The unique name for this macro. | Used to identify the macro when calling it later. |

### Tips

**Closing the Definition**:
* Every `[defmacro]` must be paired with an `[endmacro]` tag to mark the end of the definition.

**Code Reusability**:
* Macros are perfect for repetitive sequences, such as specific UI transitions, character-specific visual setups, or complex sound and visual combinations.

**Organization**:
* It is a common practice to define all your macros at the very beginning of your main script file or in a separate file that you load at the start.

**Nesting and Logic**:
* You can include almost any other tag inside a macro, including `[if]` statements and even `[returnmacro]` to exit the macro early based on certain conditions.

---

## `gui`

Show GUI

The `gui` tag loads and displays a Graphical User Interface (GUI) definition from a specified file. 
It is used to display menus, title screens, or custom interaction panels.

### Basic Usage

```
# Display the main menu GUI
[gui file="main_menu.txt"]

# Show a custom save/load screen
[gui file="save_screen.txt"]
```

### Arguments

| Argument | Omissible | Description                                 | Notes                                               |
|----------|-----------|---------------------------------------------|-----------------------------------------------------|
| `file`   | No        | The filename of the GUI definition to load. | The file must exist in the project's GUI directory. |

### Tips

**GUI Definitions**:
* The `file` argument points to a text file that defines the layout, buttons, and actions of your interface.
* These files specify where images are placed and what happens (like jumping to a label or quitting) when a user interacts with them.

**Usage in Flow**:
* Typically, a `[gui]` tag is used for a graphical menu such as title screen.

**Customization**:
* Since the GUI is defined in an external file, you can create multiple looks for your game and switch between them just by calling different files with this tag.

---

## `label`

Define Label

The `label` tag defines a specific point in the script that can be targeted by jump commands like `[goto]` or `[load]`.
It acts as a bookmark for navigation within your story.

### Basic Usage

```
# Define a label for the start of a new chapter
[label name="chapter_01_start"]

# Use a jump command to reach this label
[goto name="chapter_01_start"]
```

### Arguments

| Argument | Omissible | Description                     | Notes                                                  |
|----------|-----------|---------------------------------|--------------------------------------------------------|
| `name`   | No        | The unique name for this label. | Case-sensitive. Avoid using spaces or special symbols. |

### Tips

**Navigation Control**:
* Labels are useful for creating branching paths.
* For example, you can put a `label` at the begining of the section of your story for a long jump.

**Unique Naming**:
* Every label name within a single script file must be unique.
* If you have two labels with the same name, the engine might not know where to jump, and that's no fun for anyone!

**Organization**:
* It's a good habit to use descriptive names like `label_evening_park` instead of `label1`.
* It makes it much easier for you (and me!) to read the script later and understand what's happening.

---

## `text`

Display Text

The `text` tag displays a message in the message box. 
It can show the main dialogue or narration, and optionally display a character's name in the name box.

### Basic Usage

```
# Narration style (no name displayed)
[text text="The wind was howling through the trees."]

# Character dialogue
[text name="Keith" text="I've been waiting for you here in the small room."]
```

### Arguments

| Argument         | Omissible | Description                                      | Notes                                            |
|------------------|-----------|--------------------------------------------------|--------------------------------------------------|
| `text`           | No        | The message content to be displayed.             |                                                  |
| `text-<locale>`  | Yes       | The message content to be displayed. (localized) |                                                  |
| `voice`          | Yes       | The voice file.                                  |                                                  |
| `voice-<locale>` | Yes       | The voice file. (localized)                      |                                                  |
| `name`           | Yes       | The character's name to display in the name box. | If omitted, the name box will usually be hidden. |
| `action`         | Yes       | For NVL mode and manual show/hide.               |                                                  |
| `space`          | Yes       | For NVL mode.                                    |                                                  |

### Localization

For example, if the user OS environment is set to Japanese, `text-ja` is preferred instead of `text`.

| Suffix      | Language                                 |
|-------------|------------------------------------------|
| -en         | English (Fallback)                       |
| -en-us      | English (America)                        |
| -en-gb      | English (British)                        |
| -en-au      | English (Austraria)                      |
| -en-nz      | English (New Zealand)                    |
| -fr         | French (Fallback)                        |
| -fr-fr      | French (France)                          |
| -fr-ca      | French (Canada)                          |
| -es         | Spanish (Spain, Fallback)                |
| -es-es      | Spanish (Spain, Fallback)                |
| -es-la      | Spanish (Latin America)                  |
| -de         | German                                   |
| -it         | Italian                                  |
| -ru         | Russian                                  |
| -el         | Greek                                    |
| -zh-cn      | Chinese (Simplified)                     |
| -zh-tw      | Chinese (Traditional, Taiwan)            |
| -ja         | Japanese                                 |
| (no suffix) | Fallback (developer decides)             |

For English OS locales including all regions, `-en` is used as the
default fallback.  If a more specific variant such as `-en-gb` is
specified in a tag and best matches with the user region, it will be
preferred. The same mechanism is applied to Spanish and French. Note
that there is no fallback from Traditional Chinese to Simplified
Chinese.

For example, if the user locale is `en-GB`, the following priority is applied:
* 1. text-en-gb
* 2. text-en
* 3. text

The following are currently not supported but planned to be supported.

| Suffix      | Language                                 |
|-------------|------------------------------------------|
| -ko         | Korean                                   |
| -vi         | Vietnamese                               |
| -id         | Indonesia                                |
| -zh-hk      | Traditional Chinese (Hong Kong)          |
| -pt         | Portuguese (Fallback)                    |
| -pt-br      | Portuguese (Brazil)                      |
| -pl         | Polish                                   |
| -tr         | Turkish                                  |
| -ta         | Tamil                                    |
| -te         | Telugu                                   |
| -kn         | Kannada                                  |
| -si         | Sinhala                                  |
| -ar         | Arabic (RTL)                             |
| -fa         | Persian (RTL)                            |

### Actions

You can use special parameters in the `text` tag.

```
# Clear the message box.
[text action="clear"]

# Clear the message box and show it.
[text action="new"]

# Show the message box.
[text action="show"]

# Hide the message box.
[text action="hide"]
```

### NVL Mode

You can enter the NVL mode by setting some config.

```
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
```

You can go back to ADV mode by resetting the config.

```
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
```

In NVM mode, you can control text messages like this:

```
# New page.
[text action="clear"]
[text text="Hello, this is NVL mode test."]
[text text="NVL mode has a fullscreen-styled message box."]
[text text="By default, each text tag will do a line feed."]
[text text="To continue a paragraph,"]
[text text="specify the space parameter." space=" "]

# New page.
[text action="clear"]
[text text="Please clear the message box explicitly."]
```

### Voice

If the current language is `en-us`, a voice file will resolved in the following order:

1. `voice-en-us` parameter
2. `voice/en-us/` + `voice` parameter
3. `voice-en` parameter
4. `voice/en/` + `voice` parameter
5. `voice` parameter

If the current language is `ja`, a voice file will resolved in the following order:

1. `voice-ja` parameter
2. `voice/ja/` + `voice` parameter
3. `voice` parameter

### Tips

**Automatic Waiting**:
* Unlike other tags, the `text` tag automatically waits for a player's click after the message is fully displayed.
* You don't need to add a `[click]` tag after every line of dialogue!

**Using Variables**:
* You can include variables within your text by using the `${variable_name}` syntax. 
* Example: `[text text="Hello, ${player_name}!"]` will greet the player using whatever name is stored in that variable.

**Line Breaks**:
* Check your project's configuration for how long a single line can be.
* If your text is too long, it might overflow the message box, so keep an eye on the length of your `text` argument!

---

## `if`

Conditional Branching

The `if` tag allows the NovelML to branch based on a specific condition. 
By comparing variables or values, you can create unique story paths or react to previous player choices.

### Basic Usage

```
# Check if a variable equals a certain value
[if lhs="${points}" op="==" rhs="100"]
    [text text="Perfect score! You're amazing."]
[elseif lhs="${points}" op=">=" rhs="80"]
    [text text="Great job! You passed."]
[else]
    [text text="Better luck next time."]
[endif]
```

### Arguments

| Argument | Omissible | Description                            | Notes                                          |
|----------|-----------|----------------------------------------|------------------------------------------------|
| `lhs`    | No        | The Left-Hand Side of the condition.   | Usually a variable like `${var_name}`.         |
| `op`     | No        | The operator used for comparison.      | See the "Operators" table below.               |
| `rhs`    | No        | The Right-Hand Side of the condition.  | The value or variable to compare against.      |

### Comparison Operators (`op`)

You can use these operators to define how the two sides are compared:

| Operator   | Description                |
|------------|----------------------------|
| `===`      | Equal (String)             |
| `==`       | Equal (Numeric)            |
| `>`        | Greater Than (Numeric)     |
| `>=`       | Greater Or Equal (Numeric) |
| `<`        | Less Than (Numeric)        |
| `<=`       | Less Or Equal (Numeric)    |

### Tips

**Closing the Block**:
* Every `[if]` block MUST end with an `[endif]` tag.
* If you forget it, the engine might get confused about where the condition ends!

**Variable Syntax**:
* When using a variable as the `lhs`, always wrap it in `${}`.
* For example, use `lhs="${flag_01}"` instead of just `lhs="flag_01"`.

**Handling Strings vs. Numbers**:
* Suika3 treats variable values as strings, but these operators allow you to perform numeric-style comparisons.
* Just be consistent with your values (e.g., using "1" for true and "0" for false).

**Multiple Branches**:
* You can use as many `[elseif]` tags as you need between `[if]` and `[endif]` to check for multiple specific conditions.

---

## `elseif`

Additional Conditional Branching

The `elseif` tag specifies an additional condition within an `[if]` block. 
It is only evaluated if the preceding `[if]` and any previous `[elseif]` conditions were false.

### Basic Usage

```
[if lhs="${rank}" op="==" rhs="A"]
    [text text="Excellent! You're a pro."]
[elseif lhs="${rank}" op="==" rhs="B"]
    [text text="Good job! Keep it up."]
[elseif lhs="${rank}" op="==" rhs="C"]
    [text text="Not bad, but you can do better."]
[else]
    [text text="Don't give up! Try again."]
[endif]
```

### Arguments

Same as `[if]`. See also [if](#if).

### Tips

**Sequential Evaluation**:
* The engine checks conditions from top to bottom.
* As soon as one `[if]` or `[elseif]` condition is met, its block is executed, and the rest of the branch (including other `[elseif]`s and the `[else]`) is skipped.

**Placement**:
* `[elseif]` must always be placed between an `[if]` tag and an `[else]` or `[endif]` tag.
* You can use as many `[elseif]` tags as you need to cover all your bases!

**Efficiency**:
* If you have a lot of conditions that check the same variable, using multiple `[elseif]` tags is much cleaner and more efficient than nesting multiple `[if]` blocks inside each other.

---

## `else`

Default Conditional Branch

The `else` tag defines a block of code to be executed if none of the preceding `[if]` or `[elseif]` conditions were met. 
It acts as the "default" path for your branching logic.

### Basic Usage

```
[if lhs="${weather}" op="==" rhs="sunny"]
    [text text="It's a beautiful day!"]
[elseif lhs="${weather}" op="==" rhs="rainy"]
    [text text="I should bring an umbrella."]
[else]
    # This runs if it's not sunny OR rainy (e.g., cloudy or snowy)
    [text text="The sky looks interesting today."]
[endif]
```

### Arguments

This tag does not take any arguments.

| Argument | Omissible | Description | Notes |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### Tips

**Final Catch-all**:
* Use `[else]` to handle any scenarios you didn't explicitly cover in your `[if]` or `[elseif]` checks.
* It ensures the game always has a valid path to follow.

**Placement**:
* `[else]` must be placed after all `[elseif]` tags (if any) and immediately before the `[endif]` tag.
* You can only have one `[else]` per `[if]` block.

**Optional Nature**:
* You don't *have* to include an `[else]` block.
* If no conditions are met and there is no `[else]`, the engine will simply skip everything and continue after the `[endif]`.

---

## `endif`

End Conditional Branch

The `endif` tag marks the end of a conditional block started by an `[if]` tag. 
It tells the engine to resume normal script execution after the branching logic is complete.

### Basic Usage

```
[if lhs="${love_points}" op=">=" rhs="50"]
    [text text="She gives you a warm smile."]
[else]
    [text text="She greets you politely."]
[endif]

# Script execution continues here regardless of the outcome above
[text text="The day continues..."]
```

### Arguments

This tag does not take any arguments.

| Argument | Omissible | Description | Notes |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### Tips

**Mandatory Closing**:
* Every single `[if]` tag must have a corresponding `[endif]`.
* Think of them like a pair of brackets that keep your story's logic organized.

**Placement**:
* Always place `[endif]` at the very end of your conditional sequence, following any `[elseif]` or `[else]` blocks. 

**Nesting**:
* If you put an `[if]` inside another `[if]`, make sure each one has its own `[endif]`.
* Proper nesting is the secret to complex, bug-free story flags!

---

## `load`

Load Script File

The `load` tag switches the current script to a different NovelML file.
It is primarily used to organize large stories into multiple chapters or to transition between different game parts like a title screen and the main story.

### Basic Usage

```
# Load and start from the beginning of scene02.novel
[load file="scene02.novel"]

# Load scene02.novel and jump directly to a specific label
[load file="scene02.novel" label="chapter2_start"]
```

### Arguments

| Argument | Omissible | Description                                      | Notes                                                   |
|----------|-----------|--------------------------------------------------|---------------------------------------------------------|
| `file`   | No        | The filename of the NovelML script to load.      | Must be a valid file in the project's script directory. |
| `label`  | Yes       | The target label to jump to within the new file. | If omitted, the script starts from the very first line. |

### Tips

**Project Organization**:
* Instead of writing your entire game in one giant file, use `[load]` to break it down into manageable chunks like `chapter1.novel`, `chapter2.novel`, and so on.

**Immediate Transition**:
* When the engine hits a `[load]` tag, it stops executing the current NovelML file immediately and switches to the new one.
* Any commands placed after `[load]` in the original file will not be executed.

**Global Flags**:
* Don't worry about your variables — any values you've set with the `[set]` tag will persist even after you load a new script file!

---

## `se`

Play Sound Effect

The `se` tag plays a sound effect (SE). 
Sound effects are used for short audio cues like door knocks, footsteps, or UI feedback, adding a layer of immersion and realism to your scenes.

### Basic Usage

```
# Play a sound effect once
[se file="door_open.ogg"]

# Stop all currently playing sound effects
[se file="none"]
```

### Arguments

| Argument | Omissible     | Description                               | Notes                                        |
|----------|---------------|-------------------------------------------|----------------------------------------------|
| `file`   | No            | The filename of the sound effect to play. | Set to `none` to stop sound effect playback. |
| `loop`   | Yes (`false`) | Whether to loop the sound effect or not.  |                                              |

### Tips

**Required Format**:
* Like BGM, Suika3 requires SE files to be in **Ogg Vorbis** format.
* The sampling rate MUST be **44,100Hz** to ensure high fidelity and compatibility.

**Layering Sounds**:
* Sound effects can usually be played while BGM is running.
* They occupy their own audio track so they won't interrupt your music.

**Volume Control**:
* To adjust the loudness of your sound effects without changing the BGM volume, use the `[volume]` tag with `track="se"`.

**Usage for Ambience**:
* While SE is often used for short sounds, you can also use it for looping ambient sounds (like wind or rain).
* A looped SE is restored when a save data file is loaded.

---

## `volume`

Set Audio Volume

The `volume` tag sets the sound volume for a specific audio track. 
It's perfect for ensuring that your background music doesn't drown out important sound effects or character voices.

### Basic Usage

```
# Set BGM volume to 50%
[volume track="bgm" volume="0.5"]

# Set SE volume to maximum
[volume track="se" volume="1.0"]

# Mute voices
[volume track="voice" volume="0.0"]
```

### Arguments

| Argument | Omissible | Description                           | Notes                                     |
|----------|-----------|---------------------------------------|-------------------------------------------|
| `track`  | No        | The audio track to adjust.            | See the "Tracks" table below.             |
| `volume` | No        | The volume level from `0.0` to `1.0`. | `0.0` is silent, `1.0` is maximum volume. |
| `time`   | Yes (`0`) | Fading time in seconds.               | `0` means instant change.                 |
### Track Types (`track`)

Suika3 categorizes audio into three main tracks:

| Track Name | Description                      |
|------------|----------------------------------|
| `bgm`      | Background Music.                |
| `se`       | Sound Effects and system sounds. |
| `voice`    | Character voice files.           |

### Tips

**Immediate Change**:
* The volume change happens gradually when `time` is greater than `0`.
* `time="0"` means an immediate change.

**Default Levels**:
* It's a good idea to set your preferred volume levels at the start of your game (e.g., in a `start` label) so the player has a consistent experience from the beginning.

---

## `skip`

Set Skip Status

The `skip` tag enables or disables the skipping function within the game. 
It is useful for preventing players from skipping through important cinematic sequences or ensuring that certain scenes are experienced at the intended pace.

### Basic Usage

```
# Enable the skip function
[skip enable="true"]

# Disable the skip function during a critical scene
[skip enable="false"]
```

### Arguments

| Argument | Omissible | Description                                  | Notes                                                 |
|----------|-----------|----------------------------------------------|-------------------------------------------------------|
| `enable` | No        | Whether the skip function is enabled or not. | Set to `true` to allow skipping, `false` to block it. |

### Tips

**Cinematic Control**:
* Skip feature is typically disabled before the title logo at startup.

**Restoring Settings**:
* Don't forget to set `[skip enable="true"]` once the critical scene is over.
* Players usually appreciate having the freedom to skip through text they've already seen.

**System Behavior**:
* This tag controls the overall "Skip" state of the engine.
* Even if the player presses a skip hotkey, the engine will ignore it if `enable` is set to `false`.

---

## `config`

Set Configuration Value

The `config` tag allows you to modify the game system's configuration settings directly from the markup. 
It is essential for dynamically adjusting the game's UI, such as moving the message box or changing system-level parameters on the fly.

### Basic Usage

```
# Change the position of the message box
[config name="msgbox.x" value="100"]
[config name="msgbox.y" value="200"]

# Update a specific system setting
[config name="msgbox.font.size" value="24"]
```

### Arguments

| Argument | Omissible | Description                                        | Notes                                                    |
|----------|-----------|----------------------------------------------------|----------------------------------------------------------|
| `name`   | No        | The name of the configuration parameter to change. | Refer to the system's config list for valid names.       |
| `value`  | No        | The new value to assign to the parameter.          | Values are handled as strings but may represent numbers. |

### Tips

**UI Customization**:
* You can use `[config]` to reposition the message box during specific scenes to create a more cinematic feel.

**Dynamic Adjustments**:
* Since this tag can be called anywhere in your script, you can change the game's "look and feel" as the story progresses.
* For example, shifting the UI for a "flashback" sequence.

**Parameter Names**:
* Be careful with the `name` argument!
* It must exactly match the internal configuration keys defined in your Suika3 project settings.
* See also [the complete list of the configurations](config.md)

---

## `layer`

Direct Layer Manipulation

The `layer` tag allows for direct control over specific image and text layers. 
While tags like `[bg]` and `[ch]` are easier for standard scenes, `[layer]` gives you the precision to modify any individual layer's position, scale, and rotation independently.

### Basic Usage

```
# Load an image directly onto the center character layer (chc)
[layer name="chc" file="heroine_smile.png"]

# Adjust only the position and opacity of the background (bg)
[layer name="bg" x="100" y="100" alpha="128"]

# Rotate the face layer
[layer name="chf" rotate="45.0" center-x="100" center-y="100"]
```

### Arguments

| Argument  | Omissible   | Description                          | Notes                              |
|-----------|-------------|--------------------------------------|------------------------------------|
| `name`    | No          | The target layer name.               | See the "Layer Names" table below. |
| `file`    | Yes         | The filename to load onto the layer. | Use `none` to clear the layer.     |
| `x`       | Yes (`0`)   | The layer's X position.              |                                    |
| `y`       | Yes (`0`)   | The layer's Y position.              |                                    |
| `alpha`   | Yes (`255`) | The layer's opacity level.           | `0` to `255`.                      |
| `scale-x` | Yes (`1.0`) | X-axis scaling factor.               | `1.0` is original size.            |
| `scale-y` | Yes (`1.0`) | Y-axis scaling factor.               | `1.0` is original size.            |
| `center-x`| Yes (`0`)   | Rotation center (X).                 | Pivot point for rotation.          |
| `center-y`| Yes (`0`)   | Rotation center (Y).                 | Pivot point for rotation.          |
| `rotate`  | Yes (`0.0`) | Rotation in degrees.                 | Positive for clockwise.            |

### Common Layer Names (`name`)

Suika3 has a rich set of predefined layers.

Here are the complete list of the layers:

|Layer Name       |Description                              |
|-----------------|-----------------------------------------|
|bg               |Background Image                         |
|bg2              |Background Image 2                       |
|efb1             |Back Effect 1                            |
|efb2             |Back Effect 2                            |
|efb3             |Back Effect 3                            |
|efb4             |Back Effect 4                            |
|chb              |Center-Back Character                    |
|chb-eye          |Center-Back Character's Eyes             |
|chb-lip          |Center-Back Character's Lips             |
|chb-fo           |Fading-Out Center-Back Character         |
|chl              |Left Character                           |
|chl-eye          |Left Character's Eyes                    |
|chl-lip          |Left Character's Lips                    |
|chl-fo           |Fading-Out Left Character                |
|chlc             |Left-Center Character                    |
|chlc-eye         |Left-Center Character's Eyes             |
|chlc-lip         |Left-Center Character's Lips             |
|chlc-fo          |Fading-Out Left-Center Character         |
|chr              |Right Character                          |
|chr-eye          |Right Character's Eyes                   |
|chr-lip          |Right Character's Lips                   |
|chr-fo           |Fading-Out Right Character               |
|chrc             |Right-Center Character                   |
|chrc-eye         |Right-Center Character's Eyes            |
|chrc-lip         |Right-Center Character's Lips            |
|chrc-fo          |Fading-Out Right-Center Character        |
|chc              |Center Character                         |
|chc-eye          |Center Character's Eyes                  |
|chc-lip          |Center Character's Lips                  |
|chc-fo           |Fading-Out Center Character              |
|msgbox           |Message box (Invisible to `[layer]`)     |
|namebox          |Name box (Invisible to `[layer]`)        |
|click            |Click animation (Invisible to `[layer]`) |
|eff1             |Front Effect 1                           |
|eff2             |Front Effect 2                           |
|eff3             |Front Effect 3                           |
|eff4             |Front Effect 4                           |
|chf              |Face Character                           |
|chf-eye          |Face Character's Eyes                    |
|chf-lip          |Face Character's Lips                    |
|chf-fo           |Fading-Out Face Character                |
|text1            |Text Layer 1                             |
|text2            |Text Layer 2                             |
|text3            |Text Layer 3                             |
|text4            |Text Layer 4                             |
|text5            |Text Layer 5                             |
|text6            |Text Layer 6                             |
|text7            |Text Layer 7                             |
|text8            |Text Layer 8                             |
|gui1             |GUI Button 1 (Invisible to `[layer]`)    |
|gui2             |GUI Button 2 (Invisible to `[layer]`)    |
|gui3             |GUI Button 3 (Invisible to `[layer]`)    |
|gui4             |GUI Button 4 (Invisible to `[layer]`)    |
|gui5             |GUI Button 5 (Invisible to `[layer]`)    |
|gui6             |GUI Button 6 (Invisible to `[layer]`)    |
|gui7             |GUI Button 7 (Invisible to `[layer]`)    |
|gui8             |GUI Button 8 (Invisible to `[layer]`)    |
|gui9             |GUI Button 9 (Invisible to `[layer]`)    |
|gui10            |GUI Button 10 (Invisible to `[layer]`)   |
|gui11            |GUI Button 11 (Invisible to `[layer]`)   |
|gui12            |GUI Button 12 (Invisible to `[layer]`)   |
|gui13            |GUI Button 13 (Invisible to `[layer]`)   |
|gui14            |GUI Button 14 (Invisible to `[layer]`)   |
|gui15            |GUI Button 15 (Invisible to `[layer]`)   |
|gui16            |GUI Button 16 (Invisible to `[layer]`)   |
|gui17            |GUI Button 17 (Invisible to `[layer]`)   |
|gui18            |GUI Button 18 (Invisible to `[layer]`)   |
|gui19            |GUI Button 19 (Invisible to `[layer]`)   |
|gui20            |GUI Button 20 (Invisible to `[layer]`)   |
|gui21            |GUI Button 21 (Invisible to `[layer]`)   |
|gui22            |GUI Button 22 (Invisible to `[layer]`)   |
|gui23            |GUI Button 23 (Invisible to `[layer]`)   |
|gui24            |GUI Button 24 (Invisible to `[layer]`)   |
|gui25            |GUI Button 25 (Invisible to `[layer]`)   |
|gui26            |GUI Button 26 (Invisible to `[layer]`)   |
|gui27            |GUI Button 27 (Invisible to `[layer]`)   |
|gui28            |GUI Button 28 (Invisible to `[layer]`)   |
|gui29            |GUI Button 29 (Invisible to `[layer]`)   |
|gui30            |GUI Button 30 (Invisible to `[layer]`)   |
|gui31            |GUI Button 31 (Invisible to `[layer]`)   |
|gui32            |GUI Button 32 (Invisible to `[layer]`)   |

### Tips

**Precision Control**:
* Use `[layer]` when you want to load an image to a layer manually when you're working with custom effect layers (`eff1` etc.) that don't have dedicated tags.

**Instant Updates**:
* Unlike `[ch]` or `[bg]`, the `layer` tag usually updates the screen instantly.
* If you want to animate these changes over time, you should use the `[move]` tag instead!

**Layer Hierarchy**:
* Remember that layers are stacked.
* For example, `chf` (Face Character) is always rendered in front of `chc` (Center Character).
* Understanding this "Z-order" is key to complex visual compositions.

---

## `move`

Animate Layer

The `move` tag animates specific layers over a set duration.
It is perfect for creating sliding effects, zooming in on characters, or rotating screen elements to add dynamic energy to your scenes.

### Basic Usage

```
# Move the center character to a new position over 2.0 seconds
[move time="2.0" center-x="150" center-y="100"]

# Relative movement: Nudge the background 50px to the right
[move time="1.0" bg-x="r50"]

# Gradually fade out a layer while rotating it
[move time="3.0" face-alpha="0" face-rotate="r360"]
```

### Arguments

**Common:**
| Argument         | Omissible     | Description                               | Notes                                      |
|------------------|---------------|-------------------------------------------|--------------------------------------------|
| `name`           | No            | The target layer to animate.              | See the "Moveable Layers" table below.     |
| `time`           | No            | The duration of the animation in seconds. | Supports decimal values (e.g., `0.5`).     |
| `async`          | Yes (`false`) | If `true`, do non-blocking animation.     |                                            |
| `accel`          | Yes (`normal`)| Acceleration type.                        | One of                                     |
| (layer)-(suffix) | Yes           |                                           |                                            |

**(layer):**
| Argument       | Description                               |
|----------------|-------------------------------------------|
| `bg`           | Background layer.                         |
| `bg2`          | Background 2.                             |
| `back          | Back-Center character.                    |
| `left`         | Left character.                           |
| `right`        | Right character.                          |
| `center`       | Center character.                         |
| `left-center`  | Left-Center character.                    |
| `right-center` | Intermediate character.                   |
| `face`         | Face character.                           |

**(suffix):**
| Suffix      | Omissible     | Description                | Notes                                                         |
|-------------|---------------|----------------------------|---------------------------------------------------------------|
| `-x`        | Yes (`0`)     | X position.                | Supports absolute (e.g., `100`) or relative (e.g., `r50`).    |
| `-y`        | Yes (`0`)     | Y position.                | Supports absolute (e.g., `100`) or relative (e.g., `r-50`).   |
| `-a`        | Yes (`255`)   | Alpha value. (opacity)     | `0` (transparent) to `255` (opaque).                          |
| `-scale-x`  | Yes (`1.0`)   | X scaling factor.          | `1.0` is original size. Supports `r` prefix.                  |
| `-scale-y`  | Yes (`1.0`)   | Y scaling factor.          | `1.0` is original size. Supports `r` prefix.                  |
| `-center-x` | Yes (`0`)     | X center for rotation.     | Pivot point for the rotation effect.                          |
| `-center-y` | Yes (`0`)     | Y center for rotation.     | Pivot point for the rotation effect.                          |
| `-rotate`   | Yes (`0`)     | Rotation in degrees.       | Positive for clockwise. Supports `r` prefix.                  |
| `-dim`      | Yes (`false`) | Dimming status.            | If `true`, the layer is rendered 50% darker.                  |

### Tips

**Non-blocking Animation (`async="true")`**:
* The script continues to the next command immediately after starting a `[move]`.
* If you want the script to wait until the animation finishes, follow it with a `[wait]` tag using the same `time` value.

**Relative Transformations**:
* Using the `r` prefix (e.g., `x="r100"`) is incredibly useful for repetitive motions, like making a character "jump" or "shake" without calculating absolute coordinates.

**Visual Polish**:
* Combine `scale-x` and `scale-y` with `move` to create "zoom-in" effects on a character's face for dramatic close-ups!

---

## `pencil`

Pencil

Draw a text on a layer.

### Basic Usage

```
[pencil layer="bg" font-size="30" text="Hello, World!"]
```

### Arguments

| Argument      | Omissible        | Description              |
|---------------|------------------|--------------------------|
| text          | No               | Text to draw.            |
| layer         | Yes (`text1`)    | Layer name.              |
| font-type     | Yes (`0`)        | Font selection. (0-3)    |
| font-size     | Yes (`16`)       | Font size.               |
| color         | Yes (`#000000`)  | Font color.              |
| outline-width | Yes (`0`)        | Font outline width.      |
| outline-color | Yes (`#ffffff`)  | Font outline color.      |
| line-margin   | Yes              | Line margin.             |
| char-margin   | Yes (`0`)        | Character margin.        |
| x             | Yes (`0`)        | Drawing area X position. |
| y             | Yes (`0`)        | Drawing area Y position. |
| width         | Yes              | Drawing area width.      |
| height        | Yes              | Drawing area height.     |

## Supported Layer Name

|Layer Name       |Description                              |
|-----------------|-----------------------------------------|
|bg               |Background Image                         |
|bg2              |Background Image 2                       |
|efb1             |Back Effect 1                            |
|efb2             |Back Effect 2                            |
|efb3             |Back Effect 3                            |
|efb4             |Back Effect 4                            |
|chb              |Center-Back Character                    |
|chl              |Left Character                           |
|chlc             |Left-Center Character                    |
|chr              |Right Character                          |
|chrc             |Right-Center Character                   |
|chc              |Center Character                         |
|eff1             |Front Effect 1                           |
|eff2             |Front Effect 2                           |
|eff3             |Front Effect 3                           |
|eff4             |Front Effect 4                           |
|chf              |Face Character                           |
|text1            |Text Layer 1                             |
|text2            |Text Layer 2                             |
|text3            |Text Layer 3                             |
|text4            |Text Layer 4                             |
|text5            |Text Layer 5                             |
|text6            |Text Layer 6                             |
|text7            |Text Layer 7                             |
|text8            |Text Layer 8                             |

---

## `returnmacro`

Return from Macro

The `returnmacro` tag immediately exits the current macro and returns the script execution to the line following the original `[callmacro]` tag.
It is particularly useful for stopping a macro early based on specific conditions within an `[if]` block.

### Basic Usage

```
[defmacro name="check_item"]
    [if lhs="${has_key}" op="==" rhs="false"]
        [text text="The door is locked."]
        [returnmacro]
    [endif]

    # This part only runs if has_key is true
    [text text="You unlocked the door with the key!"]
[endmacro]
```

### Arguments

This tag does not take any arguments.

| Argument | Omissible | Description | Notes |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### Tips

**Early Exit**:
* Use `[returnmacro]` inside an `[if]` block to skip the rest of a macro's commands if a certain condition is met.
* This keeps your macros flexible and powerful!

**Implicit Return**:
* You don't actually need to put `[returnmacro]` at the very end of every macro.
* Once the engine hits the `[endmacro]` tag, it will return to the main script automatically.

**Flow Control**:
* Remember that this tag only exits the *current* macro. It doesn't stop the whole game or jump to a different label—it just sends you back to where the macro was called from.

---

## `video`

Play Video

The `video` tag plays a movie file on the screen.
It is ideal for opening cinematics, transitional cutscenes, or high-impact visual effects that are best rendered as full-motion video.

### Basic Usage

```
# Play an opening movie (cannot be skipped)
[video file="opening.mp4"]

# Play a short cutscene that the player can skip with a click.
[video file="cutscene01.mp4" skippable="true"]
```

### Arguments

| Argument    | Omissible     | Description                                           | Notes                                                         |
|-------------|---------------|-------------------------------------------------------|---------------------------------------------------------------|
| `file`      | No            | The filename of the video to play.                    | The file must be in a supported format (e.g., .mp4).          |
| `skippable` | Yes (`false`) | Whether the video can be skipped by a player's click. | Set to `false` to force the player to watch the entire video. |

### Tips

**File Support**:
* Ensure your video file is .mp4 (H.264 + AAC) format.
* If you want to support 32-bit Windows, prepare .wmv file alongside .mp4 file, then remove extension e.g., `[video file="opening"]`.

**Transitioning**:
* Once the video finishes playing (or is skipped), the engine automatically proceeds to the next command in your script.
* It's often a good idea to follow a `[video]` tag with a `[bg]` tag to ensure the screen looks exactly how you want it after the movie ends.

**Audio in Video**:
* Most video files include their own audio track.
* Keep in mind that this audio will play alongside any `[bgm]` you have running.
* You might want to stop the music with `[bgm file="none"]` before starting a video with sound!

---

## `wait`

Wait for Time

The `wait` tag pauses the NovelML execution for a specified duration.
It is essential for controlling the pacing of visual transitions, creating dramatic pauses, or timing effects without requiring player input.

### Basic Usage

```
# Pause for 1.5 seconds before the next command
[wait time="1.5"]

# Create a brief pause between character changes
[ch center="chara01_surprised.png" time="0.5"]
[wait time="1.0"]
[text text="She couldn't believe her eyes."]
```

### Arguments

| Argument      | Omissible     | Description                    | Notes                                  |
|---------------|---------------|--------------------------------|----------------------------------------|
| `time`        | No            | The number of seconds to wait. | Supports decimal values (e.g., `0.5`). |
| `hidemsgbox`  | Yes (`false`) | Force hide the message box.    |                                        |
| `hidenamebox` | Yes (`false`) | Force hide the name box.       |                                        |

### Tips

**Non-interactive Pause**:
* Unlike `[click]`, which waits for the player to act, `[wait]` continues automatically once the time is up. 
* This is perfect for "auto-playing" segments or timed visual sequences.

**Combining with Animations**:
* If you use a `[ch]` or `[bg]` tag with a `time` argument, the engine moves to the next command immediately while the animation plays. 
* Use `[wait]` after an animation if you want the script to stop until the animation is finished (or even longer for dramatic effect).

**User Experience**:
* Be careful not to make `[wait]` times too long (like more than 3 seconds) without a visual reason, or the player might think the game has frozen!

