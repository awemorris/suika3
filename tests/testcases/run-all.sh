#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
build_arg=${1:-build-static}

case "$build_arg" in
/*)
    build_dir=$build_arg
    ;;
*)
    build_dir=$root/$build_arg
    ;;
esac

NOCT=$build_dir/noct
export NOCT

cd "$script_dir"

echo 'NoctLang Test Suites'
echo

for suite in \
    syntax cli-options typing typedop abce packed-loop hint-accel-cpu \
    optimizer-callback cse parallel-analysis \
    simd fast class scoping require bytecode-require \
    thread httpserver webapp process fileutil-mmap
do
    case "$suite" in
    cli-options|hint-accel-cpu|optimizer-callback|bytecode-require|fileutil-mmap)
        sh "run-$suite.sh" "$build_dir"
        ;;
    *)
        sh "run-$suite.sh"
        ;;
    esac
done

echo
echo 'All suites passed.'
