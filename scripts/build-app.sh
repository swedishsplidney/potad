#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if [ ! -f "buildroot/Makefile" ]; then
  echo "[+] cloning buildroot..."
  rm -rf buildroot
  git clone --depth 1 https://gitlab.com/buildroot.org/buildroot.git buildroot
fi

if [ ! -f "buildroot/.config" ]; then
  echo "[+] applying buildroot configuration..."
  make -C buildroot defconfig BR2_DEFCONFIG="$ROOT_DIR/configs/potad_qemu_defconfig"
fi

TOOLCHAIN="buildroot/output/host/bin/aarch64-buildroot-linux-gnu-g++"

if [ ! -f "$TOOLCHAIN" ]; then
  echo "[+] building buildroot ARM64 toolchain..."
  make -C buildroot toolchain
fi

mkdir -p board/qemu_aarch64/rootfs_overlay/sbin

echo "[+] compiling potad with buildroot toolchain..."
$TOOLCHAIN -O2 -std=c++17 -static \
    -Iinclude \
    src/main.cpp \
    src/fs.cpp \
    src/sys.cpp \
    src/process.cpp \
    -o board/qemu_aarch64/rootfs_overlay/sbin/potad_init


# lazyvim
LAZYVIM_DIR="board/qemu_aarch64/rootfs_overlay/root/.config/nvim"

if [ ! -f "$LAZYVIM_DIR/init.lua" ]; then
  echo "[+] installing LazyVim starter into rootfs overlay..."
  rm -rf "$LAZYVIM_DIR"
  mkdir -p "board/qemu_aarch64/rootfs_overlay/root/.config"
  git clone --depth 1 https://github.com/LazyVim/starter "$LAZYVIM_DIR"
  rm -rf "$LAZYVIM_DIR/.git"
fi

echo "[+] compiled potad successfully!"