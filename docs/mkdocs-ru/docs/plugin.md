Справочник по разработке плагинов Ray
================================

## Папка

Плагины должны храниться в каталоге `system/plugin/<PLUGIN-NAME>/`.

## Файл

Файл плагина должен храниться как `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray`.

## Функция

Плагин должен определять функцию `plugin_init_<PLUGIN-NAME>()`.

## Определение нового тега

Определите функцию с именем `Tag_mytag()` в файле `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray`, чтобы создать новый тег с именем `mytag`.
После загрузки плагина через `Suika.loadPlugin()` можно использовать `mytag` в NovelML.

## Пример

В `system/plugin/testplugin/testplugin.ray`:
```
func plugin_init_testplugin() {
    // Вызывается при загрузке.
    print("Plugin is loaded.");
}

// Новый тег.
func Tag_testplugintag(params) {
    print("Plugin tag is called.");
    print("parameter: " + params.text);

    Suika.moveToNextTag();
}
```

В `main.ray`:
```
// Вызывается перед запуском игры.
func start() {
    // Не удаляйте следующую строку.
    Suika.start();

    Suika.loadPluin("testplugin");
}
```

В `start.novel`:
```
[testplugintag text="hello"]
```