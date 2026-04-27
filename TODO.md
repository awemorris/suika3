TODO
====

## Remaining Tasks

- `Auto Focus` feature is not working.
- Unity version is not tested after the final shader change due to RC1 time limit.

---

## Text Tag Testing

### Internationalization

English:

| Check Point                            |
|----------------------------------------|
| System `en` to `text-en` mapping       |
| System `en` to `text` fallback         |
| System `en-US` to `text-en-us` mapping |
| System `en-US` to `text-en` fallback   |
| System `en-US` to `text` fallback      |
| System `en-GB` to `text-en-gb` mapping |
| System `en-GB` to `text-en` fallback   |
| System `en-GB` to `text` fallback      |

Spanish:

| Check Point                            |
|----------------------------------------|
| System `es-ES` to `text-es-es` mapping |
| System `es-ES` to `text-es` fallback   |
| System `es-ES` to `text` fallback      |
| System `es-CL` to `text-es-la` mapping |
| System `es-CL` to `text-es` fallback   |
| System `es-CL` to `text` fallback      |

Japanese:

| Check Point                            |
|----------------------------------------|
| System `ja` to `text-ja` mapping       |
| System `ja` to `text` fallback         |

Voice:

- Argument `voice-en-us` --> File `voice/en-us/FILE` --> `voice-en` --> `voice/en/FILE` --> `voice`
- Argument `voice-ja` --> File `voice/ja/FILE` --> `voice`
- (where FILE is the value of the argument `voice`)

Overall:

- Save --> Exit the game --> Change the OS locale --> Start the game --> Load --> Is the displayed language changed?

### System Menu Transition

| From                    | Action                    | To                          | Result                  |
|-------------------------|---------------------------|-----------------------------|-------------------------|
| text tag                | System Menu click         | Save GUI                    |                         |
| Save GUI                | Canel                     | text tag (back)             |                         |
| Save GUI                | Save, then canel          | text tag (back)             |                         |
| text tag                | System Menu click         | Load GUI                    |                         |
| Load GUI                | Cancel                    | text tag (back)             |                         |
| load GUI                | Load                      | Save point                  |                         |
| text tag                | System Menu click         | Auto mode                   |                         |
| Auto mode               | Cancel                    | text tag                    |                         |
| text tag                | System Menu click         | Skip mode                   |                         |
| Skip mode               | Cancel                    | text tag                    |                         |
| text tag                | System Menu click         | History GUI                 |                         |
| History GUI             | Cancel                    | text tag                    |                         |
| History GUI             | Click (voice)             | History GUI                 |                         |
| text tag                | System Menu click         | Config GUI                  |                         |

### Full Screen Novel (Horizontal Writing)

| Check Point             | Result                       |
|-------------------------|------------------------------|
| Append                  |                              |
| Flush                   |                              |
| New Page                |                              |
| Dimming                 |                              |

### Full Screen Novel (Vertical Writing)

| Check Point             | Result                       |
|-------------------------|------------------------------|
| Append                  |                              |
| Flush                   |                              |
| New Page                |                              |
| Dimming                 |                              |

---

## Choose Tag Testing

- Show options
    - 1 option
    - 2 options
    - 3 options
    - 4 options
    - 5 options
    - 6 options
    - 7 options
    - 8 options
- Auto removal of empty string option 
- Idle anime
- Hover anime
- Store the result to a variable
    - `value<N>`
    - `text` fallback
- Inline option for full screen novels

### System Menu Transition

| From                    | Action                    | To                          | Result                  |
|-------------------------|---------------------------|-----------------------------|-------------------------|
| choose tag              | System Menu click         | Save GUI                    |                         |
| Save GUI                | Canel                     | choose tag (back)           |                         |
| Save GUI                | Save, then canel          | choose tag (back)           |                         |
| choose tag              | System Menu click         | Load GUI                    |                         |
| Load GUI                | Cancel                    | choose tag (back)           |                         |
| load GUI                | Load                      | Save point                  |                         |
| choose tag              | System Menu click         | Auto mode                   |                         |
| Auto mode               | Cancel                    | choose tag                  |                         |
| choose tag              | System Menu click         | Skip mode                   |                         |
| Skip mode               | Cancel                    | choose tag                  |                         |
| choose tag              | System Menu click         | History GUI                 |                         |
| History GUI             | Cancel                    | choose tag                  |                         |
| History GUI             | Click (voice)             | History GUI                 |                         |
| choose tag              | System Menu click         | Config GUI                  |                         |

---

## GUI Tag Testing

Buzz testing on:

- Title GUI
- Save GUI
- Load GUI
- History GUI
- Config GUI

---

## Bg Tag Testing

- Displaying a background image
- Changing a background image
- Removing a background image
- With alpha value
- With scaling
- Async

---

## Ch Tag Testing

- Change 1 character
    - Show a character
    - Shift
    - Scale
- Change 2 character
    - Show characters
    - Shift
    - Scale
- Change 3 character
    - Show characters
    - Shift
    - Scale
- Remove 1 character
- Remove 2 character
- Change 1 character, remove 1 character
- With background change

---

## Move Tag Testing


- Move 1 character
- Move 2 characters
- Move 3 characters

---

## Layer Tag Testing

- Load
- Unload

---

## If/elseif/else/endif Tags Testing

- if
- if-else
- if-elseif
- if-elesif-else
- if-elesif-elseif
- if-elesif-elseif-else
- Failure on single else, elseif-else, else-else, if-endif-else

---

## Label and Goto Tags Testing

- Jumping

---

## Bgm Tag Testing

- Play
- With loop (default)
- Without loop (option)
- Stop

---

## Anime Tag Testing

- Sync
- Async
- Reg, save, load
- Stop async

---

## Defmacro/callmacro/endmacro/returnmacro Tags Testing

- Definitions must be skipped its execution
- Put the definition on the beginning of a tag file
- Put the definition on the end of a tag file
- Call
- No returnmacro
- if-returnmacro sequence

---

## chapter

- Set
- Save
- Load
- Remove (empty string)

---

## Click Tag Testing

---

## Config Tag Testing

- Change string configs
- Change int configs
- Change float configs
- Change boolean configs
- Change the message box position (must be immediately applied)

---

## Load Tag

- File
- Fiel + Label
- Loop between multiple files

---

## SE Tag Testing

- Play
- Stop
- Loop
- Stop Looping
- Save a looped SE

---

## Set Tag Testing

- set-if-set
- choose-if-set

---

## Skip Tag Testing

- Make a logo screen in the sample game

---

## Video Tag Testing

- Windows
- macOS
- Linux
- iOS
- Android
- Allow Skip

---

## Volume Tag Testing

---

## Wait Tag Testing

- Execution
- With hidemsgbox=true
- With hidenamebox=true
