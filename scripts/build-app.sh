#!/usr/bin/env bash
set -e

if [ ! -d "buildroot" ]; then
  echo "cloning Buildroot..."
  git clone --depth 1 https://gitlab.com/buildroot.org/buildroot.git buildroot

  echo "[+] applying Buildroot configuration..."
  make -C buildroot defconfig BR2_DEFCONFIG=../configs/potad_qemu_defconfig
fi

mkdir -p board/qemu_aarch64/rootfs_overlay/sbin

# select available compiler
if [ -f "buildroot/output/host/bin/aarch64-buildroot-linux-gnu-g++" ]; then
  CXX="buildroot/output/host/bin/aarch64-buildroot-linux-gnu-g++"
elif command -v aarch64-linux-gnu-g++ >/dev/null 2>&1; then
  CXX="aarch64-linux-gnu-g++"
else
  echo "[-] error: No ARM64 cross-compiler found."
  echo "    please install g++-aarch64-linux-gnu on your host machine:"
  echo "    Ubuntu/Debian: sudo apt install g++-aarch64-linux-gnu"
  echo "    Fedora: sudo dnf install gcc-c++-aarch64-linux-gnu"
  echo "    Arch: sudo pacman -S aarch64-linux-gnu-gcc"
  exit 1
fi

echo "[+] using compiler: $CXX"

$CXX -O2 -std=c++17 -static \
    -Iinclude \
    src/main.cpp \
    src/fs.cpp \
    src/sys.cpp \
    src/process.cpp \
    -o board/qemu_aarch64/rootfs_overlay/sbin/potad_init

echo "[+] compiled potad successfully!"