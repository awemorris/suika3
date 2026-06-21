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
