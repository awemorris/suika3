Referencia de sintaxis de rayos
====================

`Ray` es en realidad el lenguaje de programación `Noct` con API Suika3 adicionales.

## Asignaciones

Las variables en Noct se escriben dinámicamente y no requieren
declaración. El operador de asignación (`=`) se utiliza para crear y
asignar valores a las variables.

Como se muestra en el siguiente ejemplo, Noct admite varios tipos de datos
incluidos números enteros, números de punto flotante y cadenas. Las variables pueden
reasignarse a diferentes tipos en cualquier momento durante la ejecución.

```
func main() {
    var a = 123;
    print(a);

    var b = 1.0;
    print(b);

    var c = "string";
    print(c);
}
```

## Variables globales

Las variables globales se pueden definir en funciones y no se pueden definir
fuera de funciones.

```
func main() {
    globalVariable = 123;
    print(globalVariable);
}
```

## Variables locales

El uso de la palabra clave `var` le permite declarar una variable como
locales. Sin la declaración `var`, la asignación a una variable crea una
variables globales.

```
func main() {
    var a = 123;
    print(a);
}
```

## Matriz

Las matrices son colecciones ordenadas de valores, a las que se accede por índice. matrices
admite la iteración a través de la construcción del bucle `for`, lo que le permite
iterar a través de cada valor directamente.

```
func main() {
    var array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Las matrices pueden contener valores de diferentes tipos simultáneamente, lo que refleja
El sistema de escritura dinámica.

```
func main() {
    var array = [123, "string"];
}
```

El lenguaje proporciona una función incorporada `push()` para agregar elementos a
el final de una matriz.  Además, `pop()` elimina el elemento final.

```
func main() {
    var array = []
    array->push(0);
    array->push(1);
    array->push(2);

    var last = array->pop();

    print("Length = " + array.length);
}
```

## Diccionario

Los diccionarios almacenan pares clave-valor, similares a mapas hash u objetos en
otros idiomas. Se definen mediante llaves con clave-valor.
pares separados por dos puntos. Los diccionarios admiten la iteración donde ambos
Se puede acceder a la clave y al valor simultáneamente.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print("key = " + key);
        print("value = " + value);
    }

    print("Length = " + dict.length);
}
```

Los diccionarios se pueden construir en un solo paso. una tarea
puede ser un estilo de matriz que usa `[]`, o un estilo de objeto que usa
`.`.

```
func main() {
    var dict = {};
    dict["key1"] = "value1";
    dict.key2 = "value2";
}
```

La función incorporada `remove()` permite la eliminación de entradas por
clave.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    remove(dict, "key1");
}
```

## Bucle For

La construcción de bucle for proporciona una sintaxis concisa para iterar a través de
secuencias como rangos, matrices y diccionarios.

La sintaxis de rango (usando el operador `..`) crea un iterador que
genera valores desde el inicio hasta uno menos que el valor final.

```
func main() {
    for (i in 0..10) {
        print(i);
    }
}
```

Los bucles For también pueden iterar directamente sobre matrices y otras colecciones.
tipos.

Las matrices se pueden iterar mediante la sintaxis de valor.

```
func main() {
    array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Los diccionarios se pueden iterar mediante la sintaxis for-key-value.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

## Mientras bucles

El bucle while proporciona un mecanismo de iteración tradicional que
continúa la ejecución mientras permanezca una condición específica
cierto. A diferencia de los bucles for que están diseñados para iterar sobre
colecciones, los bucles while son más flexibles y se pueden utilizar para
implementar varios algoritmos donde el número de iteraciones no es
conocido de antemano. El ejemplo muestra una implementación de contador básica.
incrementando de 0 a 9.

```
func main() {
    var i = 0;
    while (i < 10) {
        print(i);
        i = i + 1;
    }
}
```

## Bloques If y Else

Los flujos de control permiten ejecuciones condicionales basadas en evaluaciones.
expresiones. La construcción if-else sigue una sintaxis familiar donde
Las condiciones se evalúan en secuencia.

```
func main() {
    var a = readint();
    if (a == 0) {
        print("0");
    } else if (a == 1) {
        print("1");
    } else {
        print("other");
    }
}
```

## Funciones Lambda

Las funciones son objetos de primera clase en el lenguaje. anónimo
Las funciones, también conocidas como expresiones `lambda`, le permiten crear
funciones sin nombre.

```
func main() {
    var f = (a, b) => { return a + b; }
    print(f(1, 2));
}
```

Las funciones Lambda simplemente se traducen a funciones con nombre en el
proceso de compilación. Por lo tanto, no pueden capturar variables declaradas.
en funciones exteriores.

## Incremento/Decremento (+=, -=, ++, --)

```
func main() {
    var a = 123;
    a += 321;
    a++;

    var b = 123;
    b -= 321;
    b--;
}
```

`++` y `--` solo se admiten como declaraciones independientes (`a++;`, `b--;`).
No se permite su uso dentro de expresiones para evitar efectos secundarios complejos.

## POO en noviembre

El modelo orientado a objetos en Noct es una variación ligera de la programación orientada a objetos basada en prototipos.

- Las clases son simplemente plantillas de diccionario.
- La herencia y la creación de instancias se realizan mediante la fusión de diccionarios.
- No existe una cadena de prototipos y la modificación de una clase no afecta las instancias existentes.

Este diseño trata los diccionarios como objetos de primera clase y el autor se refiere a ellos como POO basada en diccionarios (D-OOP).

```
func main() {
    // The base class definition. (A class is just a dictionary.)
    Animal = class {
        name: "Animal",
        cry: (this) => {
        }
    };

    // The subclass definition. (Just a dictionary merging.)
    Cat = extend Animal {
        name: "Cat",
        voice: "meow",
        cry: (this) => {
            print(this.name + " cries like " + this.voice);
        }
    };

    // Instantiation. (Just a dictionary merging.)
    var myCat = new Cat {
        voice: "neee"
    };

    // This-call uses -> () syntax. (Equal to myCat.cry(myCat))
    myCat->cry();
}
```

---

## Intrínsecos

### int()

```
var i = int(1.23);
```

### float()

```
var f = float(123);
```

### newArray()

```
var array = newArray(10);
```

### push()

```
var array = [1, 2, 3];
array->push(4);
```

### pop()

```
var array = [1, 2, 3];
var last = array->pop();
```

### resize()

```
var array = [1, 2, 3];
array->resize(2);
```

### charCount()

```
var s = "ABC文あいう";
var l = s->charCount();
```

### charAt()

```
var s = "ABC文あいう";
for (i in 0 .. s.length) {
   var c = s->charAt(i);
   print(c);
}
```

### substring()

```
var s1 = "ABCDEFG";
var s2 = s1.substring(0, 3); // from the char 0, three characters
var s3 = s1.substring(2, -1); // from the char 1, to the end
}
```

### unset()

```
var dic = {key1: "ABC"};
dic->unset("key1");
```

---

## API de matemáticas

## Matemáticas

### abs()

```
var a = abs(x);
```

### random()

```
var r = random(); // 0 .. 1.0
```

### Math.sin()

```
var y = sin(x);
```

### Math.cos()

```
var y = cos(x);
```

### Math.tan()

```
var y = tan(x);
```
