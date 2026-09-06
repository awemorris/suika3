#!/bin/sh

set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
MODEL_ID=project-cifar-opset12
export MODEL_ID
exec sh "$ROOT/run-onnx-mnist-opengl.sh"
