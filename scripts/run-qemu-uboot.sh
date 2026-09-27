#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a53 \
    -smp 4 \
    -m 2048M \
    -bios "$ROOT_DIR/buildroot/output/images/u-boot.bin" \
    -drive file="$ROOT_DIR/buildroot/output/images/rootfs.ext4",if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -device virtio-gpu-gl-pci \
    -device virtio-keyboard-pci \
    -device virtio-mouse-pci \
    -display default,gl=on \
    -serial mon:stdio