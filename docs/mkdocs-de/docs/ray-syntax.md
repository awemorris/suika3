Referenz zur Ray-Syntax
====================

`Ray` ist eigentlich die Programmiersprache `Noct` mit zusätzlichen Suika3-APIs.

## Aufgaben

Variablen in Noct werden dynamisch typisiert und erfordern keine explizite Deklaration. Der Zuweisungsoperator (`=`) wird zum Erstellen und Zuweisen von Werten zu Variablen verwendet.

Wie im folgenden Beispiel gezeigt, unterstützt Noct verschiedene Datentypen, darunter Ganzzahlen, Gleitkommazahlen und Zeichenfolgen. Variablen können während der Ausführung jederzeit anderen Typen zugewiesen werden.

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

## Globale Variablen

Globale Variablen können in Funktionen definiert werden und können nicht außerhalb von Funktionen definiert werden.

```
func main() {
    globalVariable = 123;
    print(globalVariable);
}
```

## Lokale Variablen

Mit dem Schlüsselwort `var` können Sie eine Variable als lokal deklarieren. Ohne `var`-Deklaration wird durch die Zuweisung zu einer Variablen eine globale Variable erstellt.

```
func main() {
    var a = 123;
    print(a);
}
```

## Array

Arrays sind geordnete Sammlungen von Werten, auf die über einen Index zugegriffen wird. Arrays unterstützen die Iteration durch das `for`-Schleifenkonstrukt, sodass Sie jeden Wert direkt durchlaufen können.

```
func main() {
    var array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Arrays können gleichzeitig Werte verschiedener Typen enthalten und spiegeln das dynamische Typisierungssystem wider.

```
func main() {
    var array = [123, "string"];
}
```

Die Sprache bietet eine integrierte Funktion `push()` zum Hinzufügen von Elementen am Ende eines Arrays. Außerdem entfernt `pop()` das letzte Element.

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

## Wörterbuch

Wörterbücher speichern Schlüssel-Wert-Paare, ähnlich wie Hash-Maps oder Objekte in anderen Sprachen. Sie werden durch geschweifte Klammern mit durch Doppelpunkte getrennten Schlüssel-Wert-Paaren definiert. Wörterbücher unterstützen die Iteration, bei der gleichzeitig auf den Schlüssel und den Wert zugegriffen werden kann.

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

Wörterbücher können in einem einzigen Schritt erstellt werden. Eine Zuweisung kann ein Array-Stil sein, der `[]` verwendet, oder ein Objektstil, der `.` verwendet.

```
func main() {
    var dict = {};
    dict["key1"] = "value1";
    dict.key2 = "value2";
}
```

Die eingebaute Funktion `remove()` ermöglicht das Löschen von Einträgen per Schlüssel.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    remove(dict, "key1");
}
```

## For-Schleife

Das for-Loop-Konstrukt bietet eine prägnante Syntax zum Durchlaufen von Sequenzen wie Bereichen, Arrays und Wörterbüchern.

Die Bereichssyntax (unter Verwendung des `..`-Operators) erstellt einen Iterator, der Werte vom Anfang bis zu einem Wert generiert, der um eins kleiner als der Endwert ist.

```
func main() {
    for (i in 0..10) {
        print(i);
    }
}
```

For-Schleifen können auch direkt über Arrays und andere Sammlungstypen iterieren.

Arrays können mit der For-Value-Syntax iteriert werden.

```
func main() {
    array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Wörterbücher können mit der For-Key-Value-Syntax iteriert werden.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

## While-Schleifen

Die While-Schleife stellt einen traditionellen Iterationsmechanismus dar, der die Ausführung so lange fortsetzt, wie eine angegebene Bedingung wahr bleibt. Im Gegensatz zu for-Schleifen, die für die Iteration über Sammlungen konzipiert sind, sind while-Schleifen flexibler und können zur Implementierung verschiedener Algorithmen verwendet werden, bei denen die Anzahl der Iterationen nicht im Voraus bekannt ist. Das Beispiel zeigt eine einfache Zählerimplementierung, die von 0 auf 9 erhöht wird.

```
func main() {
    var i = 0;
    while (i < 10) {
        print(i);
        i = i + 1;
    }
}
```

## If- und Else-Blöcke

Kontrollflüsse ermöglichen bedingte Ausführungen basierend auf ausgewerteten Ausdrücken. Das if-else-Konstrukt folgt einer bekannten Syntax, bei der Bedingungen nacheinander ausgewertet werden.

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

## Lambda-Funktionen

Funktionen sind erstklassige Objekte in der Sprache. Mit anonymen Funktionen, auch `lambda`-Ausdrücke genannt, können Sie Funktionen ohne Namen erstellen.

```
func main() {
    var f = (a, b) => { return a + b; }
    print(f(1, 2));
}
```

Lambda-Funktionen werden im Kompilierungsprozess einfach in benannte Funktionen übersetzt. Daher können sie keine in äußeren Funktionen deklarierten Variablen erfassen.

## Increment/Decrement (+=, -=, ++, --)

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

`++` und `--` werden nur als eigenständige Anweisungen (`a++;`, `b--;`) unterstützt. Die Verwendung innerhalb von Ausdrücken ist nicht zulässig, um komplexe Nebenwirkungen zu vermeiden.

## OOP im Noct

Das objektorientierte Modell in Noct ist eine leichtgewichtige Variante des prototypbasierten OOP.

- Klassen sind einfach Wörterbuchvorlagen
- Vererbung und Instanziierung werden durch Wörterbuchzusammenführung realisiert
- Es gibt keine Prototypenkette und das Ändern einer Klasse hat keine Auswirkungen auf vorhandene Instanzen

Dieses Design behandelt Wörterbücher als erstklassige Objekte und der Autor bezeichnet es als Dictionary-based OOP (D-OOP).

```
func main() {
    // Die Basisklassendefinition. (Eine Klasse ist nur ein Wörterbuch.)
    Animal = class {
        name: "Animal",
        cry: (this) => {
        }
    };

    // Die Unterklassendefinition. (Nur eine Wörterbuchzusammenführung.)
    Cat = extend Animal {
        name: "Cat",
        voice: "meow",
        cry: (this) => {
            print(this.name + " cries like " + this.voice);
        }
    };

    // Instanziierung. (Nur eine Wörterbuchzusammenführung.)
    var myCat = new Cat {
        voice: "neee"
    };

    // Dieser Aufruf verwendet die ->()-Syntax. (Gleich myCat.cry(myCat))
    myCat->cry();
}
```

---

## Eigenheiten

### int()

```
var i = int(1.23);
```

### schweben()

```
var f = float(123);
```

### newArray()

```
var array = newArray(10);
```

### drücken()

```
var array = [1, 2, 3];
array->push(4);
```

### Pop()

```
var array = [1, 2, 3];
var last = array->pop();
```

### Größe ändern()

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

### Teilzeichenfolge()

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

## Mathe-API

## Mathe

### abs()

```
var a = abs(x);
```

### zufällig()

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
