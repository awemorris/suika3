Suika3 標籤參考
====================

## 索引

| Tag Name                    | Description                                                        |
|-----------------------------|--------------------------------------------------------------------|
| [anime](#anime)             | 載入並執行動畫檔。                                                 |
| [bg](#bg)                   | 以淡出效果變更背景圖片。                                           |
| [bgm](#bgm)                 | 播放背景音樂檔（Ogg Vorbis 格式）。                                |
| [callmacro](#callmacro)     | 呼叫已定義的巨集。                                                 |
| [ch](#ch)                   | 以詳細圖層引數顯示或隱藏角色。                                     |
| [chapter](#chapter)         | 設定章節名稱。                                                     |
| [choose](#choose)           | 顯示選項並儲存選擇，或跳到標籤。                                   |
| [click](#click)             | 等待使用者點選。                                                   |
| [config](#config)           | 設定遊戲系統的設定值。                                             |
| [defmacro](#defmacro)       | 開始定義巨集。                                                     |
| [else](#else)               | if/elseif 分支中，當沒有條件成立時使用。                           |
| [elseif](#elseif)           | 在分支中指定額外條件。                                             |
| [endif](#endif)             | 結束條件分支。                                                     |
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

執行動畫

`anime` 標籤會從檔案載入並執行動畫定義。
它可用來做出複雜的視覺效果、角色移動，或超越簡單轉場的迴圈環境動畫。

### Basic Usage

```
# 執行同步動畫（等待完成）
[anime file="opening_effect.txt"]

# 執行非同步迴圈動畫
[anime file="sparkle.txt" async="true" register="my_loop"]

# 停止已註冊的非同步動畫
[anime stop="true" register="my_loop"]
```

### Arguments

| Argument      | Omissible      | Description                                        | Notes                                                             |
|---------------|----------------|----------------------------------------------------|-------------------------------------------------------------------|
| `file`        | Yes            | 動畫定義檔的檔名。                                 | 除非使用 `stop="true"`，否則為必填。                              |
| `async`       | Yes (`false`)  | 是否以非同步方式執行動畫。                         | 若為 `false`，指令碼會等待動畫結束。                                |
| `register`    | Yes            | 用來識別此動畫例項的唯一名稱。                     | 之後控制或停止非同步動畫時會用到。                                |
| `stop`        | Yes (`false`)  | 若設為 `true`，則停止已註冊的動畫。                | 需要搭配 `register` 引數。                                        |
| `showsysbtn`  | Yes (`true`)   | 播放期間是否顯示系統按鈕。                         | 只適用於同步動畫。                                                |
| `showmsgbox`  | Yes (`true`)   | 播放期間是否顯示訊息框。                           | 只適用於同步動畫。                                                |
| `shownamebox` | Yes (`true`)   | 播放期間是否顯示名稱框。                           | 只適用於同步動畫。                                                |

### Tips

**同步與非同步**：
* **同步（`async="false"`）**：適合過場動畫，讓玩家先看完動畫，再顯示文字或選項。
* **非同步（`async="true"`）**：適合背景效果，例如飄雪或閃爍燈光，讓故事繼續時動畫也持續播放。

**管理例項**：
* 透過 `register` 引數，你可以替特定動畫加上名稱。
* 這就是你在使用 `stop="true"` 時，告訴引擎要停止哪個動畫的方法。

**UI 控制**：
* 如果動畫要佔滿全螢幕，而你希望對話視窗暫時消失以保持畫面乾淨，可以使用 `showmsgbox="false"`。

---

## `bg`

變更背景

`bg` 標籤會以平滑淡入淡出效果變更背景圖片。
這是視覺小說裡設定場景的主要方式。

### Basic Usage

```
# 在 1.0 秒內切換到 background.png
[bg file="background.png" time="1.0"]

# 淡出成黑畫面（移除背景）
[bg file="none" time="1.0"]
```

### Arguments

| Argument   | Omissible      | Description                                   | Notes                                                                        |
|------------|----------------|-----------------------------------------------|------------------------------------------------------------------------------|
| `file`     | No             | 新背景圖片的檔名。                              | 設為 `none` 可移除背景。                                                     |
| `time`     | Yes (`0`)      | 淡出效果的持續時間（秒）。                      | 預設為 `0.0`（立即變更）。                                                   |
| `method`   | Yes (`normal`) | 淡出方法/樣式。                                | 可選 `normal`、`rule` 或 `melt`。                                            |
| `rule`     | Yes            | 特定轉場所需的規則圖檔。                        | 當 `method` 設為 `rule` 或 `melt` 時必填。                                   |
| `x`        | Yes (`0`)      | 背景圖片的 X 軸位移。                           | 支援絕對值（例如 `100`）或相對值（例如 `r100`）。                            |
| `y`        | Yes (`0`)      | 背景圖片的 Y 軸位移。                           | 支援絕對值（例如 `100`）或相對值（例如 `r-100`）。                           |
| `alpha`    | Yes (`255`)    | 背景圖片的 alpha 值。                           | `0` 到 `255`。                                                               |
| `clear`    | Yes (`false`)  | 是否讓角色消失。                                | 若為 `true`，所有角色都會消失。                                             |

### Transition Methods (`method`)

透過選擇適合的轉場樣式，可以營造不同氛圍：

| Type     | Description                                                                                                                          |
|----------|--------------------------------------------------------------------------------------------------------------------------------------|
| `normal` | Alpha 混合。預設方法。會在舊圖片與新圖片之間平滑交叉淡化。                              |
| `rule`   | 1-bit Universal Transition。使用灰階 `rule` 圖決定切換順序。                           |
| `melt`   | 8-bit Universal Transition。類似 `rule`，但轉場邊緣帶有柔和模糊，形成「融化」效果。       |

在 `rule` 與 `melt` 中，圖片會依照規則圖由最暗區域一路切換到最亮區域。

### Tips

**相對定位**：
* 如果你想從目前位置微調背景，請使用 `r` 字首。
* 例如，`x="r50"` 會讓圖片從目前 X 座標往右移 50 畫素。

**什麼是 `rule` 圖？**
* 這是一張灰階圖片，黑色區域會先轉場，白色區域會最後轉場。
* 透過自訂 `rule` 圖，你可以做出水平擦除、圓形展開，甚至更有藝術感的圖樣！

---

## `bgm`

播放背景音樂

`bgm` 標籤會播放背景音樂曲目。
音樂是營造場景氛圍的重要工具，並且會自動迴圈播放，直到被停止或更換。

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

`callmacro` 標籤會執行先前定義好的巨集。
它可以讓你在腳本中多次觸發特定的一串命令，例如角色登場或 UI 動畫，而不必重寫原始程式碼。

### Basic Usage

```
# 呼叫名為 "kaito_entrance" 的巨集
[callmacro name="kaito_entrance"]

# 呼叫用於特定場景轉場的巨集
[callmacro name="fade_to_white"]
```

### Arguments

| Argument | Omissible | Description                               | Notes                                              |
|----------|-----------|-------------------------------------------|----------------------------------------------------|
| `name`   | No        | 要執行的巨集名稱。                        | 必須與 `[defmacro]` 標籤定義的名稱一致。          |
| `file`   | Yes       | 巨集所在的檔案名稱。                      | 若要呼叫目前檔案內的巨集，則可省略。              |

### Tips

**效率**：
* 使用 `[callmacro]` 可以讓主劇情腳本保持專注且易讀。
* 原本要看十行動畫程式碼，現在只會看到一條清楚的命令。

**執行流程**：
* 當引擎碰到 `[callmacro]` 時，會立刻跳到已定義的巨集，執行裡面的所有標籤，然後自動回到 `[callmacro]` 標籤後的下一行。

**模組化設計**：
* 可以把巨集想成你遊戲裡的「自訂標籤」。
* 如果你想改變角色入場方式，只需要在 `[defmacro]` 區塊更新一次程式碼，每個 `[callmacro]` 都會跟著反映這個變更！

---

## `ch`

角色顯示

`ch` 標籤可在各個圖層上顯示、隱藏或更新角色圖片。
它可以同時精細控制多個角色與背景的位置、縮放與旋轉。

### Basic Usage

```
# 在中央顯示角色
[ch center="chara001.png" time="1.0"]

# 顯示多個角色並指定位置
[ch left="chara002.png" right="chara003.png" time="0.5"]

# 隱藏指定角色
[ch center="none" time="1.0"]
```

### Arguments

| Argument  | Omissible      | Description                            | Notes                                                 |
|-----------|----------------|----------------------------------------|-------------------------------------------------------|
| `time`    | Yes (`0`)      | 轉場持續時間（秒）。                   | 會影響這個標籤內所有圖層變更。                        |
| `method`  | Yes (`normal`) | 淡入淡出的方式/樣式。                  | `normal`、`rule` 或 `melt`。                         |
| `rule`    | Yes            | 轉場用的規則圖檔。                     | 當 `method` 是 `rule` 或 `melt` 時必填。              |

#### 圖層檔案引數

指定檔名即可將圖片載入到某個圖層。設為 `none` 則會解除安裝（隱藏）圖片。

| Argument       | Description                               |
|----------------|-------------------------------------------|
| `bg`           | 背景圖層。                                |
| `back`         | 後中角色。                                |
| `left`         | 左側角色。                                |
| `right`        | 右側角色。                                |
| `center`       | 中央角色。                                |
| `left-center`  | 左中角色。                                |
| `right-center` | 右中角色。                                |
| `face`         | 臉部角色。                                |

#### 圖層引數引數

上面的每個圖層（例如 `center`）都可以使用下列字尾自訂（例如 `center-x`、`center-rotate`）。

| Suffix      | Omissible     | Description                | Notes                                                         |
|-------------|---------------|----------------------------|---------------------------------------------------------------|
| `-x`        | Yes (`0`)     | X 座標。                   | 支援絕對值（例如 `100`）或相對值（例如 `r50`）。              |
| `-y`        | Yes (`0`)     | Y 座標。                   | 支援絕對值（例如 `100`）或相對值（例如 `r-50`）。             |
| `-a`        | Yes (`255`)   | Alpha 值（不透明度）。     | `0`（透明）到 `255`（不透明）。                               |
| `-scale-x`  | Yes (`1.0`)   | X 縮放倍率。               | `1.0` 為原始大小。支援 `r` 字首。                             |
| `-scale-y`  | Yes (`1.0`)   | Y 縮放倍率。               | `1.0` 為原始大小。支援 `r` 字首。                             |
| `-center-x` | Yes (`0`)     | 旋轉中心 X。               | 旋轉效果的樞軸點。                                            |
| `-center-y` | Yes (`0`)     | 旋轉中心 Y。               | 旋轉效果的樞軸點。                                            |
| `-rotate`   | Yes (`0`)     | 旋轉角度（度）。           | 正值為順時針。支援 `r` 字首。                                  |
| `-dim`      | Yes (`false`) | 暗化狀態。                 | 若為 `true`，圖層會被渲染得暗 50%。                            |

### Tips

**批次更新**：
* 你可以在單一 `[ch]` 標籤中同時更新多個角色與背景，讓它們一起完美地進行動畫。

**相對變換**：
* 和 `bg` 標籤一樣，所有數值引數都支援 `r` 字首。
* 例如 `center-y="r-50"` 會讓中央角色從目前位置往上移動 50 畫素。

---

## `chapter`

設定章節名稱

`chapter` 標籤會設定目前章節的名稱。
這個名稱通常會被遊戲系統用來在存檔/讀檔選單或遊戲畫面上顯示進度，方便玩家追蹤劇情進展。

### Basic Usage

```
# 在故事段落開頭設定章節名稱
[chapter name="Chapter 01: The Beginning"]

# 隨著故事推進更新章節名稱
[chapter name="Intermission: A Quiet Night"]
```

### Arguments

| Argument | Omissible | Description                        | Notes                                                      |
|----------|-----------|------------------------------------|------------------------------------------------------------|
| `name`   | No        | 要設定的章節名稱。                 | 這個字串會儲存在遊戲的系統變數中。                        |

### Tips

**存檔顯示**：
* 在許多 Suika3 設定中，你在這裡設定的字串會顯示在「存檔」與「讀檔」欄位上。
* 請選一個能讓玩家清楚記得目前進度的名稱！

**一致性**：
* 在開始新的主要場景或章節的 `[label]` 後立刻呼叫 `[chapter]` 標籤，是個很好的習慣。

**更新名稱**：
* 你可以隨時多次呼叫 `[chapter]`。
* 每次呼叫時，舊的章節名稱都會被新的名稱覆蓋。

---

## `choose`

顯示選擇選項

`choose` 標籤會顯示最多 8 個可互動按鈕給玩家。
它會把所選專案的文字儲存在變數中。

### Basic Usage

```
# 將選擇結果儲存在變數中
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
| `text1`          | Yes       | 每個按鈕顯示的文字。                           | 通常至少需要一個選項。                             |
| `text2`          | Yes       | 每個按鈕顯示的文字。                           | 通常至少需要一個選項。                             |
| `text3`          | Yes       | 每個按鈕顯示的文字。                           | 通常至少需要一個選項。                             |
| `text4`          | Yes       | 每個按鈕顯示的文字。                           | 通常至少需要一個選項。                             |
| `text5`          | Yes       | 每個按鈕顯示的文字。                           | 通常至少需要一個選項。                             |
| `text6`          | Yes       | 每個按鈕顯示的文字。                           | 通常至少需要一個選項。                             |
| `text7`          | Yes       | 每個按鈕顯示的文字。                           | 通常至少需要一個選項。                             |
| `text8`          | Yes       | 每個按鈕顯示的文字。                           | 通常至少需要一個選項。                             |
| `text<N>-locale` | Yes       | 每個按鈕顯示的文字（依語系）。                 | 通常至少需要一個選項。                             |
| `name`           | No        | 用來儲存結果的變數名稱。                       | 會儲存所選選項的文字。                             |
| `value1`         | Yes       | 指派給結果變數的值。                           | 通常至少需要一個選項。                             |
| `value2`         | Yes       | 指派給結果變數的值。                           | 通常至少需要一個選項。                             |
| `value3`         | Yes       | 指派給結果變數的值。                           | 通常至少需要一個選項。                             |
| `value4`         | Yes       | 指派給結果變數的值。                           | 通常至少需要一個選項。                             |
| `value5`         | Yes       | 指派給結果變數的值。                           | 通常至少需要一個選項。                             |
| `value6`         | Yes       | 指派給結果變數的值。                           | 通常至少需要一個選項。                             |
| `value7`         | Yes       | 指派給結果變數的值。                           | 通常至少需要一個選項。                             |
| `value8`         | Yes       | 指派給結果變數的值。                           | 通常至少需要一個選項。                             |
| `time`           | Yes (`0`) | 計時器（秒）。                                | 若為 `0`，則不啟用計時器。                         |

### Localization

例如，如果使用者的作業系統語系是日文，系統會優先採用 `text1-ja`，而不是 `text1`。

| Suffix      | Language                                 |
|-------------|------------------------------------------|
| -en         | 英文（預設）                             |
| -en-us      | 英文（美國）                             |
| -en-gb      | 英文（英國）                             |
| -en-au      | 英文（澳洲）                             |
| -en-nz      | 英文（紐西蘭）                           |
| -fr         | 法文（預設）                             |
| -fr-fr      | 法文（法國）                             |
| -fr-ca      | 法文（加拿大）                           |
| -es         | 西班牙文（西班牙，預設）                 |
| -es-la      | 西班牙文（拉丁美洲）                     |
| -de         | 德文                                     |
| -it         | 義大利文                                 |
| -ru         | 俄文                                     |
| -el         | 希臘文                                   |
| -zh         | 中文（簡體）                             |
| -zh-tw      | 中文（繁體，臺灣）                       |
| -ja         | 日文                                     |
| （無字尾）  | 預設值（由開發者決定）                    |

對於所有英文系統語系，`-en` 會作為預裝置援。
如果標籤中指定了像 `-en-gb` 這樣更精確的變體，而且它與使用者地區最相符，就會優先採用。
西班牙文與法文也使用相同機制。
請注意，繁體中文不會回退到簡體中文。

例如，如果使用者語系是 `en-AU`，會套用以下優先順序：
* 1. text1-en-au
* 2. text1-en
* 3. text1

以下目前尚未支援，但預計未來會支援。

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

**分支邏輯**：
* 你可以使用 `[if]` 標籤檢查儲存的值，建立複雜分支。

```
[choose
  name="var1"
  text1="Go to school"
  text2="Go to hospital"
  value1="1"
  value2="2"]

[if lhs="${var1}" op="=" rhs="1"]
  # 學校。
[else]
  # 醫院。
[endif]
```

**變數儲存**：
* 因為所有內容本質上都是字串，請記得像 `"100"` 這樣的數字也會以文字儲存。
* Suika3 的邏輯標籤（例如 `if`）可以拿這些字串來做比較。

---

## `set`

設定變數

`set` 標籤會把值指定給某個變數名稱。
在 Suika3 中，**所有變數都會被視為文字字串**，但在像 `[if]` 這類其他標籤中仍可做數值比較。

### Basic Usage

```
# 將單純字串指派給變數
[set name="player_name" value="Kaito"]

# 設定類似數字的值（仍會以字串儲存）
[set name="health" value="100"]

# 透過設定空字串來清除變數
[set name="flag_event_01" value=""]

# 將 var1 加 1。
[set name="var1" value1="${var1}" op="+" value2="1"]

```

### Arguments

| Argument | Omissible     | Description                                 | Notes                                                               |
|----------|---------------|---------------------------------------------|---------------------------------------------------------------------|
| `name`   | No            | 變數的唯一名稱。                            | 為了最佳相容性，請使用英數字元與底線。                              |
| `value`  | Yes           | 要儲存在變數中的內容。                      | 請記住：所有內容都會以字串儲存！                                     |
| `value1` | Yes           | 運算元 1。                                  |                                                                     |
| `value2` | Yes           | 運算元 2。                                  |                                                                     |
| `op`     | Yes           | 運運算元。（`+`、`-`、`*`、`/`、`//`、`%`）   |                                                                     |
| `global` | Yes (`false`) | 是否設為全域旗標。                           | 全域變數可用於成就旗標，例如「看過 ED1」。                           |

### Tips

**字串處理**：
* 由於 Suika3 會把所有內容都當成文字，`value="100"` 和 `value="May"` 在內部的處理方式是相同的。
* 你可以在其他標籤（例如 `text` 或 `if`）中使用 `${variable_name}` 語法來引用這些變數。

**旗標管理**：
* 對於遊戲旗標（例如「是否遇過主角」），常見作法是使用 `"true"` 和 `"false"`，或 `"1"` 和 `"0"`。
* 一致性很重要！如果你一開始使用 `"1"`，就請一直維持下去，這樣 `[if]` 判斷才不會混淆。

**變數命名**：
* 請避免在變數名稱中使用空格或特殊符號。`my_variable` 會比 `my variable!` 安全得多。

---

## `click`

等待點選

`click` 標籤會暫停指令碼執行，等待玩家點選滑鼠或按下按鍵。
它通常用來在畫面變化之間，或在重大轉場前製造停頓。

### Basic Usage

```
# 變更背景，然後等待玩家點選
[bg file="sunset.png" time="1.0"]
[click]

# 點選之後顯示角色
[ch center="chara01.png" time="1.0"]
```

### Arguments

這個標籤不接受任何引數。

| Argument | Omissible | Description | Notes           |
|----------|-----------|-------------|-----------------|
| -        | -         | -           | -               |

### Tips

**時機與節奏**：
* 當你想在對話繼續前，讓玩家先看一下新的背景或特定角色表情時，可以使用 `[click]`。
* 和 `[text]` 標籤不同，`[text]` 在顯示訊息後會自動等待點選，而 `[click]` 則用於非對話段落中的手動流程控制。

**視覺回饋**：
* 當指令碼執行到 `[click]` 標籤時，遊戲會停在目前畫面。請確保前面的動畫（例如 `[bg]` 或 `[ch]`）都有設定 `time`，不然畫面可能會顯得太突兀地靜止。

**若要定時等待：**
* 請使用 `[wait]` 來做定時等待。

---

## `goto`

跳轉到標籤

`goto` 標籤會立刻把 NovelML 的執行移到指定標籤。
它是控制故事流程的實用工具，可以讓你跳過某些段落，或迴圈回前面的部分。

請注意，小型分支應該用 `[if]` 來實作。

### Basic Usage

```
# 跳到早晨場景的開頭
[goto name="morning_start"]

# ... 這段指令碼會被跳過 ...

[label name="morning_start"]
[text text="The sun rises over the horizon."]
```

### Arguments

| Argument | Omissible | Description                       | Notes                                         |
|----------|-----------|-----------------------------------|-----------------------------------------------|
| `name`   | No        | 要跳轉到的目標標籤名稱。          | 必須與 `[label]` 標籤定義的名稱一致。         |

### Tips

**無條件跳轉**：
* 和 `[if]` 不同，`[goto]` 會在引擎碰到標籤時立刻跳到目標標籤。

**流程管理**：
* 可以在分支路徑的結尾使用 `[goto]`，把故事帶回「共通」路線。
* 搭配其他邏輯時，它也很適合拿來建立迴圈（例如「返回標題」流程）。

**跨檔案？**：
* 請記得 `[goto]` 通常只會在目前指令碼檔案內運作。
* 如果你想完全跳到另一個檔案，就要看 `[load]` 標籤了！

---

## `defmacro`

定義巨集

`defmacro` 標籤會開始定義一個巨集。
巨集可以把多個標籤與命令組成一個有名稱的區塊，並且能在腳本中透過 `[callmacro]` 重複使用。

### Basic Usage

```
# 為特定角色登場定義巨集
[defmacro name="enter_kaito"]
    [ch center="kaito_smile.png" time="0.5"]
    [text name="Kaito" text="Hey! Did I keep you waiting?"]
[endmacro]

# 之後在腳本中只要一行就能呼叫
[callmacro name="enter_kaito"]
```

### Arguments

| Argument | Omissible | Description                     | Notes                                             |
|----------|-----------|---------------------------------|---------------------------------------------------|
| `name`   | No        | 這個巨集的唯一名稱。            | 用來在之後呼叫時識別這個巨集。                    |

### Tips

**結束定義**：
* 每個 `[defmacro]` 都必須搭配 `[endmacro]` 標籤，標示定義結束。

**程式重用**：
* 巨集很適合重複出現的序列，例如特定 UI 轉場、角色專屬視覺配置，或複雜的音效與畫面組合。

**組織方式**：
* 常見做法是在主腳本檔的最前面，或在啟動時載入的獨立檔案中，集中定義所有巨集。

**巢狀與邏輯**：
* 你幾乎可以在巨集中放入任何其他標籤，包含 `[if]` 判斷，甚至可以用 `[returnmacro]` 依條件提前離開巨集。

---

## `gui`

顯示 GUI

`gui` 標籤會從指定檔案載入並顯示圖形化使用者介面（GUI）定義。
它可用來顯示選單、標題畫面或自訂互動面板。

### Basic Usage

```
# 顯示主選單 GUI
[gui file="main_menu.txt"]

# 顯示自訂存檔/讀檔畫面
[gui file="save_screen.txt"]
```

### Arguments

| Argument | Omissible | Description                                 | Notes                                               |
|----------|-----------|---------------------------------------------|-----------------------------------------------------|
| `file`   | No        | 要載入的 GUI 定義檔案名稱。                 | 檔案必須存在於專案的 GUI 目錄中。                  |

### Tips

**GUI 定義**：
* `file` 引數指向一個文字檔，這個檔案會定義介面的版面、按鈕與動作。
* 這些檔案會指定圖片放置的位置，以及使用者互動時會發生什麼事（例如跳到標籤或結束遊戲）。

**流程中的用途**：
* 一般來說，`[gui]` 標籤會用在像標題畫面這類圖形化選單。

**自訂化**：
* 因為 GUI 是定義在外部檔案中，所以你可以為遊戲做出多種外觀，只要用這個標籤呼叫不同檔案即可切換。

---

## `label`

定義標籤

`label` 標籤會定義腳本中的特定位置，可供 `[goto]` 或 `[load]` 之類的跳轉命令鎖定。
它就像故事導覽用的書籤。

### Basic Usage

```
# 為新章節的開頭定義標籤
[label name="chapter_01_start"]

# 使用跳轉命令前往這個標籤
[goto name="chapter_01_start"]
```

### Arguments

| Argument | Omissible | Description                     | Notes                                                  |
|----------|-----------|---------------------------------|--------------------------------------------------------|
| `name`   | No        | 這個標籤的唯一名稱。            | 大小寫有區別。請避免使用空格或特殊符號。              |

### Tips

**導覽控制**：
* 標籤很適合用來建立分支路徑。
* 例如，你可以在故事某個段落的開頭放一個 `label`，方便長距離跳轉。

**名稱唯一**：
* 同一個腳本檔內的每個標籤名稱都必須唯一。
* 如果有兩個同名標籤，引擎可能會不知道該跳去哪裡，這對誰都不方便。

**組織方式**：
* 使用像 `label_evening_park` 這種有描述性的名稱，會比 `label1` 更好。
* 這會讓你（也讓我！）之後更容易閱讀腳本並理解發生了什麼。

---

## `text`

顯示文字

`text` 標籤會在訊息框中顯示訊息。
它可以顯示主要對話或旁白，也可以選擇在名稱框中顯示角色名稱。

### Basic Usage

```
# 旁白樣式（不顯示名稱）
[text text="The wind was howling through the trees."]

# 角色對話
[text name="Keith" text="I've been waiting for you here in the small room."]
```

### Arguments

| Argument         | Omissible | Description                                      | Notes                                            |
|------------------|-----------|--------------------------------------------------|--------------------------------------------------|
| `text`           | No        | 要顯示的訊息內容。                               |                                                  |
| `text-<locale>`  | Yes       | 要顯示的訊息內容（依語系）。                    |                                                  |
| `voice`          | Yes       | 聲音檔案。                                       |                                                  |
| `voice-<locale>` | Yes       | 聲音檔案（依語系）。                             |                                                  |
| `name`           | Yes       | 要顯示在名稱框中的角色名稱。                    | 若省略，名稱框通常會隱藏。                       |
| `action`         | Yes       | 用於 NVL 模式與手動顯示/隱藏。                  |                                                  |
| `space`          | Yes       | 用於 NVL 模式。                                  |                                                  |

### Localization

例如，如果使用者的作業系統語系是日文，系統會優先採用 `text-ja`，而不是 `text`。

| Suffix      | Language                                 |
|-------------|------------------------------------------|
| -en         | 英文（預設）                             |
| -en-us      | 英文（美國）                             |
| -en-gb      | 英文（英國）                             |
| -en-au      | 英文（澳洲）                             |
| -en-nz      | 英文（紐西蘭）                           |
| -fr         | 法文（預設）                             |
| -fr-fr      | 法文（法國）                             |
| -fr-ca      | 法文（加拿大）                           |
| -es         | 西班牙文（西班牙，預設）                 |
| -es-es      | 西班牙文（西班牙，預設）                 |
| -es-la      | 西班牙文（拉丁美洲）                     |
| -de         | 德文                                     |
| -it         | 義大利文                                 |
| -ru         | 俄文                                     |
| -el         | 希臘文                                   |
| -zh-cn      | 中文（簡體）                             |
| -zh-tw      | 中文（繁體，台灣）                       |
| -ja         | 日文                                     |
| （無字尾）  | 預設值（由開發者決定）                    |

對於所有英文系統語系，`-en` 會作為預設備援。
如果標籤中指定了像 `-en-gb` 這樣更精確的變體，而且它與使用者地區最相符，就會優先採用。
西班牙文與法文也使用相同機制。
請注意，繁體中文不會回退到簡體中文。

例如，如果使用者語系是 `en-GB`，會套用以下優先順序：
* 1. text-en-gb
* 2. text-en
* 3. text

以下目前尚未支援，但預計未來會支援。

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

### 動作

你可以在 `text` 標籤中使用特殊參數。

```
# 清除訊息框。
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

條件分支

`if` 標籤可讓 NovelML 根據特定條件分支。
透過比較變數或值，你可以建立獨特的故事路徑，或對玩家先前的選擇做出反應。

### Basic Usage

```
# 檢查變數是否等於某個值
[if lhs="${points}" op="==" rhs="100"]
    [text text="滿分！你太棒了。"]
[elseif lhs="${points}" op=">=" rhs="80"]
    [text text="做得好！你過關了。"]
[else]
    [text text="下次再接再厲。"]
[endif]
```

### Arguments

| Argument | Omissible | Description                            | Notes                                          |
|----------|-----------|----------------------------------------|------------------------------------------------|
| `lhs`    | No        | 條件左側。                             | 通常是像 `${var_name}` 這樣的變數。           |
| `op`     | No        | 用於比較的運算子。                     | 請參考下方的「運算子」表。                     |
| `rhs`    | No        | 條件右側。                             | 要拿來比較的值或變數。                         |

### Comparison Operators (`op`)

你可以使用這些運算子來定義兩側的比較方式：

| Operator   | Description                |
|------------|----------------------------|
| `===`      | 相等（字串）               |
| `==`       | 相等（數值）               |
| `>`        | 大於（數值）               |
| `>=`       | 大於等於（數值）           |
| `<`        | 小於（數值）               |
| `<=`       | 小於等於（數值）           |

### Tips

**結束區塊**：
* 每個 `[if]` 區塊都必須以 `[endif]` 標籤結束。
* 如果忘了加，引擎可能會搞不清楚條件結束在哪裡！

**變數語法**：
* 當你把變數用在 `lhs` 時，請一律用 `${}` 包起來。
* 例如請使用 `lhs="${flag_01}"`，不要只寫 `lhs="flag_01"`。

**字串與數字處理**：
* Suika3 會把變數值當成字串，但這些運算子仍可讓你進行類似數值比較。
* 只要保持值的一致性即可（例如用 `"1"` 表示真、`"0"` 表示假）。

**多重分支**：
* 你可以在 `[if]` 與 `[endif]` 之間使用任意多個 `[elseif]` 標籤，來檢查多個特定條件。

---

## `elseif`

附加條件分支

`elseif` 標籤會在 `[if]` 區塊中指定額外條件。
只有在前面的 `[if]` 與任何先前的 `[elseif]` 條件都為假時才會進行評估。

### Basic Usage

```
[if lhs="${rank}" op="==" rhs="A"]
    [text text="太棒了！你是高手。"]
[elseif lhs="${rank}" op="==" rhs="B"]
    [text text="做得好！繼續加油。"]
[elseif lhs="${rank}" op="==" rhs="C"]
    [text text="不錯，但你還可以更好。"]
[else]
    [text text="別放棄！再試一次。"]
[endif]
```

### Arguments

與 `[if]` 相同。另請參閱 [if](#if)。

### Tips

**順序評估**：
* 引擎會由上到下檢查條件。
* 只要有一個 `[if]` 或 `[elseif]` 條件成立，就會執行其區塊，並略過其餘分支（包含其他 `[elseif]` 與 `[else]`）。

**位置**：
* `[elseif]` 必須放在 `[if]` 標籤與 `[else]` 或 `[endif]` 標籤之間。
* 你可以根據需要使用任意多個 `[elseif]` 標籤來涵蓋所有情況！

**維護性**：
* 如果有很多條件都在檢查同一個變數，使用多個 `[elseif]` 標籤會比把多個 `[if]` 區塊互相巢狀更乾淨，也更有效率。

---

## `else`

預設條件分支

`else` 標籤會定義一個當前面 `[if]` 或 `[elseif]` 條件都不成立時要執行的程式區塊。
它會成為分支邏輯中的「預設」路徑。

### Basic Usage

```
[if lhs="${weather}" op="==" rhs="sunny"]
    [text text="It's a beautiful day!"]
[elseif lhs="${weather}" op="==" rhs="rainy"]
    [text text="I should bring an umbrella."]
[else]
    # 如果不是晴天也不是雨天就會執行（例如多雲或下雪）
    [text text="今天的天空看起來很有意思。"]
[endif]
```

### Arguments

這個標籤不接受任何引數。

| Argument | Omissible | Description | Notes |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### Tips

**最後保底**：
* 使用 `[else]` 來處理你在 `[if]` 或 `[elseif]` 中沒有明確涵蓋的情況。
* 這可以確保遊戲永遠有一條可走的有效路徑。

**位置**：
* `[else]` 必須放在所有 `[elseif]` 標籤之後（如果有的話），並且緊接在 `[endif]` 標籤之前。
* 每個 `[if]` 區塊只能有一個 `[else]`。

**可選性**：
* 你不一定要包含 `[else]` 區塊。
* 如果沒有任何條件成立，而且也沒有 `[else]`，引擎就會直接略過全部內容，繼續執行 `[endif]` 之後的部分。

---

## `endif`

結束條件分支

`endif` 標籤會標示由 `[if]` 標籤開始的條件區塊結束。
它會告訴引擎在分支邏輯完成後，回到正常的腳本執行流程。

### Basic Usage

```
[if lhs="${love_points}" op=">=" rhs="50"]
    [text text="She gives you a warm smile."]
[else]
    [text text="She greets you politely."]
[endif]

# 不論上面的結果如何，腳本都會繼續執行到這裡
[text text="這一天仍在繼續..."]
```

### Arguments

這個標籤不接受任何引數。

| Argument | Omissible | Description | Notes |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### 提示

**必要結尾**：
* 每一個 `[if]` 標籤都必須有對應的 `[endif]`。
* 可以把它們想成一對括號，用來維持故事邏輯的整齊。

**位置**：
* 請一律把 `[endif]` 放在條件序列的最後，也就是所有 `[elseif]` 或 `[else]` 區塊之後。

**巢狀**：
* 如果你把一個 `[if]` 放在另一個 `[if]` 裡面，請確保每一個都有自己的 `[endif]`。
* 正確的巢狀結構，是讓複雜劇情旗標不出錯的關鍵！

---

## `load`

載入腳本檔案

`load` 標籤會把目前腳本切換到另一個 NovelML 檔案。
它主要用來把大型故事整理成多個章節，或在標題畫面與主劇情等不同遊戲部分之間切換。

### Basic Usage

```
# 載入並從 scene02.novel 的開頭開始
[load file="scene02.novel"]

# 載入 scene02.novel，並直接跳到特定標籤
[load file="scene02.novel" label="chapter2_start"]
```

### Arguments

| Argument | Omissible | Description                                      | Notes                                                   |
|----------|-----------|--------------------------------------------------|---------------------------------------------------------|
| `file`   | No        | 要載入的 NovelML 腳本檔案名稱。                 | 必須是專案腳本目錄中的有效檔案。                        |
| `label`  | Yes       | 新檔案中要跳轉到的目標標籤。                     | 若省略，腳本會從第一行開始執行。                        |

### Tips

**專案組織**：
* 不要把整個遊戲都寫在同一個超大檔案裡，請使用 `[load]` 將內容拆成好管理的區塊，例如 `chapter1.novel`、`chapter2.novel` 等。

**立即切換**：
* 當引擎碰到 `[load]` 標籤時，會立刻停止執行目前的 NovelML 檔案，並切換到新檔案。
* 原檔案中 `[load]` 之後的任何命令都不會被執行。

**全域旗標**：
* 不用擔心你的變數——即使載入新的腳本檔案，先前用 `[set]` 標籤設定的值仍會保留！

---

## `se`

播放音效

`se` 標籤會播放音效（SE）。
音效通常用於敲門、腳步聲或 UI 反饋這類短促聲音，能替場景增加沉浸感與真實感。

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
| `file`   | No            | 要播放的音效檔案名稱。                    | 設為 `none` 可停止音效播放。                 |
| `loop`   | Yes (`false`) | 是否循環播放音效。                        |                                              |

### Tips

**必要格式**：
* 和 BGM 一樣，Suika3 要求 SE 檔案必須是 **Ogg Vorbis** 格式。
* 取樣率必須是 **44,100Hz**，才能確保高音質與相容性。

**音效疊加**：
* 音效通常可以在 BGM 播放時一併播放。
* 它們會佔用自己的音軌，因此不會中斷音樂。

**音量控制**：
* 若要調整音效音量而不影響 BGM，請在 `[volume]` 標籤中使用 `track="se"`。

**環境音用途**：
* 雖然 SE 常用於短音效，但你也可以拿來做循環的環境音（例如風聲或雨聲）。
* 載入存檔時，循環中的 SE 會被還原。

---

## `volume`

設定音訊音量

`volume` 標籤會設定特定音軌的音量。
它很適合用來確保背景音樂不會蓋過重要的音效或角色語音。

### Basic Usage

```
# 將 BGM 音量設為 50%
[volume track="bgm" volume="0.5"]

# 將 SE 音量設為最大
[volume track="se" volume="1.0"]

# 將語音靜音
[volume track="voice" volume="0.0"]
```

### Arguments

| Argument | Omissible | Description                           | Notes                                     |
|----------|-----------|---------------------------------------|-------------------------------------------|
| `track`  | No        | 要調整的音軌。                        | 請參考下方的「音軌」表。                  |
| `volume` | No        | 音量大小，範圍從 `0.0` 到 `1.0`。     | `0.0` 表示靜音，`1.0` 表示最大音量。      |
| `time`   | Yes (`0`) | 淡出/淡入時間（秒）。                 | `0` 表示立即變更。                        |
### Track Types (`track`)

Suika3 將音訊分為三種主要音軌：

| Track Name | Description                      |
|------------|----------------------------------|
| `bgm`      | 背景音樂。                      |
| `se`       | 音效與系統音效。                |
| `voice`    | 角色語音檔案。                  |

### Tips

**立即變更**：
* 當 `time` 大於 `0` 時，音量變更會逐漸發生。
* `time="0"` 則表示立即變更。

**預設音量**：
* 建議在遊戲一開始就設定好你偏好的音量，例如在 `start` 標籤中，讓玩家從頭就有一致的體驗。

---

## `skip`

設定快轉狀態

`skip` 標籤可以啟用或停用遊戲中的快轉功能。
它很適合用來避免玩家跳過重要的過場，或確保某些場景能以預定節奏體驗。

### Basic Usage

```
# 啟用快轉功能
[skip enable="true"]

# 在關鍵場景中停用快轉功能
[skip enable="false"]
```

### Arguments

| Argument | Omissible | Description                                  | Notes                                                 |
|----------|-----------|----------------------------------------------|-------------------------------------------------------|
| `enable` | No        | 快轉功能是否啟用。                      | 設為 `true` 可允許快轉，`false` 則禁止。 |

### Tips

**過場控制**：
* 快轉功能通常會在啟動畫面與標誌出現前停用。

**還原設定**：
* 關鍵場景結束後，別忘了設定 `[skip enable="true"]`。
* 玩家通常會喜歡能夠跳過已看過文字的自由。

**系統行為**：
* 這個標籤會控制引擎整體的「快轉」狀態。
* 即使玩家按下快轉快捷鍵，只要 `enable` 設為 `false`，引擎也會忽略它。

---

## `config`

設定組態值

`config` 標籤可以直接在標記中修改遊戲系統的設定。
它對於動態調整遊戲 UI 非常重要，例如移動訊息框或即時修改系統層級參數。

### Basic Usage

```
# 變更訊息框位置
[config name="msgbox.x" value="100"]
[config name="msgbox.y" value="200"]

# 更新特定系統設定
[config name="msgbox.font.size" value="24"]
```

### Arguments

| Argument | Omissible | Description                                        | Notes                                                    |
|----------|-----------|----------------------------------------------------|----------------------------------------------------------|
| `name`   | No        | 要變更的設定參數名稱。                   | 請參考系統的設定清單以確認合法名稱。       |
| `value`  | No        | 要指派給參數的新值。                     | 值會以字串處理，但也可能代表數字。         |

### Tips

**UI 自訂**：
* 你可以在特定場景中使用 `[config]` 重新定位訊息框，營造更具電影感的效果。

**動態調整**：
* 因為這個標籤可以在腳本中的任何地方呼叫，所以你可以隨著故事推進改變遊戲的「外觀與手感」。
* 例如為「回憶」段落調整 UI。

**參數名稱**：
* `name` 引數要特別小心！
* 它必須與你的 Suika3 專案設定中定義的內部設定鍵完全一致。
* 另請參閱 [完整設定列表](config.md)

---

## `layer`

直接操作圖層

`layer` 標籤可以直接控制特定的圖片與文字圖層。
雖然像 `[bg]` 和 `[ch]` 這類標籤更適合一般場景，但 `[layer]` 能讓你精準地獨立修改任一圖層的位置、縮放與旋轉。

### Basic Usage

```
# 直接將圖片載入中央角色圖層（chc）
[layer name="chc" file="heroine_smile.png"]

# 只調整背景（bg）的位置與透明度
[layer name="bg" x="100" y="100" alpha="128"]

# 旋轉臉部圖層
[layer name="chf" rotate="45.0" center-x="100" center-y="100"]
```

### Arguments

| Argument  | Omissible   | Description                          | Notes                              |
|-----------|-------------|--------------------------------------|------------------------------------|
| `name`    | No          | 目標圖層名稱。                       | 另請參閱下方的「圖層名稱」表。     |
| `file`    | Yes         | 要載入到圖層上的檔案名稱。           | 使用 `none` 可清除圖層。           |
| `x`       | Yes (`0`)   | 圖層的 X 座標。                      |                                    |
| `y`       | Yes (`0`)   | 圖層的 Y 座標。                      |                                    |
| `alpha`   | Yes (`255`) | 圖層的不透明度。                     | `0` 到 `255`。                     |
| `scale-x` | Yes (`1.0`) | X 軸縮放倍率。                       | `1.0` 為原始大小。                |
| `scale-y` | Yes (`1.0`) | Y 軸縮放倍率。                       | `1.0` 為原始大小。                |
| `center-x`| Yes (`0`)   | 旋轉中心（X）。                     | 旋轉的樞軸點。                    |
| `center-y`| Yes (`0`)   | 旋轉中心（Y）。                     | 旋轉的樞軸點。                    |
| `rotate`  | Yes (`0.0`) | 旋轉角度（度）。                     | 正值代表順時針。                  |

### Common Layer Names (`name`)

Suika3 內建了相當豐富的預定義圖層。

以下是完整圖層清單：

|圖層名稱         |說明                                      |
|-----------------|-----------------------------------------|
|bg               |背景圖像。                                |
|bg2              |背景圖像 2。                              |
|efb1             |後景效果 1。                              |
|efb2             |後景效果 2。                              |
|efb3             |後景效果 3。                              |
|efb4             |後景效果 4。                              |
|chb              |後中角色。                                |
|chb-eye          |後中角色眼睛。                            |
|chb-lip          |後中角色嘴唇。                            |
|chb-fo           |淡出中的後中角色。                        |
|chl              |左側角色。                                |
|chl-eye          |左側角色眼睛。                            |
|chl-lip          |左側角色嘴唇。                            |
|chl-fo           |淡出中的左側角色。                        |
|chlc             |左中角色。                                |
|chlc-eye         |左中角色眼睛。                            |
|chlc-lip         |左中角色嘴唇。                            |
|chlc-fo          |淡出中的左中角色。                        |
|chr              |右側角色。                                |
|chr-eye          |右側角色眼睛。                            |
|chr-lip          |右側角色嘴唇。                            |
|chr-fo           |淡出中的右側角色。                        |
|chrc             |右中角色。                                |
|chrc-eye         |右中角色眼睛。                            |
|chrc-lip         |右中角色嘴唇。                            |
|chrc-fo          |淡出中的右中角色。                        |
|chc              |中央角色。                                |
|chc-eye          |中央角色眼睛。                            |
|chc-lip          |中央角色嘴唇。                            |
|chc-fo           |淡出中的中央角色。                        |
|msgbox           |訊息框（`[layer]` 無法看見）。            |
|namebox          |名稱框（`[layer]` 無法看見）。            |
|click            |點擊動畫（`[layer]` 無法看見）。          |
|eff1             |前景效果 1。                              |
|eff2             |前景效果 2。                              |
|eff3             |前景效果 3。                              |
|eff4             |前景效果 4。                              |
|chf              |臉部角色。                                |
|chf-eye          |臉部角色眼睛。                            |
|chf-lip          |臉部角色嘴唇。                            |
|chf-fo           |淡出中的臉部角色。                        |
|text1            |文字圖層 1。                              |
|text2            |文字圖層 2。                              |
|text3            |文字圖層 3。                              |
|text4            |文字圖層 4。                              |
|text5            |文字圖層 5。                              |
|text6            |文字圖層 6。                              |
|text7            |文字圖層 7。                              |
|text8            |文字圖層 8。                              |
|gui1             |GUI 按鈕 1（`[layer]` 無法看見）。        |
|gui2             |GUI 按鈕 2（`[layer]` 無法看見）。        |
|gui3             |GUI 按鈕 3（`[layer]` 無法看見）。        |
|gui4             |GUI 按鈕 4（`[layer]` 無法看見）。        |
|gui5             |GUI 按鈕 5（`[layer]` 無法看見）。        |
|gui6             |GUI 按鈕 6（`[layer]` 無法看見）。        |
|gui7             |GUI 按鈕 7（`[layer]` 無法看見）。        |
|gui8             |GUI 按鈕 8（`[layer]` 無法看見）。        |
|gui9             |GUI 按鈕 9（`[layer]` 無法看見）。        |
|gui10            |GUI 按鈕 10（`[layer]` 無法看見）。       |
|gui11            |GUI 按鈕 11（`[layer]` 無法看見）。       |
|gui12            |GUI 按鈕 12（`[layer]` 無法看見）。       |
|gui13            |GUI 按鈕 13（`[layer]` 無法看見）。       |
|gui14            |GUI 按鈕 14（`[layer]` 無法看見）。       |
|gui15            |GUI 按鈕 15（`[layer]` 無法看見）。       |
|gui16            |GUI 按鈕 16（`[layer]` 無法看見）。       |
|gui17            |GUI 按鈕 17（`[layer]` 無法看見）。       |
|gui18            |GUI 按鈕 18（`[layer]` 無法看見）。       |
|gui19            |GUI 按鈕 19（`[layer]` 無法看見）。       |
|gui20            |GUI 按鈕 20（`[layer]` 無法看見）。       |
|gui21            |GUI 按鈕 21（`[layer]` 無法看見）。       |
|gui22            |GUI 按鈕 22（`[layer]` 無法看見）。       |
|gui23            |GUI 按鈕 23（`[layer]` 無法看見）。       |
|gui24            |GUI 按鈕 24（`[layer]` 無法看見）。       |
|gui25            |GUI 按鈕 25（`[layer]` 無法看見）。       |
|gui26            |GUI 按鈕 26（`[layer]` 無法看見）。       |
|gui27            |GUI 按鈕 27（`[layer]` 無法看見）。       |
|gui28            |GUI 按鈕 28（`[layer]` 無法看見）。       |
|gui29            |GUI 按鈕 29（`[layer]` 無法看見）。       |
|gui30            |GUI 按鈕 30（`[layer]` 無法看見）。       |
|gui31            |GUI 按鈕 31（`[layer]` 無法看見）。       |
|gui32            |GUI 按鈕 32（`[layer]` 無法看見）。       |

### Tips

**精準控制**：
* 當你在處理沒有專用標籤的自訂效果圖層（例如 `eff1` 等）時，可以用 `[layer]` 手動將圖片載入圖層。

**即時更新**：
* 和 `[ch]` 或 `[bg]` 不同，`layer` 標籤通常會立即更新畫面。
* 如果你想讓這些變化隨時間動畫化，應該改用 `[move]` 標籤。

**圖層階層**：
* 請記住圖層是疊放的。
* 例如 `chf`（臉部角色）永遠會渲染在 `chc`（中央角色）前方。
* 理解這個「Z 順序」是建立複雜視覺構圖的關鍵。

---

## `move`

圖層動畫

`move` 標籤會在指定時間內讓特定圖層產生動畫。
它很適合做滑動效果、角色拉近，或旋轉螢幕元素，為場景加入動態感。

### Basic Usage

```
# 在 2.0 秒內將中央角色移到新位置
[move time="2.0" center-x="150" center-y="100"]

# 相對移動：把背景往右推 50 像素
[move time="1.0" bg-x="r50"]

# 在旋轉圖層的同時逐漸淡出
[move time="3.0" face-alpha="0" face-rotate="r360"]
```

### Arguments

**共通：**
| Argument         | Omissible     | Description                               | Notes                                      |
|------------------|---------------|-------------------------------------------|--------------------------------------------|
| `name`           | No            | 要動畫化的目標圖層。                      | 另請參閱下方的「可移動圖層」表。          |
| `time`           | No            | 動畫持續時間（秒）。                      | 支援小數值（例如 `0.5`）。               |
| `async`          | Yes (`false`) | 若為 `true`，則非阻塞執行動畫。           |                                            |
| `accel`          | Yes (`normal`)| 加速度類型。                              | 其一                                       |
| (layer)-(suffix) | Yes           |                                           |                                            |

**(layer):**
| Argument       | Description                               |
|----------------|-------------------------------------------|
| `bg`           | 背景圖層。                                |
| `bg2`          | 背景 2。                                  |
| `back`         | 後中角色。                                |
| `left`         | 左側角色。                                |
| `right`        | 右側角色。                                |
| `center`       | 中央角色。                                |
| `left-center`  | 左中角色。                                |
| `right-center` | 右中角色。                                |
| `face`         | 臉部角色。                                |

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

**非阻塞動畫（`async="true"`）**：
* 腳本會在開始 `[move]` 後立刻繼續執行下一個命令。
* 如果你想讓腳本等到動畫結束，可以在後面接一個 `[wait]` 標籤，並使用相同的 `time` 值。

**相對變換**：
* 使用 `r` 前綴（例如 `x="r100"`）對重複動作非常有用，例如讓角色「跳動」或「晃動」，而不必計算絕對座標。

**視覺修飾**：
* 把 `scale-x` 和 `scale-y` 與 `move` 結合，可以在角色臉部上做出「放大」效果，營造戲劇性的特寫。

---

## `pencil`

鉛筆

在圖層上繪製文字。

### Basic Usage

```
[pencil layer="bg" font-size="30" text="Hello, World!"]
```

### Arguments

| Argument      | Omissible        | Description              |
|---------------|------------------|--------------------------|
| text          | No               | 要繪製的文字。           |
| layer         | Yes (`text1`)    | 圖層名稱。               |
| font-type     | Yes (`0`)        | 字型選擇。（0-3）        |
| font-size     | Yes (`16`)       | 字型大小。               |
| color         | Yes (`#000000`)  | 字型顏色。               |
| outline-width | Yes (`0`)        | 字型外框寬度。           |
| outline-color | Yes (`#ffffff`)  | 字型外框顏色。           |
| line-margin   | Yes              | 行距。                   |
| char-margin   | Yes (`0`)        | 字元間距。               |
| x             | Yes (`0`)        | 繪製區域 X 座標。        |
| y             | Yes (`0`)        | 繪製區域 Y 座標。        |
| width         | Yes              | 繪製區域寬度。           |
| height        | Yes              | 繪製區域高度。           |

## Supported Layer Name

|Layer Name       |Description                              |
|-----------------|-----------------------------------------|
|bg               |背景圖像。                                |
|bg2              |背景圖像 2。                              |
|efb1             |後景效果 1。                              |
|efb2             |後景效果 2。                              |
|efb3             |後景效果 3。                              |
|efb4             |後景效果 4。                              |
|chb              |後中角色。                                |
|chl              |左側角色。                                |
|chlc             |左中角色。                                |
|chr              |右側角色。                                |
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

從巨集返回

`returnmacro` 標籤會立刻結束目前的巨集，並將腳本執行返回到原始 `[callmacro]` 標籤後的那一行。
它特別適合在 `[if]` 區塊中，依特定條件提前停止巨集。

### Basic Usage

```
[defmacro name="check_item"]
    [if lhs="${has_key}" op="==" rhs="false"]
        [text text="The door is locked."]
        [returnmacro]
    [endif]

    # 只有在 has_key 為真時才會執行這一段
    [text text="你用鑰匙打開了門！"]
[endmacro]
```

### Arguments

This tag does not take any arguments.

| Argument | Omissible | Description | Notes |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### 提示

**提前退出**：
* 在 `[if]` 區塊中使用 `[returnmacro]`，可在滿足條件時跳過巨集的其餘命令。
* 這會讓你的巨集更有彈性，也更強大。

**隱式返回**：
* 你其實不必在每個巨集的最後都放 `[returnmacro]`。
* 一旦引擎碰到 `[endmacro]` 標籤，就會自動返回主腳本。

**流程控制**：
* 請記得，這個標籤只會結束「目前」的巨集。它不會停止整個遊戲，也不會跳到其他標籤，只會把你送回巨集被呼叫的地方。

---

## `video`

播放影片

`video` 標籤會在畫面上播放影片檔。
它非常適合用於開場動畫、轉場過場，或最適合以動態影片呈現的高衝擊視覺效果。

### Basic Usage

```
# 播放開場影片（不可跳過）
[video file="opening.mp4"]

# 播放一段可由玩家點擊跳過的短過場。
[video file="cutscene01.mp4" skippable="true"]
```

### Arguments

| Argument    | Omissible     | Description                                           | Notes                                                         |
|-------------|---------------|-------------------------------------------------------|---------------------------------------------------------------|
| `file`      | No            | 要播放的影片檔案名稱。                                | 檔案必須是支援的格式（例如 .mp4）。                        |
| `skippable` | Yes (`false`) | 影片是否可由玩家點擊跳過。                            | 設為 `false` 可強制玩家看完整段影片。                    |

### Tips

**檔案支援**：
* 請確認你的影片檔是 .mp4（H.264 + AAC）格式。
* 如果你要支援 32 位元 Windows，請另外準備 .wmv 檔，再與 .mp4 檔一同提供；此時可省略副檔名，例如 `[video file="opening"]`。

**轉場**：
* 影片播放完畢（或被跳過）後，引擎會自動繼續執行腳本中的下一個命令。
* 通常會建議在 `[video]` 標籤後接一個 `[bg]` 標籤，確保影片結束後畫面正好符合你的預期。

**影片中的音訊**：
* 大多數影片檔都會包含自己的音軌。
* 請記得，這個音訊會和正在播放的 `[bgm]` 一起播放。
* 如果影片本身有聲音，你可能會想在開始播放前先用 `[bgm file="none"]` 停掉音樂。

---

## `wait`

等待時間

`wait` 標籤會暫停 NovelML 的執行一段指定時間。
它對於控制視覺轉場節奏、製造戲劇性停頓，或在不需要玩家輸入的情況下精準控制時機都很重要。

### Basic Usage

```
# 在下一個命令前暫停 1.5 秒
[wait time="1.5"]

# 在角色變更之間製造短暫停頓
[ch center="chara01_surprised.png" time="0.5"]
[wait time="1.0"]
[text text="She couldn't believe her eyes."]
```

### Arguments

| Argument      | Omissible     | Description                    | Notes                                  |
|---------------|---------------|--------------------------------|----------------------------------------|
| `time`        | No            | 要等待的秒數。                 | 支援小數值（例如 `0.5`）。             |
| `hidemsgbox`  | Yes (`false`) | 強制隱藏訊息框。               |                                        |
| `hidenamebox` | Yes (`false`) | 強制隱藏名稱框。               |                                        |

### Tips

**非互動停頓**：
* 和等待玩家操作的 `[click]` 不同，`[wait]` 會在時間到後自動繼續。
* 這很適合用在「自動播放」段落或有時間限制的視覺序列。

**與動畫搭配**：
* 如果你在 `[ch]` 或 `[bg]` 標籤中使用 `time` 引數，引擎會在動畫播放時立即前往下一個命令。
* 若你想讓腳本停在動畫結束前，或停得更久以營造戲劇效果，可以在動畫後加上 `[wait]`。

**使用者體驗**：
* 請小心不要在沒有視覺理由的情況下，把 `[wait]` 設太久（例如超過 3 秒），不然玩家可能會以為遊戲卡住了！
