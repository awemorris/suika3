Referencia de sintaxis de NovelML
========================

Este documento explica la sintaxis básica de NovelML y cómo se ejecuta.

Para obtener una explicación detallada de cada etiqueta, consulte "Referencia de etiquetas Suika3".

---

## 1. NovelML es una lista de comandos del motor llamados "etiquetas"

NovelML es una lista de **etiquetas**. Una etiqueta es un **comando para el motor**.

- Cada etiqueta le dice al motor que haga algo.
- Las etiquetas se ejecutan **una por una**, en el orden en que aparecen
- Después de que se ejecuta una etiqueta, se "termina" y la "posición de ejecución" continúa

Si vuelves a escribir la misma etiqueta, hará lo mismo **cada vez que se ejecute**.

---

## 2. La ejecución va de arriba a abajo

NovelML se ejecuta desde la parte superior del archivo hasta el final, una etiqueta a la vez.

- La ejecución normalmente avanza.
- Sólo hay un **puesto de ejecución** actual

```PlainText
[text text="Hello"]
[text text="World"]
```

En este caso:

1. Se muestra `Hello`
2. Luego se muestra `World`

---

## 3. Todo debe escribirse como etiquetas.

En NovelML, **cada línea debe ser una etiqueta**.

- Todo el escenario está escrito usando etiquetas.
- Incluso el texto debe usar la etiqueta `[text]`

```PlainText
[text text="It's a beautiful day."]
```

---

## 4. Etiquetas que esperan y etiquetas que no esperan

Hay dos tipos principales de etiquetas:

- **Etiquetas que se ejecutan y pasan inmediatamente a la siguiente etiqueta**
- **Etiquetas que esperan** la entrada del usuario o que algo termine

Las etiquetas comunes de "espera" incluyen:

- `text` (espera un clic)
- `click` (espera un clic)
- `wait` (espera un tiempo específico)
- `video` (espera hasta que finalice la reproducción)

Estas etiquetas pausan la ejecución **sin ninguna sintaxis especial**.

### 4.1 Etiquetas que se ejecutan en segundo plano (asincrónicamente)

Algunas etiquetas pueden **ejecutarse en segundo plano** (de forma asincrónica).

Ejemplos típicos:

- `anime`
- `move`

Estas etiquetas:

- Crear e iniciar una animación o movimiento.
- Si se especifica `async="true"`, el motor **no espera** a que finalice y continúa inmediatamente con la siguiente etiqueta.

Debido a esto, las etiquetas después de una etiqueta `async="true"` pueden ejecutarse **mientras la animación aún se está reproduciendo**.

Esto te permite hacer cosas como:

- Mover un fondo o personaje
- Al mismo tiempo que muestra texto o ejecuta otros efectos

---

## 5. Etiquetas que cambian el flujo de ejecución

Normalmente la ejecución va de arriba a abajo, pero algunas etiquetas **cambian donde continúa la ejecución**.

### 5.1 Etiquetas

```PlainText
[label start]
```

- Una etiqueta marca una posición con nombre en el escenario.
- Ejecutar una etiqueta no hace nada por sí solo.

### 5.2 Saltos

```PlainText
[goto name="start"]
```

---

## 6. Ramificación condicional (if / elseif / else)

```PlainText
[if ...]
  ...
[elseif ...]
  ...
[else]
  ...
[endif]
```

- Sólo se ejecuta el primer bloque coincidente.
- Los bloques que no se eligen se **omiten por completo**
- Puedes escribir `elseif` cero o más veces
- `else` es opcional

---

## 7. Variables y expansión de variables.

### 7.1 Configuración de variables

Utilice la etiqueta `[set]` para establecer una variable.

```PlainText
[set name="player_name" value="Alice"]
```

- Todas las variables son **cadenas**
- Los números y valores booleanos también se almacenan como cadenas.

### 7.2 Expansión variable

Puede utilizar la expansión de variables dentro de los valores y el texto de las etiquetas.

```PlainText
[text text="${player_name} stands up"]
```

- `${...}` se reemplaza con el valor de la variable en tiempo de ejecución
- La expansión ocurre **cuando se ejecuta la etiqueta**

```PlainText
[set name="x" value="1"]
[text text="${x}"]
[set name="x" value="2"]
[text text="${x}"]
```

En este ejemplo, la salida es `1` y luego `2`.

---

## 8. Las macros son bloques que puedes ejecutar como una unidad

Las macros le permiten agrupar varias etiquetas y ejecutarlas juntas.

```PlainText
[defmacro name="greet"]
  [text text="Hello"]
[endmacro]

[callmacro name="greet"]
```

---

## 9. Cambiar archivos (cargar)

```PlainText
[load file="scene2.txt" label="start"]
```

`file=` es obligatorio y `label=` es opcional.

---

## 10. ¿Qué sucede al final de un archivo?

Si se ejecuta:

- Llega al final del archivo y
- No ocurren más etiquetas `goto` o `load`

Entonces la **ejecución del escenario finaliza**.

Sin embargo, en algunas plataformas (por ejemplo, iOS o consolas de juegos), no se permite cerrar la aplicación.

Por lo tanto, se recomienda que su escenario **explícitamente**:

- Vuelve al título usando `goto`, o
- Carga una escena de título usando `load`

en lugar de finalizar automáticamente.

---

## 11. Cómo encaja este documento con otros documentos

Este documento está destinado a ir con:

- Referencia de etiqueta: explica qué hace cada etiqueta
- Este documento: explica cómo se ejecuta un escenario.

Para obtener detalles sobre etiquetas individuales, consulte la "Referencia de etiquetas".
