GUI
===

## Introducción

GUI es la función de creación UI/UX de Suika3.

En Suika3, los botones se definen en un modo GUI dedicado y funcionan en
de manera sincrónica, es decir, llamar a una GUI da como resultado un clic en un botón o
una cancelación.

Devoluciones de llamada asincrónicas, como mostrar botones mientras se etiqueta texto
La ejecución se evita intencionalmente porque puede ser difícil para
programadores principiantes para entender y gestionar.

Un archivo GUI contiene una lista de definiciones de botones. Cada botón incluye
un tipo de comportamiento, imágenes inactivas y suspendidas, y adicionales
propiedades. Los botones se asignan a capas de animación y la animación
Los archivos se pueden activar cuando cambia el estado de un botón.

---

## Muestra de GUI

```
# Global settings section.
global {
    fadein:       0.2;            # Fading-in time in seconds.
    fadeout:      0.2;            # Fading-out time in seconds.
}

# A block describes a button.
# The name of a block can be whatever you like and it won't affect anything.
button1 {
    # Layer ID (`1`-`32`)
    # `1` means the top-most and `32` means the bottom-most in the GUI layers, respectively.
    id: 1;
 
    # Behavior Type (`goto` means go to a label when clicked.)
    type: goto;

    # Label to go to.
    label: button1_clicked;

    # Position
    x: 39;
    y: 99;

    # `width:` and `height:` are omissible as they can be inferred from the image size.

    # Images
    image-idle:  gui/item-idle.png;    # Shown when the button is not pointed.
    image-hover: gui/item-hover.png;   # Shown when the button is pointed.
    image-press: gui/item-press.png;   # Shown when the button is pressed.

    # Animations
    anime-idle:  gui/item-idle.anime;   # Executed when the button state is changed to `idle`.
    anime-hover: gui/item-hover.anime;  # Executed when the button state is changed to `hover`.
    anime-press: gui/item-press.anime;  # Executed when the button is pressed.

    # Note that `press` is not an independent state, it's an additional flag to the state `hover`.
    # When a `hover` anime runs, `idle` anime will be canceled.
    # And when `idle` anime runs, `hover` anime will be canceled.
    # However, `press` anime doesn't cancel anything.
}
```

En la sección `global`, puede especificar opciones para la GUI.

Otras secciones se interpretan como definiciones de botones.
Aquí, `button1` crea un botón en la posición `(39, 99)`.
Si se hace clic en el botón, se producirá un salto llamado `goto`.

---

## Tipos de botones

| Descripciﾃｳn | Significado |
|---------------------------------|---------------------------------------------------|
| tipo: ninguna acción;                 | Imagen en la que no se puede hacer clic.                              |
| tipo: ir a;                     | Salta a una etiqueta cuando se hace clic.                    |
| tipo: interfaz gráfica de usuario;                      | Salta a una GUI cuando se hace clic.                      |
| tipo: mastervol;                | Se muestra como un control deslizante de volumen maestro.                  |
| tipo: bgmvol;                   | Se muestra como un control deslizante de volumen BGM.                     |
| tipo: sevol;                    | Se muestra como un control deslizante de volumen SE.                     |
| tipo: vozvol;                 | Se muestra como un control deslizante de volumen de voz.                  |
| tipo: personajevol;             | Se muestra como un control deslizante de volumen de caracteres.               |
| tipo: velocidad de texto;                | Se muestra como un control deslizante de velocidad del texto.                     |
| tipo: velocidad automática;                | Se muestra como un control deslizante de velocidad del modo automático.               |
| tipo: vista previa;                  | Se muestra como un área de vista previa de texto.                     |
| tipo: pantalla completa;               | Se muestra como un botón de modo de pantalla completa.               |
| tipo: ventana;                   | Se muestra como un botón de modo de ventana.                    |
| tipo: predeterminado;                  | Restablece la configuración cuando se presiona.                     |
| tipo: guardar página;                 | Se muestra como un botón para guardar/cargar página.                 |
| escriba: guardar;                     | Se muestra como una ranura para guardar.                             |
| tipo: carga;                     | Se muestra como una ranura de carga.                             |
| tipo: automático;                     | Se muestra como un botón de modo automático.                     |
| tipo: saltar;                     | Se muestra como un botón de modo de omisión.                      |
| tipo: título;                    | Se muestra como un botón para volver al título.                  |
| tipo: historia;                  | Se muestra como un botón de historial.                        |
| tipo: desplazamiento histórico;            | Se muestra como un control deslizante de desplazamiento vertical del historial.        |
| tipo: historial de desplazamiento horizontal; | Se muestra como un control deslizante de desplazamiento del historial horizontal.      |
| tipo: cancelar;                   | Se muestra como un botón de cancelar.                         |
| tipo: varnombre;                  | Se muestra como un área para obtener una vista previa del valor de una variable de nombre. |
| tipo: carbón;                     | Se muestra como un botón para ingresar un carácter.           |
| tipo: idioma | Cambie el idioma al hacer clic.                 |

### `noaction`

Una imagen en la que no se puede hacer clic.

```
background {
    type: noaction;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `goto`

Un botón en el que se puede hacer clic. Cuando se hace clic, la ejecución de la etiqueta salta a una etiqueta especificada por el parámetro `label:`.

```
button1 {
    type: goto;
    label: label_clicked_button1;
    x: 0;
    y: 0;
    image-idle:  idle.png;
    image-hover: hover.png;
}
```

### `gui`

Un botón en el que se puede hacer clic. Cuando se hace clic, la ejecución de la GUI se encadena a una nueva GUI especificada por el parámetro `file:`.

```
button1 {
    type: gui;
    label: gui2.gui;
    x: 0;
    y: 0;
    image-idle:  idle.png;
    image-hover: hover.png;
}
```

### `mastervol`

Un control deslizante para configurar el volumen principal.

```
slider1 {
    type: mastervol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `bgmvol`

Un control deslizante para configurar el volumen de BGM.

Este control deslizante establece el volumen que se almacenará en el archivo de datos de guardado global.
Esto es diferente al volumen que se establecerá mediante la etiqueta `[volume]` y se almacenará en los datos guardados locales.

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `sevol`

Un control deslizante para configurar el volumen SE.

Este control deslizante establece el volumen que se almacenará en el archivo de datos de guardado global.
Esto es diferente al volumen que se establecerá mediante la etiqueta `[volume]` y se almacenará en los datos guardados locales.

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `voicevol`

Un control deslizante para configurar el volumen SE.

Este control deslizante establece el volumen que se almacenará en el archivo de datos de guardado global.
Esto es diferente al volumen que se establecerá mediante la etiqueta `[volume]` y se almacenará en los datos guardados locales.

```
slider1 {
    type: bgmvol;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `charactervol`

Un control deslizante para establecer un volumen por carácter.

El índice de caracteres se pasa mediante el parámetro `index:`.
El índice 0 es para caracteres no definidos y del 1 al 32 para caracteres definidos.

```
slider1 {
    type: charactervol;
    index: 1;  # Character Index
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `textspeed`

Un control deslizante para establecer la velocidad del texto.

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `autospeed`

Un control deslizante para configurar la velocidad del modo automático.

```
slider1 {
    type: textspeed;
    x: 0;
    y: 0;
    image-idle:  idle.png;   # Slider base bar
    image-hover: hover.png;  # Slider base bar (bright)
    image-knob:  knob.png;   # Slider knob
}
```

### `preview`

Un área de texto para obtener una vista previa de la fuente, el idioma y la velocidad.

```
preview1 {
    type: preview;
    x: 0;
    y: 0;
    image-idle: idle.png;
}
```

### `fullscreen`

Un botón en el que se puede hacer clic.
Al hacer clic, el motor entrará en modo de pantalla completa.

```
fullscreen1 {
    type: fullscreen;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # For when in the full screen mode.
}
```

### `window`

Un botón en el que se puede hacer clic.
Al hacer clic, el motor entrará en modo de ventana.

```
window1 {
    type: window;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png;  # For when in the windowed mode.
}
```

### `default`

Un botón en el que se puede hacer clic.
Cuando se hace clic, restablece todas las configuraciones.

```
reset1 {
    type: default;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `savepage`

Un botón en el que se puede hacer clic.
Cuando se hace clic, cambia la página de la pantalla de guardar.

```
savepage1 {
    type: savepage;
    index: 0; # Page index (0-)
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-active:  active.png;  # For when the page is active.
}
```

### `save`

Un botón en el que se puede hacer clic.
Cuando se hace clic, ejecuta un guardado.

```
save1 {
    type: save;
    index: 0; # Index in apage. actual save slot = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/save/save-item-idle.png;
    image-hover: system/save/save-item-hover.png;

    index-x:   10;   # Number
    index-y:   0;
    date-x:    60;   # Date
    date-y:    0;
    thumb-x:   27;   # Thumbnail
    thumb-y:   77;
    chapter-x: 260;  # Chapter title
    chapter-y: 48;
    msg-x:     260;  # Last message
    msg-y:     96;
}
```

### `load`

Un botón en el que se puede hacer clic.
Al hacer clic, ejecuta una carga.

```
load1 {
    type: load;
    index: 0; # Index in apage. actual save slot = page * saveslots + index

    x: 0;
    y: 0;

    image-idle: system/load/load-item-idle.png;
    image-hover: system/load/load-item-hover.png;

    index-x:   10;   # Number
    index-y:   0;
    date-x:    60;   # Date
    date-y:    0;
    thumb-x:   27;   # Thumbnail
    thumb-y:   77;
    chapter-x: 260;  # Chapter title
    chapter-y: 48;
    msg-x:     260;  # Last message
    msg-y:     96;
}
```

### `auto`

Un botón en el que se puede hacer clic.
Cuando se hace clic, inicia el modo automático.

```
auto1 {
    type: auto;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `skip`

Un botón en el que se puede hacer clic.
Cuando se hace clic, inicia el modo de omisión.

```
skip1 {
    type: skip;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `title`

Un botón en el que se puede hacer clic.
Cuando se hace clic, el motor vuelve al archivo NovelML especificado.

```
title1 {
    type: title;
    file: title.novel;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `history`

Un botón en el que se puede hacer clic.
Muestra un elemento del historial de mensajes.

```
history {
    type: history;
    index: 0; # Index in a page
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;

    name-x: 20;   # Name
    name-y: 10;
    text-x:  20;  # Text (for when there is a name)
    text-y:  50;
    msg-x:  20;   # Message (for when no name)
    msg-y:  10;
}
```

### `historyscroll`

Una barra de desplazamiento vertical para la pantalla de historial.

```
scroll1 {
    type: historyscroll;
    x: 1200;
    y: 40;

    image-idle:  system/history/history-bar-idle.png;
    image-hover: system/history/history-bar-hover.png;
    image-knob:  system/history/history-bar-knob.png;
}
```

### `historyscroll-horizontal`

Una barra de desplazamiento horizontal para la pantalla del historial de escritura vertical.

```
scroll1 {
    type: historyscroll-horizontal;
    x: 40;
    y: 650;

    image-idle:  system/history/history-bar-idle.png;
    image-hover: system/history/history-bar-hover.png;
    image-knob:  system/history/history-bar-knob.png;
}
```

### `cancel`

Un botón en el que se puede hacer clic.
Al hacer clic, la GUI se cancelará.

```
cancel1 {
    type: cancel;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `var`

Un área de texto para mostrar un valor de variable.
Se utiliza para editar nombres.

```
var1 {
    type: var;
    variable: variable_name;
    x: 0;
    y: 0;
    image-idle:    idle.png;
}
```

### `char`

Un botón para agregar un carácter a una variable.
Se utiliza para editar nombres.

```
char1 {
    type: char;
    variable: variable_name; # Variable name.
    msg: A;                  # Text to append.
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
}
```

### `language`

Un botón para cambiar el idioma.

```
language_english1 {
    type: language;
    langu: en;
    x: 0;
    y: 0;
    image-idle:    idle.png;
    image-hover:   hover.png;
    image-disable: disable.png; # For when English is active.
}
```

---

## Opciones de botones comunes

| Descripciﾃｳn | Significado |
|-----------------------|-------------------------------------------------------------|
| tipo: | Tipo de botón.                                         |
| x: | Posición X del botón.                                   |
| y: | Posición Y del botón.                                   |
| ancho: | Ancho del botón. (de forma predeterminada, se establece en el ancho de la imagen inactiva) |
| altura: | Altura del botón.                                       |
| puntos: | Efecto de sonido para cuando se apunta el botón.                |
| haga clic en: | Efecto de sonido para cuando se hace clic en el botón.                |

### puntos:

`pointse: btn-change.ogg;` indica que se reproduce un archivo de efectos de sonido
atrás cuando el cursor del mouse ingresa al botón.

### haga clic en:

`clickse: click.ogg;` indica que se reproduce un archivo de efectos de sonido
cuando se hace clic en el botón.
