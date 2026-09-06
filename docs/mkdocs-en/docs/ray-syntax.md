Ray Syntax Reference
====================

`Ray` is actually the `Noct` programming language with additional Suika3 APIs.

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

    let b = 1.0;   // let is also okay!
    print(b);

    var c = "string";
    print(c);
}
```

## Global Variables

Global variables can be defined outside functions.

```
var globalVariable = 123;

func main() {
    print(globalVariable);
}
```

## Local Variables

Using the `var` or `let` keywords allows you to declare a variable as
local. Without `var` declaration, assigning to a variable may create a
global variable.

```
func main() {
    var a = 123;
    print(a);
}
```

## Type Annotations

Noct remains dynamically typed, but function parameters, local
`var`/`let` declarations, and function returns may carry
optimization-oriented type annotations:

```noct
func copy_words(dst: rpackeduint32, src: rpackeduint32, count: int): long {
    var copied: long = count;
    return copied;
}
```

The recognized scalar and object annotations are:

- `int`, `long`, `float`, and `double` for the four numeric runtime
  types,
- `string`, `array`, `dict`, `packed`, and `func` for the
  corresponding runtime value types, and
- `i8`, `i16`, `i32`, `u8`, `u16`, and `u32` as `int` annotations,
  plus `i64` and `u64` as `long` annotations.

The sized integer spellings do not create narrow runtime integer
values and do not perform range checks; they are aliases for the
corresponding `int` or `long` runtime type check.

Element-specific Packed annotations are `packedint8`, `packeduint8`,
`packedint16`, `packeduint16`, `packedint32`, `packeduint32`,
`packedint64`, `packeduint64`, `packedfloat` (float32 elements), and
`packeddouble` (float64 elements).  The plain `packed` annotation
accepts any Packed element type.  Prefixing an element-specific name
with `r` produces a restricted parameter annotation, for example
`rpackeduint8` or `rpackedfloat`.

In an ordinary function, annotations have the following behavior:

- At optimization level 0, annotation names and placement are
  validated, but values retain ordinary dynamic behavior.
- At level 1 and above, every annotated parameter is checked on
  function entry.  An element-specific Packed parameter must have
  exactly that element type.
- At level 1 and above, a local declared as `int`, `long`, `float`, or
  `double` is checked at its initializer and every reassignment.  The
  other local annotations remain optimization metadata and do not
  impose this reassignment check.
- At level 2 and above, an annotated return value is checked exactly.
  A restricted Packed annotation is not valid as a return type; `void`
  is used for a function that returns no value.

Parameter and local checks permit `int` where `long` is requested and
`float` where `double` is requested, converting the value to the wider
runtime type.  Return checks do not perform these conversions.

The `r` prefix is a non-aliasing contract supplied by the caller.
While the call is active, storage accessed through one restricted
parameter must not overlap storage accessed through another parameter.
The runtime does not compare backing ranges, and the optimizer may
rely on the contract without proving it.  Violating the contract has
unspecified results.

Shape syntax such as `rpackedfloat(rows, columns)` is reserved for
`__fast` parameters and is described below.  Unknown annotation names
and shaped types in other contexts are compile errors at every
optimization level.

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
// The base class definition. (A class is just a dictionary.)
let Animal = class {
    name: "Animal",
    cry: (this) => {
    }
};

// The subclass definition. (Just a dictionary merging.)
let Cat = extend Animal {
    name: "Cat",
    voice: "meow",
    cry: (this) => {
        print(this.name + " cries like " + this.voice);
    }
};

func main() {
    // Instantiation. (Just a dictionary merging.)
    var myCat = new Cat {
        voice: "neee"
    };

    // This-call uses -> () syntax. (Equal to myCat.cry(myCat))
    myCat->cry();
}
```


## `__fast func`

`__fast` is an opt-in contract for statically typed CPU optimization,
inspired by Fortran 77.

The two accepted declaration forms are:

```noct
__fast func calculate(value: int): int {
    return value * 2;
}

static __inline __fast func convert(value: int): float {
    return float(value);
}
```

The `static __inline` form is file-local.  It is accepted as an
optimization declaration, but current fast-to-fast calls are not
source-inlined merely because `__inline` is present.

At optimization level 1 and above in a build with optimizer support, a
fast function must satisfy all of these signature rules:

- every parameter has an exact annotation;
- scalar parameters use exactly `int`, `long`, `float`, or `double`;
- every explicit `var` or `let` local uses exactly one of those four
  scalar annotations;
- the return annotation is exactly one of those four types or `void`;
  and
- a Packed parameter uses an element-specific `rpacked*` annotation
  with an exact shape.

The sized integer aliases are not exact fast scalar spellings.  Fast
arithmetic does not implicitly widen or otherwise mix scalar types;
use the `int`, `long`, `float`, and `double` conversion intrinsics
where a conversion is required.

Packed parameters support all element-specific restricted types
described above.  A shape has 1 through 8 dimensions, and every extent
is either a positive decimal integer literal or the name of an
`int`/`long` parameter:

```noct
__fast func scale(
    image: rpackedfloat(channels, 224, 224),
    channels: int,
    factor: float): void
{
    for (c in 0..channels) {
        for (y in 0..224) {
            for (x in 0..224) {
                image[c, y, x] = image[c, y, x] * factor;
            }
        }
    }
}
```

The shape is exact: `rpackedfloat(3, 224, 224)` requires exactly `3 *
224 * 224` elements.  Multi-dimensional indices are zero-based and use
C row-major order, with the last axis contiguous.  Calls check
annotated primitive runtime types, Packed element types, positive
dynamic extents, shape-product overflow, and the exact element count.
The checked CPU path uses the ordinary `int`-to-`long` and
`float`-to-`double` widening rules.  Each index is bounds-checked on
that path.  These checks also apply at optimization level 0 and in
builds without the optimizer.  The `rpacked` non-aliasing promise
remains a caller contract and is not checked at runtime.

When the optimizer commits the fast contract at level 1 or above, the
function body is limited to statically typed numeric operations:

- numeric literals, typed parameters, typed locals, and shaped Packed
  parameter access,
- `if`/`else if`/`else`, `while`, and ranged `for` control flow,
- `int` conditions, matching `int` or `long` ranged-loop bounds, and
  `int` or `long` Packed indices,
- direct calls to other fast functions, and
- `min`, `max`, `abs`, `sqrt`, `sin`, `cos`, `tan`, `asin`, `acos`,
  `atan`, `atan2`, `exp`, `ln`, `log2`, `log10`, `int`, `long`,
  `float`, and `double` intrinsics.

Globals, closures, methods, ordinary function calls, and for-each
loops are not part of the optimized subset.  Operands and assignment
values must match their exact scalar or Packed element type.  Every
reachable path of a non-`void` function must return a value, and
direct or mutual recursion between fast functions is rejected.

At level 0, or in a build without optimizer support, `__fast` uses the
ordinary checked CPU lowering.  Annotated argument, Packed element,
and exact shape checks remain active, but the optimizer-only body
validation and fast bytecode metadata are not committed.  At level 1
and above, a provably safe multi-dimensional access may be flattened
to unchecked row-major arithmetic, a provably out-of-range access is a
compile error, and an unproven access remains checked.  Exact shape
and non-aliasing facts are then available to later ABCE and SIMD
passes.

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

Searches for a substring and returns the **character index** of the
first match, or -1 if there is none. The index is in characters, the
same unit `String.charAt()` and `String.substring()` use, so the three
combine correctly on multibyte text.

```
var index1 = String.indexOf("ABCDEF", "CD");
print(index1); // => 2

var index2 = String.indexOf("ABCDEF", "DC");
print(index2); // => -1

var s = "あいu";
print(String.indexOf(s, "u"));              // => 2, not the byte offset 6
print(String.substring(s, String.indexOf(s, "い"), 1)); // => "い"
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

### Packed.copy(dst, dstIndex, src, srcIndex, count)

Copies `count` elements from `src` to `dst` and returns `count`.
Indices and the count are in elements, the same unit `Packed.size()`
and the `[]` notation use.

Both arrays must hold the same element type. The two regions may
overlap, so this also serves to move a block inside one array, as a
gap buffer does.

```
var src = Packed.uint8(8);
var dst = Packed.uint8(8);
Packed.copy(dst, 2, src, 0, 4);

// Slide a block down by two, over itself.
Packed.copy(src, 0, src, 2, 6);
```

### Packed.fill(dst, index, count, value)

Sets `count` elements of `dst` to `value`, starting at `index`, and
returns `count`. The value is converted to the array's element type.

```
var buf = Packed.uint8(1024);
Packed.fill(buf, 0, 1024, 0);
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

### Type.of(value)

Gets the type of a value as a string. The result is one of `"int"`,
`"long"`, `"float"`, `"double"`, `"string"`, `"array"`, `"dict"`,
`"packed"` or `"func"`.

```
print(Type.of(1));         // int
print(Type.of("s"));       // string
print(Type.of([1, 2]));    // array
print(Type.of({a: 1}));    // dict
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
