#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if [[ "$ROOT_DIR" =~ [[:space:]] ]]; then
  echo "[-] error: project path contains spaces: '$ROOT_DIR'"
  echo "    buildroot will freak out if its in a directory with a space in it. please use a directory without spaces!"
  exit 1
fi

REQUIRED_TOOLS=(git make gcc g++ sed rsync flex bison bc)
for tool in "${REQUIRED_TOOLS[@]}"; do
  if ! command -v "$tool" &>/dev/null; then
    echo "[-] error: required host tool '$tool' is not installed."
    exit 1
  fi
done

REQUIRED_SOURCES=("src/main.cpp" "src/fs.cpp" "src/sys.cpp" "src/process.cpp")
for src in "${REQUIRED_SOURCES[@]}"; do
  if [ ! -f "$src" ]; then
    echo "[-] error: source file '$src' missing."
    exit 1
  fi
done

if [ ! -f "buildroot/Makefile" ]; then
  echo "[+] cloning buildroot..."
  rm -rf buildroot
  git clone --depth 1 https://gitlab.com/buildroot.org/buildroot.git buildroot
fi

DEFCONFIG="configs/potad_qemu_defconfig"
if [ ! -f "$DEFCONFIG" ]; then
  echo "[-] error: config file '$DEFCONFIG' not found."
  exit 1
fi

if grep -q "BR2_ROOTFS_OVERLAY=" "$DEFCONFIG"; then
  sed -i 's|BR2_ROOTFS_OVERLAY=.*|BR2_ROOTFS_OVERLAY="../board/qemu_aarch64/rootfs_overlay"|' "$DEFCONFIG"
else
  echo 'BR2_ROOTFS_OVERLAY="../board/qemu_aarch64/rootfs_overlay"' >> "$DEFCONFIG"
fi

if ! grep -q "BR2_LINUX_KERNEL_INSTALL_TARGET=y" "$DEFCONFIG"; then
  echo 'BR2_LINUX_KERNEL_INSTALL_TARGET=y' >> "$DEFCONFIG"
fi

if grep -q "BR2_TARGET_ROOTFS_EXT2_SIZE=" "$DEFCONFIG"; then
  sed -i 's|BR2_TARGET_ROOTFS_EXT2_SIZE=.*|BR2_TARGET_ROOTFS_EXT2_SIZE="512M"|' "$DEFCONFIG"
else
  echo 'BR2_TARGET_ROOTFS_EXT2_SIZE="512M"' >> "$DEFCONFIG"
fi

echo "[+] applying buildroot configuration..."
make -C buildroot defconfig BR2_DEFCONFIG="$ROOT_DIR/$DEFCONFIG"

TOOLCHAIN="buildroot/output/host/bin/aarch64-buildroot-linux-gnu-g++"
if [ ! -f "$TOOLCHAIN" ]; then
  echo "[+] building buildroot ARM64 toolchain (this may take a while)..."
  make -C buildroot toolchain
fi

mkdir -p board/qemu_aarch64/rootfs_overlay/sbin
echo "[+] compiling potad with buildroot toolchain..."
$TOOLCHAIN -O2 -std=c++17 -static \
    -Iinclude \
    "${REQUIRED_SOURCES[@]}" \
    -o board/qemu_aarch64/rootfs_overlay/sbin/potad_init

LAZYVIM_DIR="board/qemu_aarch64/rootfs_overlay/root/.config/nvim"
if [ ! -f "$LAZYVIM_DIR/init.lua" ]; then
  echo "[+] Installing LazyVim starter into rootfs overlay..."
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
make -C buildroot -j"$(nproc)"

echo "[+] build complete! artifacts located in buildroot/output/images/"