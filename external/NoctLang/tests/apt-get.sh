#!/bin/sh

echo "Installing..."

sudo apt-get install -y \
     qemu-user-static \
     crossbuild-essential-i386 \
     crossbuild-essential-amd64 \
     crossbuild-essential-powerpc \
     crossbuild-essential-ppc64el \
     crossbuild-essential-armhf \
     crossbuild-essential-arm64 \
     crossbuild-essential-mips \
     crossbuild-essential-mips64 \
     crossbuild-essential-riscv64
