#!/bin/sh

set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
MODEL_ID=squeezenet1.1-7
export MODEL_ID
exec sh "$ROOT/run-onnx-mnist-opengl.sh"
