Reusable REPL sessions
======================

Noct provides a host-independent REPL session engine when CMake is
configured with `NOCT_ENABLE_REPL=ON`. The engine is part of
`libnoct`, it does not read a terminal, print prompts, install signal
handlers, or own a VM. This separation allows the same implementation
to serve the desktop CLI and embedded hosts.

Include the public interface with:

```c
#include <noct/repl.h>
```

## Ownership and lifetime

The host creates and owns one `NoctVM` and its `NoctEnv`. It then
creates a `NoctReplSession` for that environment. Destroy the session
before destroying the VM:

```c
NoctVM *vm;
NoctEnv *env;
NoctReplSession *repl;

if (!noct_create_vm(&vm, &env, NULL))
        return false;

repl = noct_repl_create(env, 32768);
if (repl == NULL) {
        noct_destroy_vm(vm);
        return false;
}

/* Register host APIs and submit input here. */

noct_repl_destroy(repl);
noct_destroy_vm(vm);
```

`max_source_size` is a hard bound for accumulated multiline source,
including the small wrapper used for statements. The session uses
Noct's configured `noct_malloc()` and `noct_free()` hooks.

## Submitting input

Call `noct_repl_submit()` once for each physical input line. A
trailing newline is optional, the engine supplies one when
necessary. The result tells the host what to do next:

| Result                | Meaning                                                     |
| --------------------- | ------------------------------------------------------------|
| `NOCT_REPL_READY`     | Blank or comment-only input was consumed.                   |
| `NOCT_REPL_NEED_MORE` | Delimiters or a block statement are incomplete.             |
| `NOCT_REPL_EXECUTED`  | The source compiled and, for a statement, executed.         |
| `NOCT_REPL_ERROR`     | Scanning, compilation, execution, or size checking failed.  |
| `NOCT_REPL_EXIT`      | The host submitted `NULL` to end the session.               |

After `NOCT_REPL_ERROR`, retrieve details through the normal
`noct_get_error_*()` API. The pending multiline source is already
discarded, so the next line starts a fresh
submission. `noct_repl_cancel()` explicitly discards pending multiline
input without destroying the VM.

The scanner balances `()`, `[]`, and `{}` while ignoring delimiters in
strings and line comments. Top-level function definitions remain
registered in the VM and may be called by later submissions. Ordinary
statements execute in a synthetic function, so their local variables
have the same lifetime as normal function locals, hosts must not
promise persistent local declarations.

## Interrupts and terminal policy

The reusable engine deliberately has no signal or console
dependency. A host that receives an interrupt while waiting for input
should call `noct_repl_cancel()` or submit `NULL`, according to its UI
policy. The Linux CLI uses `SIGINT` to end the session cleanly and
destroys the session and VM on all exit paths.

An interrupt does not preempt Noct bytecode or JIT code that is
already executing. Safe interruption of an infinite script requires VM
safepoints and is outside this interface.

The CLI rejects an overlong physical line as one unit and drains it
before reading another line, it never executes a truncated prefix.
