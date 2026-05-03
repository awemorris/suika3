Ray 插件开发参考
================================

## 文件夹

插件必须存放在 `system/plugin/<PLUGIN-NAME>/` 目录中。

## 文件

插件文件必须存放于 `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray`  文件中。

## 函数

插件必须定义 `plugin_init_<PLUGIN-NAME>()` 函数（作为入口点）。

## 定义新标签

在 `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray` 文件中定义一个名为 `Tag_mytag()` 的函数，即可创建一个名为 `mytag` 的新标签。
通过 `Suika.loadPlugin()` 加载插件后，你就可以在 NovelML 脚本中使用 `mytag` 了。

## 示例

在 `system/plugin/testplugin/testplugin.ray` 中：
```
func plugin_init_testplugin() {
    // 加载时被调用。
    print("Plugin is loaded.");
}
// 标签。
func Tag_testplugintag(params) {
    print("Plugin tag is called.");
    print("parameter: " + params.text);
    Suika.moveToNextTag();
}
```

在 `main.ray` 中：
```
// 在游戏开始前被调用。
func start() {
    // 不要删除下面这行。
    Suika.start();
    Suika.loadPluin("testplugin");
}
```

在 `start.novel` 中：
```
[testplugintag text="hello"]
```
