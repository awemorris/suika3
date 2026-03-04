Suika3 Tag Reference
====================

## Index

| Tag Name                    | Description                                                        |
|-----------------------------|--------------------------------------------------------------------|
| [anime](#anime)             | Loads and runs an animation file.                                  |
| [bg](bg)                    | Changes the background image with a fading effect.                 |
| [bgm](bgm)                  | Plays a background music file (Ogg Vorbis format).                 |
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
| `bg`           | Background and Background-overlay layers. |
| `back          | Back-Center character.                    |
| `left`         | Left character.                           |
| `right`        | Right character.                          |
| `center`       | Center character.                         |
| `left-center`  | Left-Center character.                    |
| `right-center` | Intermediate character.                   |
| `face`         | Face character.                           |

#### Layer Parameter Arguments

Each layer above (e.g., `center`) can be customized using the following suffixes (e.g., `center-x`, `center-rotate`).

| Suffix    | Omissible     | Description                | Notes                                                         |
|-----------|---------------|----------------------------|---------------------------------------------------------------|
| `-x`      | Yes (`0`)     | X position.                | Supports absolute (e.g., `100`) or relative (e.g., `r50`).    |
| `-y`      | Yes (`0`)     | Y position.                | Supports absolute (e.g., `100`) or relative (e.g., `r-50`).   |
| `-a`      | Yes (`255`)   | Alpha value. (opacity)     | `0` (transparent) to `255` (opaque).                          |
| `-sx`     | Yes (`1.0`)   | X scaling factor.          | `1.0` is original size. Supports `r` prefix.                  |
| `-sy`     | Yes (`1.0`)   | Y scaling factor.          | `1.0` is original size. Supports `r` prefix.                  |
| `-cx`     | Yes (`0`)     | X center for rotation.     | Pivot point for the rotation effect.                          |
| `-cy`     | Yes (`0`)     | Y center for rotation.     | Pivot point for the rotation effect.                          |
| `-rotate` | Yes (`0`)     | Rotation in degrees.       | Positive for clockwise. Supports `r` prefix.                  |
| `-dim`    | Yes (`false`) | Dimming status.            | If `true`, the layer is rendered 50% darker.                  |

### Tips

**Batch Updates**:
* You can update multiple characters and the background simultaneously in a single `[ch]` tag to ensure they animate together perfectly.

**Relative Transformation**:
* Like the `bg` tag, all numeric parameters support the `r` prefix.
* For example, `center-y="r-50"` will hop the center character 50 pixels upward from its current position.

---

## `choose`

Display Selection Options

The `choose` tag displays up to 8 interactive buttons for the player. 
It stores the text of the chosen item in a variable.

### Basic Usage

```
# Store selection in a variable
[choose
    name="user_choice"
    text1="Red Pill"
    text2="Blue Pill"]
```

### Arguments

| Argument | Omissible | Description                                 | Notes                                              |
|----------|-----------|---------------------------------------------|--------------------------------------------------- |
| `name`   | No        | The variable name to store the result.      | Stores the text of the selected option.            |
| `text1`  | Yes       | The text displayed on each button.          | At least one option are typically required.        |
| `text2`  | Yes       | The text displayed on each button.          | At least one option are typically required.        |
| `text3`  | Yes       | The text displayed on each button.          | At least one option are typically required.        |
| `text4`  | Yes       | The text displayed on each button.          | At least one option are typically required.        |
| `text5`  | Yes       | The text displayed on each button.          | At least one option are typically required.        |
| `text6`  | Yes       | The text displayed on each button.          | At least one option are typically required.        |
| `text7`  | Yes       | The text displayed on each button.          | At least one option are typically required.        |
| `text8`  | Yes       | The text displayed on each button.          | At least one option are typically required.        |
| `time`   | Yes (`0`) | Timer in seconds.                           | If `0`, no timer is enabled.                       |

### Tips

**Branching Logic**:
* You can use the `[if]` tag to check the stored value and create complex branches.

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
```

### Arguments

| Argument | Omissible     | Description                              | Notes                                                               |
|----------|---------------|------------------------------------------|---------------------------------------------------------------------|
| `name`   | No            | The unique name of the variable.         | Use alphanumeric characters and underscores for best compatibility. |
| `value`  | No            | The content to store in the variable.    | Remember: everything is stored as a string!                         |
| `global` | Yes (`false`) | Make the flag global.                    | Global variables are for achievement flags e.g., "Saw ED1".         |

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

| Argument   | Omissible | Description                                      | Notes                                            |
|------------|-----------|--------------------------------------------------|--------------------------------------------------|
| `text`     | No        | The message content to be displayed.             |                                                  |
| `name`     | Yes       | The character's name to display in the name box. | If omitted, the name box will usually be hidden. |

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

| Argument | Omissible | Description                    | Notes                                  |
|----------|-----------|--------------------------------|----------------------------------------|
| `time`   | No        | The number of seconds to wait. | Supports decimal values (e.g., `0.5`). |

### Tips

**Non-interactive Pause**:
* Unlike `[click]`, which waits for the player to act, `[wait]` continues automatically once the time is up. 
* This is perfect for "auto-playing" segments or timed visual sequences.

**Combining with Animations**:
* If you use a `[ch]` or `[bg]` tag with a `time` argument, the engine moves to the next command immediately while the animation plays. 
* Use `[wait]` after an animation if you want the script to stop until the animation is finished (or even longer for dramatic effect).

**User Experience**:
* Be careful not to make `[wait]` times too long (like more than 3 seconds) without a visual reason, or the player might think the game has frozen!

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

The `volume` tag sets a sound volume for a track with a fading.

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

## `chapter`

The `chapter` tag sets a chapter name.

Basic Usage:
```
[chapter name="Chapter 01"]
```

### `name` argument

This argument specifies the chapter name.

---

## `skip`

The `skip` tag sets a skip enable status.

Basic Usage:
```
[skip enable="true"]
[skip enable="false"]
```

### `enable` argument

This argument specifies whether skip is enabled or not.

---

## `config`

The `config` tag sets a config value.

Basic Usage:
```
[config name="msgbox.x" value="100"]
```

### `name` argument

This argument specifies the config name.

### `value` argument

This argument specifies the config value.

---

## `layer`

The `layer` tag loads an image, unload an image, or sets parameters for a layer.

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

## `move`

The `move` tag animates a character.

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

## `defmacro`

The `defmacro` tag starts a macro definition.

Basic Usage:
```
[defmacro name="MyMacro1"]
    [text text="Some messages."]
[endmacro]
```

---

## `callmacro`

The `callmacro` tag calls a macro.

Basic Usage:
```
[callmacro name="MyMacro1"]
```

---

## `returnmacro`

The `returnmacro` tag returns from a macro.

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

## `video`

The `video` tag plays a video.

Basic Usage:
```
[video file="myvideo.mp4" skippable="false"]
```

### `file` arguments

This arguments specifies a file name to play.

### `skippable` arguments

This arguments specifies whether the video is skippable by a click or not.

## `goto`

Jump to Label

The `goto` tag immediately moves the script execution to a specified label.

### Basic Usage

Jump to a label named "next_day"
[goto name="next_day"]


### Arguments

| Argument | Omissible | Description | Notes |
| :--- | :--- | :--- | :--- |
| `name` | No | The target label name. | Must match a `[label]` defined in the script. |

---

### Tips

**Branching Logic**:
* If you use `choose` with `label1`, the game jumps immediately to that section.
* If you use `name`, the chosen text is stored, and the script continues to the next line.
* You can then use the `[if]` tag to check the stored value and create complex branches.

**Variable Persistence**:
* Since everything is a string, remember that even numbers like "100" are stored as text.
* Suika3's logic tags (like `if`) can handle these strings for comparisons.

---
