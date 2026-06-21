🌙 Noct Programming Language
============================

`Noct` is a tiny yet mighty programming language for sandboxed scripting.
Its syntax is lightweight, but its runtime is built for high-end performance.

**Small enough to learn today, powerful enough to ship tomorrow!**

---

## Feature Highlights

### Small, Fast, and Robust

Only about 200 KB — with a fast JIT compiler, a robust generational GC,
and a clean C/JS-like syntax featuring a novel Dictionary-based OOP model.

JIT execution is typically 4-13x faster than interpreter execution.

### Portable

Written in portable ANSI C with no external dependencies,
it runs virtually everywhere — from desktop PCs down to Raspberry Pi.

Remarkably, Noct runs well on
[MS-DOS](https://github.com/awemorris/NoctLang/releases/latest/download/noct-dos.exe),
even with the JIT compiler enabled!

### Embeddable

Noct can be easily embedded into your applications.

By adding your own APIs, you can build a customized scripting runtime,
and you are free to rebrand Noct as your own scripting language.

For example, in
[Playfield Engine](https://github.com/awemorris/PlayfieldEngine),
and
[Suika3](https://github.com/awemorris/suika3),
they integrate Noct with game-specific APIs and refer to it as Ray scripting.

---

## Status

**Stable,** the current version is 1.0.x.

The core virtual machine is completed, and is already being used
through integration with other projects.

- [Playfield Engine](https://github.com/awemorris/PlayfieldEngine)
    - A framework to build custom 2D game engines that run everywhere.

- [Suika3](https://github.com/awemorris/suika3).
    - A visual novel engine for the mobile era.

The primary objective of this project, "embedded sandbox scripting",
has been achieved. However, we are continuing to develop this software
in the hope of finding new applications, e.g.,:

- Declative UI framework for use with C/C++/Rust, inspired by ArkTS.
- Modern scripting host for retro computers.

Our current roadmap is:

- Expanding the standard library with modular, opt-in API components.

---

## Platform Support

### JIT Backends:

- x86, x86_64
- ARMv7, Arm64
- RISC-V 32/64
- PowerPC 32/64
- MIPS 32/64

### Supported OSes:

- Desktop: Windows, macOS, Linux, FreeBSD
- Mobile: iOS, Android, OpenHarmony
- Exotic: Solaris 10/11, NetBSD, OpenBSD, Haiku
- Retro: DPMI (DOS, OS/2, Windows 3.1-XP)
- Consoles: Switch, PlayStation 4/5, Xbox Series X|S
- Any POSIX compliant OS

Note: On major smartphones and consoles, runtime code generation (JIT)
is generally prohibited or tightly restricted by platform
policies. Noct runs there with interpreter or AOT compilation.

### Speedup

A synthetic benchmark shows that our JIT compiler speeds up execution
time by 4.1-13.5 times.

```
func main() {
    var sum = 0;
    for(i in 0..10000) {
        for(j in 0..100000) {
            sum = sum + 1;
        }
    }
}
```

| CPU                     | Arch     | JIT (s) | Interpreter (s) | Scaling (JIT vs Interpreter) |
|-------------------------|----------|---------|-----------------|------------------------------|
| PowerPC 970FX           | ppc64    | 29.47   | 397.22          | 13.5x                        |
| Ingenic JZ4770          | mips32   | 129.75  | 1447.36         | 11.2x                        |
| Intel Core Ultra 5 228V | x86_64   | 5.93    | 34.95           | 5.9x                         |
| Apple M5                | arm64    | 2.76    | 11.34           | 4.1x                         |

---

## Core Design & Features

Noct combines simplicity, speed, and portability — traits rarely
found together in scripting languages:

- **Familiar Syntax** — C/JS-like and easy to learn.
- **Lightweight JIT** — Fast execution in a tiny runtime.
- **Generational GC** — Young semi-space copying + old mark-sweep-compact.
- **Portable ANSI C** — No dependencies; runs everywhere.
- **Tiny Footprint** — Runtime fits in ~200 KB.
- **AOT Compilation** — Translate to C for JIT-restricted platforms. (e.g. iOS, Android)

While most languages compromise on at least one of these,  
Noct delivers all without sacrificing clarity or speed.

In addition, Noct introduces **Dictionary-based OOP (D-OOP)** — a
novel paradigm for object-oriented programming, achieved through
dictionary merging instead of prototype chains or heavyweight class
hierarchies. Thanks to this design, Noct programs can even be
translated into Emacs Lisp code, reflecting the language's
Lisp-inspired roots.

---

## Why Noct?

_"What if a programming language could be learned in a single
afternoon — and used the next day to create real games?"_

Noct was born from this question: a desire to create a language
that's minimal yet meaningful — **simple enough for beginners, fast
enough for production**.  It bridges the gap between **play** and
**production**, letting you focus on making games, not fighting tools.

At the same time, Noct brings **commercial-grade VM technology** —
once limited to large industrial runtimes — into a form small enough
for game projects, built on proven techniques from engines like Java
and .NET.

---

## Try it!

### Your First Program

Noct is simple enough to try right now — no setup, no hassle.

Save the following as `first.noct`, and just run `noct first.noct`.

Source:
```
func main() {
    Person = class {
        name: ""
    };

    var jessie = new Person { name: "Jessie" };
    var tom = new Person { name: "Tom" };

    var people = [jessie, tom];
    for (person in people) {
        print("Hello, " + person.name + "!");
    }
}
```

Run:
```
$ noct first.noct
```

Output:
```
Hello, Jessie!
Hello, Tom!
```

That's it. You've written your first Noct program.

---

## Installation

### Download Prebuild Binaries

Visit
[the release page](https://github.com/awemorris/NoctLang/releases)
to obtain the latest prebuilt binaries.

### Manually Build from Source

Clone the repository, build it with CMake, and you're ready to go:

```
git clone https://github.com/awemorris/NoctLang.git
cd NoctLang
cmake --preset static
cd build-static
make
make install
```

### Run

To run a script:
```
noct script.noct
```

To disable the JIT compiler:
```
noct --disable-jit script.noct
```

To forcibly enable the JIT compiler from the startup:
```
noct --force-jit script.noct
```

### Compile into Bytecode

To compile a script into a bytecode file:

```
noct --compile script.nb script.noct
```

### Compile into Emacs Lisp

To compile a script into an Emacs Lisp file:

```
noct --elisp script.el script.noct
```

### JIT Option

```
  --jit-threshold=N    ... call-count threshold for compilation
```

### Garbage Collection Options

```
  --gc-nursery-size=N  ... first GC space size in bytes (default: 2MB = 2097152)
  --gc-graduate-size=N ... second GC space size in bytes (default: 256KB = 262144)
  --gc-tenure-size=N   ... final GC space size in bytes (default: 256MB = 268435456)
  --gc-lop-threshold=N ... move objects larger than N-bytes to final GC space (default: 32KB = 32768)
```

---

## Examples

Noct programs consist of functions, expressions, and control structures
similar to C and JavaScript. The `main` function is the entry point.

### Arrays

```
func main() {
    var array = [1, 2, 3];
    for (v in array) {
        print(v);
    }
}
```

### Dictionaries

```
func main() {
    var dict = {name: "Apple", price: 100};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

### Lambda Functions

```
func main() {
    // Lambda notation.
    var f = (x) => { return x + 1; }
    print(f(1));

    // No closures. Use the 'with' argument explicitly.
    var g = (x, with) => {
        return x + with.y;
    };
    var y = 2;
    var z = g(1, {y: y});
}
```

### Typed Array (Packed)

```
func main() {
    // Allocate uint8.
    // (Available Types: int8/int16/int32/int64/uint8/uint16/uint32/uint64/float32/float64)
    var a = Packed.uint8(1024);

    // Access via [] notation.
    for(i in 0..Packed.size(a))
        a[i] = i;
}
```

### File I/O

```
func main() {
    // Read the entire text.
    var text = File.readText("text.txt");

    // Read lines.
    File.readForEachLine("text.txt", (line) => { print(line); });
}
```

```
func main() {
    // Write a string.
    File.writeText("text.txt", "aaa\nbbb");

    // Write lines.
    File.writeForEachLine("text.txt", ["aaa", "bbb"]);
}
```

### Object-Oriented Model

The object-oriented model in Noct is a lightweight variation of prototype-based OOP.

- Classes are simply dictionary templates
- Inheritance and instantiation are realized by dictionary copying and merging
- There is no prototype chain, and modifying a class does not affect existing instances

This design treats dictionaries as first-class objects, and the author refers to it as Dictionary-based OOP (D-OOP).

```
func main() {
    // Base class definition. (A class is just a dictionary.)
    Animal = class {
        name: "Animal",
        cry: (this) => {
        }
    };

    // Subclass definition. (This is just a dictionary merging.)
    Cat = extend Animal {
        name: "Cat",
        voice: "meow",
        cry: (this) => {
            print(this.name + " cries like " + this.voice);
        }
    };

    // Instance generation. (This is just a dictionary merging.)
    var myCat = new Cat {
        voice: "neee"
    };

    // This-call uses the "-> ()" syntax. (Equal to myCat.cry(myCat))
    myCat->cry();
}
```

### Why D-OOP?

- Moving Beyond "Implementation Inheritance":
    - Traditional class-based inheritance is increasingly viewed as outdated,
      with modern languages like Rust proving that composition is often a
      superior alternative.
    - Noct embraces this shift by focusing on the "composition of dictionaries"
      rather than maintaining complex, dynamic inheritance hierarchies.
- Eliminating Prototype Chains for `O(1)` Access:
    - In conventional prototype-based languages, resolving properties through a
      chain is a high-overhead operation that frequently triggers cache
      misses, requiring heavy optimizations like Inline Caching.
    - Noct eliminates the prototype chain entirely.
    - By merging dictionaries at the time of creation, it achieves guaranteed
      `O(1)` access to all properties.
- Memory Locality and Hardware-Aware Design:
    - By creating flattened copies of dictionaries, Noct ensures superior
      memory locality.
    - This approach is specifically designed to align with modern hardware
      architectures, maximizing CPU cache hit rates and ensuring high affinity
      with NUMA (Non-Uniform Memory Access) environments.

---

## JIT Pipeline

### Intermediate Representations

Noct employs two distinct intermediate representations (IRs) to
balance high-level program analysis with efficient execution:

- **HIR (High-level Intermediate Representation)**
    - Structured control flow graph (CFG) for program analysis.
    - Expression DAG for algebraic simplification.
    - Basis for future advanced optimizations.

```
CFG for "func foo(a) { if (a > 0) { return a; } else { return -a; } }"

  +---------------+
  | 0: Func Block |         -- pred: none, succ: 1
  +---------------+
     +-------------+
     | 1: IF Block |        -- pred: 0, succ: 2 (true), 3 (false)
     +-------------+
        +----------------+
        | 2: Basic Block |  -- pred 1, succ 5
        +----------------+
     +---------------+
     | 3: Else Block |      -- pred 1, succ 4
     +---------------+
        +----------------+
        | 4: Basic Block |  -- pred 3, succ 5
        +----------------+
  +--------------+
  | 5: End Block |          -- pred 2, 4
  +--------------+
                                 (pred = predecessor, succ = successor)
     
```

```
  DAG for "a = 1 + 2"


     LHS   ---- ASSIGN  ----   RHS
      |                         |
     term                      ADD
      |                        / \
   symbol a                 term  term
                             |       |
                           int 1   int 2
```

- **LIR (Low-level Intermediate Representation)**
    - VM bytecode, serving as the primary format for both interpretation and JIT codegen input.
    - High abstraction level to achieve fast, portable interpretation.
    - Compact enough for efficient machine code lowering in the JIT.

```
  LIR for "a = 1 + 2"

    ICONST       %0, 1               ; Load constant 1
    ICONST       %1, 2               ; Load constant 2
    ADD          %2, %0, %1          ; Compute sum
    STORESYMBOL  "a", %2             ; Store result into global variable "a"
```

### Compilation Pipeline

```
 +-----+     +-----+     +-----+     +-----+
 | SRC | --> | AST | --> | HIR | --> | LIR | ----> <<Interpreter>> Crescente Interpreter Backend
 +-----+     +-----+     +-----+     +-----+
                                        |
                                        +--------> <<JIT Codegen>> Piena JIT Backend
                                        |
                                        +--------> <<AOT Codegen>> Nuova C AOT Backend

```

- The AST captures the syntactic structure.
- The HIR provides an analyzable, optimization-friendly form.
- The LIR bridges execution, serving both the interpreter and JIT.

### Design Rationale

The separation of HIR and LIR enables:

- **A lightweight JIT pipeline**: minimal overhead from analysis to code generation.
- **Clarity in architecture**: each stage has a well-defined role, simplifying maintenance.
- **Portability**: the same LIR can be interpreted directly or lowered into optimized machine code.

As shown above, HIR expresses structure, while LIR expresses execution.
This split allows Noct to keep the JIT pipeline lightweight without sacrificing optimization opportunities.

Because all JIT backends translate from the same LIR, portability across architectures comes naturally.
This unified approach is what makes Noct both portable and maintainable.

---

## Native API

The Noct runtime can be embedded in C applications. This allows you to
load, compile, and execute scripts and bytecode directly within your
software.

```
void call_noct(const char *file_name, const char *file_text)
{
    // Create a VM.
    NoctVM *vm;
    NoctEnv *env;
    noct_create_vm(&vm, &env);

    // Compile source.
    noct_register_source(env, file_name, file_text);

    // Call the main() function with no arguments.
    NoctValue ret = NOCT_ZERO;
    noct_enter_vm(env, "main", 0, NULL, &ret);

    // Destroy the runtime.
    noct_destroy_vm(vm);
}
```

This API requires linking against the Noct runtime and including the
appropriate header (`noct/noct.h`).

Error handling and result introspection are left to the host
application, giving full control over integration.

For more details, see [the Native API document](docs/napi.md).

---

## Test and CI

Noct is tested on Windows, macOS, Linux, and FreeBSD.

Our CI is running on GitHub Actions. Each push to the main branch
triggers builds and binary releases, ensuring stability across
supported platforms.

### Testing Machines and Environments

| Vendor          | Machine                    | Processor               | Architecture        | ABI             | OS                     | JIT     |
|-----------------|----------------------------|-------------------------|---------------------|-----------------|------------------------|---------|
| Apple           | MacBook Pro M5             | Apple M5                | Armv9               | arm64           | macOS 26               | OK      |
|                 | MacBook Air                | Intel Core i5           | Intel64             | x86_64          | macOS 10.13            | OK      |
|                 | PowerMac G5                | PowerPC G5 970FX        | PowerPC 64          | ppc64 (ELFv1)   | Debian forky           | OK      |
|                 |                            |                         |                     | ppc32           | Debian forky           | OK      |
| IBM             | Power8 S814                | Power8                  | Power8              | ppc64le (ELFv2) | Debian trixie          | OK      |
| Fujitsu         | Primergy TX2550 M5         | Intel Xeon Gold 6130 x2 | Intel64 Skylake-SP  | x86_64          | Debian trixie          | OK      |
|                 |                            |                         |                     | x86             | Debian trixie          | OK      |
| Lenovo          | ThinkPad X9                | Intel Core Ultra 5 228V | Intel64 Lunar-Lake  | x64 (MS ABI)    | Windows 11             | OK      |
|                 |                            |                         |                     | x86_64          | Debian trixie          | OK      |
|                 | ThinkPad X260              | Intel Core i5-6500U     | Intel64 Skylake     | x86_64          | Solaris 11             | OK      |
| Panasonic       | Let's Note LX6             | Intel Core i5-7200U     | Intel64 Kaby Lake   | x86_64          | Haiku                  | OK      |
|                 | Let's Note SV8             | Intel Core i5-8250U     | Intel64 Coffee Lake | x86_64          | Windows 11             | OK      |
| Dell            | Latitude 5330              | Intel Core i5-1240P     | Intel64 Alder Lake  | x86_64          | Debian trixie          | OK      |
|                 | Latitude 5230              | Intel Core i5-1135G7    | Intel64 Tiger Lake  | x86_64          | FreeBSD 15             | OK      |
| RPi             | Raspberry Pi 4B            | Broadcom BCM2711        | Arm Cortex-A72      | aarch64         | Raspberry Pi OS 64-bit | OK      |
|                 |                            |                         |                     | armv7-eabihf    | Raspberry Pi OS 32-bit | OK      |
|                 | Raspberry Pi Zero 2        | Broadcom RP3A0          | Arm Cortex-A53      | aarch64         | Raspberry Pi OS 64-bit | OK      |
|                 |                            |                         |                     | armv7-eabihf    | Raspberry Pi OS 32-bit | OK      |
| Anbernic        | RG350                      | Ingenic JZ4770          | MIPS32r2            | mipsel          | Debian forky           | OK      |
| Cobalt Networks | Cobalt Qube 2800           | QED RM5231              | MIPS-IV             | mipseb          | NetBSD                 | OK      |
| StarFive        | VisionFive 2               | JH7110                  | RISC-V RV64GC       | riscv64         | Debian Trixie          | OK      |
| qemu            | qemu                       | RISC-V 32               | RV32GC              | riscv32         | qemu-user              | OK      |
|                 |                            | 68040                   | 680x0               | m68k            | qemu-user              | N/A     |

Now preparing...

| Vendor          | Machine                    | Processor               | Architecture        | ABI             | OS                     |
|-----------------|----------------------------|-------------------------|---------------------|-----------------|------------------------|
| Apple           | Macintosh LC630            | Motorola 68040          | 680x0               | m68k            | Debian forky           |
| Fujitsu         | SPARC Enterprise M3000     | SPARC64 VII+            | SPARC V9            | sparc64         | Solaris 10             |
| Imagination     | MIPS Creator CI 40         | MIPS interAptive        | MIPS32r3            | mips            | Debian forky           |
| Generic         | Loongson PC                | Loongson 3A4000         | MIPS64              | mips64          | Debian forky           |

---

## Documentation

- [Syntax](docs/syntax.md)
- [Library](docs/library.md)
- [Native API](docs/napi.md)
- [Virtual Machine Specification](docs/vmspec.md)

---

## License

Noct is free/libre software, released under the permissive zlib license.

This means you can use it freely — for personal, educational, or
commercial purposes.  You're also free to modify, redistribute, and
build upon it, with minimal restrictions.

---

## Contributing

While the core sandboxed virtual machine of Noct is already stable,
the Standard API is under active development. We welcome all kinds of
contributions — bug fixes, examples, documentation, ideas, or new
features.

We're also building the broader `NoctVM` family, including a game
engine designed to empower creators.

Whether you're here to code, teach, test, or explore — we'd love to
have you with us.
