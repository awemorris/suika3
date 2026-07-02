Standard API
============

The Standard API is enabled only when a build option is specified. It
is implemented in a modular library called `libnoctapi`, separated
with the core `libnoct` library. The separation allows you to include
only the core components or the entire API in your binary through
build options.

The `noct` CLI command includes the standard APIs.

---

## File

`File.*` is a standard API and not included in the intrinsics.

### File.open(path, mode)

Opens a file.

```
var file = File.open("test.bin", "r");
var data = File.read(file, 100);
for (i in 0..100)
    print(data[i]);
File.close(file);
```

### File.close(file)

Closes a file.

```
var file = File.open("test.bin", "r");
var data = File.read(file, 100);
File.close(file);
```

### File.tell(file)

Gets the position in a file.

```
var file = File.open("test.bin", "r");
var offset = File.tell(file);
```

### File.seek(file, offset)

Moves to an offset in a file.

```
var file = File.open("test.bin", "r");
File.seek(file, 100);
```

### File.read(file, len)

Reads bytes and returns a UInt8 packed array.

```
var file = File.open("test.bin", "r");
var data = File.read(file, 100);
for (i in 0..100)
    print(data[i]);
File.close(file);
```

### File.write(file, data, offset, len)

Write bytes.

```
var data = Packed.uint8(100);
for (i in 0..100)
    data[i] = i;

var file = File.open("test.bin", "w");
File.write(file, data, 0, Packed.size(data));
File.close(file);
```

---

## FileUtil

`FileUtil.*` is a standard API and not included in the intrinsics.

### FileUtil.checkFileExists()

Checks whether a file exists.

```
if (File.checkFileExists("text.txt"))
    print("File exists!")
```

### FileUtil.readText(file)

Reads a text file as a string.

```
var text = File.readText("text.txt");
print(text);
```

### FileUtil.writeText(file, text)

Writes a string to a text file.

```
var text = "abc";
File.writeText("text.txt", text);
```

### FileUtil.readForEachLine(file, func)

Reads lines from a text file.

```
File.readForEachLine("text.txt", (line) => {
    print(line);
});
```

### FileUtil.writeForEachLine(file, lines)

Write lines to a text file.

```
File.writeForEachLine("text.txt", [
    "aaa",
    "bbb",
    "ccc"
]);
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
