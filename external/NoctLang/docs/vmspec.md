NoctVM Specification
====================

## Table of Contents

1. [Virtual Machine Value Model](#1-virtual-machine-value-model)  
2. [Variables](#2-variables)  
3. [Bytecode File Format](#3-bytecode-file-format)  
4. [Bytecode Overview](#4-bytecode-overview)  
5. [Instruction Semantics](#5-instruction-semantics)  
   - [5.1 NOP](#51-nop)  
   - [5.2 Assignment and Constant Loading](#52-assignment-and-constant-loading)  
   - [5.3 Arithmetic and Logical Operations](#53-arithmetic-and-logical-operations)  
   - [5.4 Comparison Instructions](#54-comparison-instructions)  
   - [5.5 Array and Dictionary Instructions](#55-array-and-dictionary-instructions)  
   - [5.6 Field Access Instructions](#56-field-access-instructions)  
   - [5.7 Global Symbol Instructions](#57-global-symbol-instructions)  
6. [Control Flow Instructions](#6-control-flow-instructions)  
7. [Debug Instructions](#7-debug-instructions)  
8. [Error Model](#8-error-model)  
9. [Future Extensions](#9-future-extensions)  
10. [Appendix: Opcode List](#10-appendix-opcode-list)

---

# 1. Virtual Machine Value Model

The Noct Virtual Machine (VM) defines six core value types that can be
manipulated during execution. Each value is classified as either a
**primitive** or a **reference** type. This classification affects how
the value is stored, copied, and passed within the VM.

| Type               | Description                      | Category   |
|--------------------|----------------------------------|------------|
| `NOCT_VALUE_INT`   | 32-bit signed integer            | Primitive  |
| `NOCT_VALUE_FLOAT` | 32-bit IEEE 754 floating-point   | Primitive  |
| `NOCT_VALUE_STRING`| UTF-8 encoded string             | Reference  |
| `NOCT_VALUE_ARRAY` | Dynamically-sized array          | Reference  |
| `NOCT_VALUE_DICT`  | Dictionary (key-value map)       | Reference  |
| `NOCT_VALUE_FUNC`  | Function object                  | Reference  |

## 1.1 Primitive Types

Primitive types (`NOCT_VALUE_INT` and `NOCT_VALUE_FLOAT`) are stored
directly by value. Assigning a primitive variable copies the
underlying data into the target.

> **Example:**  
> If `B` holds a primitive value, the assignment `A = B;` results in
> `A` holding an independent copy of the value from `B`. Modifying `A`
> afterward has no effect on `B`.

## 1.2 Reference Types

Reference types (`NOCT_VALUE_STRING`, `NOCT_VALUE_ARRAY`,
`NOCT_VALUE_DICT`, and `NOCT_VALUE_FUNC`) are stored as pointers to
heap-allocated objects. Assigning a reference variable copies the
reference, not the actual object.

> **Example:**  
> If `B` holds a reference, the assignment `A = B;` causes both `A`
> and `B` to point to the same underlying object. Changes made through
> one reference are reflected in the other.

## 1.3 Arrays

Arrays are ordered collections of values. Noct arrays are:

- **Dynamically sized**: they grow automatically when indexed beyond  
  the current size.
- **Untyped**: each element may hold a value of any VM-supported type.

Arrays support:

- Indexed access and assignment (`arr[i] = value`)
- Automatic expansion to accommodate large indices
- Mutation of existing elements

Because arrays are reference types, modifications are visible across
all references to the same array instance.

> **Note:**  
> Newly added elements via automatic expansion are initialized to the
> integer value `0`.

## 1.4 Dictionaries

Dictionaries are mutable key-value maps. Keys must be **strings**, and
values can be of **any NoctVM-supported type**, including both
primitive and reference types.

Dictionaries support:

- Insertion, lookup, and mutation of values by string key
- Arbitrary mixing of value types across keys
- Dynamic growth as new keys are added

Because dictionaries are reference types, assignments only copy the
reference. All references to the same dictionary instance share the
same underlying state.

> **Example:**  
> If `A = B;` and `B` is a dictionary, then `A["foo"] = 42;` will also
> be visible via `B["foo"]`.

## 1.5 Functions

A function is the minimal executable unit within the NoctVM. Function
objects are first-class values and may be passed, stored, and invoked
dynamically.

Each function object contains:

- An **internal name** (used for debugging and introspection only)
- A **bytecode array** representing the compiled instruction sequence
- A **parameter count**
- A list of **parameter names** (for debugging purposes)
- A **temporary variable count**, including parameters

Function names are **not** used for resolution or invocation. Instead,
functions are typically **bound to global variables** with matching
names, allowing them to be invoked through global symbol lookup.

Because functions are reference types, assigning a function value
copies the reference, not the function body. Multiple references can
point to the same function object.

> **Example:**  
> Functions can be stored in arrays, passed to other functions, or
> reassigned to different global symbols.

---

# 2. Variables

The VM defines multiple classes of variables, each with distinct
lifetimes and scopes. All variables store typed VM values and are
accessed either by **index** (for temporary variables) or by
**symbolic name** (for global variables).

## 2.1 Temporary Variables

Temporary variables are register-like storage slots used internally
within a function. They are:

- Indexed numerically
- Allocated per function invocation
- Used as operands for most bytecode instructions

Temporary variables are **not accessible from scripts**, and are
entirely managed by the compiler and runtime. Their lifetime is
limited to the duration of a single function call.

The number of temporary variables is fixed at compile time and
determined per function. The VM follows a consistent layout
convention:

- **Argument values** are stored in the lower-numbered temporary slots  
  (starting at index `0`).
  - Index `0` is also used to return a function’s result to the caller.
- **Lexical local variables** declared inside the function body are  
  assigned to the next available slots.
- **Intermediate results** from expression evaluation are stored in  
  higher-numbered slots.

This layout allows for efficient execution and static register
allocation without the need for dynamic name resolution or stack
manipulation during runtime.

## 2.2 Global Variables

Global variables bind symbolic names to VM values that persist across
all function call frames. They are resolved by name at runtime using
the VM's global symbol table.

Global variables provide a persistent, shared runtime context. Typical
use cases include:

- Built-in functions and runtime services
- Language-level global constants or state
- Application-wide shared configuration or singletons

Because global variables are reference bindings, changes made to their
values are immediately visible throughout the program unless shadowed
by local variables (if any).

---

# 3. Bytecode File Format

A Noct bytecode file is a **text-based**, line-oriented format that
encodes one or more function definitions. Each function is compiled
separately and is automatically bound to a global variable with the
same name as the function upon loading. This enables runtime lookup
and invocation via the global symbol table.

## 3.1 Format Overview

The file begins with a version header and source information, followed
by one or more function blocks. Each function includes metadata and a
binary bytecode payload.

```
Noct Bytecode 1.0
Source
<FILE NAME>
Number Of Functions
<FUNCTION COUNT>
Begin Function
Name
<FUNCTION NAME>
Parameters
<PARAMETER COUNT>
<PARAMETER NAME...>
Temporary Size
<TEMPORARY VARIABLE COUNT>
Bytecode Size
<BYTECODE SIZE>
<RAW BYTECODE>
End Function
```

## 3.2 Notes

- All section headers (e.g., `Begin Function`, `Name`, `Parameters`)  
  are **case-sensitive** and must match exactly.
- Parameter names are used only for **debugging purposes**; the VM  
  does not use them at runtime.
- `Bytecode Size` indicates the number of bytes to read as raw binary  
  following the line.
- `RAW BYTECODE` is not newline-terminated and may contain null bytes  
  or non-printable characters.
- The VM assumes all bytecode files are encoded in **UTF-8**.
- Additional sections (e.g., metadata or debug information) may be  
  supported in future versions.

---

# 4. Bytecode Overview

The NoctVM uses a compact bytecode format to represent executable
instructions. Each instruction consists of a 1-byte opcode followed by
zero or more operands, depending on the instruction type. Only `CALL`
and `THISCALL` have variable-length operand lists.

Bytecode operates primarily on temporary variables, using a
register-like index. All multi-byte fields are encoded in
**big-endian** format unless otherwise specified.

## 4.1 Operand Types

- `Td:2` — 2-byte destination temporary variable index  
- `Ts:2` — 2-byte destination temporary variable index (single source)
- `Ts1:2` and `Ts2:2` — 2-byte source temporary variable indices (dual source)
- `Imm:N` — N-byte immediate value  
- `Str` — NUL-terminated UTF-8 encoded string  
- `Argc1` — 1-byte argument count  
- `ArgnN` — 2-byte argument temporary variable index

## 4.2 Execution Model

The VM maintains a **program counter (PC)**, initially set to
0. During execution:

- The VM fetches an instruction at the current PC
- Decodes the opcode and operands
- Executes the instruction
- Advances the PC by the length of the instruction

Execution halts and control returns to the caller when the PC reaches
the end of the bytecode array.

This register-based model allows efficient evaluation of expressions
and function calls without requiring an explicit operand stack.

---

# 5. Instruction Semantics

This section describes the behavior of each bytecode instruction
defined in NoctVM. Instructions are grouped by their purpose: loading,
arithmetic, branching, and so on. Each opcode operates on temporary
variables and may use immediate values or symbolic names depending on
context.

## 5.1 NOP

| Mnemonic | Opcode | Operands | Description        |
|----------|--------|----------|--------------------|
| `NOP`    | 0x00   | --       | No operation. Does nothing. |

## 5.2 Assignment and Constant Loading

| Mnemonic     | Opcode | Operands              | Description                          |
|--------------|--------|-----------------------|--------------------------------------|
| `ASSIGN`     | 0x01   | `Td:2`, `Ts:2`        | Copy value from source to destination |
| `ICONST`     | 0x02   | `Td:2`, `Imm:4`       | Load 32-bit signed integer constant   |
| `FCONST`     | 0x03   | `Td:2`, `Imm:4        | Load 32-bit float constant            |
| `SCONST`     | 0x04   | `Td:2`, `Str`         | Load UTF-8 string constant            |
| `ACONST`     | 0x05   | `Td:2`                | Load empty array                      |
| `DCONST`     | 0x06   | `Td:2`                | Load empty dictionary                 |

> These instructions are typically used during initialization and
> expression evaluation.

## 5.3 Arithmetic and Logical Operations

These instructions operate on numeric or boolean values. Operands must
be of valid types, or a runtime error will occur.

### 5.3.1 Unary Operations

| Mnemonic | Opcode | Operands       | Description                  |
|----------|--------|----------------|------------------------------|
| `INC`    | 0x07   | `Td:2`, `Ts:2` | `dst = src + 1` (int only)   |
| `NEG`    | 0x08   | `Td:2`, `Ts:2` | `dst = -src` (int only)      |
| `NOT`    | 0x09   | `Td:2`, `Ts:2` | `dst = !src` (int only)      |

**Types**
|Operand   |Result Type  |Instruction     |Notes                                 |
|----------|-------------|----------------|--------------------------------------|
|int       |int          |INC,NEG, NOT    |                                      |
|Other     |Error        |                |Type error                            |

### 5.3.2 Binary Arithmetic

| Mnemonic | Opcode | Operands                  | Description             |
|----------|--------|---------------------------|-------------------------|
| `ADD`    | 0x0A   | `Td:2`, `Ts1:2`, `Ts2:2`  | Addition                |
| `SUB`    | 0x0B   | `Td:2`, `Ts1:2`, `Ts2:2`  | Subtraction             |
| `MUL`    | 0x0C   | `Td:2`, `Ts1:2`, `Ts2:2`  | Multiplication          |
| `DIV`    | 0x0D   | `Td:2`, `Ts1:2`, `Ts2:2`  | Division                |
| `MOD`    | 0x0E   | `Td:2`, `Ts1:2`, `Ts2:2`  | Modulo (int only)       |

**Types (ADD)**
|Operand1   |Operand2   |Result Type  |Semantics                                 |
|-----------|-----------|-------------|------------------------------------------|
|int        |int        |int          |Integer addition                          |
|int        |float      |float        |Promote to float and add                  |
|int        |string     |string       |Convert int to string, then concatenate   |
|float      |int        |float        |Promote to float and add                  |
|float      |float      |float        |Float addition                            |
|float      |string     |string       |Convert float to string, then concatenate |
|string     |int        |string       |Convert int to string, then concatenate   |
|string     |float      |string       |Convert float to string, then concatenate |
|string     |string     |string       |String concatenation                      |
|Other      |Any        |Error        |Type error                                |

**Types (SUB, MUL, DIV)**
|Operand1   |Operand2   |Result Type  |Notes                                 |
|-----------|-----------|-------------|--------------------------------------|
|int        |int        |int          |                                      |
|int        |float      |float        |Promote to float                      |
|float      |int        |float        |Promote to float                      |
|float      |float      |float        |                                      |
|Other      |Any        |Error        |Type error                            |

**Types (MOD)**
|Operand1   |Operand2   |Result Type  |Notes                                 |
|-----------|-----------|-------------|--------------------------------------|
|int        |int        |int          |                                      |
|Other      |Any        |Error        |Type error                            |

### 5.3.3 Bitwise and Logical Binary

| Mnemonic | Opcode | Operands                  | Description         |
|----------|--------|---------------------------|---------------------|
| `AND`    | 0x0F   | `Td:2`, `Ts1:2`, `Ts2:2`  | Bitwise AND (int)   |
| `OR`     | 0x10   | `Td:2`, `Ts1:2`, `Ts2:2`  | Bitwise OR (int)    |
| `XOR`    | 0x11   | `Td:2`, `Ts1:2`, `Ts2:2`  | Bitwise XOR (int)   |

**Types**
|Operand1   |Operand2   |Result Type  |Notes                                 |
|-----------|-----------|-------------|--------------------------------------|
|int        |int        |int          |                                      |
|Other      |Any        |Error        |Type error                            |

> Type errors will be raised if operands are not integers or valid
> combinations of numbers for `ADD`, `SUB`, etc.  String concatenation
> is only supported by `ADD` when both operands are strings.

## 5.4 Comparison Instructions

These instructions compare two operands and produce an integer result
(`0` = false, non-zero = true).

| Mnemonic | Opcode | Operands                  | Description            |
|----------|--------|---------------------------|------------------------|
| `LT`     | 0x12   | `Td:2`, `Ts1:2`, `Ts2:2`  | Less than              |
| `LTE`    | 0x13   | `Td:2`, `Ts1:2`, `Ts2:2`  | Less than or equal     |
| `GT`     | 0x14   | `Td:2`, `Ts1:2`, `Ts2:2`  | Greater than           |
| `GTE`    | 0x15   | `Td:2`, `Ts1:2`, `Ts2:2`  | Greater than or equal  |
| `EQ`     | 0x16   | `Td:2`, `Ts1:2`, `Ts2:2`  | Equality (deep compare)|
| `NEQ`    | 0x17   | `Td:2`, `Ts1:2`, `Ts2:2`  | Inequality             |
| `EQI`    | 0x18   | `Td:2`, `Ts1:2`, `Ts2:2`  | Integer equality       |

**Comparison Methods**
|Operand1   |Operand2   |Behavior     |Notes                                          |
|-----------|-----------|-------------|-----------------------------------------------|
|int        |int        |Subtract     |Integer comparison                             |
|int        |float      |Subtract     |Promote int to float                           |
|float      |int        |Subtract     |Promote int to float                           |
|float      |float      |Subtract     |Float comparison                               |
|string     |string     |strcmp()     |UTF-8 binary string comparison (lexicographic) |
|Other      |Any        |Type error   |Invalid comparison                             |

> Numeric comparisons support automatic type promotion (`int + float
> → float`).  String comparisons are lexicographic. All other type
> combinations raise runtime errors.  `EQI` is used for optimized
> integer comparison (typically followed by `JMPIFEQ`).

## 5.5 Array and Dictionary Instructions

These instructions perform element access and updates on arrays and
dictionaries.

### 5.5.1 `LOADARRAY`

| Mnemonic     | Opcode | Operands                  | Description                          |
|--------------|--------|---------------------------|--------------------------------------|
| `LOADARRAY`  | 0x19   | `Td:2`, `Ts1:2`, `Ts2:2`  | `dst = src1[src2]`                   |

**Behavior:**

- If `src1` is an array and `src2` is an integer: returns the element  
  at index `src2`. Out-of-bounds access raises an error.
- If `src1` is a dictionary and `src2` is a string: returns the value  
  for the given key. If key not present, raises an error.
- All other combinations raise a type error.

### 5.5.2 `STOREARRAY`

| Mnemonic      | Opcode | Operands                  | Description                          |
|---------------|--------|---------------------------|--------------------------------------|
| `STOREARRAY`  | 0x1A   | `Td:2`, `Ts1:2`, `Ts2:2`  | `dst[src1] = src2`                   |

**Behavior:**

- If `dst` is an array and `src1` is an integer: stores `src2` at  
  index `src1`. Array expands if needed.
- If `dst` is a dictionary and `src1` is a string: stores `src2` under  
  the key `src1`.
- Otherwise, raises a type error.
- For arrays, newly expanded slots are initialized to integer 0.

### 5.5.3 `LEN`

| Mnemonic | Opcode | Operands         | Description                     |
|----------|--------|------------------|---------------------------------|
| `LEN`    | 0x1B   | `Td:2`, `Ts:2`   | `dst = len(src)`               |

**Behavior:**
- Strings: UTF-8 byte length (not character count)
- Arrays: number of elements
- Dictionaries: number of key-value pairs
- All other types: raises a type error

## 5.6 Field Access Instructions

These instructions implement field-style dictionary access using the
`obj.key` notation.

### 5.6.1 `LOADDOT`

| Mnemonic   | Opcode | Operands                 | Description              |
|------------|--------|--------------------------|--------------------------|
| `LOADDOT`  | 0x1F   | `Td:2`, `Ts:2`, `Str`    | `dst = obj.field`        |

**Behavior:**
- The object (`Ts`) must be a dictionary.
- Loads the value stored at the key given by the string `Str`.
- Raises runtime error if key not present.
- Raises type error if `Ts` is not a dictionary.

### 5.6.2 `STOREDOT`

| Mnemonic    | Opcode | Operands                  | Description              |
|-------------|--------|---------------------------|--------------------------|
| `STOREDOT`  | 0x1E   | `Ts1:2`, `Str`, `Ts2:2`   | `obj.field = value`      |

**Behavior:**
- The object (`Ts1`) must be a dictionary.
- Stores `Ts2` into key `Str`.
- Raises type error if `Ts1` is not a dictionary.

> These are syntactic sugar for dictionary `LOADARRAY` / `STOREARRAY`
> using string keys, optimized for clarity and JIT.

### 5.6 `GETDICTKEYBYINDEX`

| Mnemonic             | Opcode | Operands                  | Description                   |
|----------------------|--------|---------------------------|-------------------------------|
| `GETDICTKEYBYINDEX`  | 0x1C   | `Td:2`, `Ts1:2`, `Ts2:2`  | `dst = src1.keyAtIndex(src2)` |

### 5.7 `GETDICTVALBYINDEX`

| Mnemonic             | Opcode | Operands                  | Description                   |
|----------------------|--------|---------------------------|-------------------------------|
| `GETDICTVALBYINDEX`  | 0x1C   | `Td:2`, `Ts1:2`, `Ts2:2`  | `dst = src1.valAtIndex(src2)` |

## 5.8 Global Symbol Instructions

These instructions manipulate global symbols, which bind names to
values at runtime. They provide access to shared state across all
functions.

### 5.8.1 `LOADSYMBOL`

| Mnemonic     | Opcode | Operands         | Description                          |
|--------------|--------|------------------|--------------------------------------|
| `LOADSYMBOL` | 0x21   | `Td:2`, `Str`    | `dst = getGlobal("str")`             |

**Behavior:**
- Loads the value bound to the global symbol named `Str`.
- If the symbol is not found, raises a runtime error.

### 5.8.2 `STORESYMBOL`

| Mnemonic      | Opcode | Operands         | Description                          |
|---------------|--------|------------------|--------------------------------------|
| `STORESYMBOL` | 0x20   | `Str`, `Ts:2`    | `setGlobal("str", src)`              |

**Behavior:**
- Binds `src` to the global symbol named `Str`, replacing any previous value.

**Notes:**
- Global symbols are UTF-8 strings and case-sensitive.
- Typically used to expose functions, constants, or modules globally.

---

## 6. Control Flow Instructions

These instructions manage execution flow through function calls and jumps.

### 6.1 Function Calls

#### 6.1.1 `CALL`

| Mnemonic | Opcode | Operands                               | Description                     |
|----------|--------|----------------------------------------|---------------------------------|
| `CALL`   | 0x22   | `Td:2`, `Ts:2`, `Argc:1`, `ArgN:2...`  | Call a function with arguments  |

**Behavior:**
- `Ts` must contain a function value.
- Creates a new call frame with `Argc1` arguments passed into the  
  callee’s temporaries.
- Executes the callee.
- Upon return, the callee’s temporary 0 is copied into `Td`.
- Raises runtime error if argument count mismatches or `Ts` is not a  
  function.

#### 6.1.2 `THISCALL`

| Mnemonic    | Opcode | Operands                                      | Description                                  |
|-------------|--------|-----------------------------------------------|----------------------------------------------|
| `THISCALL`  | 0x23   | `Td:2`, `Ts:2`, `Str`, `Argc:1`, `ArgN:2...`  | Call method stored in `obj.Str`              |

**Behavior:**
- Retrieves function from `Ts[Str]`, prepends `Ts` as first argument.
- Invokes method with arguments. Return value placed in `Td`.
- Errors if field missing, value is not a function, or argument mismatch.

### 6.2 Jumps

#### 6.2.1 `JMP`

| Mnemonic | Opcode | Operands  | Description                |
|----------|--------|-----------|----------------------------|
| `JMP`    | 0x24   | `Imm:4`   | Unconditional jump         |

#### 6.2.2 `JMPIFTRUE` / `JMPIFFALSE`

| Mnemonic      | Opcode | Operands           | Description                    |
|---------------|--------|--------------------|--------------------------------|
| `JMPIFTRUE`   | 0x25   | `Imm:4`, `Ts:2`    | Jump if `src` is non-zero      |
| `JMPIFFALSE`  | 0x26   | `Imm:4`, `Ts:2`    | Jump if `src` is zero          |

> Behavior is undefined if `src` is not an integer.

#### 6.2.3 `JMPIFEQ`

| Mnemonic  | Opcode | Operands          | Description                                 |
|-----------|--------|-------------------|---------------------------------------------|
| `JMPIFEQ` | 0x27   | `Imm:4`, `Ts:2`   | Optimized jump for result of `EQI` = false  |

> Semantically identical to `JMPIFFALSE`, but allows optimization by
> following `EQI`.

---

## 7. Debug Instructions

These instructions provide metadata for debugging purposes. They do
not affect program execution but assist in diagnostics, error
reporting, and tooling.

### 7.1 `LINEINFO`

| Mnemonic   | Opcode | Operands   | Description                         |
|------------|--------|------------|-------------------------------------|
| `LINEINFO` | 0x28   | `Imm:4`    | Annotates source line number        |

**Behavior:**

- Associates the following bytecode instruction with source line  
  number `Imm4`.
- This information is used in runtime error messages and stack traces.
- `LINEINFO` has no effect on execution and may be safely omitted or  
  stripped.

> Typical use: emitted by compilers or debuggers to trace VM
> instructions back to the original source file.

---

## 8. Error Model

This section defines how the VM responds to various runtime
errors. All errors are **fatal** unless otherwise handled by the host
environment (e.g., embedding interpreter).

### 8.1 Type Errors

Occurs when an instruction receives operands of incorrect or
unsupported types.

**Examples:**
- Arithmetic on non-numeric values
- Accessing a non-container with indexing
- Calling a non-function value

**Behavior:**
- Raises a fatal runtime error
- Optional: VM may include `LINEINFO` location in error messages

### 8.2 Runtime Errors

Arise from invalid program state even when operand types are valid.

**Examples:**
- Out-of-bounds array access
- Dictionary key not found
- Undefined global symbol

**Behavior:**
- Raises a fatal runtime error
- Error messages should be descriptive

### 8.3 Undefined Behavior

Occurs if bytecode or VM state is inconsistent or malformed.

**Examples:**
- Jumping into middle of an instruction
- Using uninitialized temporary variables
- Calling a function with incorrect arity

**Behavior:**
- Behavior is unspecified
- VM may raise an error, reject file, or behave unpredictably

> Recommendation: Code generators must ensure bytecode is well-formed
> to avoid undefined behavior.

---

## 9. Future Extensions

This section outlines reserved areas and long-term design decisions in
the NoctVM architecture.

### 9.1 Opcode Reservation

- All opcodes not explicitly defined in this document are **reserved**.
- Reserved opcodes must not be emitted or interpreted.
- This ensures forward compatibility for potential new instructions.

### 9.2 Modules

- The current version does **not support modules**.
- However, the file format and symbol system are designed to  
  accommodate future modularity.
- Future extensions may introduce module namespacing and import/export  
  metadata.

### 9.3 Exceptions

- NoctVM does **not support exceptions**.
- All runtime errors are **fatal** and cannot be caught or recovered.
- This design choice is **intentional and permanent**.

> Rationale: Simplicity, determinism, and embeddability are
> prioritized over structured exception handling. This ensures VM
> behavior is predictable and easier to implement across platforms.

---

## 10. Appendix: Opcode List

| Mnemonic            | Opcode | Description                               |
|---------------------|--------|-------------------------------------------|
| `NOP`               | 0x00   | No operation                              |
| `ASSIGN`            | 0x01   | Copy a value                              |
| `ICONST`            | 0x02   | Load integer constant                     |
| `FCONST`            | 0x03   | Load float constant                       |
| `SCONST`            | 0x04   | Load string constant                      |
| `ACONST`            | 0x05   | Load empty array                          |
| `DCONST`            | 0x06   | Load empty dictionary                     |
| `INC`               | 0x07   | Increment integer                         |
| `NEG`               | 0x08   | Negate integer                            |
| `NOT`               | 0x09   | Logical NOT on integer                    |
| `ADD`               | 0x0A   | Addition                                  |
| `SUB`               | 0x0B   | Subtraction                               |
| `MUL`               | 0x0C   | Multiplication                            |
| `DIV`               | 0x0D   | Division                                  |
| `MOD`               | 0x0E   | Modulo                                    |
| `AND`               | 0x0F   | Bitwise AND                               |
| `OR`                | 0x10   | Bitwise OR                                |
| `XOR`               | 0x11   | Bitwise XOR                               |
| `LT`                | 0x12   | Less than                                 |
| `LTE`               | 0x13   | Less than or equal                        |
| `GT`                | 0x14   | Greater than                              |
| `GTE`               | 0x15   | Greater than or equal                     |
| `EQ`                | 0x16   | Equality                                  |
| `NEQ`               | 0x17   | Inequality                                |
| `EQI`               | 0x18   | Integer equality                          |
| `LOADARRAY`         | 0x19   | Load value from array or dictionary       |
| `STOREARRAY`        | 0x1A   | Store value to array or dictionary        |
| `LEN`               | 0x1B   | Get length of string/array/dictionary     |
| `GETDICTKEYBYINDEX` | 0x1C | Get dictionary key at index          |
| `GETDICTVALBYINDEX` | 0x1D | Get dictionary value at index        |
| `STOREDOT`          | 0x1E   | Store dictionary value by field name      |
| `LOADDOT`           | 0x1F   | Load dictionary value by field name       |
| `STORESYMBOL`       | 0x20   | Store value in global symbol              |
| `LOADSYMBOL`        | 0x21   | Load value from global symbol             |
| `CALL`              | 0x22   | Call function                             |
| `THISCALL`          | 0x23   | Call method on object                     |
| `JMP`               | 0x24   | Unconditional jump                        |
| `JMPIFTRUE`         | 0x25   | Jump if true                              |
| `JMPIFFALSE`        | 0x26   | Jump if false                             |
| `JMPIFEQ`           | 0x27   | Jump if EQI result is false               |
| `LINEINFO`          | 0x28   | Annotate line number for debugging        |
