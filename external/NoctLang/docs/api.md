API
===

The API is a modular library that allows you to include only the
necessary components in libnoctlang through build options. There are
no mandatory APIs — developers are free to choose which ones to
integrate into their applications. The `noct` command includes all
APIs except those related to the GUI, while the `gnoct` command
includes all APIs, including the GUI ones.

---

## System

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

---

## Math

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

---

## Console

### Console.print()

Prints a text to the console.

```
Console.print("Hello, world!");
```
