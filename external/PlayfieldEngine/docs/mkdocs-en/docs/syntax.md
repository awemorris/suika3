Ray Syntax
==========

`Playfield Engine` uses `Noct` as a scripting language.

## Assignments

Variables in Noct are dynamically typed and don't require explicit
declaration. The assignment operator (`=`) is used to create and
assign values to variables.

As shown in the example below, Noct supports various data types
including integers, floating-point numbers, and strings. Variables can
be reassigned to different types at any time during execution.

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

## Global Variables

Global variables can be defined in functions, and cannot be defined
outside functions.

```
func main() {
    globalVariable = 123;
    print(globalVariable);
}
```

## Local Variables

Using the `var` keyword allows you to declare a variable as
local. Without `var` declaration, assigning to a variable may create a
global variable.

```
func main() {
    var a = 123;
    print(a);
}
```

## Array

Arrays are ordered collections of values, accessed by index. Arrays
support iteration through the `for` loop construct, allowing you to
iterate through each value directly.

```
func main() {
    var array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Arrays can hold values of different types simultaneously, reflecting
the dynamic typing system.

```
func main() {
    var array = [123, "string"];
}
```

The language provides a built-in function `push()` to add elements to
the end of an array.  Also, `pop()` removes the final element.

```
func main() {
    var array = []
    Array.push(array, 0);
    Array.push(array, 1);
    Array.push(array, 2);

    var last = Array.pop(array);

    print("Length = " + Array.size(array));
}
```

## Dictionary

Dictionaries store key-value pairs, similar to hash maps or objects in
other languages. They are defined using curly braces with key-value
pairs separated by colons. Dictionaries support iteration where both
the key and value can be accessed simultaneously.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print("key = " + key);
        print("value = " + value);
    }

    print("Length = " + Dict.size(dict));
}
```

Dictionaries may be constructed in a single step way. An assignment
can be an array style which uses `[]`, or an object style which uses
`.`.

```
func main() {
    var dict = {};
    dict["key1"] = "value1";
    dict.key2 = "value2";
}
```

The built-in function `remove()` allows for the deletion of entries by
key.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    remove(dict, "key1");
}
```

## For-loop

The for-loop construct provides a concise syntax for iterating through
sequences such as ranges, arrays, and dictionaries.

The range syntax (using the `..` operator) creates an iterator that
generates values from the start to one less than the end value.

```
func main() {
    for (i in 0..10) {
        print(i);
    }
}
```

For-loops can also iterate directly over arrays and other collection
types.

Arrays can be iterated by the for-value syntax.

```
func main() {
    array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Dictionaries can be iterated by the for-key-value syntax.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

## While Loops

The while-loop provides a traditional iteration mechanism that
continues execution as long as a specified condition remains
true. Unlike for-loops which are designed for iterating over
collections, while-loops are more flexible and can be used for
implementing various algorithms where the number of iterations isn't
known in advance. The example shows a basic counter implementation
incrementing from 0 to 9.

```
func main() {
    var i = 0;
    while (i < 10) {
        print(i);
        i = i + 1;
    }
}
```

## If and Else Blocks

Control flows allow for conditional executions based on evaluated
expressions. The if-else construct follows a familiar syntax where
conditions are evaluated in sequence.

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

## Lambda Functions

Functions are first-class objects in the language. Anonymous
functions, also known as `lambda` expressions, allow you to create
functions without names.

```
func main() {
    var f = (a, b) => { return a + b; }
    print(f(1, 2));
}
```

Lambda functions are simply translated to named functions in the
compilation process. Therefore, they can't capture variables declared
in outer functions.

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

`++` and `--` are supported only as standalone statements (`a++;`, `b--;`).
Using them inside expressions is disallowed to avoid complex side-effects.

## OOP in Noct

The object-oriented model in Noct is a lightweight variation of prototype-based OOP.

- Classes are simply dictionary templates
- Inheritance and instantiation are realized by dictionary merging
- There is no prototype chain, and modifying a class does not affect existing instances

This design treats dictionaries as first-class objects, and the author refers to it as Dictionary-based OOP (D-OOP).

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

Noct Syntax
===========

## Assignments

Variables in Noct are dynamically typed and don't require explicit
declaration. The assignment operator (`=`) is used to create and
assign values to variables.

As shown in the example below, Noct supports various data types
including integers, floating-point numbers, and strings. Variables can
be reassigned to different types at any time during execution.

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

## Global Variables

Global variables can be defined in functions, and cannot be defined
outside functions.

```
func main() {
    globalVariable = 123;
    print(globalVariable);
}
```

## Local Variables

Using the `var` keyword allows you to declare a variable as
local. Without `var` declaration, assigning to a variable may create a
global variable.

```
func main() {
    var a = 123;
    print(a);
}
```

## Array

Arrays are ordered collections of values, accessed by index. Arrays
support iteration through the `for` loop construct, allowing you to
iterate through each value directly.

```
func main() {
    var array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Arrays can hold values of different types simultaneously, reflecting
the dynamic typing system.

```
func main() {
    var array = [123, "string"];
}
```

The language provides a built-in function `push()` to add elements to
the end of an array.  Also, `pop()` removes the final element.

```
func main() {
    var array = []
    Array.push(array, 0);
    Array.push(array, 1)
    Array.push(array, 2);

    var last = Array.pop(array);
}
```

## Dictionary

Dictionaries store key-value pairs, similar to hash maps or objects in
other languages. They are defined using curly braces with key-value
pairs separated by colons. Dictionaries support iteration where both
the key and value can be accessed simultaneously.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print("key = " + key);
        print("value = " + value);
    }

}
```

Dictionaries may be constructed in a single step way. An assignment
can be an array style which uses `[]`, or an object style which uses
`.`.

```
func main() {
    var dict = {};
    dict["key1"] = "value1";
    dict.key2 = "value2";
}
```

The built-in function `remove()` allows for the deletion of entries by
key.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    remove(dict, "key1");
}
```

## For-loop

The for-loop construct provides a concise syntax for iterating through
sequences such as ranges, arrays, and dictionaries.

The range syntax (using the `..` operator) creates an iterator that
generates values from the start to one less than the end value.

```
func main() {
    for (i in 0..10) {
        print(i);
    }
}
```

For-loops can also iterate directly over arrays and other collection
types.

Arrays can be iterated by the for-value syntax.

```
func main() {
    array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Dictionaries can be iterated by the for-key-value syntax.

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

## While Loops

The while-loop provides a traditional iteration mechanism that
continues execution as long as a specified condition remains
true. Unlike for-loops which are designed for iterating over
collections, while-loops are more flexible and can be used for
implementing various algorithms where the number of iterations isn't
known in advance. The example shows a basic counter implementation
incrementing from 0 to 9.

```
func main() {
    var i = 0;
    while (i < 10) {
        print(i);
        i = i + 1;
    }
}
```

## If and Else Blocks

Control flows allow for conditional executions based on evaluated
expressions. The if-else construct follows a familiar syntax where
conditions are evaluated in sequence.

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

## Lambda Functions

Functions are first-class objects in the language. Anonymous
functions, also known as `lambda` expressions, allow you to create
functions without names.

```
func main() {
    var f = (a, b) => { return a + b; }
    print(f(1, 2));
}
```

Lambda functions are simply translated to named functions in the
compilation process. Therefore, they can't capture variables declared
in outer functions.

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

`++` and `--` are supported only as standalone statements (`a++;`, `b--;`).
Using them inside expressions is disallowed to avoid complex side-effects.

## OOP in Noct

The object-oriented model in Noct is a lightweight variation of prototype-based OOP.

- Classes are simply dictionary templates
- Inheritance and instantiation are realized by dictionary merging
- There is no prototype chain, and modifying a class does not affect existing instances

This design treats dictionaries as first-class objects, and the author refers to it as Dictionary-based OOP (D-OOP).

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

## Intrinsics

### Int.from(val)

Converts a value to an int value.

```
var a = Int.from(1.2);
print(a); // => 1
```

### Long.from(val)

Converts a value to a long value.

```
var a = Long.from(1.2);
print(a); // => 1
```

### Float.from(val)

Converts a value to a float value.

```
var a = Float.from("1.2");
print(a); // => 1.2
```

### Double.from(val)

Converts a value to a double value.

```
var a = Double.from("1.2");
print(a); // => 1.2
```

### String.from(val)

Converts a value to a string value.

```
var a = String.from(1.2);
print(a); // => "1.2"
```

### String.charCount(s)

Returns the number of the Unicode characters in a string value.

```
var count1 = String.charCount("ABC");
print(count1); // => 3

var count2 = String.charCount("文ABC");
print(count1); // => 4
```

### String.charAt(s, index)

Returns the character at the index in a string.
The character is returned as a string.

```
var c = String.charAt("ABC", 1);
print(c); // => "B"
```

### String.substring(s, start, len)

Returns a substring.

```
var s1 = String.substring("ABC", 0, 1);
print(s1); // => "A"

var s2 = String.substring("ABC", 1, 2);
print(s2); // => "BC"
```

### String.indexOf(s1, s2)

Search a substring.

```
var index1 = String.indexOf("ABCDEF", "CD");
print(index1); // => 2

var index2 = String.indexOf("ABCDEF", "DC");
print(index2); // => -1
```

### Array.make(size)

Make a new array with an initial size.

```
var a = Array.make(128);
print(Array.size(a)); // => 128
```

### Array.size(arr)

Returns the size of an array.

```
var a = Array.make(128);
print(Array.size(a)); // => 128
```

### Array.push(arr, val)

Adds an element to the tail of an array.

```
var arr = [1, 2, 3];
Array.push(arr, 4);
print(arr); // => [1, 2, 3, 4]
```

### Array.pop(arr)

Removes the tail element from an array.

```
var arr = [1, 2, 3];
var v = Array.pop(arr);
print(arr); // => [1, 2]
print(v); // => 3
```

### Array.resize(arr, size)

Makes a resized array.

```
var arr1 = [1, 2, 3, 4, 5];
var arr2 = Array.resize(arr1, 3);
print(arr2); // => [1, 2, 3]
```

### Array.copy(arr)

Makes a copy of an array.

```
var arr1 = [1, 2, 3, 4, 5];
var arr2 = Array.copy(arr1);
print(arr2); // => [1, 2, 3, 4, 5]
```

### Dict.make()

Makes a new dictionary.

```
var d = Dict.make();
```

### Dict.merge(src1, src2)

Merges two dictionaries into a new dictionary.

```
var d1 = {foo: "FOO"};
var d2 = {bar: "BAR"};
var d3 = Dict.merge(src1, src2);
print(d3); // => {bar: "BAR", foo: "FOO"}
```

### Dict.size(dict)

Returns the size of a dictionary.

```
var d = {foo: "FOO", bar: "BAR"};
print(Dict.size(d)); // => 2
```

### Dict.hasKey(dict, key)

Checks whether a key exists in a dictionary.

``
var d = {foo: "FOO", bar: "BAR"};
if (Dict.hasKey(d, "foo"))
    print("foo exists")
```

### Dict.remove(dict, key)

Removes a key from a dictionary.

```
var d = {foo: "FOO", bar: "BAR"};
Dict.remove(d, "foo");
print(d); // => {bar: "BAR"}
```
```

### Dict.copy(dict)

Makes a shallow copy of a dictionary.

```
var d1 = {foo: "FOO", bar: "BAR"};
var d2 = Dict.copy(d1);
```

### Packed.int8(size)

Makes an int8 packed array.

```
var pi8 = Packed.int8(128);
pi8[0] = 0;
```

### Packed.int16(size)

Makes an int16 packed array.

```
var pi16 = Packed.int16(128);
pi16[0] = 0;
```

### Packed.int32(size)

Makes an int32 packed array.

```
var pi32 = Packed.int32(128);
pi32[0] = 0;
```

### Packed.int64(size)

Makes an int64 packed array.

```
var pi64 = Packed.int64(128);
pi64[0] = 0;
```

### Packed.uint8(size)

Makes an int8 packed array.

```
var pu8 = Packed.uint8(128);
pu8[0] = 0;
```

### Packed.uint16(size)

Makes an uint16 packed array.

```
var pu16 = Packed.uint16(128);
pu16[0] = 0;
```

### Packed.uint32(size)

Makes an uint32 packed array.

```
var pu32 = Packed.uint32(128);
pu32[0] = 0;
```

### Packed.uint64(size)

Makes an uint64 packed array.

```
var pu64 = Packed.uint64(128);
pu64[0] = 0;
```

### Packed.float32(size)

Makes a float32 packed array.

```
var pf32 = Packed.float32(128);
pf32[0] = 0;
```

### Packed.float64(size)

Makes a float64 packed array.

```
var pf64 = Packed.float64(128);
pf64[0] = 0;
```

### Packed.size(packed)

Returns the element count of a packed array.

```
var pi8 = Packed.int8(128);
print(Packed.size(pi8)); // => 128
```

### Packed.type(packed)

Returns the element type of a packed array.

```
var pi8 = Packed.int8(128);
print(Packed.type(pi8)); // => "int8"
```

### Math.abs(x)

Gets an absolute value.

```
var a = abs(-1);
print(a); // => 1
```

### Math.random()

Gets a float random value. (0.0 to 1.0)

```
var r = random(); // 0 .. 1.0
```

### Math.sin()

Gets a sin(x) value.

```
var y = sin(x);
```

### Math.cos()

Gets a cos(x) value.

```
var y = cos(x);
```

### Math.tan()

Gets a tan(x) value.

```
var y = tan(x);
```

### Global.hasVariable(name)

Checks whether a global variable exists.

```
globalVar1 = 1;
if (Global.hasVariable("globalVar1")
    print("globalVar1 exists");
```

### GC.youngGC()

Executes a young GC.

```
GC.youngGC();
```

### GC.oldGC()

Executes an old GC.

```
GC.youngGC();
```

### GC.compactGC()

Executes a compact GC.

```
GC.compactGC();
```
