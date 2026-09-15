#!/usr/bin/env bash
qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a53 \
    -smp 4 \
    -m 2048M \
    -kernel buildroot/output/images/Image \
    -drive file=buildroot/output/images/rootfs.ext4,if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic \
    -append "console=ttyAMA0 root=/dev/vda rw earlycon"
