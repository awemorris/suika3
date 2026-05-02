Referencia de etiqueta Suika3
====================

## Índice

| Nombre de etiqueta | Descripciﾃｳn |
|-----------------------------|--------------------------------------------------------------------|
| [anime](#anime) | Carga y ejecuta un archivo de animación.                                  |
| [bg](#bg) | Cambia la imagen de fondo con un efecto de desvanecimiento.                 |
| [bgm](#bgm) | Reproduce un archivo de música de fondo (formato Ogg Vorbis).                 |
| [callmacro](#callmacro) | Llama a una macro definida.                                             |
| [ch](#ch) | Muestra u oculta caracteres con parámetros de capa detallados.          |
| [capítulo](#chapter) | Establece un nombre de capítulo.                                               |
| [choose](#choose) | Muestra opciones y almacena la selección o salta a una etiqueta.     |
| [click](#click) | Espera un clic del usuario.                                            |
| [config](#config) | Establece un valor de configuración para el sistema de juego.                    |
| [defmacro](#defmacro) | Inicia una definición de macro.                                         |
| [else](#else) | Parte de la rama if/elseif para cuando no se cumplen condiciones.       |
| [elseif](#elseif) | Especifica una condición adicional en una rama.                     |
| [endif](#endif) | Finaliza una rama condicional.                                         |
| [endmacro](#endmacro) | Finaliza una definición de macro.                                           |
| [goto](#goto) | Salta a una etiqueta de etiqueta especificada.                                    |
| [gui](#gui) | Muestra una GUI de un archivo específico.                                 |
| [si](#if) | Bifurca el proceso según una condición específica.               |
| [label](#label) | Define una etiqueta para los objetivos de salto.                                  |
| [layer](#layer) | Carga/descarga imágenes o establece parámetros para capas específicas.       |
| [load](#load) | Carga un archivo NovelML y puede saltar a una etiqueta específica.             |
| [move](#move) | Anima capas de personajes durante un tiempo específico.                   |
| [lápiz](#pencil) | Dibuja un texto en una capa.                                            |
| [returnmacro](#returnmacro) | Devuelve una ejecución de macro.                                    |
| [se](#se) | Reproduce un archivo de efectos de sonido (formato Ogg Vorbis).                     |
| [establecer](#set) | Establece un valor de variable (todas las variables se tratan como texto).         |
| [skip](#skip) | Activa o desactiva el estado de omisión.                               |
| [texto](#text) | Muestra texto en el cuadro de mensaje, opcionalmente con un nombre.          |
| [vídeo](#video) | Reproduce un archivo de vídeo (admite configuraciones que se pueden omitir).                  |
| [volumen](#volume) | Establece el volumen del sonido para las pistas BGM, SE o Voice.                |
| [espera](#wait) | Espera un número específico de segundos.                           |

---

## `anime`

Ejecutar animación

La etiqueta `anime` carga y ejecuta una definición de animación desde un archivo. 
Permite efectos visuales complejos, movimientos de personajes o animaciones ambientales en bucle más allá de simples transiciones.

### Uso básico

```
# Run a synchronous animation (waits for completion)
[anime file="opening_effect.txt"]

# Run an asynchronous looping animation
[anime file="sparkle.txt" async="true" register="my_loop"]

# Stop a registered asynchronous animation
[anime stop="true" register="my_loop"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|---------------|----------------|----------------------------------------------------|-------------------------------------------------------------------|
| `file` | Yes | El nombre de archivo de la definición de animación.          | *Obligatorio a menos que se utilice `stop="true"`.                           |
| `async` | Sí (`false`) | Si se debe ejecutar la animación de forma asincrónica.       | Si es `false`, el script espera hasta que finalice la animación.        |
| `register` | Yes | Un nombre único para identificar esta instancia de animación. | Requerido para controlar o detener animaciones asíncronas más adelante.      |
| `stop` | Sí (`false`) | Detiene una animación registrada si se establece en `true`.     | Requiere el argumento `register`.                                 |
| `showsysbtn` | Sí (`true`) | Si se muestran los botones del sistema durante la reproducción.    | Sólo válido para animaciones sincrónicas.                            |
| `showmsgbox` | Sí (`true`) | Si se debe mostrar el cuadro de mensaje durante la reproducción.   | Sólo válido para animaciones sincrónicas.                            |
| `shownamebox` | Sí (`true`) | Si se muestra el cuadro de nombre durante la reproducción.      | Sólo válido para animaciones sincrónicas.                            |

### Consejos

**Síncrono versus asíncrono**:
* **Sincrónico (`async="false"`)**: Ideal para escenas en las que desea que el jugador vea la animación antes de que aparezca cualquier texto u opciones.
* **Asincrónico (`async="true"`)**: Perfecto para efectos de fondo (como nieve que cae o una luz parpadeante) que deben continuar mientras avanza la historia.

**Gestión de instancias**:
* Al utilizar el argumento `register`, puedes etiquetar una animación específica.
* Así es como le dices al motor exactamente qué animación detener cuando usas `stop="true"`.

**Control de interfaz de usuario**:
* Utilice `showmsgbox="false"` si su animación debe ocupar toda la pantalla y desea que la ventana de diálogo desaparezca temporalmente para una apariencia más limpia.

---

## `bg`

Cambiar fondo

La etiqueta `bg` cambia la imagen de fondo con un efecto de desvanecimiento suave.
Es la forma principal de preparar el escenario de tu novela visual.

### Uso básico

```
# Transition to background.png over 1.0 second
[bg file="background.png" time="1.0"]

# Fade to a black screen (removes the background)
[bg file="none" time="1.0"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|------------|----------------|-----------------------------------------------|------------------------------------------------------------------------------|
| `file` | No | El nombre de archivo de la nueva imagen de fondo.     | Establezca en `none` para eliminar el fondo.                                      |
| `time` | Sí (`0`) | La duración del efecto de desvanecimiento en segundos. | El valor predeterminado es `0.0` (cambio instantáneo).                                           |
| `method` | Sí (`normal`) | El método/estilo de desvanecimiento.                      | Elija entre `normal`, `rule` o `melt`.                                     |
| `rule` | Yes | El archivo de imagen de regla para transiciones específicas. | Requerido cuando `method` está configurado en `rule` o `melt`.                           |
| `x` | Sí (`0`) | El desplazamiento del eje X para la imagen de fondo.   | Admite valores absolutos (p. ej., `100`) o valores relativos (p. ej., `r100`).    |
| `y` | Sí (`0`) | El desplazamiento del eje Y de la imagen de fondo.   | Admite valores absolutos (p. ej., `100`) o valores relativos (p. ej., `r-100`).   |
| `alpha` | Sí (`255`) | El valor alfa de la imagen de fondo.      | `0` a `255`.                                                                |
| `clear` | Sí (`false`) | Si desaparecer los personajes o no.      | Si es `true`, todos los caracteres desaparecerán.                                  |

### Métodos de transición (`method`)

Puedes crear diferentes atmósferas eligiendo el estilo de transición adecuado:

| Tipo | Descripciﾃｳn |
|----------|--------------------------------------------------------------------------------------------------------------------------------------|
| `normal` | Mezcla alfa. El método predeterminado. Realiza un fundido cruzado suave entre las imágenes antiguas y nuevas.                                     |
| `rule` | Transición universal de 1 bit. Utiliza una imagen de "regla" en escala de grises para determinar el orden de cambio.                                          |
| `melt` | Transición universal de 8 bits. Similar a `rule`, pero con bordes suaves y borrosos en el límite de transición, creando un efecto de "fusión". |

Para `rule` y `melt`, la imagen cambia píxel a píxel desde las áreas más oscuras a las más claras del mapa de reglas.

### Consejos

**Posicionamiento relativo**: 
* Si desea mover el fondo desde su posición actual, use el prefijo `r`.
* Por ejemplo, `x="r50"` mueve la imagen 50 píxeles a la derecha de su coordenada X actual.

**¿Qué es una imagen de regla?**:
* Es una imagen en escala de grises donde las áreas negras pasan primero y las áreas blancas pasan al final.
* Al crear imágenes de reglas personalizadas, puedes lograr efectos como barridos horizontales, revelaciones circulares o incluso patrones más artísticos.

---

## `bgm`

Reproducir música de fondo

La etiqueta `bgm` reproduce una pista de música de fondo. 
La música es una herramienta esencial para establecer el ambiente de su escena y continuará reproduciéndose automáticamente hasta que se detenga o cambie.

### Uso básico

```
# Start playing a BGM track
[bgm file="field_theme.ogg"]

# Stop the current BGM (use "none")
[bgm file="none"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|---------------|------------------------------------|----------------------------------------|
| `file` | No | El nombre del archivo de la música a reproducir. | Configúrelo en `none` para detener la BGM actual. |
| `once` | Sí (`false`) | No hagas bucles.                        |                                        |

### Consejos

**Formato requerido**:
* Por compatibilidad y rendimiento, Suika3 requiere que los archivos BGM estén en formato **Ogg Vorbis**.
* La frecuencia de muestreo DEBE ser **44.100 Hz**.

**Bucle**:
* La música de fondo está diseñada para reproducirse en bucle de forma predeterminada, por lo que no necesita preocuparse de que la música termine abruptamente durante una escena de diálogo larga.

**Transiciones suaves**:
* Si llamas a `[bgm]` mientras ya se está reproduciendo otra pista, el motor normalmente manejará la transición. 
* Para ajustar el volumen de la música, querrás usar la etiqueta `[volume]`.

**Detener música**:
* Cuando una escena termina o el ambiente cambia a silencio, ¡recuerda usar `[bgm file="none"]` para darle un descanso a los oídos del jugador!

---

## `callmacro`

Llamar macro

La etiqueta `callmacro` ejecuta una macro previamente definida.
Le permite activar una secuencia específica de comandos, como entradas de personajes o animaciones de la interfaz de usuario, varias veces a lo largo de su secuencia de comandos sin tener que reescribir el código original.

### Uso básico

```
# Call a macro named "kaito_entrance"
[callmacro name="kaito_entrance"]

# Call a macro for a specific scene transition
[callmacro name="fade_to_white"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|-------------------------------------------|----------------------------------------------------|
| `name` | No | El nombre de la macro a ejecutar.         | Debe coincidir con un nombre definido por una etiqueta `[defmacro]`.   |
| `file` | Yes | El nombre del archivo donde se define la macro. | Omita esto para llamar a una macro dentro del archivo actual. |

### Consejos

**Eficiencia**:
* Al usar `[callmacro]`, puedes mantener el guión de tu historia principal enfocado y legible.
* En lugar de ver 10 líneas de código de animación, solo verás un comando claro.

**Flujo de ejecución**:
* Cuando el motor llega a `[callmacro]`, salta inmediatamente a la macro definida, ejecuta todas las etiquetas dentro de ella y luego regresa automáticamente a la siguiente línea después de la etiqueta `[callmacro]`.

**Diseño modular**:
* Piensa en las macros como "etiquetas personalizadas" para tu juego.
* Si decides cambiar la forma en que un personaje ingresa a una escena, solo necesitas actualizar el código una vez en el bloque `[defmacro]`, ¡y cada `[callmacro]` reflejará ese cambio!

---

## `ch`

Visualización de caracteres

La etiqueta `ch` muestra, oculta o actualiza imágenes de personajes en varias capas.
Permite un control detallado sobre el posicionamiento, la escala y las rotaciones de varios personajes y fondos a la vez.

### Uso básico

```
# Show a character at the center
[ch center="chara001.png" time="1.0"]

# Show multiple characters with specific positions
[ch left="chara002.png" right="chara003.png" time="0.5"]

# Hide a specific character
[ch center="none" time="1.0"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|-----------|----------------|----------------------------------------|-------------------------------------------------------|
| `time` | Sí (`0`) | Duración de la transición en segundos. | Afecta a todos los cambios de capa dentro de esta etiqueta.            |
| `method` | Sí (`normal`) | El método/estilo de desvanecimiento.               | `normal`, `rule` o `melt`.                          |
| `rule` | Yes | El archivo de imagen de regla para transiciones.   | Requerido cuando `method` es `rule` o `melt`.           |

#### Argumentos del archivo de capa

Especifique un nombre de archivo para cargar una imagen en una capa. Establezca en `none` para descargar (ocultar) la imagen.

| Argumento | Descripciﾃｳn |
|----------------|-------------------------------------------|
| `bg` | Capa de fondo.                         |
| `volver | Carácter del centro trasero.                    |
| `left` | Carácter izquierdo.                           |
| `right` | Carácter correcto.                          |
| `center` | Carácter central.                         |
| `left-center` | Carácter del centro izquierdo.                    |
| `right-center` | Carácter intermedio.                   |
| `face` | Carácter facial.                           |

#### Argumentos de parámetros de capa

Cada capa superior (por ejemplo, `center`) se puede personalizar utilizando los siguientes sufijos (por ejemplo, `center-x`, `center-rotate`).

| Sufijo | Omisible | Descripciﾃｳn | Notas |
|-------------|---------------|----------------------------|---------------------------------------------------------------|
| `-x` | Sí (`0`) | Posición X.                | Admite absoluto (p. ej., `100`) o relativo (p. ej., `r50`).    |
| `-y` | Sí (`0`) | Posición Y.                | Admite absoluto (p. ej., `100`) o relativo (p. ej., `r-50`).   |
| `-a` | Sí (`255`) | Valor alfa. (opacidad) | `0` (transparente) a `255` (opaco).                          |
| `-scale-x` | Sí (`1.0`) | factor de escala X.          | `1.0` es el tamaño original. Admite el prefijo `r`.                  |
| `-scale-y` | Sí (`1.0`) | Factor de escala Y.          | `1.0` es el tamaño original. Admite el prefijo `r`.                  |
| `-center-x` | Sí (`0`) | Centro X para rotación.     | Punto de pivote para el efecto de rotación.                          |
| `-center-y` | Sí (`0`) | Centro Y para rotación.     | Punto de pivote para el efecto de rotación.                          |
| `-rotate` | Sí (`0`) | Rotación en grados.       | Positivo para el sentido de las agujas del reloj. Admite el prefijo `r`.                  |
| `-dim` | Sí (`false`) | Estado de atenuación.            | Si es `true`, la capa se oscurece un 50%.                  |

### Consejos

**Actualizaciones por lotes**:
* Puedes actualizar varios personajes y el fondo simultáneamente en una sola etiqueta `[ch]` para asegurar que se animen juntos perfectamente.

**Transformación relativa**:
* Al igual que la etiqueta `bg`, todos los parámetros numéricos admiten el prefijo `r`.
* Por ejemplo, `center-y="r-50"` saltará el carácter central 50 píxeles hacia arriba desde su posición actual.

---

## `chapter`

Establecer nombre del capítulo

La etiqueta `chapter` establece el nombre del capítulo actual. 
El sistema de juego suele utilizar este nombre para mostrar el progreso en el menú guardar/cargar o en la pantalla del juego, lo que ayuda a los jugadores a realizar un seguimiento de su viaje.

### Uso básico

```
# Set the chapter name at the beginning of a story segment
[chapter name="Chapter 01: The Beginning"]

# Update the chapter name as the story progresses
[chapter name="Intermission: A Quiet Night"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|------------------------------------|------------------------------------------------------------|
| `name` | No | El nombre del capítulo que se establecerá. | Esta cadena se almacenará en las variables del sistema del juego. |

### Consejos

**Guardar visibilidad de datos**:
* En muchas configuraciones de Suika3, la cadena que configuras aquí es la que aparece en las ranuras "Guardar" y "Cargar".
* ¡Elige un nombre que ayude al jugador a recordar exactamente dónde se encontraba en la historia!

**Consistencia**:
* Es una buena práctica llamar a la etiqueta `[chapter]` inmediatamente después de un `[label]` que inicia una nueva escena o capítulo importante.

**Actualización de nombres**:
* Puedes llamar a `[chapter]` tantas veces como quieras.
* Cada vez que lo hace, el nombre del capítulo anterior se sobrescribe con el nuevo.

---

## `choose`

Opciones de selección de pantalla

La etiqueta `choose` muestra hasta 8 botones interactivos para el jugador. 
Almacena el texto del elemento elegido en una variable.

### Uso básico

```
# Store selection in a variable
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

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|------------------|-----------|------------------------------------------------|--------------------------------------------------- |
| `text1` | Yes | El texto que se muestra en cada botón.             | Normalmente se requiere al menos una opción.        |
| `text2` | Yes | El texto que se muestra en cada botón.             | Normalmente se requiere al menos una opción.        |
| `text3` | Yes | El texto que se muestra en cada botón.             | Normalmente se requiere al menos una opción.        |
| `text4` | Yes | El texto que se muestra en cada botón.             | Normalmente se requiere al menos una opción.        |
| `text5` | Yes | El texto que se muestra en cada botón.             | Normalmente se requiere al menos una opción.        |
| `text6` | Yes | El texto que se muestra en cada botón.             | Normalmente se requiere al menos una opción.        |
| `text7` | Yes | El texto que se muestra en cada botón.             | Normalmente se requiere al menos una opción.        |
| `text8` | Yes | El texto que se muestra en cada botón.             | Normalmente se requiere al menos una opción.        |
| `text<N>-locale` | Yes | El texto que se muestra en cada botón. (localizado) | Normalmente se requiere al menos una opción.        |
| `name` | No | El nombre de la variable para almacenar el resultado.         | Almacena el texto de la opción seleccionada.            |
| `value1` | Yes | El valor asignado a la variable de resultado.     | Normalmente se requiere al menos una opción.        |
| `value2` | Yes | El valor asignado a la variable de resultado.     | Normalmente se requiere al menos una opción.        |
| `value3` | Yes | El valor asignado a la variable de resultado.     | Normalmente se requiere al menos una opción.        |
| `value4` | Yes | El valor asignado a la variable de resultado.     | Normalmente se requiere al menos una opción.        |
| `value5` | Yes | El valor asignado a la variable de resultado.     | Normalmente se requiere al menos una opción.        |
| `value6` | Yes | El valor asignado a la variable de resultado.     | Normalmente se requiere al menos una opción.        |
| `value7` | Yes | El valor asignado a la variable de resultado.     | Normalmente se requiere al menos una opción.        |
| `value8` | Yes | El valor asignado a la variable de resultado.     | Normalmente se requiere al menos una opción.        |
| `time` | Sí (`0`) | Temporizador en segundos.                              | Si `0`, no hay ningún temporizador habilitado.                       |

### Localización

Por ejemplo, si el entorno del sistema operativo del usuario está configurado en japonés, se prefiere `text1-ja` en lugar de `text1`.

| Sufijo | Idioma |
|-------------|------------------------------------------|
| -en | Inglés (alternativa) |
| -en-us | Inglés (América) |
| -en-gb | Inglés (británico) |
| -en-au | Inglés (Austraria) |
| -en-nz | Inglés (Nueva Zelanda) |
| -fr | Francés (alternativo) |
| -fr-fr | Francés (Francia) |
| -fr-ca | Francés (Canadá) |
| -es | Español (España, Reserva) |
| -es-la | Español (Latinoamérica) |
| -de | German |
| -it | Italian |
| -ru | Russian |
| -el | Greek |
| -zh | Chino (simplificado) |
| -zh-tw | Chino (tradicional, Taiwán) |
| -ja | Japanese |
| (sin sufijo) | Respaldo (el desarrollador decide) |

Para las configuraciones regionales del sistema operativo en inglés, incluidas todas las regiones, `-en` se utiliza como
respaldo predeterminado.  Si una variante más específica como `-en-gb` es
especificado en una etiqueta y que mejor coincida con la región del usuario, será
preferido. El mismo mecanismo se aplica al español y al francés. Nota
que no hay alternativa del chino tradicional al simplificado
Chino.

Por ejemplo, si la configuración regional del usuario es `en-AU`, se aplica la siguiente prioridad:
* 1. texto1-en-au
* 2. texto1-es
* 3. texto1

Los siguientes no son actualmente compatibles, pero está previsto que lo sean.

| Sufijo | Idioma |
|-------------|------------------------------------------|
| -ko | Korean |
| -vi | Vietnamese |
| -id | Indonesia |
| -zh-hk | Chino tradicional (Hong Kong) |
| -pt | Portugués (alternativo) |
| -pt-br | Portugués (Brasil) |
| -pl | Polish |
| -tr | Turkish |
| -ta | Tamil |
| -te | Telugu |
| -kn | Kannada |
| -si | Sinhala |
| -ar | Árabe (RTL) |
| -fa | Persa (RTL) |

### Consejos

**Lógica de ramificación**:
* Puede usar la etiqueta `[if]` para verificar el valor almacenado y crear ramas complejas.

```
[choose
  name="var1"
  text1="Go to school"
  text2="Go to hospital"
  value1="1"
  value2="2"]

[if lhs="${var1}" op="=" rhs="1"]
  # School.
[else]
  # Hospital.
[endif]
```

**Persistencia variable**:
* Como todo es una cadena, recuerda que incluso los números como "100" se almacenan como texto.
* Las etiquetas lógicas de Suika3 (como `if`) pueden manejar estas cadenas para realizar comparaciones.

---

## `set`

Establecer variable

La etiqueta `set` asigna un valor a un nombre de variable. 
En Suika3, **todas las variables se tratan como cadenas de texto**, pero se pueden comparar numéricamente en otras etiquetas como `[if]`.

### Uso básico

```
# Assign a simple string to a variable
[set name="player_name" value="Kaito"]

# Set a numeric-like value (stored as a string)
[set name="health" value="100"]

# Clear a variable by setting it to an empty string
[set name="flag_event_01" value=""]

# Add 1 to var1.
[set name="var1" value1="${var1}" op="+" value2="1"]

```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|---------------|---------------------------------------------|---------------------------------------------------------------------|
| `name` | No | El nombre único de la variable.            | Utilice caracteres alfanuméricos y guiones bajos para una mejor compatibilidad. |
| `value` | Yes | El contenido a almacenar en la variable.       | Recuerde: ¡todo se almacena como una cadena!                         |
| `value1` | Yes | El operando 1 para el código de operación.                   |                                                                     |
| `value2` | Yes | El operando 2 para el código de operación.                   |                                                                     |
| `op` | Yes | El código de operación. (`+`, `-`, `*`, `/`, `//`, `%`) |                                                                     |
| `global` | Sí (`false`) | Haz que la bandera sea global.                       | Las variables globales son para indicadores de logros, por ejemplo, "Saw ED1".         |

### Consejos

**Manejo de cadenas**:
* Dado que Suika3 trata todo como texto, `value="100"` y `value="May"` se manejan de la misma manera internamente.
* Puede hacer referencia a estas variables en otras etiquetas (como `text` o `if`) usando la sintaxis `${variable_name}`.

**Gestión de banderas**:
* Para indicadores de juego (como "ha conocido al héroe"), es una práctica común usar `"true"` y `"false"` o `"1"` y `"0"`. 
* ¡La consistencia es clave! Si comienza a usar `"1"`, continúe con él para que sus comprobaciones de `[if]` no se confundan.

**Nombres de variables**:
* Evite el uso de espacios o símbolos especiales en los nombres de sus variables. `my_variable` es mucho más seguro que `my variable!`.

---

## `click`

Espere el clic

La etiqueta `click` pausa la ejecución del script y espera a que el jugador haga clic con el mouse o presione una tecla.
Se utiliza comúnmente para crear pausas entre cambios visuales o antes de una transición importante.

### Uso básico

```
# Change the background, then wait for the player to click
[bg file="sunset.png" time="1.0"]
[click]

# After the click, show the character
[ch center="chara01.png" time="1.0"]
```

### Argumentos

Esta etiqueta no acepta ningún argumento.

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|-------------|-----------------|
| -        | -         | -           | -               |

### Consejos

**Tiempo y ritmo**:
* Usa `[click]` cuando quieras darle al jugador un momento para mirar un nuevo fondo o una expresión de personaje específica antes de que continúe el diálogo.
* A diferencia de la etiqueta `[text]`, que espera un clic automáticamente después de mostrar un mensaje, `[click]` se utiliza para el control de flujo manual durante secuencias sin diálogo.

**Comentarios visuales**:
* Cuando el script llegue a una etiqueta `[click]`, el juego permanecerá quieto. Asegúrese de que cualquier animación anterior (como `[bg]` o `[ch]`) tenga un `time` configurado, o la pantalla podría sentirse estática de manera demasiado abrupta.

**Para esperas cronometradas:**
* Utilice `[wait]` para esperas cronometradas.

---

## `goto`

Saltar a la etiqueta

La etiqueta `goto` mueve inmediatamente la ejecución de NovelML a una etiqueta especificada. 
Es una herramienta útil para controlar el flujo de su historia, permitiéndole saltar secciones o volver a partes anteriores.

Tenga en cuenta que las sucursales pequeñas deben realizarse antes del `[if]`.

### Uso básico

```
# Jump to the beginning of the morning scene
[goto name="morning_start"]

# ... this part of the script will be skipped ...

[label name="morning_start"]
[text text="The sun rises over the horizon."]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|-----------------------------------|-----------------------------------------------|
| `name` | No | El nombre de la etiqueta de destino a la que saltar. | Debe coincidir con un nombre definido por una etiqueta `[label]`. |

### Consejos

**Salto incondicional**:
* A diferencia de `[if]`, `[goto]` siempre salta a la etiqueta de destino tan pronto como el motor llega a la etiqueta.

**Gestión de flujo**:
* Utilice `[goto]` al final de una ruta de bifurcación para devolver la historia a una ruta "común". 
* También es excelente para crear bucles (como una secuencia de "Volver al título") cuando se combina con otra lógica.

**¿Entre archivos?**:
* Recuerde que `[goto]` normalmente funciona dentro del archivo de script actual.
* Si desea saltar a un archivo completamente diferente, querrá mirar la etiqueta `[load]`.

---

## `defmacro`

Definir macro

La etiqueta `defmacro` inicia la definición de una macro. 
Una macro le permite agrupar varias etiquetas y comandos en un único bloque con nombre, que se puede reutilizar en todo el script utilizando la etiqueta `[callmacro]`.

### Uso básico

```
# Define a macro for a specific character's entrance
[defmacro name="enter_kaito"]
    [ch center="kaito_smile.png" time="0.5"]
    [text name="Kaito" text="Hey! Did I keep you waiting?"]
[endmacro]

# Later in your script, call it with a single line
[callmacro name="enter_kaito"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|---------------------------------|---------------------------------------------------|
| `name` | No | El nombre único de esta macro. | Se utiliza para identificar la macro cuando se llama más tarde. |

### Consejos

**Cerrando la definición**:
* Cada `[defmacro]` debe combinarse con una etiqueta `[endmacro]` para marcar el final de la definición.

**Reutilización del código**:
* Las macros son perfectas para secuencias repetitivas, como transiciones de interfaz de usuario específicas, configuraciones visuales específicas de personajes o combinaciones complejas de sonido y imágenes.

**Organización**:
* Es una práctica común definir todas sus macros al principio de su archivo de script principal o en un archivo separado que carga al principio.

**Anidamiento y Lógica**:
* Puede incluir casi cualquier otra etiqueta dentro de una macro, incluidas declaraciones `[if]` e incluso `[returnmacro]` para salir de la macro antes de tiempo según ciertas condiciones.

---

## `gui`

Mostrar interfaz gráfica de usuario

La etiqueta `gui` carga y muestra una definición de interfaz gráfica de usuario (GUI) de un archivo específico. 
Se utiliza para mostrar menús, pantallas de título o paneles de interacción personalizados.

### Uso básico

```
# Display the main menu GUI
[gui file="main_menu.txt"]

# Show a custom save/load screen
[gui file="save_screen.txt"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|---------------------------------------------|-----------------------------------------------------|
| `file` | No | El nombre de archivo de la definición de GUI que se va a cargar. | El archivo debe existir en el directorio GUI del proyecto. |

### Consejos

**Definiciones de GUI**:
* El argumento `file` apunta a un archivo de texto que define el diseño, los botones y las acciones de su interfaz.
* Estos archivos especifican dónde se colocan las imágenes y qué sucede (como saltar a una etiqueta o salir) cuando un usuario interactúa con ellas.

**Uso en flujo**:
* Normalmente, se utiliza una etiqueta `[gui]` para un menú gráfico como la pantalla de título.

**Personalización**:
* Dado que la GUI está definida en un archivo externo, puedes crear múltiples estilos para tu juego y cambiar entre ellos simplemente llamando a diferentes archivos con esta etiqueta.

---

## `label`

Definir etiqueta

La etiqueta `label` define un punto específico en el script al que se pueden dirigir comandos de salto como `[goto]` o `[load]`.
Actúa como un marcador para la navegación dentro de su historia.

### Uso básico

```
# Define a label for the start of a new chapter
[label name="chapter_01_start"]

# Use a jump command to reach this label
[goto name="chapter_01_start"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|---------------------------------|--------------------------------------------------------|
| `name` | No | El nombre exclusivo de esta etiqueta. | Distingue mayúsculas y minúsculas. Evite el uso de espacios o símbolos especiales. |

### Consejos

**Control de navegación**:
* Las etiquetas son útiles para crear rutas de ramificación.
* Por ejemplo, puedes poner un `label` al principio de la sección de tu historia para un salto de longitud.

**Nombre único**:
* Cada nombre de etiqueta dentro de un único archivo de script debe ser único.
* Si tienes dos etiquetas con el mismo nombre, es posible que el motor no sepa dónde saltar, ¡y eso no es divertido para nadie!

**Organización**:
* Es un buen hábito utilizar nombres descriptivos como `label_evening_park` en lugar de `label1`.
* Hace que sea mucho más fácil para ti (¡y para mí!) leer el guión más tarde y comprender lo que está sucediendo.

---

## `text`

Mostrar texto

La etiqueta `text` muestra un mensaje en el cuadro de mensaje. 
Puede mostrar el diálogo o la narración principal y, opcionalmente, mostrar el nombre de un personaje en el cuadro de nombre.

### Uso básico

```
# Narration style (no name displayed)
[text text="The wind was howling through the trees."]

# Character dialogue
[text name="Keith" text="I've been waiting for you here in the small room."]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|------------------|-----------|--------------------------------------------------|--------------------------------------------------|
| `text` | No | El contenido del mensaje que se mostrará.             |                                                  |
| `text-<locale>` | Yes | El contenido del mensaje que se mostrará. (localizado) |                                                  |
| `voice` | Yes | El archivo de voz.                                  |                                                  |
| `voice-<locale>` | Yes | El archivo de voz. (localizado) |                                                  |
| `name` | Yes | El nombre del personaje que se mostrará en el cuadro de nombre. | Si se omite, el cuadro de nombre normalmente estará oculto. |
| `action` | Yes | Para modo NVL y mostrar/ocultar manualmente.               |                                                  |
| `space` | Yes | Para modo NVL.                                    |                                                  |

### Localización

Por ejemplo, si el entorno del sistema operativo del usuario está configurado en japonés, se prefiere `text-ja` en lugar de `text`.

| Sufijo | Idioma |
|-------------|------------------------------------------|
| -en | Inglés (alternativa) |
| -en-us | Inglés (América) |
| -en-gb | Inglés (británico) |
| -en-au | Inglés (Austraria) |
| -en-nz | Inglés (Nueva Zelanda) |
| -fr | Francés (alternativo) |
| -fr-fr | Francés (Francia) |
| -fr-ca | Francés (Canadá) |
| -es | Español (España, Reserva) |
| -es-es | Español (España, Reserva) |
| -es-la | Español (Latinoamérica) |
| -de | German |
| -it | Italian |
| -ru | Russian |
| -el | Greek |
| -zh-cn | Chino (simplificado) |
| -zh-tw | Chino (tradicional, Taiwán) |
| -ja | Japanese |
| (sin sufijo) | Respaldo (el desarrollador decide) |

Para las configuraciones regionales del sistema operativo en inglés, incluidas todas las regiones, `-en` se utiliza como
respaldo predeterminado.  Si una variante más específica como `-en-gb` es
especificado en una etiqueta y que mejor coincida con la región del usuario, será
preferido. El mismo mecanismo se aplica al español y al francés. Nota
que no hay alternativa del chino tradicional al simplificado
Chino.

Por ejemplo, si la configuración regional del usuario es `en-GB`, se aplica la siguiente prioridad:
* 1. texto-es-es
* 2. texto-en
* 3. texto

Los siguientes no son actualmente compatibles, pero está previsto que lo sean.

| Sufijo | Idioma |
|-------------|------------------------------------------|
| -ko | Korean |
| -vi | Vietnamese |
| -id | Indonesia |
| -zh-hk | Chino tradicional (Hong Kong) |
| -pt | Portugués (alternativo) |
| -pt-br | Portugués (Brasil) |
| -pl | Polish |
| -tr | Turkish |
| -ta | Tamil |
| -te | Telugu |
| -kn | Kannada |
| -si | Sinhala |
| -ar | Árabe (RTL) |
| -fa | Persa (RTL) |

### Acciones

Puede utilizar parámetros especiales en la etiqueta `text`.

```
# Clear the message box.
[text action="clear"]

# Clear the message box and show it.
[text action="new"]

# Show the message box.
[text action="show"]

# Hide the message box.
[text action="hide"]
```

### Modo NVL

Puede ingresar al modo NVL configurando alguna configuración.

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

Puede volver al modo ADV restableciendo la configuración.

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

En el modo NVM, puedes controlar mensajes de texto como este:

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

### Voz

Si el idioma actual es `en-us`, un archivo de voz se resolverá en el siguiente orden:

1. parámetro `voice-en-us`
2. Parámetro `voice/en-us/` + `voice`
3. parámetro `voice-en`
4. Parámetro `voice/en/` + `voice`
5. Parámetro `voice`

Si el idioma actual es `ja`, un archivo de voz se resolverá en el siguiente orden:

1. parámetro `voice-ja`
2. Parámetro `voice/ja/` + `voice`
3. parámetro `voice`

### Consejos

**Espera automática**:
* A diferencia de otras etiquetas, la etiqueta `text` espera automáticamente el clic del jugador después de que el mensaje se muestra por completo.
* ¡No es necesario agregar una etiqueta `[click]` después de cada línea de diálogo!

**Usando variables**:
* Puedes incluir variables dentro de tu texto usando la sintaxis `${variable_name}`. 
* Ejemplo: `[text text="Hello, ${player_name}!"]` saludará al jugador usando cualquier nombre almacenado en esa variable.

**Saltos de línea**:
* Verifique la configuración de su proyecto para saber cuánto tiempo puede tener una sola línea.
* Si su texto es demasiado largo, podría desbordar el cuadro de mensaje, ¡así que esté atento a la longitud de su argumento `text`!

---

## `if`

Ramificación condicional

La etiqueta `if` permite que NovelML se bifurque según una condición específica. 
Al comparar variables o valores, puedes crear historias únicas o reaccionar a elecciones anteriores de los jugadores.

### Uso básico

```
# Check if a variable equals a certain value
[if lhs="${points}" op="==" rhs="100"]
    [text text="Perfect score! You're amazing."]
[elseif lhs="${points}" op=">=" rhs="80"]
    [text text="Great job! You passed."]
[else]
    [text text="Better luck next time."]
[endif]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|----------------------------------------|------------------------------------------------|
| `lhs` | No | El lado izquierdo de la condición.   | Generalmente una variable como `${var_name}`.         |
| `op` | No | El operador utilizado para la comparación.      | Consulte la tabla "Operadores" a continuación.               |
| `rhs` | No | El lado derecho de la condición.  | El valor o variable con el que comparar.      |

### Operadores de comparación (`op`)

Puede utilizar estos operadores para definir cómo se comparan los dos lados:

| Operador | Descripciﾃｳn |
|------------|----------------------------|
| `===` | Igual (cadena) |
| `==` | Igual (Numérico) |
| `>` | Mayor que (numérico) |
| `>=` | Mayor o igual (numérico) |
| `<` | Menor que (numérico) |
| `<=` | Menor o igual (numérico) |

### Consejos

**Cerrando el bloque**:
* Cada bloque `[if]` DEBE terminar con una etiqueta `[endif]`.
* ¡Si lo olvida, el motor podría confundirse acerca de dónde termina la condición!

**Sintaxis variable**:
* Cuando utilice una variable como `lhs`, siempre envuélvala en `${}`.
* Por ejemplo, utilice `lhs="${flag_01}"` en lugar de solo `lhs="flag_01"`.

**Manejo de cadenas frente a números**:
* Suika3 trata los valores de las variables como cadenas, pero estos operadores le permiten realizar comparaciones de estilo numérico.
* Simplemente sea coherente con sus valores (por ejemplo, utilice "1" para verdadero y "0" para falso).

**Múltiples sucursales**:
* Puede usar tantas etiquetas `[elseif]` como necesite entre `[if]` y `[endif]` para verificar múltiples condiciones específicas.

---

## `elseif`

Ramificación condicional adicional

La etiqueta `elseif` especifica una condición adicional dentro de un bloque `[if]`. 
Solo se evalúa si las condiciones `[if]` anteriores y cualquier condición `[elseif]` anterior eran falsas.

### Uso básico

```
[if lhs="${rank}" op="==" rhs="A"]
    [text text="Excellent! You're a pro."]
[elseif lhs="${rank}" op="==" rhs="B"]
    [text text="Good job! Keep it up."]
[elseif lhs="${rank}" op="==" rhs="C"]
    [text text="Not bad, but you can do better."]
[else]
    [text text="Don't give up! Try again."]
[endif]
```

### Argumentos

Igual que `[if]`. Véase también [if](#if).

### Consejos

**Evaluación secuencial**:
* El motor comprueba las condiciones de arriba a abajo.
* Tan pronto como se cumple una condición `[if]` o `[elseif]`, su bloque se ejecuta y el resto de la rama (incluidos otros `[elseif]` y `[else]`) se omite.

**Ubicación**:
* `[elseif]` siempre debe colocarse entre una etiqueta `[if]` y una etiqueta `[else]` o `[endif]`.
* ¡Puedes usar tantas etiquetas `[elseif]` como necesites para cubrir todas tus bases!

**Eficiencia**:
* Si tiene muchas condiciones que verifican la misma variable, usar múltiples etiquetas `[elseif]` es mucho más limpio y eficiente que anidar múltiples bloques `[if]` uno dentro de otro.

---

## `else`

Rama condicional predeterminada

La etiqueta `else` define un bloque de código que se ejecutará si no se cumple ninguna de las condiciones `[if]` o `[elseif]` anteriores. 
Actúa como la ruta "predeterminada" para su lógica de ramificación.

### Uso básico

```
[if lhs="${weather}" op="==" rhs="sunny"]
    [text text="It's a beautiful day!"]
[elseif lhs="${weather}" op="==" rhs="rainy"]
    [text text="I should bring an umbrella."]
[else]
    # This runs if it's not sunny OR rainy (e.g., cloudy or snowy)
    [text text="The sky looks interesting today."]
[endif]
```

### Argumentos

Esta etiqueta no acepta ningún argumento.

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### Consejos

**Último comodín**:
* Utilice `[else]` para manejar cualquier escenario que no haya cubierto explícitamente en sus comprobaciones `[if]` o `[elseif]`.
* Garantiza que el juego siempre tenga un camino válido a seguir.

**Ubicación**:
* `[else]` debe colocarse después de todas las etiquetas `[elseif]` (si las hay) e inmediatamente antes de la etiqueta `[endif]`.
* Solo puedes tener un `[else]` por bloque `[if]`.

**Naturaleza Opcional**:
* No *tienes* que incluir un bloque `[else]`.
* Si no se cumplen ningunas condiciones y no hay `[else]`, el motor simplemente omitirá todo y continuará después de `[endif]`.

---

## `endif`

Finalizar rama condicional

La etiqueta `endif` marca el final de un bloque condicional iniciado por una etiqueta `[if]`. 
Le indica al motor que reanude la ejecución normal del script una vez completada la lógica de bifurcación.

### Uso básico

```
[if lhs="${love_points}" op=">=" rhs="50"]
    [text text="She gives you a warm smile."]
[else]
    [text text="She greets you politely."]
[endif]

# Script execution continues here regardless of the outcome above
[text text="The day continues..."]
```

### Argumentos

Esta etiqueta no acepta ningún argumento.

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### Consejos

**Cierre Obligatorio**:
* Cada etiqueta `[if]` debe tener un `[endif]` correspondiente.
* Piense en ellos como un par de corchetes que mantienen organizada la lógica de su historia.

**Ubicación**:
* Coloque siempre `[endif]` al final de su secuencia condicional, después de cualquier bloque `[elseif]` o `[else]`.

**Anidación**:
* Si pones un `[if]` dentro de otro `[if]`, asegúrate de que cada uno tenga su propio `[endif]`.
* ¡El anidamiento adecuado es el secreto para lograr banderas de historias complejas y sin errores!

---

## `load`

Cargar archivo de secuencia de comandos

La etiqueta `load` cambia el script actual a un archivo NovelML diferente.
Se utiliza principalmente para organizar historias grandes en múltiples capítulos o para hacer transiciones entre diferentes partes del juego, como la pantalla de título y la historia principal.

### Uso básico

```
# Load and start from the beginning of scene02.novel
[load file="scene02.novel"]

# Load scene02.novel and jump directly to a specific label
[load file="scene02.novel" label="chapter2_start"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|--------------------------------------------------|---------------------------------------------------------|
| `file` | No | El nombre de archivo del script NovelML que se cargará.      | Debe ser un archivo válido en el directorio de secuencias de comandos del proyecto. |
| `label` | Yes | La etiqueta de destino a la que saltar dentro del nuevo archivo. | Si se omite, el guión comienza desde la primera línea. |

### Consejos

**Organización del proyecto**:
* En lugar de escribir todo el juego en un archivo gigante, usa `[load]` para dividirlo en partes manejables como `chapter1.novel`, `chapter2.novel`, etc.

**Transición Inmediata**:
* Cuando el motor encuentra una etiqueta `[load]`, deja de ejecutar el archivo NovelML actual inmediatamente y cambia al nuevo.
* Cualquier comando colocado después de `[load]` en el archivo original no se ejecutará.

**Banderas globales**:
* No se preocupe por sus variables: cualquier valor que haya establecido con la etiqueta `[set]` persistirá incluso después de cargar un nuevo archivo de script.

---

## `se`

Reproducir efecto de sonido

La etiqueta `se` reproduce un efecto de sonido (SE). 
Los efectos de sonido se utilizan para señales de audio breves, como golpes de puerta, pasos o comentarios de la interfaz de usuario, lo que agrega una capa de inmersión y realismo a sus escenas.

### Uso básico

```
# Play a sound effect once
[se file="door_open.ogg"]

# Stop all currently playing sound effects
[se file="none"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|---------------|-------------------------------------------|----------------------------------------------|
| `file` | No | El nombre de archivo del efecto de sonido que se reproducirá. | Establezca en `none` para detener la reproducción del efecto de sonido. |
| `loop` | Sí (`false`) | Ya sea para repetir el efecto de sonido o no.  |                                              |

### Consejos

**Formato requerido**:
* Al igual que BGM, Suika3 requiere que los archivos SE estén en formato **Ogg Vorbis**.
* La frecuencia de muestreo DEBE ser **44,100 Hz** para garantizar alta fidelidad y compatibilidad.

**Superposición de sonidos**:
* Los efectos de sonido normalmente se pueden reproducir mientras se ejecuta BGM.
* Ocupan su propia pista de audio para no interrumpir tu música.

**Control de volumen**:
* Para ajustar el volumen de sus efectos de sonido sin cambiar el volumen de BGM, use la etiqueta `[volume]` con `track="se"`.

**Uso para ambiente**:
* Si bien SE se usa a menudo para sonidos cortos, también puedes usarlo para bucles de sonidos ambientales (como el viento o la lluvia).
* Un SE en bucle se restaura cuando se carga un archivo de datos guardados.

---

## `volume`

Establecer volumen de audio

La etiqueta `volume` establece el volumen del sonido para una pista de audio específica. 
Es perfecto para garantizar que la música de fondo no ahogue efectos de sonido importantes o voces de personajes.

### Uso básico

```
# Set BGM volume to 50%
[volume track="bgm" volume="0.5"]

# Set SE volume to maximum
[volume track="se" volume="1.0"]

# Mute voices
[volume track="voice" volume="0.0"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|---------------------------------------|-------------------------------------------|
| `track` | No | La pista de audio para ajustar.            | Consulte la tabla "Pistas" a continuación.             |
| `volume` | No | El nivel de volumen de `0.0` a `1.0`. | `0.0` está en silencio, `1.0` es el volumen máximo. |
| `time` | Sí (`0`) | Tiempo de desvanecimiento en segundos.               | `0` significa cambio instantáneo.                 |
### Tipos de pistas (`track`)

Suika3 clasifica el audio en tres pistas principales:

| Nombre de la pista | Descripciﾃｳn |
|------------|----------------------------------|
| `bgm` | Música de fondo.                |
| `se` | Efectos de sonido y sonidos del sistema. |
| `voice` | Archivos de voz de personajes.           |

### Consejos

**Cambio inmediato**:
* El cambio de volumen ocurre gradualmente cuando `time` es mayor que `0`.
* `time="0"` significa un cambio inmediato.

**Niveles predeterminados**:
* Es una buena idea establecer tus niveles de volumen preferidos al comienzo del juego (por ejemplo, en una etiqueta `start`) para que el jugador tenga una experiencia consistente desde el principio.

---

## `skip`

Establecer estado de omisión

La etiqueta `skip` habilita o deshabilita la función de omisión dentro del juego. 
Es útil para evitar que los jugadores se salten secuencias cinematográficas importantes o para garantizar que ciertas escenas se experimenten al ritmo previsto.

### Uso básico

```
# Enable the skip function
[skip enable="true"]

# Disable the skip function during a critical scene
[skip enable="false"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|----------------------------------------------|-------------------------------------------------------|
| `enable` | No | Si la función de salto está habilitada o no. | Establezca en `true` para permitir omitir, `false` para bloquearlo. |

### Consejos

**Control cinematográfico**:
* La función de omitir normalmente está deshabilitada antes del logotipo del título al inicio.

**Restaurar configuración**:
* No olvides configurar `[skip enable="true"]` una vez que termine la escena crítica.
* Los jugadores normalmente aprecian tener la libertad de saltar el texto que ya han visto.

**Comportamiento del sistema**:
* Esta etiqueta controla el estado general de "Saltar" del motor.
* Incluso si el jugador presiona una tecla de acceso rápido para omitir, el motor la ignorará si `enable` está configurado en `false`.

---

## `config`

Establecer valor de configuración

La etiqueta `config` te permite modificar los ajustes de configuración del sistema de juego directamente desde el marcado. 
Es esencial para ajustar dinámicamente la interfaz de usuario del juego, como mover el cuadro de mensaje o cambiar los parámetros a nivel del sistema sobre la marcha.

### Uso básico

```
# Change the position of the message box
[config name="msgbox.x" value="100"]
[config name="msgbox.y" value="200"]

# Update a specific system setting
[config name="msgbox.font.size" value="24"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|----------------------------------------------------|----------------------------------------------------------|
| `name` | No | El nombre del parámetro de configuración que se va a cambiar. | Consulte la lista de configuración del sistema para obtener nombres válidos.       |
| `value` | No | El nuevo valor que se asignará al parámetro.          | Los valores se manejan como cadenas pero pueden representar números. |

### Consejos

**Personalización de la interfaz de usuario**:
* Puedes usar `[config]` para reposicionar el cuadro de mensaje durante escenas específicas para crear una sensación más cinematográfica.

**Ajustes dinámicos**:
* Dado que esta etiqueta se puede invocar en cualquier parte del guión, puedes cambiar la "apariencia" del juego a medida que avanza la historia.
* Por ejemplo, cambiar la interfaz de usuario para una secuencia de "flashback".

**Nombres de parámetros**:
* ¡Cuidado con el argumento `name`!
* Debe coincidir exactamente con las claves de configuración interna definidas en la configuración de su proyecto Suika3.
* Ver también [la lista completa de las configuraciones](config.md)

---

## `layer`

Manipulación directa de capas

La etiqueta `layer` permite el control directo sobre capas de texto e imágenes específicas. 
Si bien etiquetas como `[bg]` y `[ch]` son más fáciles para escenas estándar, `[layer]` le brinda la precisión para modificar la posición, escala y rotación de cualquier capa individual de forma independiente.

### Uso básico

```
# Load an image directly onto the center character layer (chc)
[layer name="chc" file="heroine_smile.png"]

# Adjust only the position and opacity of the background (bg)
[layer name="bg" x="100" y="100" alpha="128"]

# Rotate the face layer
[layer name="chf" rotate="45.0" center-x="100" center-y="100"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|-----------|-------------|--------------------------------------|------------------------------------|
| `name` | No | El nombre de la capa de destino.               | Consulte la tabla "Nombres de capas" a continuación. |
| `file` | Yes | El nombre del archivo que se cargará en la capa. | Utilice `none` para borrar la capa.     |
| `x` | Sí (`0`) | La posición X de la capa.              |                                    |
| `y` | Sí (`0`) | La posición Y de la capa.              |                                    |
| `alpha` | Sí (`255`) | El nivel de opacidad de la capa.           | `0` a `255`.                      |
| `scale-x` | Sí (`1.0`) | Factor de escala del eje X.               | `1.0` es el tamaño original.            |
| `scale-y` | Sí (`1.0`) | Factor de escala del eje Y.               | `1.0` es el tamaño original.            |
| `center-x`| Sí (`0`) | Centro de rotación (X).                 | Punto de pivote para rotación.          |
| `center-y`| Sí (`0`) | Centro de rotación (Y).                 | Punto de pivote para rotación.          |
| `rotate` | Sí (`0.0`) | Rotación en grados.                 | Positivo para el sentido de las agujas del reloj.            |

### Nombres de capas comunes (`name`)

Suika3 tiene un rico conjunto de capas predefinidas.

Aquí está la lista completa de las capas:

| Nombre de capa | Descripciﾃｳn |
|-----------------|-----------------------------------------|
|bg |Imagen de fondo |
|bg2 |Imagen de fondo 2 |
|efb1 |Efecto espalda 1 |
|efb2 |Efecto espalda 2 |
|efb3 |Efecto espalda 3 |
|efb4 |Efecto espalda 4 |
|chb |Personaje de defensa central |
|chb-eye |Ojos del personaje central |
|chb-lip |Labios del personaje central |
|chb-fo |Personaje de defensa central que se desvanece |
|chl |Carácter izquierdo |
|chl-eye |Ojos del personaje izquierdo |
|chl-lip |Labios del personaje izquierdo |
|chl-fo |Carácter izquierdo que desaparece |
|chlc |Carácter del centro izquierdo |
|chlc-eye |Ojos del personaje del centro izquierdo |
|chlc-lip |Labios del personaje central izquierdo |
|chlc-fo |Carácter central izquierdo que se desvanece |
|chr |Carácter correcto |
|chr-eye |Ojos del personaje derecho |
|chr-lip |Labios del personaje correcto |
|chr-fo |Carácter derecho que desaparece gradualmente |
|chrc |Carácter del centro derecho |
|chrc-eye |Ojos del personaje del centro derecho |
|chrc-lip |Labios del personaje central derecho |
|chrc-fo |Carácter central derecho que se desvanece |
|chc |Carácter central |
|chc-eye |Ojos del personaje central |
|chc-lip |Labios del personaje central |
|chc-fo |Carácter central que se desvanece |
|msgbox |Cuadro de mensaje (Invisible para `[layer]`) |
|namebox |Cuadro de nombre (Invisible para `[layer]`) |
|click |Haga clic en animación (Invisible para `[layer]`) |
|eff1 |Efecto frontal 1 |
|eff2 |Efecto frontal 2 |
|eff3 |Efecto frontal 3 |
|eff4 |Efecto frontal 4 |
|chf |Personaje facial |
|chf-eye |Ojos del personaje de la cara |
|chf-lip |Labios del personaje de la cara |
|chf-fo |Carácter de cara que se desvanece |
|text1 |Capa de texto 1 |
|text2 |Capa de texto 2 |
|text3 |Capa de texto 3 |
|text4 |Capa de texto 4 |
|text5 |Capa de texto 5 |
|text6 |Capa de texto 6 |
|text7 |Capa de texto 7 |
|text8 |Capa de texto 8 |
|gui1 |Botón GUI 1 (Invisible para `[layer]`) |
|gui2 |Botón GUI 2 (Invisible para `[layer]`) |
|gui3 |Botón GUI 3 (Invisible para `[layer]`) |
|gui4 |Botón GUI 4 (Invisible para `[layer]`) |
|gui5 |Botón GUI 5 (Invisible para `[layer]`) |
|gui6 |Botón GUI 6 (Invisible para `[layer]`) |
|gui7 |Botón GUI 7 (Invisible para `[layer]`) |
|gui8 |Botón GUI 8 (Invisible para `[layer]`) |
|gui9 |Botón GUI 9 (Invisible para `[layer]`) |
|gui10 |Botón GUI 10 (Invisible para `[layer]`) |
|gui11 |Botón GUI 11 (Invisible para `[layer]`) |
|gui12 |Botón GUI 12 (Invisible para `[layer]`) |
|gui13 |Botón GUI 13 (Invisible para `[layer]`) |
|gui14 |Botón GUI 14 (Invisible para `[layer]`) |
|gui15 |Botón GUI 15 (Invisible para `[layer]`) |
|gui16 |Botón GUI 16 (Invisible para `[layer]`) |
|gui17 |Botón GUI 17 (Invisible para `[layer]`) |
|gui18 |Botón GUI 18 (Invisible para `[layer]`) |
|gui19 |Botón GUI 19 (Invisible para `[layer]`) |
|gui20 |Botón GUI 20 (Invisible para `[layer]`) |
|gui21 |Botón GUI 21 (Invisible para `[layer]`) |
|gui22 |Botón GUI 22 (Invisible para `[layer]`) |
|gui23 |Botón GUI 23 (Invisible para `[layer]`) |
|gui24 |Botón GUI 24 (Invisible para `[layer]`) |
|gui25 |Botón GUI 25 (Invisible para `[layer]`) |
|gui26 |Botón GUI 26 (Invisible para `[layer]`) |
|gui27 |Botón GUI 27 (Invisible para `[layer]`) |
|gui28 |Botón GUI 28 (Invisible para `[layer]`) |
|gui29 |Botón GUI 29 (Invisible para `[layer]`) |
|gui30 |Botón GUI 30 (Invisible para `[layer]`) |
|gui31 |Botón GUI 31 (Invisible para `[layer]`) |
|gui32 |Botón GUI 32 (Invisible para `[layer]`) |

### Consejos

**Control de precisión**:
* Utilice `[layer]` cuando desee cargar una imagen en una capa manualmente cuando esté trabajando con capas de efectos personalizados (`eff1`, etc.) que no tienen etiquetas dedicadas.

**Actualizaciones instantáneas**:
* A diferencia de `[ch]` o `[bg]`, la etiqueta `layer` generalmente actualiza la pantalla al instante.
* Si desea animar estos cambios a lo largo del tiempo, ¡debe usar la etiqueta `[move]` en su lugar!

**Jerarquía de capas**:
*Recuerda que las capas se apilan.
* Por ejemplo, `chf` (carácter de la cara) siempre se representa delante de `chc` (carácter central).
* Comprender este "orden Z" es clave para composiciones visuales complejas.

---

## `move`

Animar capa

La etiqueta `move` anima capas específicas durante un período determinado.
Es perfecto para crear efectos deslizantes, hacer zoom en personajes o rotar elementos de la pantalla para agregar energía dinámica a sus escenas.

### Uso básico

```
# Move the center character to a new position over 2.0 seconds
[move time="2.0" center-x="150" center-y="100"]

# Relative movement: Nudge the background 50px to the right
[move time="1.0" bg-x="r50"]

# Gradually fade out a layer while rotating it
[move time="3.0" face-alpha="0" face-rotate="r360"]
```

### Argumentos

**Común:**
| Argumento | Omisible | Descripciﾃｳn | Notas |
|------------------|---------------|-------------------------------------------|--------------------------------------------|
| `name` | No | La capa de destino a animar.              | Consulte la tabla "Capas móviles" a continuación.     |
| `time` | No | La duración de la animación en segundos. | Admite valores decimales (por ejemplo, `0.5`).     |
| `async` | Sí (`false`) | Si `true`, haz una animación sin bloqueo.     |                                            |
| `accel` | Sí (`normal`)| Tipo de aceleración.                        | Uno de |
| (capa)-(sufijo) | Yes |                                           |                                            |

**(capa):**
| Argumento | Descripciﾃｳn |
|----------------|-------------------------------------------|
| `bg` | Capa de fondo.                         |
| `bg2` | Antecedentes 2. |
| `volver | Carácter del centro trasero.                    |
| `left` | Carácter izquierdo.                           |
| `right` | Carácter correcto.                          |
| `center` | Carácter central.                         |
| `left-center` | Carácter del centro izquierdo.                    |
| `right-center` | Carácter intermedio.                   |
| `face` | Carácter facial.                           |

**(sufijo):**
| Sufijo | Omisible | Descripciﾃｳn | Notas |
|-------------|---------------|----------------------------|---------------------------------------------------------------|
| `-x` | Sí (`0`) | Posición X.                | Admite absoluto (p. ej., `100`) o relativo (p. ej., `r50`).    |
| `-y` | Sí (`0`) | Posición Y.                | Admite absoluto (p. ej., `100`) o relativo (p. ej., `r-50`).   |
| `-a` | Sí (`255`) | Valor alfa. (opacidad) | `0` (transparente) a `255` (opaco).                          |
| `-scale-x` | Sí (`1.0`) | factor de escala X.          | `1.0` es el tamaño original. Admite el prefijo `r`.                  |
| `-scale-y` | Sí (`1.0`) | Factor de escala Y.          | `1.0` es el tamaño original. Admite el prefijo `r`.                  |
| `-center-x` | Sí (`0`) | Centro X para rotación.     | Punto de pivote para el efecto de rotación.                          |
| `-center-y` | Sí (`0`) | Centro Y para rotación.     | Punto de pivote para el efecto de rotación.                          |
| `-rotate` | Sí (`0`) | Rotación en grados.       | Positivo para el sentido de las agujas del reloj. Admite el prefijo `r`.                  |
| `-dim` | Sí (`false`) | Estado de atenuación.            | Si es `true`, la capa se oscurece un 50%.                  |

### Consejos

**Animación sin bloqueo (`async="true")`**:
* El script continúa con el siguiente comando inmediatamente después de iniciar `[move]`.
* Si desea que el script espere hasta que finalice la animación, sígalo con una etiqueta `[wait]` usando el mismo valor `time`.

**Transformaciones relativas**:
* Usar el prefijo `r` (por ejemplo, `x="r100"`) es increíblemente útil para movimientos repetitivos, como hacer que un personaje "salte" o "sacuda" sin calcular coordenadas absolutas.

**Pulido visual**:
* ¡Combina `scale-x` y `scale-y` con `move` para crear efectos de "acercamiento" en la cara de un personaje para primeros planos dramáticos!

---

## `pencil`

Lápiz

Dibuja un texto en una capa.

### Uso básico

```
[pencil layer="bg" font-size="30" text="Hello, World!"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn |
|---------------|------------------|--------------------------|
| text | No | Texto para dibujar.            |
| layer | Sí (`text1`) | Nombre de la capa.              |
| font-type | Sí (`0`) | Selección de fuentes. (0-3) |
| font-size | Sí (`16`) | Tamaño de fuente.               |
| color | Sí (`#000000`) | Color de fuente.              |
| outline-width | Sí (`0`) | Ancho del contorno de la fuente.      |
| outline-color | Sí (`#ffffff`) | Color del contorno de la fuente.      |
| line-margin | Yes | Margen de línea.             |
| char-margin | Sí (`0`) | Margen de carácter.        |
| x | Sí (`0`) | Posición X del área de dibujo. |
| y | Sí (`0`) | Posición Y del área de dibujo. |
| width | Yes | Ancho del área de dibujo.      |
| height | Yes | Altura del área de dibujo.     |

## Nombre de capa admitida

| Nombre de capa | Descripciﾃｳn |
|-----------------|-----------------------------------------|
|bg |Imagen de fondo |
|bg2 |Imagen de fondo 2 |
|efb1 |Efecto espalda 1 |
|efb2 |Efecto espalda 2 |
|efb3 |Efecto espalda 3 |
|efb4 |Efecto espalda 4 |
|chb |Personaje de defensa central |
|chl |Carácter izquierdo |
|chlc |Carácter central izquierdo |
|chr |Carácter correcto |
|chrc |Carácter del centro derecho |
|chc |Carácter central |
|eff1 |Efecto frontal 1 |
|eff2 |Efecto frontal 2 |
|eff3 |Efecto frontal 3 |
|eff4 |Efecto frontal 4 |
|chf |Personaje facial |
|text1 |Capa de texto 1 |
|text2 |Capa de texto 2 |
|text3 |Capa de texto 3 |
|text4 |Capa de texto 4 |
|text5 |Capa de texto 5 |
|text6 |Capa de texto 6 |
|text7 |Capa de texto 7 |
|text8 |Capa de texto 8 |

---

## `returnmacro`

Regresar de Macro

La etiqueta `returnmacro` sale inmediatamente de la macro actual y devuelve la ejecución del script a la línea que sigue a la etiqueta `[callmacro]` original.
Es particularmente útil para detener una macro anticipadamente en función de condiciones específicas dentro de un bloque `[if]`.

### Uso básico

```
[defmacro name="check_item"]
    [if lhs="${has_key}" op="==" rhs="false"]
        [text text="The door is locked."]
        [returnmacro]
    [endif]

    # This part only runs if has_key is true
    [text text="You unlocked the door with the key!"]
[endmacro]
```

### Argumentos

Esta etiqueta no acepta ningún argumento.

| Argumento | Omisible | Descripciﾃｳn | Notas |
|----------|-----------|-------------|-------|
| -        | -         | -           | -     |

### Consejos

**Salida anticipada**:
* Utilice `[returnmacro]` dentro de un bloque `[if]` para omitir el resto de los comandos de una macro si se cumple una determinada condición.
* ¡Esto mantiene tus macros flexibles y potentes!

**Retorno implícito**:
* En realidad, no es necesario poner `[returnmacro]` al final de cada macro.
* Una vez que el motor llegue a la etiqueta `[endmacro]`, volverá al script principal automáticamente.

**Control de flujo**:
* Recuerde que esta etiqueta solo sale de la macro *actual*. No detiene todo el juego ni salta a una etiqueta diferente; simplemente te envía de regreso al lugar desde donde se llamó la macro.

---

## `video`

Reproducir vídeo

La etiqueta `video` reproduce un archivo de película en la pantalla.
Es ideal para cinemáticas de apertura, escenas de transición o efectos visuales de alto impacto que se reproducen mejor como vídeo en movimiento completo.

### Uso básico

```
# Play an opening movie (cannot be skipped)
[video file="opening.mp4"]

# Play a short cutscene that the player can skip with a click.
[video file="cutscene01.mp4" skippable="true"]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|-------------|---------------|-------------------------------------------------------|---------------------------------------------------------------|
| `file` | No | El nombre de archivo del vídeo a reproducir.                    | El archivo debe estar en un formato compatible (por ejemplo, .mp4).          |
| `skippable` | Sí (`false`) | Si el video se puede omitir con el clic de un jugador. | Configúrelo en `false` para obligar al reproductor a ver el video completo. |

### Consejos

**Soporte de archivos**:
* Asegúrese de que su archivo de video tenga el formato .mp4 (H.264 + AAC).
* Si desea admitir Windows de 32 bits, prepare el archivo .wmv junto con el archivo .mp4 y luego elimine la extensión, por ejemplo, `[video file="opening"]`.

**Transición**:
* Una vez que el video termina de reproducirse (o se omite), el motor pasa automáticamente al siguiente comando en su secuencia de comandos.
* A menudo es una buena idea seguir una etiqueta `[video]` con una etiqueta `[bg]` para garantizar que la pantalla se vea exactamente como desea después de que termine la película.

**Audio en vídeo**:
* La mayoría de los archivos de vídeo incluyen su propia pista de audio.
* Tenga en cuenta que este audio se reproducirá junto con cualquier `[bgm]` que esté ejecutando.
* ¡Quizás quieras detener la música con `[bgm file="none"]` antes de comenzar un video con sonido!

---

## `wait`

Espera el tiempo

La etiqueta `wait` pausa la ejecución de NovelML durante un período específico.
Es esencial para controlar el ritmo de las transiciones visuales, crear pausas dramáticas o efectos de sincronización sin requerir la intervención del jugador.

### Uso básico

```
# Pause for 1.5 seconds before the next command
[wait time="1.5"]

# Create a brief pause between character changes
[ch center="chara01_surprised.png" time="0.5"]
[wait time="1.0"]
[text text="She couldn't believe her eyes."]
```

### Argumentos

| Argumento | Omisible | Descripciﾃｳn | Notas |
|---------------|---------------|--------------------------------|----------------------------------------|
| `time` | No | El número de segundos a esperar. | Admite valores decimales (por ejemplo, `0.5`). |
| `hidemsgbox` | Sí (`false`) | Forzar ocultar el cuadro de mensaje.    |                                        |
| `hidenamebox` | Sí (`false`) | Forzar ocultar el cuadro de nombre.       |                                        |

### Consejos

**Pausa no interactiva**:
* A diferencia de `[click]`, que espera a que el jugador actúe, `[wait]` continúa automáticamente una vez que se acaba el tiempo. 
* Esto es perfecto para segmentos de "reproducción automática" o secuencias visuales cronometradas.

**Combinando con animaciones**:
* Si usa una etiqueta `[ch]` o `[bg]` con un argumento `time`, el motor pasa al siguiente comando inmediatamente mientras se reproduce la animación. 
* Utilice `[wait]` después de una animación si desea que el guión se detenga hasta que finalice la animación (o incluso más para lograr un efecto dramático).

**Experiencia de usuario**:
* ¡Ten cuidado de no hacer `[wait]` veces demasiado largas (como más de 3 segundos) sin una razón visual, o el jugador podría pensar que el juego se ha congelado!

