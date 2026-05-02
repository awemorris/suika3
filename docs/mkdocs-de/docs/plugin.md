Ray Plugin Development Reference
================================

## Ordner

Plugins müssen im Verzeichnis `system/plugin/<PLUGIN-NAME>/` abgelegt werden.

## Datei

Eine Plugin-Datei muss unter `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray` gespeichert werden.

## Funktion

Das Plugin muss die Funktion `plugin_init_<PLUGIN-NAME>()` definieren.

## Einen neuen Tag definieren

Definiere in der Datei `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray` eine Funktion namens `Tag_mytag()`, um einen neuen Tag mit dem Namen `mytag` zu erstellen.
Nachdem das Plugin über `Suika.loadPlugin()` geladen wurde, kannst du `mytag` in NovelML verwenden.

## Beispiel

In `system/plugin/testplugin/testplugin.ray`:
```
func plugin_init_testplugin() {
    // Wird beim Laden aufgerufen.
    print("Plugin wurde geladen.");
}

// Neuer Tag.
func Tag_testplugintag(params) {
    print("Plugin-Tag wurde aufgerufen.");
    print("parameter: " + params.text);

    Suika.moveToNextTag();
}
```

In `main.ray`:
```
// Wird vor dem Spielstart aufgerufen.
func start() {
    // Die folgende Zeile nicht löschen.
    Suika.start();

    Suika.loadPluin("testplugin");
}
```

In `start.novel`:
```
[testplugintag text="hello"]
```
