#!/bin/sh

rm -rf build
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/Qt/6.9.1/macos ..
make
cd ..
