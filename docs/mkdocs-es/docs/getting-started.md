Suika3: Guía de introducción
=============================

¡Bienvenidos a Suika3! Esta guía le ayudará a poner en marcha su primera
Proyecto de novela visual en tan solo unos sencillos pasos.

## 1. Instalación

¡Pongamos el motor en marcha para que puedas ver cómo ocurre la magia!

### ventanas

- **Descargar y extraer**
    - Descarga [Suika3-SDK-Full.zip](https://github.com/awemorris/suika3/releases/latest/download/Suika3-SDK-Full.zip) y extráelo a tu carpeta preferida.
- **Lanzamiento**
    - ¡Abre la carpeta y ejecuta `suika3.exe` para comenzar con el juego de muestra!

### MacOS

- **Descargar y extraer**
    - Descarga Suika3-full.zip y extráelo a tu carpeta preferida.
- **Montar la imagen del disco**
    - Navegue hasta `SDK/macos/` y abra `Suika3.dmg`.
- **Configurar el paquete de aplicaciones**
    - Copie la aplicación `Suika3` del DMG en la misma carpeta donde se encuentra `suika3.exe` (y la carpeta de datos).
    - Nota: el paquete de aplicaciones debe residir junto con los datos de tu juego para funcionar correctamente.
- **Lanzamiento**
    - ¡Haz doble clic en la aplicación `Suika3` para iniciar el juego de muestra!

###Linux

- **Descargar y extraer**
    - Descarga Suika3-full.zip y extráelo a tu directorio preferido.
- **Instalar el paquete Flatpak**
    - Navegue hasta `SDK/linux/` y abra `Suika3.flatpak` (o ejecute `flatpak install --user Suika3.flatpak`).
    - Esto asocia los archivos `.novel` y `.ray` con el motor Suika3.
- **Lanzamiento**
    - ¡Abre la carpeta extraída y luego haz doble clic en `start.novel` para iniciar el juego de muestra!

## 2. Integración del código de Visual Studio

¡La integración VSCode está disponible en Windows, macOS y Linux!

Además, [NovelML-Helper](https://github.com/lalalll-lalalll/NovelML-Helper) está disponible para resaltar la sintaxis.

- Abra la carpeta extraída por `Visual Studio Code`.
- Haga clic en la paleta de comandos.
- Haga clic en `Run Task`.
- Elige entre:
    - `Suika3: Run` (o `Ctrl+Shift+B`)
    - `Suika3: Create a package`
    - `Suika3: Build Android APK`
    - `Suika3: Build iOS IPA`
- Haga clic en `PROBLEMS` si ocurrió un error.

## 3. Personaliza tu historia (`start.novel`)

Ahora, hagamos que el juego diga exactamente lo que quieres.

- **Abierto:**
    - Busque el archivo `start.novel` en la carpeta de su proyecto y ábralo con su editor de texto favorito.
- **Editar:**
    - Añade la siguiente etiqueta al principio del archivo:
    ```
    [text text="Hello, world! This is my first game."]
    ```
- **Prueba:**
    - Guarde el archivo y ejecute Suika3 nuevamente.
    - ¡Deberías ver tu nuevo mensaje en la pantalla!

## 4. Personaliza la pantalla (main.ray)

Puedes cambiar fácilmente la apariencia de la ventana de tu juego.

- **Ubicar:**
    - Abra el archivo `main.ray` en su editor.
- **Modificar:**
    - Busque la sección `func setup()`.
    - Puedes cambiar la resolución y el título de tu ventana aquí:
    ```
    // Called when the window is opened.
    func setup() {
        return {
            width:      1280,            // The width of your game
            height:     720,             // The height of your game
            title:      "My First Game", // Your game's title
            fullscreen: false            // Set to true for full-screen mode
        };
    }
    ```

## 5. Debajo del capó (consejos avanzados)

La parte inferior de su archivo `main.ray` contiene el motor principal
lógica. Es mejor dejar estas funciones como están a menos que estés
haciendo personalización avanzada:

- `func start()`:
    - Esto se llama una vez cuando se inicia el juego.
- `func update()`:
    - Esto ejecuta cada cuadro para manejar la lógica del juego.
- `func render()`:
    - Esto dibuja todo en la pantalla una vez realizada la actualización.

```
// Called before the game starts.
func start() {
    // Load plugins here.
    // Suika.loadPlugin("testplugin");

    // Do not delete the following line.
    Suika.start();
}

// Called before a frame rendering.
func update() {
    // Do not delete the following line.
    Suika.update();
}

// Called every frame rendering.
func render() {
    // Do not delete the following line.
    Suika.render();
}
```

> [!CONSEJOS]
> Estas funciones son el mecanismo central del `Playfield Engine` que
> potencia Suika3. Suika.start(), Suika.update() y Suika.render() deben
> permanecer en su lugar para que el juego funcione correctamente.
