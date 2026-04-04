Ray Plugin Development Reference
================================

## Folder

Plugins must be stored in the `system/plugin/<PLUGIN-NAME>/` directory.

## File

A plugin file must be stored to the `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray` file.

## Function

The plugin must define the `plugin_init_<PLUGIN-NAME>()` function.

## Defining a New Tag

Define a function named `Tag_mytag()` in the `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray` file to create a new tag named `mytag`.
After loading the plugin via `Suika.loadPlugin()`, you can use `mytag` in NovelML.

## Sample

In `system/plugin/testplugin/testplugin.ray`:
```
func plugin_init_testplugin() {
    // Called when loaded.
    Suika.print("Plugin is loaded.");
}

// New tag.
func Tag_testplugintag(params) {
    Suika.print("Plugin tag is called.");
    Suika.moveToNextTag();
}
```

In `main.ray`:
```
// Called before the game starts.
func start() {
    // Do not delete the following line.
    Suika.start();

    Suika.loadPluin("testplugin");
}
```

In `start.novel`:
```
[testplugintag]
```
