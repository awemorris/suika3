API
===

Noct has an intrinsic API and a standard API.

The intrinsics is included in the `libnoct` library and can be used
for any build configurations.

The standard API is usable only when it is enabled in the build
configuration. It is implemented in a modular library called
`libnoctapi`, separated with the core `libnoct` library. The
separation allows you to include only the core components or the
entire API in your binary through build options.

Tthe `noct` CLI command includes the standard APIs.

---

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

---

## File

`File.*` is a standard API and not included in the intrinsics.

### File.readText(file)

Reads a text file as a string.

```
var text = File.readText("text.txt");
print(text);
```

### File.writeText(file, text)

Writes a string to a text file.

```
var text = "abc";
File.writeText("text.txt", text);
```

### File.readForEachLine(file, func)

Reads lines from a text file.

```
File.readForEachLine("text.txt", (line) => {
    print(line);
});
```

### File.writeForEachLine(file, lines)

Write lines to a text file.

```
File.writeForEachLine("text.txt", [
    "aaa",
    "bbb",
    "ccc"
]);
```

### File.checkFileExists()

Checks whether a file exists.

```
if (File.checkFileExists("text.txt"))
    print("File exists!")
```

---

## System

`System.*` is a standard API and not included in the intrinsics.

### System.import()

Imports a script file or a bytecode file.

```
System.import("script.noct");
```

### System.shell()

Runs a shell command.

```
System.shell("ls -lha");
```

### System.runCommand(command, workDir, waitForFinish)

### System.getOSName()

---

## Console

`Console.*` is a standard API and not included in the intrinsics.

### Console.print()

Prints a text to the console.

```
Console.print("Hello, world!");
```
