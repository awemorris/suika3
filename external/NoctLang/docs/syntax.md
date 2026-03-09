NoctLang Syntax
===============

NoctLang is a simple scripting language.

## Assignments

Variables in NoctLang are dynamically typed and don't require explicit
declaration. The assignment operator (`=`) is used to create and
assign values to variables.

As shown in the example below, NoctLang supports various data types
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
    array->push(0);
    array->push(1);
    array->push(2);

    var last = array->pop();
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

## OOP in NoctLang

The object-oriented model in NoctLang is a lightweight variation of prototype-based OOP.

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

Count characters. A wide character sequence will be counted as one character.

```
var s = "ABC文あいう";
var l = s.charCount();
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

### fast_gc(), full_gc(), compact_gc()

```
// Do young GC.
fast_gc();

// Do young + old GC.
full_gc();

// Do young + old + compact GC.
compact_gc();
```

### unset()

Unset a pair of dictionary key and value.

```
var dic = {key1: "ABC"};
dic->unset("key1");
```
