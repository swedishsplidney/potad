#!/usr/bin/env bash
set -e

mkdir -p board/qemu_aarch64/rootfs_overlay/sbin

buildroot/output/host/bin/aarch64-buildroot-linux-gnu-g++ \
    -O2 -std=c++17 -static \
    -Iinclude \
    src/main.cpp \
    src/fs.cpp \
    src/sys.cpp \
    src/process.cpp \
    -o board/qemu_aarch64/rootfs_overlay/sbin/potad_init

echo "[+] compiled potad_init successfully!"