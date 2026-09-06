#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
case_dir="$test_dir/testcases"

show_help()
{
    cat <<'EOF'
Usage: tests/test.sh [command] [arguments]

Main suites:
  all              Run the normal host test suite (default).
  syntax           Parser, language syntax, interpreter and JIT tests.
  cli              Command-line optimization/JIT option tests.
  typing           Type annotations and bytecode metadata.
  typedop          Typed LIR operation generation.
  abce             Array-bounds-check elimination.
  packed-loop      Width-1 Packed loop and JIT register-cache tests.
  hint-accel-cpu   __accel CPU semantics and HIR marker tests.
  optimizer-callback Nullable accelerator optimizer callback tests.
  cse              Common-subexpression elimination.
  simd             SIMD vectorization, fallback and bytecode tests.
  fast             __fast functions, exact shapes and row-major indexing.
  class            Class freezing and top-level declarations.
  scoping          Block scope, let and TDZ behavior.
  require          CLI source-module resolution.
  bytecode-require Standalone .nbc require graph and compatibility tests.
  thread           Thread API tests.
  thread-stress    Repeat promotion/expansion races (default: 100 times).
  httpserver       HTTP server API tests.
  webapp           Web application framework tests.
  process          Process API tests.
  mmap [build-dir] FileUtil mmap and Packed native-finalizer tests.
  parallel-analysis Target-neutral loop fact/classification tests.
  accel-plan [dir] Target-neutral accelerator compiler-plan tests.
  accel-rewrite [dir] Transactional accelerator HIR rewrite tests.

Accelerator hardware suites:
  accel-vulkan-plan [dir] Vulkan/shaderc backend plan tests.
  gpu [dir]         Explicit selected-backend hardware execution tests.
  gpu-vulkan [dir] Compatibility alias for the accelerator hardware suite.

Toolchain/integration suites:
  api [build-dir]  Public Regex/File/Term registration test (default: build-static).
  ctrans [dir]     ANSI C translation tests (default: build-static).
  repl [dir]       REPL session tests (default: build-static).
  fma [dir]        FMA helper C test (default: build-debug).
  jit-slab [dir]   JIT slab allocator and retry tests.
  jit-branch ...   Long-branch test; arguments are [emulator ...] noct.
  beui [dir]       Independent BeUI platform/core host tests.
  elisp            Emacs Lisp translation tests.

Cross-architecture suites:
  multiarch        Build and run available targets from multiarch.noct via QEMU.
  simd-qemu ...    Run SIMD tiers: ARCH NOCT_BINARY [SYSROOT].

Environment variables such as NOCT, CC, QEMU and QEMU_CPU are passed to
the concrete scripts in tests/testcases/.  Relative build directories are
resolved from the repository root.  The Vulkan hardware suite is explicit
and is never run by the normal host suite.
EOF
}

run_script()
{
    script=$1
    shift
    cd "$case_dir"
    exec sh "./$script" "$@"
}

command=${1:-all}
if [ "$#" -gt 0 ]; then
    shift
fi

case "$command" in
help|-h|--help) show_help ;;
all)             run_script run-all.sh "$@" ;;
syntax)          run_script run-syntax.sh "$@" ;;
cli)             run_script run-cli-options.sh "$@" ;;
typing)          run_script run-typing.sh "$@" ;;
typedop)         run_script run-typedop.sh "$@" ;;
abce)            run_script run-abce.sh "$@" ;;
packed-loop)     run_script run-packed-loop.sh "$@" ;;
hint-accel-cpu)  run_script run-hint-accel-cpu.sh "$@" ;;
optimizer-callback) run_script run-optimizer-callback.sh "$@" ;;
cse)             run_script run-cse.sh "$@" ;;
simd)            run_script run-simd.sh "$@" ;;
fast)            run_script run-fast.sh "$@" ;;
class)           run_script run-class.sh "$@" ;;
scoping)         run_script run-scoping.sh "$@" ;;
require)         run_script run-require.sh "$@" ;;
bytecode-require) run_script run-bytecode-require.sh "$@" ;;
thread)          run_script run-thread.sh "$@" ;;
thread-stress)   run_script run-thread-stress.sh "$@" ;;
httpserver)      run_script run-httpserver.sh "$@" ;;
webapp)          run_script run-webapp.sh "$@" ;;
process)         run_script run-process.sh "$@" ;;
mmap)            run_script run-fileutil-mmap.sh "$@" ;;
parallel-analysis) run_script run-parallel-analysis.sh "$@" ;;
accel-plan)      run_script run-plan-accel.sh "$@" ;;
accel-rewrite)   run_script run-rewrite-accel.sh "$@" ;;
accel-vulkan-plan) run_script run-vulkan-accel-plan.sh "$@" ;;
gpu|gpu-vulkan)  run_script run-gpu-vulkan.sh "$@" ;;
api)             run_script run-api.sh "$@" ;;
ctrans)          run_script run-ctrans.sh "$@" ;;
repl)            run_script run-repl.sh "$@" ;;
fma)             run_script run-fma-helper.sh "$@" ;;
jit-slab)        run_script run-jit-slab.sh "$@" ;;
jit-branch)      run_script run-jit-long-branch.sh "$@" ;;
beui)            run_script run-beui.sh "$@" ;;
elisp)           run_script run-elisp.sh "$@" ;;
multiarch)       run_script run-multiarch.sh "$@" ;;
simd-qemu)       run_script run-simd-qemu.sh "$@" ;;
*)
    echo "Unknown test command: $command" >&2
    echo "Run '$0 help' for the command list." >&2
    exit 2
    ;;
esac
