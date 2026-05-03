Ray 外挂开发参考
================================

## 资料夹

外挂必须存放在 `system/plugin/<PLUGIN-NAME>/` 目录中。

## 档案

外挂档必须存放为 `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray`。

## 函式

外挂必须定义 `plugin_init_<PLUGIN-NAME>()` 函式。

## 定义新标签

在 `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray` 档案中定义一个名为 `Tag_mytag()` 的函式，就能建立名为 `mytag` 的新标签。
透过 `Suika.loadPlugin()` 载入外挂之后，就可以在 NovelML 中使用 `mytag`。

## 范例

在 `system/plugin/testplugin/testplugin.ray` 中：
```
func plugin_init_testplugin() {
    // 载入时呼叫。
    print("Plugin is loaded.");
}

// 新标签。
func Tag_testplugintag(params) {
    print("Plugin tag is called.");
    print("parameter: " + params.text);

    Suika.moveToNextTag();
}
```

在 `main.ray` 中：
```
// 游戏开始前呼叫。
func start() {
    // 请勿删除以下这行。
    Suika.start();

    Suika.loadPluin("testplugin");
}
```

在 `start.novel` 中：
```
[testplugintag text="hello"]
```
