Referencia de desarrollo de complementos de Ray
================================

## Carpeta

Los complementos deben almacenarse en el directorio `system/plugin/<PLUGIN-NAME>/`.

## Archivo

Se debe almacenar un archivo de complemento en el archivo `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray`.

## Función

El complemento debe definir la función `plugin_init_<PLUGIN-NAME>()`.

## Definiendo una nueva etiqueta

Defina una función denominada `Tag_mytag()` en el archivo `system/plugin/<PLUGIN-NAME>/<PLUGIN-NAME>.ray` para crear una nueva etiqueta denominada `mytag`.
Después de cargar el complemento a través de `Suika.loadPlugin()`, puede usar `mytag` en NovelML.

## Muestra

En `system/plugin/testplugin/testplugin.ray`:
```
func plugin_init_testplugin() {
    // Called when loaded.
    print("Plugin is loaded.");
}

// New tag.
func Tag_testplugintag(params) {
    print("Plugin tag is called.");
    print("parameter: " + params.text);

    Suika.moveToNextTag();
}
```

En `main.ray`:
```
// Called before the game starts.
func start() {
    // Do not delete the following line.
    Suika.start();

    Suika.loadPluin("testplugin");
}
```

En `start.novel`:
```
[testplugintag text="hello"]
```
