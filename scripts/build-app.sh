#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if [ ! -f "buildroot/Makefile" ]; then
  echo "[+] cloning buildroot..."
  rm -rf buildroot
  git clone --depth 1 https://gitlab.com/buildroot.org/buildroot.git buildroot
fi

if grep -q "BR2_ROOTFS_OVERLAY=" configs/potad_qemu_defconfig; then
  sed -i 's|BR2_ROOTFS_OVERLAY=.*|BR2_ROOTFS_OVERLAY="../board/qemu_aarch64/rootfs_overlay"|' configs/potad_qemu_defconfig
else
  echo 'BR2_ROOTFS_OVERLAY="../board/qemu_aarch64/rootfs_overlay"' >> configs/potad_qemu_defconfig
fi

if ! grep -q "BR2_LINUX_KERNEL_INSTALL_TARGET=y" configs/potad_qemu_defconfig; then
  echo 'BR2_LINUX_KERNEL_INSTALL_TARGET=y' >> configs/potad_qemu_defconfig
fi

if grep -q "BR2_TARGET_ROOTFS_EXT2_SIZE=" configs/potad_qemu_defconfig; then
  sed -i 's|BR2_TARGET_ROOTFS_EXT2_SIZE=.*|BR2_TARGET_ROOTFS_EXT2_SIZE="512M"|' configs/potad_qemu_defconfig
else
  echo 'BR2_TARGET_ROOTFS_EXT2_SIZE="512M"' >> configs/potad_qemu_defconfig
fi

echo "[+] applying buildroot configuration..."
make -C buildroot defconfig BR2_DEFCONFIG="$ROOT_DIR/configs/potad_qemu_defconfig"

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

LAZYVIM_DIR="board/qemu_aarch64/rootfs_overlay/root/.config/nvim"
if [ ! -f "$LAZYVIM_DIR/init.lua" ]; then
  echo "[+] installing LazyVim starter into rootfs overlay..."
  rm -rf "$LAZYVIM_DIR"
  mkdir -p "board/qemu_aarch64/rootfs_overlay/root/.config"
  git clone --depth 1 https://github.com/LazyVim/starter "$LAZYVIM_DIR"
  rm -rf "$LAZYVIM_DIR/.git"
fi

echo "[+] writing bootloader config (extlinux.conf)..."
mkdir -p board/qemu_aarch64/rootfs_overlay/boot/extlinux
cat << 'EOF' > board/qemu_aarch64/rootfs_overlay/boot/extlinux/extlinux.conf
label potad-linux
    kernel /boot/Image
    append console=ttyAMA0 root=/dev/vda rw earlycon init=/sbin/potad_init
EOF

echo "[+] building target binaries, kernel, U-Boot, and rootfs..."
make -C buildroot

echo "[+] build complete! artifacts located in buildroot/output/images/"