Suika3 标签参考
====================

## 索引

| Tag Name                    | Description                                                        |
|-----------------------------|--------------------------------------------------------------------|
| [anime](#anime)             | 载入并执行动画档。                                                 |
| [bg](#bg)                   | 以淡出效果变更背景图片。                                           |
| [bgm](#bgm)                 | 播放背景音乐档（Ogg Vorbis 格式）。                                |
| [callmacro](#callmacro)     | 呼叫已定义的巨集。                                                 |
| [ch](#ch)                   | 以详细图层引数显示或隐藏角色。                                     |
| [chapter](#chapter)         | 设定章节名称。                                                     |
| [choose](#choose)           | 显示选项并储存选择，或跳到标签。                                   |
| [click](#click)             | 等待使用者点选。                                                   |
| [config](#config)           | 设定游戏系统的设定值。                                             |
| [defmacro](#defmacro)       | 开始定义巨集。                                                     |
| [else](#else)               | if/elseif 分支中，当没有条件成立时使用。                           |
| [elseif](#elseif)           | 在分支中指定额外条件。                                             |
| [endif](#endif)             | 结束条件分支。                                                     |
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

执行动画

`anime` 标签会从档案载入并执行动画定义。
它可用来做出复杂的视觉效果、角色移动，或超越简单转场的回圈环境动画。

### Basic Usage

```
# 执行同步动画（等待完成）
[anime file="opening_effect.txt"]

# 执行非同步回圈动画
[anime file="sparkle.txt" async="true" register="my_loop"]

# 停止已注册的非同步动画
[anime stop="true" register="my_loop"]
```

### Arguments

| Argument      | Omissible      | Description                                        | Notes                                                             |
|---------------|----------------|----------------------------------------------------|-------------------------------------------------------------------|
| `file`        | Yes            | 动画定义档的档名。                                 | 除非使用 `stop="true"`，否则为必填。                              |
| `async`       | Yes (`false`)  | 是否以非同步方式执行动画。                         | 若为 `false`，指令码会等待动画结束。                                |
| `register`    | Yes            | 用来识别此动画例项的唯一名称。                     | 之后控制或停止非同步动画时会用到。                                |
| `stop`        | Yes (`false`)  | 若设为 `true`，则停止已注册的动画。                | 需要搭配 `register` 引数。                                        |
| `showsysbtn`  | Yes (`true`)   | 播放期间是否显示系统按钮。                         | 只适用于同步动画。                                                |
| `showmsgbox`  | Yes (`true`)   | 播放期间是否显示讯息框。                           | 只适用于同步动画。                                                |
| `shownamebox` | Yes (`true`)   | 播放期间是否显示名称框。                           | 只适用于同步动画。                                                |

### Tips

**同步与非同步**：
* **同步（`async="false"`）**：适合过场动画，让玩家先看完动画，再显示文字或选项。
* **非同步（`async="true"`）**：适合背景效果，例如飘雪或闪烁灯光，让故事继续时动画也持续播放。

**管理例项**：
* 透过 `register` 引数，你可以替特定动画加上名称。
* 这就是你在使用 `stop="true"` 时，告诉引擎要停止哪个动画的方法。

**UI 控制**：
* 如果动画要占满全萤幕，而你希望对话视窗暂时消失以保持画面干净，可以使用 `showmsgbox="false"`。

---

## `bg`

变更背景

`bg` 标签会以平滑淡入淡出效果变更背景图片。
这是视觉小说里设定场景的主要方式。

### Basic Usage

```
# 在 1.0 秒内切换到 background.png
[bg file="background.png" time="1.0"]

# 淡出成黑画面（移除背景）
[bg file="none" time="1.0"]
```

### Arguments

| Argument   | Omissible      | Description                                   | Notes                                                                        |
|------------|----------------|-----------------------------------------------|------------------------------------------------------------------------------|
| `file`     | No             | 新背景图片的档名。                              | 设为 `none` 可移除背景。                                                     |
| `time`     | Yes (`0`)      | 淡出效果的持续时间（秒）。                      | 预设为 `0.0`（立即变更）。                                                   |
| `method`   | Yes (`normal`) | 淡出方法/样式。                                | 可选 `normal`、`rule` 或 `melt`。                                            |
| `rule`     | Yes            | 特定转场所需的规则图档。                        | 当 `method` 设为 `rule` 或 `melt` 时必填。                                   |
| `x`        | Yes (`0`)      | 背景图片的 X 轴位移。                           | 支援绝对值（例如 `100`）或相对值（例如 `r100`）。                            |
| `y`        | Yes (`0`)      | 背景图片的 Y 轴位移。                           | 支援绝对值（例如 `100`）或相对值（例如 `r-100`）。                           |
| `alpha`    | Yes (`255`)    | 背景图片的 alpha 值。                           | `0` 到 `255`。                                                               |
| `clear`    | Yes (`false`)  | 是否让角色消失。                                | 若为 `true`，所有角色都会消失。                                             |

### Transition Methods (`method`)

透过选择适合的转场样式，可以营造不同氛围：

| Type     | Description                                                                                                                          |
|----------|--------------------------------------------------------------------------------------------------------------------------------------|
| `normal` | Alpha 混合。预设方法。会在旧图片与新图片之间平滑交叉淡化。                              |
| `rule`   | 1-bit Universal Transition。使用灰阶 `rule` 图决定切换顺序。                           |
| `melt`   | 8-bit Universal Transition。类似 `rule`，但转场边缘带有柔和模糊，形成「融化」效果。       |

在 `rule` 与 `melt` 中，图片会依照规则图由最暗区域一路切换到最亮区域。

### Tips

**相对定位**：
* 如果你想从目前位置微调背景，请使用 `r` 字首。
* 例如，`x="r50"` 会让图片从目前 X 座标往右移 50 画素。

**什么是 `rule` 图？**
* 这是一张灰阶图片，黑色区域会先转场，白色区域会最后转场。
* 透过自订 `rule` 图，你可以做出水平擦除、圆形展开，甚至更有艺术感的图样！

---

## `bgm`

播放背景音乐

`bgm` 标签会播放背景音乐曲目。
音乐是营造场景氛围的重要工具，并且会自动回圈播放，直到被停止或更换。

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

呼叫巨集

`callmacro` 标签会执行先前定义好的巨集。
它可以让你在脚本中多次触发特定的一串命令，例如角色登场或 UI 动画，而不必重写原始程式码。

### Basic Usage

```
# 呼叫名为 "kaito_entrance" 的巨集
[callmacro name="kaito_entrance"]

# 呼叫用于特定场景转场的巨集
[callmacro name="fade_to_white"]
```

### Arguments

| Argument | Omissible | Description                               | Notes                                              |
|----------|-----------|-------------------------------------------|----------------------------------------------------|
| `name`   | No        | 要执行的巨集名称。                        | 必须与 `[defmacro]` 标签定义的名称一致。          |
| `file`   | Yes       | 巨集所在的档案名称。                      | 若要呼叫目前档案内的巨集，则可省略。              |

### Tips

**效率**：
* 使用 `[callmacro]` 可以让主剧情脚本保持专注且易读。
* 原本要看十行动画程式码，现在只会看到一条清楚的命令。

**执行流程**：
* 当引擎碰到 `[callmacro]` 时，会立刻跳到已定义的巨集，执行里面的所有标签，然后自动回到 `[callmacro]` 标签后的下一行。

**模组化设计**：
* 可以把巨集想成你游戏里的「自订标签」。
* 如果你想改变角色入场方式，只需要在 `[defmacro]` 区块更新一次程式码，每个 `[callmacro]` 都会跟著反映这个变更！

---

## `ch`

角色显示

`ch` 标签可在各个图层上显示、隐藏或更新角色图片。
它可以同时精细控制多个角色与背景的位置、缩放与旋转。

### Basic Usage

```
# 在中央显示角色
[ch center="chara001.png" time="1.0"]

# 显示多个角色并指定位置
[ch left="chara002.png" right="chara003.png" time="0.5"]

# 隐藏指定角色
[ch center="none" time="1.0"]
```

### Arguments

| Argument  | Omissible      | Description                            | Notes                                                 |
|-----------|----------------|----------------------------------------|-------------------------------------------------------|
| `time`    | Yes (`0`)      | 转场持续时间（秒）。                   | 会影响这个标签内所有图层变更。                        |
| `method`  | Yes (`normal`) | 淡入淡出的方式/样式。                  | `normal`、`rule` 或 `melt`。                         |
| `rule`    | Yes            | 转场用的规则图档。                     | 当 `method` 是 `rule` 或 `melt` 时必填。              |

#### 图层档案引数

指定档名即可将图片载入到某个图层。设为 `none` 则会解除安装（隐藏）图片。

| Argument       | Description                               |
|----------------|-------------------------------------------|
| `bg`           | 背景图层。                                |
| `back`         | 后中角色。                                |
| `left`         | 左侧角色。                                |
| `right`        | 右侧角色。                                |
| `center`       | 中央角色。                                |
| `left-center`  | 左中角色。                                |
| `right-center` | 右中角色。                                |
| `face`         | 脸部角色。                                |

#### 图层引数引数

上面的每个图层（例如 `center`）都可以使用下列字尾自订（例如 `center-x`、`center-rotate`）。

| Suffix      | Omissible     | Description                | Notes                                                         |
|-------------|---------------|----------------------------|---------------------------------------------------------------|
| `-x`        | Yes (`0`)     | X 座标。                   | 支援绝对值（例如 `100`）或相对值（例如 `r50`）。              |
| `-y`        | Yes (`0`)     | Y 座标。                   | 支援绝对值（例如 `100`）或相对值（例如 `r-50`）。             |
| `-a`        | Yes (`255`)   | Alpha 值（不透明度）。     | `0`（透明）到 `255`（不透明）。                               |
| `-scale-x`  | Yes (`1.0`)   | X 缩放倍率。               | `1.0` 为原始大小。支援 `r` 字首。                             |
| `-scale-y`  | Yes (`1.0`)   | Y 缩放倍率。               | `1.0` 为原始大小。支援 `r` 字首。                             |
| `-center-x` | Yes (`0`)     | 旋转中心 X。               | 旋转效果的枢轴点。                                            |
| `-center-y` | Yes (`0`)     | 旋转中心 Y。               | 旋转效果的枢轴点。                                            |
| `-rotate`   | Yes (`0`)     | 旋转角度（度）。           | 正值为顺时针。支援 `r` 字首。                                  |
| `-dim`      | Yes (`false`) | 暗化状态。                 | 若为 `true`，图层会被渲染得暗 50%。                            |

### Tips

**批次更新**：
* 你可以在单一 `[ch]` 标签中同时更新多个角色与背景，让它们一起完美地进行动画。

**相对变换**：
* 和 `bg` 标签一样，所有数值引数都支援 `r` 字首。
* 例如 `center-y="r-50"` 会让中央角色从目前位置往上移动 50 画素。

---

## `chapter`

设定章节名称

`chapter` 标签会设定目前章节的名称。
这个名称通常会被游戏系统用来在存档/读档选单或游戏画面上显示进度，方便玩家追踪剧情进展。

### Basic Usage

```
# 在故事段落开头设定章节名称
[chapter name="Chapter 01: The Beginning"]

# 随著故事推进更新章节名称
[chapter name="Intermission: A Quiet Night"]
```

### Arguments

| Argument | Omissible | Description                        | Notes                                                      |
|----------|-----------|------------------------------------|------------------------------------------------------------|
| `name`   | No        | 要设定的章节名称。                 | 这个字串会储存在游戏的系统变数中。                        |

### Tips

**存档显示**：
* 在许多 Suika3 设定中，你在这里设定的字串会显示在「存档」与「读档」栏位上。
* 请选一个能让玩家清楚记得目前进度的名称！

**一致性**：
* 在开始新的主要场景或章节的 `[label]` 后立刻呼叫 `[chapter]` 标签，是个很好的习惯。

**更新名称**：
* 你可以随时多次呼叫 `[chapter]`。
* 每次呼叫时，旧的章节名称都会被新的名称覆盖。

---

## `choose`

显示选择选项

`choose` 标签会显示最多 8 个可互动按钮给玩家。
它会把所选专案的文字储存在变数中。

### Basic Usage

```
# 将选择结果储存在变数中
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
| `text1`          | Yes       | 每个按钮显示的文字。                           | 通常至少需要一个选项。                             |
| `text2`          | Yes       | 每个按钮显示的文字。                           | 通常至少需要一个选项。                             |
| `text3`          | Yes       | 每个按钮显示的文字。                           | 通常至少需要一个选项。                             |
| `text4`          | Yes       | 每个按钮显示的文字。                           | 通常至少需要一个选项。                             |
| `text5`          | Yes       | 每个按钮显示的文字。                           | 通常至少需要一个选项。                             |
| `text6`          | Yes       | 每个按钮显示的文字。                           | 通常至少需要一个选项。                             |
| `text7`          | Yes       | 每个按钮显示的文字。                           | 通常至少需要一个选项。                             |
| `text8`          | Yes       | 每个按钮显示的文字。                           | 通常至少需要一个选项。                             |
| `text<N>-locale` | Yes       | 每个按钮显示的文字（依语系）。                 | 通常至少需要一个选项。                             |
| `name`           | No        | 用来储存结果的变数名称。                       | 会储存所选选项的文字。                             |
| `value1`         | Yes       | 指派给结果变数的值。                           | 通常至少需要一个选项。                             |
| `value2`         | Yes       | 指派给结果变数的值。                           | 通常至少需要一个选项。                             |
| `value3`         | Yes       | 指派给结果变数的值。                           | 通常至少需要一个选项。                             |
| `value4`         | Yes       | 指派给结果变数的值。                           | 通常至少需要一个选项。                             |
| `value5`         | Yes       | 指派给结果变数的值。                           | 通常至少需要一个选项。                             |
| `value6`         | Yes       | 指派给结果变数的值。                           | 通常至少需要一个选项。                             |
| `value7`         | Yes       | 指派给结果变数的值。                           | 通常至少需要一个选项。                             |
| `value8`         | Yes       | 指派给结果变数的值。                           | 通常至少需要一个选项。                             |
| `time`           | Yes (`0`) | 计时器（秒）。                                | 若为 `0`，则不启用计时器。                         |

### Localization

例如，如果使用者的作业系统语系是日文，系统会优先采用 `text1-ja`，而不是 `text1`。

| Suffix      | Language                                 |
|-------------|------------------------------------------|
| -en         | 英文（预设）                             |
| -en-us      | 英文（美国）                             |
| -en-gb      | 英文（英国）                             |
| -en-au      | 英文（澳洲）                             |
| -en-nz      | 英文（纽西兰）                           |
| -fr         | 法文（预设）                             |
| -fr-fr      | 法文（法国）                             |
| -fr-ca      | 法文（加拿大）                           |
| -es         | 西班牙文（西班牙，预设）                 |
| -es-la      | 西班牙文（拉丁美洲）                     |
| -de         | 德文                                     |
| -it         | 义大利文                                 |
| -ru         | 俄文                                     |
| -el         | 希腊文                                   |
| -zh         | 中文（简体）                             |
| -zh-tw      | 中文（繁体，台湾）                       |
| -ja         | 日文                                     |
| （无字尾）  | 预设值（由开发者决定）                    |

对于所有英文系统语系，`-en` 会作为预装置援。
如果标签中指定了像 `-en-gb` 这样更精确的变体，而且它与使用者地区最相符，就会优先采用。
西班牙文与法文也使用相同机制。
请注意，繁体中文不会回退到简体中文。

例如，如果使用者语系是 `en-AU`，会套用以下优先顺序：
* 1. text1-en-au
* 2. text1-en
* 3. text1

以下目前尚未支援，但预计未来会支援。

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

**分支逻辑**：
* 你可以使用 `[if]` 标签检查储存的值，建立复杂分支。

```
[choose
  name="var1"
  text1="Go to school"
  text2="Go to hospital"
  value1="1"
  value2="2"]

[if lhs="${var1}" op="=" rhs="1"]
  # 学校。
[else]
  # 医院。
[endif]
```

**变数储存**：
* 因为所有内容本质上都是字串，请记得像 `"100"` 这样的数字也会以文字储存。
* Suika3 的逻辑标签（例如 `if`）可以拿这些字串来做比较。

---

## `set`

设定变数

`set` 标签会把值指定给某个变数名称。
在 Suika3 中，**所有变数都会被视为文字字串**，但在像 `[if]` 这类其他标签中仍可做数值比较。

### Basic Usage

```
# 将单纯字串指派给变数
[set name="player_name" value="Kaito"]

# 设定类似数字的值（仍会以字串储存）
[set name="health" value="100"]

# 透过设定空字串来清除变数
[set name="flag_event_01" value=""]

# 将 var1 加 1。
[set name="var1" value1="${var1}" op="+" value2="1"]

```

### Arguments

| Argument | Omissible     | Description                                 | Notes                                                               |
|----------|---------------|---------------------------------------------|---------------------------------------------------------------------|
| `name`   | No            | 变数的唯一名称。                            | 为了最佳相容性，请使用英数字元与底线。                              |
| `value`  | Yes           | 要储存在变数中的内容。                      | 请记住：所有内容都会以字串储存！                                     |
| `value1` | Yes           | 运算元 1。                                  |                                                                     |
| `value2` | Yes           | 运算元 2。                                  |                                                                     |
| `op`     | Yes           | 运运算元。（`+`、`-`、`*`、`/`、`//`、`%`）   |                                                                     |
| `global` | Yes (`false`) | 是否设为全域旗标。                           | 全域变数可用于成就旗标，例如「看过 ED1」。                           |

### Tips

**字串处理**：
* 由于 Suika3 会把所有内容都当成文字，`value="100"` 和 `value="May"` 在内部的处理方式是相同的。
* 你可以在其他标签（例如 `text` 或 `if`）中使用 `${variable_name}` 语法来引用这些变数。

**旗标管理**：
* 对于游戏旗标（例如「是否遇过主角」），常见作法是使用 `"true"` 和 `"false"`，或 `"1"` 和 `"0"`。
* 一致性很重要！如果你一开始使用 `"1"`，就请一直维持下去，这样 `[if]` 判断才不会混淆。

**变数命名**：
* 请避免在变数名称中使用空格或特殊符号。`my_variable` 会比 `my variable!` 安全得多。

---

## `click`

等待点选

`click` 标签会暂停指令码执行，等待玩家点选滑鼠或按下按键。
它通常用来在画面变化之间，或在重大转场前制造停顿。

### Basic Usage

```
# 变更背景，然后等待玩家点选
[bg file="sunset.png" time="1.0"]
[click]

# 点选之后显示角色
[ch center="chara01.png" time="1.0"]
```

### Arguments

这个标签不接受任何引数。

| Argument | Omissible | Description | Notes           |
|----------|-----------|-------------|-----------------|
| -        | -         | -           | -               |

### Tips

**时机与节奏**：
* 当你想在对话继续前，让玩家先看一下新的背景或特定角色表情时，可以使用 `[click]`。
* 和 `[text]` 标签不同，`[text]` 在显示讯息后会自动等待点选，而 `[click]` 则用于非对话段落中的手动流程控制。

**视觉回馈**：
* 当指令码执行到 `[click]` 标签时，游戏会停在目前画面。请确保前面的动画（例如 `[bg]` 或 `[ch]`）都有设定 `time`，不然画面可能会显得太突兀地静止。

**若要定时等待：**
* 请使用 `[wait]` 来做定时等待。

---

## `goto`

跳转到标签

`goto` 标签会立刻把 NovelML 的执行移到指定标签。
它是控制故事流程的实用工具，可以让你跳过某些段落，或回圈回前面的部分。

请注意，小型分支应该用 `[if]` 来实作。

### Basic Usage

```
# 跳到早晨场景的开头
[goto name="morning_start"]

# ... 这段指令码会被跳过 ...

[label name="morning_start"]
[text text="The sun rises over the horizon."]
```

### Arguments

| Argument | Omissible | Description                       | Notes                                         |
|----------|-----------|-----------------------------------|-----------------------------------------------|
| `name`   | No        | 要跳转到的目标标签名称。          | 必须与 `[label]` 标签定义的名称一致。         |

### Tips

**无条件跳转**：
* 和 `[if]` 不同，`[goto]` 会在引擎碰到标签时立刻跳到目标标签。

**流程管理**：
* 可以在分支路径的结尾使用 `[goto]`，把故事带回「共通」路线。
* 搭配其他逻辑时，它也很适合拿来建立回圈（例如「返回标题」流程）。

**跨档案？**：
* 请记得 `[goto]` 通常只会在目前指令码档案内运作。
* 如果你想完全跳到另一个档案，就要看 `[load]` 标签了！

---

## `defmacro`

定义巨集

`defmacro` 标签会开始定义一个巨集。
巨集可以把多个标签与命令组成一个有名称的区块，并且能在脚本中透过 `[callmacro]` 重复使用。

### Basic Usage

```
# 为特定角色登场定义巨集
[defmacro name="enter_kaito"]
    [ch center="kaito_smile.png" time="0.5"]
    [text name="Kaito" text="Hey! Did I keep you waiting?"]
[endmacro]

# 之后在脚本中只要一行就能呼叫
[callmacro name="enter_kaito"]
```

### Arguments

| Argument | Omissible | Description                     | Notes                                             |
|----------|-----------|---------------------------------|---------------------------------------------------|
| `name`   | No        | 这个巨集的唯一名称。            | 用来在之后呼叫时识别这个巨集。                    |

### Tips

**结束定义**：
* 每个 `[defmacro]` 都必须搭配 `[endmacro]` 标签，标示定义结束。

**程式重用**：
* 巨集很适合重复出现的序列，例如特定 UI 转场、角色专属视觉配置，或复杂的音效与画面组合。

**组织方式**：
* 常见做法是在主脚本档的最前面，或在启动时载入的独立档案中，集中定义所有巨集。

**巢状与逻辑**：
* 你几乎可以在巨集中放入任何其他标签，包含 `[if]` 判断，甚至可以用 `[returnmacro]` 依条件提前离开巨集。

---

## `gui`

显示 GUI

`gui` 标签会从指定档案载入并显示图形化使用者介面（GUI）定义。
它可用来显示选单、标题画面或自订互动面板。

### Basic Usage

```
# 显示主选单 GUI
[gui file="main_menu.txt"]

# 显示自订存档/读档画面
[gui file="save_screen.txt"]
```

### Arguments

| Argument | Omissible | Description                                 | Notes                                               |
|----------|-----------|---------------------------------------------|-----------------------------------------------------|
| `file`   | No        | 要载入的 GUI 定义档案名称。                 | 档案必须存在于专案的 GUI 目录中。                  |

### Tips

**GUI 定义**：
* `file` 引数指向一个文字档，这个档案会定义介面的版面、按钮与动作。
* 这些档案会指定图片放置的位置，以及使用者互动时会发生什么事（例如跳到标签或结束游戏）。

**流程中的用途**：
* 一般来说，`[gui]` 标签会用在像标题画面这类图形化选单。

**自订化**：
* 因为 GUI 是定义在外部档案中，所以你可以为游戏做出多种外观，只要用这个标签呼叫不同档案即可切换。

---

## `label`

定义标签

`label` 标签会定义脚本中的特定位置，可供 `[goto]` 或 `[load]` 之类的跳转命令锁定。
它就像故事导览用的书签。

### Basic Usage

```
# 为新章节的开头定义标签
[label name="chapter_01_start"]

# 使用跳转命令前往这个标签
[goto name="chapter_01_start"]
```

### Arguments

| Argument | Omissible | Description                     | Notes                                                  |
|----------|-----------|---------------------------------|--------------------------------------------------------|
| `name`   | No        | 这个标签的唯一名称。            | 大小写有区别。请避免使用空格或特殊符号。              |

### Tips

**导览控制**：
* 标签很适合用来建立分支路径。
* 例如，你可以在故事某个段落的开头放一个 `label`，方便长距离跳转。

**名称唯一**：
* 同一个脚本档内的每个标签名称都必须唯一。
* 如果有两个同名标签，引擎可能会不知道该跳去哪里，这对谁都不方便。

**组织方式**：
* 使用像 `label_evening_park` 这种有描述性的名称，会比 `label1` 更好。
* 这会让你（也让我！）之后更容易阅读脚本并理解发生了什么。

---

## `text`

显示文字

`text` 标签会在讯息框中显示讯息。
它可以显示主要对话或旁白，也可以选择在名称框中显示角色名称。

### Basic Usage

```
# 旁白样式（不显示名称）
[text text="The wind was howling through the trees."]

# 角色对话
[text name="Keith" text="I've been waiting for you here in the small room."]
```

### Arguments

| Argument         | Omissible | Description                                      | Notes                                            |
|------------------|-----------|--------------------------------------------------|--------------------------------------------------|
| `text`           | No        | 要显示的讯息内容。                               |                                                  |
| `text-<locale>`  | Yes       | 要显示的讯息内容（依语系）。                    |                                                  |
| `voice`          | Yes       | 声音档案。                                       |                                                  |
| `voice-<locale>` | Yes       | 声音档案（依语系）。                             |                                                  |
| `name`           | Yes       | 要显示在名称框中的角色名称。                    | 若省略，名称框通常会隐藏。                       |
| `action`         | Yes       | 用于 NVL 模式与手动显示/隐藏。                  |                                                  |
| `space`          | Yes       | 用于 NVL 模式。                                  |                                                  |

### Localization

例如，如果使用者的作业系统语系是日文，系统会优先采用 `text-ja`，而不是 `text`。

| Suffix      | Language                                 |
|-------------|------------------------------------------|
| -en         | 英文（预设）                             |
| -en-us      | 英文（美国）                             |
| -en-gb      | 英文（英国）                             |
| -en-au      | 英文（澳洲）                             |
| -en-nz      | 英文（纽西兰）                           |
| -fr         | 法文（预设）                             |
| -fr-fr      | 法文（法国）                             |
| -fr-ca      | 法文（加拿大）                           |
| -es         | 西班牙文（西班牙，预设）                 |
| -es-es      | 西班牙文（西班牙，预设）                 |
| -es-la      | 西班牙文（拉丁美洲）                     |
| -de         | 德文                                     |
| -it         | 义大利文                                 |
| -ru         | 俄文                                     |
| -el         | 希腊文                                   |
| -zh-cn      | 中文（简体）                             |
| -zh-tw      | 中文（繁体，台湾）                       |
| -ja         | 日文                                     |
| （无字尾）  | 预设值（由开发者决定）                    |

对于所有英文系统语系，`-en` 会作为预设备援。
如果标签中指定了像 `-en-gb` 这样更精确的变体，而且它与使用者地区最相符，就会优先采用。
西班牙文与法文也使用相同机制。
请注意，繁体中文不会回退到简体中文。

例如，如果使用者语系是 `en-GB`，会套用以下优先顺序：
* 1. text-en-gb
* 2. text-en
* 3. text

以下目前尚未支援，但预计未来会支援。

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

### 动作

你可以在 `text` 标签中使用特殊参数。

```
# 清除讯息框。
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

条件分支

`if` 标签可让 NovelML 根据特定条件分支。
透过比较变数或值，你可以建立独特的故事路径，或对玩家先前的选择做出反应。

### Basic Usage

```
# 检查变数是否等于某个值
[if lhs="${points}" op="==" rhs="100"]
    [text text="满分！你太棒了。"]
[elseif lhs="${points}" op=">=" rhs="80"]
    [text text="做得好！你过关了。"]
[else]
    [text text="下次再接再厉。"]
[endif]
```

### Arguments

| Argument | Omissible | Description                            | Notes                                          |
|----------|-----------|----------------------------------------|------------------------------------------------|
| `lhs`    | No        | 条件左侧。                             | 通常是像 `${var_name}` 这样的变数。           |
| `op`     | No        | 用于比较的运算子。                     | 请参考下方的「运算子」表。                     |
| `rhs`    | No        | 条件右侧。                             | 要拿来比较的值或变数。                         |

### Comparison Operators (`op`)

你可以使用这些运算子来定义两侧的比较方式：

| Operator   | Description                |
|------------|----------------------------|
| `===`      | 相等（字串）               |
| `==`       | 相等（数值）               |
| `>`        | 大于（数值）               |
| `>=`       | 大于等于（数值）           |
| `<`        | 小于（数值）               |
| `<=`       | 小于等于（数值）           |

### Tips

**结束区块**：
* 每个 `[if]` 区块都必须以 `[endif]` 标签结束。
* 如果忘了加，引擎可能会搞不清楚条件结束在哪里！

**变数语法**：
* 当你把变数用在 `lhs` 时，请一律用 `${}` 包起来。
* 例如请使用 `lhs="${flag_01}"`，不要只写 `lhs="flag_01"`。

**字串与数字处理**：
* Suika3 会把变数值当成字串，但这些运算子仍可让你进行类似数值比较。
* 只要保持值的一致性即可（例如用 `"1"` 表示真、`"0"` 表示假）。

**多重分支**：
* 你可以在 `[if]` 与 `[endif]` 之间使用任意多个 `[elseif]` 标签，来检查多个特定条件。

---

## `elseif`

附加条件分支

`elseif` 标签会在 `[if]` 区块中指定额外条件。
只有在前面的 `[if]` 与任何先前的 `[elseif]` 条件都为假时才会进行评估。

### Basic Usage

```
[if lhs="${rank}" op="==" rhs="A"]
    [text text="太棒了！你是高手。"]
[elseif lhs="${rank}" op="==" rhs="B"]
    [text text="做得好！继续加油。"]
[elseif lhs="${rank}" op="==" rhs="C"]
    [text text="不错，但你还可以更好。"]
[else]
    [text text="别放弃！再试一次。"]
[endif]
```

### Arguments

与 `[if]` 相同。另请参阅 [if](#if)。

### Tips

**顺序评估**：
* 引擎会由上到下检查条件。
* 只要有一个 `[if]` 或 `[elseif]` 条件成立，就会执行其区块，并略过其余分支（包含其他 `[elseif]` 与 `[else]`）。

**位置**：
* `[elseif]` 必须放在 `[if]` 标签与 `[else]` 或 `[endif]` 标签之间。
* 你可以根据需要使用任意多个 `[elseif]` 标签来涵盖所有情况！

**维护性**：
* 如果有很多条件都在检查同一个变数，使用多个 `[elseif]` 标签会比把多个 `[if]` 区块互相巢状更干净，也更有效率。

---

## `else`

预设条件分支

`else` 标签会定义一个当前面 `[if]` 或 `[elseif]` 条件都不成立时要执行的程式区块。
它会成为分支逻辑中的「预设」路径。

### Basic Usage

```
[if lhs="${weather}" op="==" rhs="sunny"]
    [text text="It's a beautiful day!"]
[elseif lhs="${weather}" op="==" rhs="rainy"]
    [text text="I should bring an umbrella."]
[else]
    # 如果不是晴天也不是雨天就会执行（例如多云或下雪）
    [text text="今天的天空看起来很有意思。"]
[endif]
```

### Arguments

这个标签不接受任何引数。

| Argument | Omissible | Description | Notes |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### Tips

**最后保底**：
* 使用 `[else]` 来处理你在 `[if]` 或 `[elseif]` 中没有明确涵盖的情况。
* 这可以确保游戏永远有一条可走的有效路径。

**位置**：
* `[else]` 必须放在所有 `[elseif]` 标签之后（如果有的话），并且紧接在 `[endif]` 标签之前。
* 每个 `[if]` 区块只能有一个 `[else]`。

**可选性**：
* 你不一定要包含 `[else]` 区块。
* 如果没有任何条件成立，而且也没有 `[else]`，引擎就会直接略过全部内容，继续执行 `[endif]` 之后的部分。

---

## `endif`

结束条件分支

`endif` 标签会标示由 `[if]` 标签开始的条件区块结束。
它会告诉引擎在分支逻辑完成后，回到正常的脚本执行流程。

### Basic Usage

```
[if lhs="${love_points}" op=">=" rhs="50"]
    [text text="She gives you a warm smile."]
[else]
    [text text="She greets you politely."]
[endif]

# 不论上面的结果如何，脚本都会继续执行到这里
[text text="这一天仍在继续..."]
```

### Arguments

这个标签不接受任何引数。

| Argument | Omissible | Description | Notes |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### 提示

**必要结尾**：
* 每一个 `[if]` 标签都必须有对应的 `[endif]`。
* 可以把它们想成一对括号，用来维持故事逻辑的整齐。

**位置**：
* 请一律把 `[endif]` 放在条件序列的最后，也就是所有 `[elseif]` 或 `[else]` 区块之后。

**巢状**：
* 如果你把一个 `[if]` 放在另一个 `[if]` 里面，请确保每一个都有自己的 `[endif]`。
* 正确的巢状结构，是让复杂剧情旗标不出错的关键！

---

## `load`

载入脚本档案

`load` 标签会把目前脚本切换到另一个 NovelML 档案。
它主要用来把大型故事整理成多个章节，或在标题画面与主剧情等不同游戏部分之间切换。

### Basic Usage

```
# 载入并从 scene02.novel 的开头开始
[load file="scene02.novel"]

# 载入 scene02.novel，并直接跳到特定标签
[load file="scene02.novel" label="chapter2_start"]
```

### Arguments

| Argument | Omissible | Description                                      | Notes                                                   |
|----------|-----------|--------------------------------------------------|---------------------------------------------------------|
| `file`   | No        | 要载入的 NovelML 脚本档案名称。                 | 必须是专案脚本目录中的有效档案。                        |
| `label`  | Yes       | 新档案中要跳转到的目标标签。                     | 若省略，脚本会从第一行开始执行。                        |

### Tips

**专案组织**：
* 不要把整个游戏都写在同一个超大档案里，请使用 `[load]` 将内容拆成好管理的区块，例如 `chapter1.novel`、`chapter2.novel` 等。

**立即切换**：
* 当引擎碰到 `[load]` 标签时，会立刻停止执行目前的 NovelML 档案，并切换到新档案。
* 原档案中 `[load]` 之后的任何命令都不会被执行。

**全域旗标**：
* 不用担心你的变数——即使载入新的脚本档案，先前用 `[set]` 标签设定的值仍会保留！

---

## `se`

播放音效

`se` 标签会播放音效（SE）。
音效通常用于敲门、脚步声或 UI 反馈这类短促声音，能替场景增加沉浸感与真实感。

### Basic Usage

```
# 播放一次音效
[se file="door_open.ogg"]

# 停止目前所有正在播放的音效
[se file="none"]
```

### Arguments

| Argument | Omissible     | Description                               | Notes                                        |
|----------|---------------|-------------------------------------------|----------------------------------------------|
| `file`   | No            | 要播放的音效档案名称。                    | 设为 `none` 可停止音效播放。                 |
| `loop`   | Yes (`false`) | 是否循环播放音效。                        |                                              |

### Tips

**必要格式**：
* 和 BGM 一样，Suika3 要求 SE 档案必须是 **Ogg Vorbis** 格式。
* 取样率必须是 **44,100Hz**，才能确保高音质与相容性。

**音效叠加**：
* 音效通常可以在 BGM 播放时一并播放。
* 它们会占用自己的音轨，因此不会中断音乐。

**音量控制**：
* 若要调整音效音量而不影响 BGM，请在 `[volume]` 标签中使用 `track="se"`。

**环境音用途**：
* 虽然 SE 常用于短音效，但你也可以拿来做循环的环境音（例如风声或雨声）。
* 载入存档时，循环中的 SE 会被还原。

---

## `volume`

设定音讯音量

`volume` 标签会设定特定音轨的音量。
它很适合用来确保背景音乐不会盖过重要的音效或角色语音。

### Basic Usage

```
# 将 BGM 音量设为 50%
[volume track="bgm" volume="0.5"]

# 将 SE 音量设为最大
[volume track="se" volume="1.0"]

# 将语音静音
[volume track="voice" volume="0.0"]
```

### Arguments

| Argument | Omissible | Description                           | Notes                                     |
|----------|-----------|---------------------------------------|-------------------------------------------|
| `track`  | No        | 要调整的音轨。                        | 请参考下方的「音轨」表。                  |
| `volume` | No        | 音量大小，范围从 `0.0` 到 `1.0`。     | `0.0` 表示静音，`1.0` 表示最大音量。      |
| `time`   | Yes (`0`) | 淡出/淡入时间（秒）。                 | `0` 表示立即变更。                        |
### Track Types (`track`)

Suika3 将音讯分为三种主要音轨：

| Track Name | Description                      |
|------------|----------------------------------|
| `bgm`      | 背景音乐。                      |
| `se`       | 音效与系统音效。                |
| `voice`    | 角色语音档案。                  |

### Tips

**立即变更**：
* 当 `time` 大于 `0` 时，音量变更会逐渐发生。
* `time="0"` 则表示立即变更。

**预设音量**：
* 建议在游戏一开始就设定好你偏好的音量，例如在 `start` 标签中，让玩家从头就有一致的体验。

---

## `skip`

设定快转状态

`skip` 标签可以启用或停用游戏中的快转功能。
它很适合用来避免玩家跳过重要的过场，或确保某些场景能以预定节奏体验。

### Basic Usage

```
# 启用快转功能
[skip enable="true"]

# 在关键场景中停用快转功能
[skip enable="false"]
```

### Arguments

| Argument | Omissible | Description                                  | Notes                                                 |
|----------|-----------|----------------------------------------------|-------------------------------------------------------|
| `enable` | No        | 快转功能是否启用。                      | 设为 `true` 可允许快转，`false` 则禁止。 |

### Tips

**过场控制**：
* 快转功能通常会在启动画面与标志出现前停用。

**还原设定**：
* 关键场景结束后，别忘了设定 `[skip enable="true"]`。
* 玩家通常会喜欢能够跳过已看过文字的自由。

**系统行为**：
* 这个标签会控制引擎整体的「快转」状态。
* 即使玩家按下快转快捷键，只要 `enable` 设为 `false`，引擎也会忽略它。

---

## `config`

设定组态值

`config` 标签可以直接在标记中修改游戏系统的设定。
它对于动态调整游戏 UI 非常重要，例如移动讯息框或即时修改系统层级参数。

### Basic Usage

```
# 变更讯息框位置
[config name="msgbox.x" value="100"]
[config name="msgbox.y" value="200"]

# 更新特定系统设定
[config name="msgbox.font.size" value="24"]
```

### Arguments

| Argument | Omissible | Description                                        | Notes                                                    |
|----------|-----------|----------------------------------------------------|----------------------------------------------------------|
| `name`   | No        | 要变更的设定参数名称。                   | 请参考系统的设定清单以确认合法名称。       |
| `value`  | No        | 要指派给参数的新值。                     | 值会以字串处理，但也可能代表数字。         |

### Tips

**UI 自订**：
* 你可以在特定场景中使用 `[config]` 重新定位讯息框，营造更具电影感的效果。

**动态调整**：
* 因为这个标签可以在脚本中的任何地方呼叫，所以你可以随著故事推进改变游戏的「外观与手感」。
* 例如为「回忆」段落调整 UI。

**参数名称**：
* `name` 引数要特别小心！
* 它必须与你的 Suika3 专案设定中定义的内部设定键完全一致。
* 另请参阅 [完整设定列表](config.md)

---

## `layer`

直接操作图层

`layer` 标签可以直接控制特定的图片与文字图层。
虽然像 `[bg]` 和 `[ch]` 这类标签更适合一般场景，但 `[layer]` 能让你精准地独立修改任一图层的位置、缩放与旋转。

### Basic Usage

```
# 直接将图片载入中央角色图层（chc）
[layer name="chc" file="heroine_smile.png"]

# 只调整背景（bg）的位置与透明度
[layer name="bg" x="100" y="100" alpha="128"]

# 旋转脸部图层
[layer name="chf" rotate="45.0" center-x="100" center-y="100"]
```

### Arguments

| Argument  | Omissible   | Description                          | Notes                              |
|-----------|-------------|--------------------------------------|------------------------------------|
| `name`    | No          | 目标图层名称。                       | 另请参阅下方的「图层名称」表。     |
| `file`    | Yes         | 要载入到图层上的档案名称。           | 使用 `none` 可清除图层。           |
| `x`       | Yes (`0`)   | 图层的 X 座标。                      |                                    |
| `y`       | Yes (`0`)   | 图层的 Y 座标。                      |                                    |
| `alpha`   | Yes (`255`) | 图层的不透明度。                     | `0` 到 `255`。                     |
| `scale-x` | Yes (`1.0`) | X 轴缩放倍率。                       | `1.0` 为原始大小。                |
| `scale-y` | Yes (`1.0`) | Y 轴缩放倍率。                       | `1.0` 为原始大小。                |
| `center-x`| Yes (`0`)   | 旋转中心（X）。                     | 旋转的枢轴点。                    |
| `center-y`| Yes (`0`)   | 旋转中心（Y）。                     | 旋转的枢轴点。                    |
| `rotate`  | Yes (`0.0`) | 旋转角度（度）。                     | 正值代表顺时针。                  |

### Common Layer Names (`name`)

Suika3 内建了相当丰富的预定义图层。

以下是完整图层清单：

|图层名称         |说明                                      |
|-----------------|-----------------------------------------|
|bg               |背景图像。                                |
|bg2              |背景图像 2。                              |
|efb1             |后景效果 1。                              |
|efb2             |后景效果 2。                              |
|efb3             |后景效果 3。                              |
|efb4             |后景效果 4。                              |
|chb              |后中角色。                                |
|chb-eye          |后中角色眼睛。                            |
|chb-lip          |后中角色嘴唇。                            |
|chb-fo           |淡出中的后中角色。                        |
|chl              |左侧角色。                                |
|chl-eye          |左侧角色眼睛。                            |
|chl-lip          |左侧角色嘴唇。                            |
|chl-fo           |淡出中的左侧角色。                        |
|chlc             |左中角色。                                |
|chlc-eye         |左中角色眼睛。                            |
|chlc-lip         |左中角色嘴唇。                            |
|chlc-fo          |淡出中的左中角色。                        |
|chr              |右侧角色。                                |
|chr-eye          |右侧角色眼睛。                            |
|chr-lip          |右侧角色嘴唇。                            |
|chr-fo           |淡出中的右侧角色。                        |
|chrc             |右中角色。                                |
|chrc-eye         |右中角色眼睛。                            |
|chrc-lip         |右中角色嘴唇。                            |
|chrc-fo          |淡出中的右中角色。                        |
|chc              |中央角色。                                |
|chc-eye          |中央角色眼睛。                            |
|chc-lip          |中央角色嘴唇。                            |
|chc-fo           |淡出中的中央角色。                        |
|msgbox           |讯息框（`[layer]` 无法看见）。            |
|namebox          |名称框（`[layer]` 无法看见）。            |
|click            |点击动画（`[layer]` 无法看见）。          |
|eff1             |前景效果 1。                              |
|eff2             |前景效果 2。                              |
|eff3             |前景效果 3。                              |
|eff4             |前景效果 4。                              |
|chf              |脸部角色。                                |
|chf-eye          |脸部角色眼睛。                            |
|chf-lip          |脸部角色嘴唇。                            |
|chf-fo           |淡出中的脸部角色。                        |
|text1            |文字图层 1。                              |
|text2            |文字图层 2。                              |
|text3            |文字图层 3。                              |
|text4            |文字图层 4。                              |
|text5            |文字图层 5。                              |
|text6            |文字图层 6。                              |
|text7            |文字图层 7。                              |
|text8            |文字图层 8。                              |
|gui1             |GUI 按钮 1（`[layer]` 无法看见）。        |
|gui2             |GUI 按钮 2（`[layer]` 无法看见）。        |
|gui3             |GUI 按钮 3（`[layer]` 无法看见）。        |
|gui4             |GUI 按钮 4（`[layer]` 无法看见）。        |
|gui5             |GUI 按钮 5（`[layer]` 无法看见）。        |
|gui6             |GUI 按钮 6（`[layer]` 无法看见）。        |
|gui7             |GUI 按钮 7（`[layer]` 无法看见）。        |
|gui8             |GUI 按钮 8（`[layer]` 无法看见）。        |
|gui9             |GUI 按钮 9（`[layer]` 无法看见）。        |
|gui10            |GUI 按钮 10（`[layer]` 无法看见）。       |
|gui11            |GUI 按钮 11（`[layer]` 无法看见）。       |
|gui12            |GUI 按钮 12（`[layer]` 无法看见）。       |
|gui13            |GUI 按钮 13（`[layer]` 无法看见）。       |
|gui14            |GUI 按钮 14（`[layer]` 无法看见）。       |
|gui15            |GUI 按钮 15（`[layer]` 无法看见）。       |
|gui16            |GUI 按钮 16（`[layer]` 无法看见）。       |
|gui17            |GUI 按钮 17（`[layer]` 无法看见）。       |
|gui18            |GUI 按钮 18（`[layer]` 无法看见）。       |
|gui19            |GUI 按钮 19（`[layer]` 无法看见）。       |
|gui20            |GUI 按钮 20（`[layer]` 无法看见）。       |
|gui21            |GUI 按钮 21（`[layer]` 无法看见）。       |
|gui22            |GUI 按钮 22（`[layer]` 无法看见）。       |
|gui23            |GUI 按钮 23（`[layer]` 无法看见）。       |
|gui24            |GUI 按钮 24（`[layer]` 无法看见）。       |
|gui25            |GUI 按钮 25（`[layer]` 无法看见）。       |
|gui26            |GUI 按钮 26（`[layer]` 无法看见）。       |
|gui27            |GUI 按钮 27（`[layer]` 无法看见）。       |
|gui28            |GUI 按钮 28（`[layer]` 无法看见）。       |
|gui29            |GUI 按钮 29（`[layer]` 无法看见）。       |
|gui30            |GUI 按钮 30（`[layer]` 无法看见）。       |
|gui31            |GUI 按钮 31（`[layer]` 无法看见）。       |
|gui32            |GUI 按钮 32（`[layer]` 无法看见）。       |

### Tips

**精准控制**：
* 当你在处理没有专用标签的自订效果图层（例如 `eff1` 等）时，可以用 `[layer]` 手动将图片载入图层。

**即时更新**：
* 和 `[ch]` 或 `[bg]` 不同，`layer` 标签通常会立即更新画面。
* 如果你想让这些变化随时间动画化，应该改用 `[move]` 标签。

**图层阶层**：
* 请记住图层是叠放的。
* 例如 `chf`（脸部角色）永远会渲染在 `chc`（中央角色）前方。
* 理解这个「Z 顺序」是建立复杂视觉构图的关键。

---

## `move`

图层动画

`move` 标签会在指定时间内让特定图层产生动画。
它很适合做滑动效果、角色拉近，或旋转萤幕元素，为场景加入动态感。

### Basic Usage

```
# 在 2.0 秒内将中央角色移到新位置
[move time="2.0" center-x="150" center-y="100"]

# 相对移动：把背景往右推 50 像素
[move time="1.0" bg-x="r50"]

# 在旋转图层的同时逐渐淡出
[move time="3.0" face-alpha="0" face-rotate="r360"]
```

### Arguments

**共通：**
| Argument         | Omissible     | Description                               | Notes                                      |
|------------------|---------------|-------------------------------------------|--------------------------------------------|
| `name`           | No            | 要动画化的目标图层。                      | 另请参阅下方的「可移动图层」表。          |
| `time`           | No            | 动画持续时间（秒）。                      | 支援小数值（例如 `0.5`）。               |
| `async`          | Yes (`false`) | 若为 `true`，则非阻塞执行动画。           |                                            |
| `accel`          | Yes (`normal`)| 加速度类型。                              | 其一                                       |
| (layer)-(suffix) | Yes           |                                           |                                            |

**(layer):**
| Argument       | Description                               |
|----------------|-------------------------------------------|
| `bg`           | 背景图层。                                |
| `bg2`          | 背景 2。                                  |
| `back`         | 后中角色。                                |
| `left`         | 左侧角色。                                |
| `right`        | 右侧角色。                                |
| `center`       | 中央角色。                                |
| `left-center`  | 左中角色。                                |
| `right-center` | 右中角色。                                |
| `face`         | 脸部角色。                                |

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

**非阻塞动画（`async="true"`）**：
* 脚本会在开始 `[move]` 后立刻继续执行下一个命令。
* 如果你想让脚本等到动画结束，可以在后面接一个 `[wait]` 标签，并使用相同的 `time` 值。

**相对变换**：
* 使用 `r` 前缀（例如 `x="r100"`）对重复动作非常有用，例如让角色「跳动」或「晃动」，而不必计算绝对座标。

**视觉修饰**：
* 把 `scale-x` 和 `scale-y` 与 `move` 结合，可以在角色脸部上做出「放大」效果，营造戏剧性的特写。

---

## `pencil`

铅笔

在图层上绘制文字。

### Basic Usage

```
[pencil layer="bg" font-size="30" text="Hello, World!"]
```

### Arguments

| Argument      | Omissible        | Description              |
|---------------|------------------|--------------------------|
| text          | No               | 要绘制的文字。           |
| layer         | Yes (`text1`)    | 图层名称。               |
| font-type     | Yes (`0`)        | 字型选择。（0-3）        |
| font-size     | Yes (`16`)       | 字型大小。               |
| color         | Yes (`#000000`)  | 字型颜色。               |
| outline-width | Yes (`0`)        | 字型外框宽度。           |
| outline-color | Yes (`#ffffff`)  | 字型外框颜色。           |
| line-margin   | Yes              | 行距。                   |
| char-margin   | Yes (`0`)        | 字元间距。               |
| x             | Yes (`0`)        | 绘制区域 X 座标。        |
| y             | Yes (`0`)        | 绘制区域 Y 座标。        |
| width         | Yes              | 绘制区域宽度。           |
| height        | Yes              | 绘制区域高度。           |

## Supported Layer Name

|Layer Name       |Description                              |
|-----------------|-----------------------------------------|
|bg               |背景图像。                                |
|bg2              |背景图像 2。                              |
|efb1             |后景效果 1。                              |
|efb2             |后景效果 2。                              |
|efb3             |后景效果 3。                              |
|efb4             |后景效果 4。                              |
|chb              |后中角色。                                |
|chl              |左侧角色。                                |
|chlc             |左中角色。                                |
|chr              |右侧角色。                                |
|chrc             |右中角色。                                |
|chc              |中央角色。                                |
|eff1             |前景效果 1。                              |
|eff2             |前景效果 2。                              |
|eff3             |前景效果 3。                              |
|eff4             |前景效果 4。                              |
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

从巨集返回

`returnmacro` 标签会立刻结束目前的巨集，并将脚本执行返回到原始 `[callmacro]` 标签后的那一行。
它特别适合在 `[if]` 区块中，依特定条件提前停止巨集。

### Basic Usage

```
[defmacro name="check_item"]
    [if lhs="${has_key}" op="==" rhs="false"]
        [text text="The door is locked."]
        [returnmacro]
    [endif]

    # 只有在 has_key 为真时才会执行这一段
    [text text="你用钥匙打开了门！"]
[endmacro]
```

### Arguments

This tag does not take any arguments.

| Argument | Omissible | Description | Notes |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### 提示

**提前退出**：
* 在 `[if]` 区块中使用 `[returnmacro]`，可在满足条件时跳过巨集的其余命令。
* 这会让你的巨集更有弹性，也更强大。

**隐式返回**：
* 你其实不必在每个巨集的最后都放 `[returnmacro]`。
* 一旦引擎碰到 `[endmacro]` 标签，就会自动返回主脚本。

**流程控制**：
* 请记得，这个标签只会结束「目前」的巨集。它不会停止整个游戏，也不会跳到其他标签，只会把你送回巨集被呼叫的地方。

---

## `video`

播放影片

`video` 标签会在画面上播放影片档。
它非常适合用于开场动画、转场过场，或最适合以动态影片呈现的高冲击视觉效果。

### Basic Usage

```
# 播放开场影片（不可跳过）
[video file="opening.mp4"]

# 播放一段可由玩家点击跳过的短过场。
[video file="cutscene01.mp4" skippable="true"]
```

### Arguments

| Argument    | Omissible     | Description                                           | Notes                                                         |
|-------------|---------------|-------------------------------------------------------|---------------------------------------------------------------|
| `file`      | No            | 要播放的影片档案名称。                                | 档案必须是支援的格式（例如 .mp4）。                        |
| `skippable` | Yes (`false`) | 影片是否可由玩家点击跳过。                            | 设为 `false` 可强制玩家看完整段影片。                    |

### Tips

**档案支援**：
* 请确认你的影片档是 .mp4（H.264 + AAC）格式。
* 如果你要支援 32 位元 Windows，请另外准备 .wmv 档，再与 .mp4 档一同提供；此时可省略副档名，例如 `[video file="opening"]`。

**转场**：
* 影片播放完毕（或被跳过）后，引擎会自动继续执行脚本中的下一个命令。
* 通常会建议在 `[video]` 标签后接一个 `[bg]` 标签，确保影片结束后画面正好符合你的预期。

**影片中的音讯**：
* 大多数影片档都会包含自己的音轨。
* 请记得，这个音讯会和正在播放的 `[bgm]` 一起播放。
* 如果影片本身有声音，你可能会想在开始播放前先用 `[bgm file="none"]` 停掉音乐。

---

## `wait`

等待时间

`wait` 标签会暂停 NovelML 的执行一段指定时间。
它对于控制视觉转场节奏、制造戏剧性停顿，或在不需要玩家输入的情况下精准控制时机都很重要。

### Basic Usage

```
# 在下一个命令前暂停 1.5 秒
[wait time="1.5"]

# 在角色变更之间制造短暂停顿
[ch center="chara01_surprised.png" time="0.5"]
[wait time="1.0"]
[text text="She couldn't believe her eyes."]
```

### Arguments

| Argument      | Omissible     | Description                    | Notes                                  |
|---------------|---------------|--------------------------------|----------------------------------------|
| `time`        | No            | 要等待的秒数。                 | 支援小数值（例如 `0.5`）。             |
| `hidemsgbox`  | Yes (`false`) | 强制隐藏讯息框。               |                                        |
| `hidenamebox` | Yes (`false`) | 强制隐藏名称框。               |                                        |

### Tips

**非互动停顿**：
* 和等待玩家操作的 `[click]` 不同，`[wait]` 会在时间到后自动继续。
* 这很适合用在「自动播放」段落或有时间限制的视觉序列。

**与动画搭配**：
* 如果你在 `[ch]` 或 `[bg]` 标签中使用 `time` 引数，引擎会在动画播放时立即前往下一个命令。
* 若你想让脚本停在动画结束前，或停得更久以营造戏剧效果，可以在动画后加上 `[wait]`。

**使用者体验**：
* 请小心不要在没有视觉理由的情况下，把 `[wait]` 设太久（例如超过 3 秒），不然玩家可能会以为游戏卡住了！
