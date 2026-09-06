#!/usr/bin/env python3

import os
import select
import signal
import subprocess
import sys
import time


def fail(message, output=""):
    print(f"REPL test failed: {message}", file=sys.stderr)
    if output:
        print(output, file=sys.stderr)
    raise SystemExit(1)


def test_transcript(noct):
    source = """print(\"M13_SINGLE\")
if (1) {
print(\"M13_MULTI\");
}
var text = \"{ // ignored }\"
var = 1
print(\"M13_RECOVERED\")
"""
    result = subprocess.run(
        [noct], input=source, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, timeout=10, check=False)
    output = result.stdout
    if result.returncode != 0:
        fail(f"transcript returned {result.returncode}", output)
    for marker in ("M13_SINGLE", "M13_MULTI", "M13_RECOVERED"):
        if marker not in output:
            fail(f"missing marker {marker}", output)
    if "Error:" not in output:
        fail("syntax error was not reported", output)
    if ". " not in output:
        fail("multiline prompt was not shown", output)


def test_ctrl_c(noct):
    proc = subprocess.Popen(
        [noct], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT)
    output = bytearray()
    deadline = time.monotonic() + 10
    try:
        while b"> " not in output:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                fail("timed out waiting for prompt", output.decode(errors="replace"))
            readable, _, _ = select.select([proc.stdout], [], [], remaining)
            if not readable:
                continue
            chunk = os.read(proc.stdout.fileno(), 4096)
            if not chunk:
                fail("REPL exited before prompt", output.decode(errors="replace"))
            output.extend(chunk)

        os.kill(proc.pid, signal.SIGINT)
        tail, _ = proc.communicate(timeout=5)
        output.extend(tail)
        if proc.returncode != 0:
            fail(f"Ctrl-C returned {proc.returncode}",
                 output.decode(errors="replace"))
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()


def test_long_line(noct):
    source = ("x" * 33000) + "\nprint(\"M13_LONG_RECOVERED\")\n"
    result = subprocess.run(
        [noct], input=source, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, timeout=10, check=False)
    output = result.stdout
    if result.returncode != 0:
        fail(f"long-line test returned {result.returncode}", output)
    if "Input line is too long." not in output:
        fail("long physical line was not rejected", output)
    if "M13_LONG_RECOVERED" not in output:
        fail("REPL did not recover after a long line", output)


def main():
    if len(sys.argv) != 2:
        fail("usage: run-repl.py PATH-TO-NOCT")
    noct = os.path.abspath(sys.argv[1])
    test_transcript(noct)
    test_long_line(noct)
    test_ctrl_c(noct)
    print("Noct Linux CLI REPL tests: PASS")


if __name__ == "__main__":
    main()
