#!/usr/bin/env bash
set -e
./scripts/build-app.sh
make -C buildroot
./scripts/run-qemu.sh
