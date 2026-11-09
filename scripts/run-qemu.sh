#!/usr/bin/env bash

usage() {
  cat <<'USAGE'
Usage: scripts/run-qemu.sh [--uboot] [--gui] [-- extra qemu args]

  --uboot   boot through U-Boot (extlinux.conf) instead of -kernel
  --gui     virtio-gpu window plus serial on stdio, instead of -nographic

Environment: BOARD [qemu_aarch64], SMP [4], MEM [2048] (MiB), CONSOLE [ttyAMA0], KERNEL_ARGS
USAGE
}

set -euo pipefail

die() { echo "[-] error: $*" >&2; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR=""
for candidate in "$SCRIPT_DIR" "$SCRIPT_DIR/.."; do
  if [[ -f "$candidate/src/main.cpp" ]]; then
    ROOT_DIR="$(cd "$candidate" && pwd)"
    break
  fi
done
[[ -n "$ROOT_DIR" ]] || die "cannot find the project root from $SCRIPT_DIR"

BOARD="${BOARD:-qemu_aarch64}"
IMAGES="$ROOT_DIR/build/br-$BOARD/images"
HOME_IMG="$ROOT_DIR/build/home.img"

MODE=kernel
GUI=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --uboot) MODE=uboot ;;
    --gui)   GUI=1 ;;
    -h|--help) usage; exit 0 ;;
    --) shift; break ;;
    *) die "unknown option '$1' (try --help)" ;;
  esac
  shift
done

command -v qemu-system-aarch64 &>/dev/null || die "qemu-system-aarch64 not installed"
[[ -f "$IMAGES/rootfs.ext4" ]] || die "no rootfs.ext4; run scripts/build-app.sh first"

QEMU=(qemu-system-aarch64 -M virt -smp "${SMP:-4}" -m "${MEM:-2048}M")

if [[ "$(uname -m)" == aarch64 && -w /dev/kvm ]]; then
  QEMU+=(-accel kvm -cpu host)
else
  QEMU+=(-cpu cortex-a53)
fi

QEMU+=(-drive "file=$IMAGES/rootfs.ext4,if=none,format=raw,id=rootdisk"
       -device virtio-blk-pci,drive=rootdisk)
if [[ -f "$HOME_IMG" ]]; then
  QEMU+=(-drive "file=$HOME_IMG,if=none,format=raw,id=homedisk"
         -device virtio-blk-pci,drive=homedisk)
fi
QEMU+=(-netdev user,id=net0 -device virtio-net-pci,netdev=net0)

if [[ "$MODE" == uboot ]]; then
  [[ -f "$IMAGES/u-boot.bin" ]] || die "no u-boot.bin in $IMAGES"
  QEMU+=(-bios "$IMAGES/u-boot.bin")
else
  [[ -f "$IMAGES/Image" ]] || die "no kernel Image in $IMAGES"
  QEMU+=(-kernel "$IMAGES/Image"
         -append "console=${CONSOLE:-ttyAMA0} root=/dev/vda rw rootwait earlycon init=/sbin/potad_init ${KERNEL_ARGS:-}")
fi

if (( GUI )); then
  QEMU+=(-device virtio-gpu-gl-pci -device virtio-keyboard-pci -device virtio-mouse-pci
         -display default,gl=on -serial mon:stdio)
else
  QEMU+=(-nographic)
fi

exec "${QEMU[@]}" ${@+"$@"}