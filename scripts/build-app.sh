#!/usr/bin/env bash
set -e

if [ ! -d "buildroot" ]; then
  echo "[+] cloning Buildroot..."
  git clone --depth 1 https://gitlab.com/buildroot.org/buildroot.git buildroot

  echo "[+] applying Buildroot configuration..."
  make -C buildroot defconfig BR2_DEFCONFIG=../configs/potad_qemu_defconfig
fi

TOOLCHAIN="buildroot/output/host/bin/aarch64-buildroot-linux-gnu-g++"

if [ ! -f "$TOOLCHAIN" ]; then
  echo "[+] building Buildroot ARM64 toolchain"
  make -C buildroot toolchain
fi

mkdir -p board/qemu_aarch64/rootfs_overlay/sbin

echo "[+] compiling potad with Buildroot toolchain..."
$TOOLCHAIN -O2 -std=c++17 -static \
    -Iinclude \
    src/main.cpp \
    src/fs.cpp \
    src/sys.cpp \
    src/process.cpp \
    -o board/qemu_aarch64/rootfs_overlay/sbin/potad_init

echo "[+] compiled potad successfully!"