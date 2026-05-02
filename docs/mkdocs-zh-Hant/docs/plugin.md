Ray 外掛開發參考
================================

## 資料夾

外掛必須存放在 `system/plugin/<PLUGIN-NAME>/` 目錄中。

## 檔案

外掛檔必須存放為 `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray`。

## 函式

外掛必須定義 `plugin_init_<PLUGIN-NAME>()` 函式。

## 定義新標籤

在 `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray` 檔案中定義一個名為 `Tag_mytag()` 的函式，就能建立名為 `mytag` 的新標籤。
透過 `Suika.loadPlugin()` 載入外掛之後，就可以在 NovelML 中使用 `mytag`。

## 範例

在 `system/plugin/testplugin/testplugin.ray` 中：
```
func plugin_init_testplugin() {
    // 載入時呼叫。
    print("Plugin is loaded.");
}

// 新標籤。
func Tag_testplugintag(params) {
    print("Plugin tag is called.");
    print("parameter: " + params.text);

    Suika.moveToNextTag();
}
```

在 `main.ray` 中：
```
// 遊戲開始前呼叫。
func start() {
    // 請勿刪除以下這行。
    Suika.start();

    Suika.loadPluin("testplugin");
}
```

在 `start.novel` 中：
```
[testplugintag text="hello"]
```
