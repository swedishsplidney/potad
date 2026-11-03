#!/usr/bin/env bash

set -euo pipefail

say()  { echo "[+] $*"; }
warn() { echo "[!] $*" >&2; }
die()  { echo "[-] error: $*" >&2; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR=""
for candidate in "$SCRIPT_DIR" "$SCRIPT_DIR/.."; do
  if [[ -f "$candidate/src/main.cpp" ]]; then
    ROOT_DIR="$(cd "$candidate" && pwd)"
    break
  fi
done
[[ -n "$ROOT_DIR" ]] || die "cannot find src/main.cpp relative to $SCRIPT_DIR"
cd "$ROOT_DIR"

if [[ "$ROOT_DIR" == *[[:space:]]* ]]; then
  die "project path contains spaces: '$ROOT_DIR' (Buildroot cannot cope with that)"
fi

if [[ "${1:-}" == "clean" ]]; then
  make -C "$ROOT_DIR" clean
  exit 0
fi

BOARD="${BOARD:-qemu_aarch64}"
DEFCONFIG="${DEFCONFIG:-configs/potad_qemu_defconfig}"
ROOT_DEV="${ROOT_DEV:-/dev/vda}"
HOME_DEV="${HOME_DEV:-/dev/vdb}"
CONSOLE="${CONSOLE:-ttyAMA0}"
KERNEL_ARGS="${KERNEL_ARGS:-}"
ROOTFS_SIZE="${ROOTFS_SIZE:-512M}"
HOME_IMG_MB="${HOME_IMG_MB:-128}"
JOBS="${JOBS:-$(nproc)}"

BR_DIR="$ROOT_DIR/buildroot"
BR_URL="https://gitlab.com/buildroot.org/buildroot.git"
OVERLAY_DIR="$ROOT_DIR/board/$BOARD/rootfs_overlay"
BUILD_DIR="$ROOT_DIR/build"
GEN_DEFCONFIG="$BUILD_DIR/$BOARD.defconfig"
HOME_IMG="$BUILD_DIR/home.img"

PATH="$PATH:/sbin:/usr/sbin"

clean_path=""
IFS=: read -ra path_parts <<< "$PATH"
for p in "${path_parts[@]}"; do
  [[ -z "$p" || "$p" == "." || "$p" =~ [[:space:]] ]] && continue
  clean_path+="${clean_path:+:}$p"
done
export PATH="$clean_path"

missing=()
for tool in git make gcc g++ sed rsync flex bison bc perl cpio unzip file patch tar gzip bzip2 \
            find awk which wget mkfs.ext4 truncate cmp; do
  command -v "$tool" &>/dev/null || missing+=("$tool")
done
if (( ${#missing[@]} )); then
  die "missing host tools: ${missing[*]}"
fi

for hdr in openssl/engine.h gnutls/gnutls.h uuid/uuid.h; do
  echo "#include <$hdr>" | gcc -E -x c - &>/dev/null \
    || die "host header <$hdr> not found. On fedora: sudo dnf install openssl-devel openssl-devel-engine gnutls-devel libuuid-devel"
done

for src in main fs sys process service control_socket log client; do
  [[ -f "src/$src.cpp" ]] || die "source file 'src/$src.cpp' missing"
done
[[ -f "$DEFCONFIG" ]] || die "config file '$DEFCONFIG' not found"

latest_release_tag() {
  git ls-remote --tags --refs "$BR_URL" 2>/dev/null \
    | sed 's|.*refs/tags/||' \
    | grep -E '^[0-9]{4}\.[0-9]{2}(\.[0-9]+)?$' \
    | sort -V | tail -n1 || true
}

if [[ ! -f "$BR_DIR/Makefile" ]]; then
  ref="${BUILDROOT_REF:-$(latest_release_tag)}"
  rm -rf "$BR_DIR"
  if [[ -n "$ref" ]]; then
    say "cloning buildroot $ref..."
    git clone --depth 1 --branch "$ref" "$BR_URL" "$BR_DIR"
  else
    warn "could not determine a release tag; cloning the development branch"
    git clone --depth 1 "$BR_URL" "$BR_DIR"
  fi
fi

mkdir -p "$BUILD_DIR"
cp "$DEFCONFIG" "$GEN_DEFCONFIG"

set_cfg() {
  sed -i -e "/^$1=/d" -e "/^# $1 is not set/d" "$GEN_DEFCONFIG"
  printf '%s=%s\n' "$1" "$2" >> "$GEN_DEFCONFIG"
}

set_cfg BR2_ROOTFS_OVERLAY "\"$OVERLAY_DIR\""
set_cfg BR2_LINUX_KERNEL_INSTALL_TARGET y
set_cfg BR2_TARGET_ROOTFS_EXT2 y
set_cfg BR2_TARGET_ROOTFS_EXT2_4 y
set_cfg BR2_TARGET_ROOTFS_EXT2_SIZE "\"$ROOTFS_SIZE\""
if ! grep -q '^BR2_TOOLCHAIN_EXTERNAL=y' "$GEN_DEFCONFIG"; then
  set_cfg BR2_TOOLCHAIN_BUILDROOT_CXX y
fi
if [[ "${CCACHE:-0}" == 1 ]]; then
  set_cfg BR2_CCACHE y
fi

STAMP="$BUILD_DIR/$BOARD.defconfig.applied"
if [[ ! -f "$BR_DIR/.config" ]] || ! cmp -s "$GEN_DEFCONFIG" "$STAMP" 2>/dev/null; then
  say "applying buildroot configuration..."
  make -C "$BR_DIR" defconfig BR2_DEFCONFIG="$GEN_DEFCONFIG"
  cp "$GEN_DEFCONFIG" "$STAMP"
fi

find_cxx() {
  local f
  for f in "$BR_DIR"/output/host/bin/*-g++; do
    [[ -x "$f" ]] && { printf '%s' "$f"; return 0; }
  done
  return 1
}

CXX_BIN="$(find_cxx || true)"
if [[ -z "$CXX_BIN" ]]; then
  say "building buildroot host-gcc-final (might take a while)..."
  make -C "$BR_DIR" host-gcc-final-rebuild || make -C "$BR_DIR" host-gcc-final
  CXX_BIN="$(find_cxx || true)"
  [[ -n "$CXX_BIN" ]] || die "toolchain built but no g++ found; the config needs BR2_TOOLCHAIN_BUILDROOT_CXX=y"
fi

say "compiling potad with $(basename "$CXX_BIN")..."
make -C "$ROOT_DIR" -j"$JOBS" CXX="$CXX_BIN" OVERLAY="$OVERLAY_DIR" install

write_if_changed() {
  local dest="$1" tmp
  tmp="$(mktemp)"
  cat > "$tmp"
  mkdir -p "$(dirname "$dest")"
  if [[ -f "$dest" ]] && cmp -s "$tmp" "$dest"; then
    rm -f "$tmp"
  else
    mv "$tmp" "$dest"
    chmod 644 "$dest"
  fi
}

say "writing /etc/fstab and extlinux.conf..."
write_if_changed "$OVERLAY_DIR/etc/fstab" << EOF
# /etc/fstab: static file system information
# <file system>  <mount point>  <type>  <options>     <dump>  <pass>
$ROOT_DEV        /              ext4    rw,relatime   0       1
$HOME_DEV        /home          ext4    rw,relatime   0       2
EOF

write_if_changed "$OVERLAY_DIR/boot/extlinux/extlinux.conf" << EOF
label potad-linux
    kernel /boot/Image
    append console=$CONSOLE root=$ROOT_DEV rw rootwait earlycon init=/sbin/potad_init $KERNEL_ARGS
EOF

LAZYVIM_DIR="$OVERLAY_DIR/root/.config/nvim"
if [[ "${LAZYVIM:-1}" == 1 && ! -f "$LAZYVIM_DIR/init.lua" ]]; then
  say "installing lazyvim starter into rootfs overlay..."
  rm -rf "$LAZYVIM_DIR"
  mkdir -p "$(dirname "$LAZYVIM_DIR")"
  if git clone --depth 1 https://github.com/LazyVim/starter "$LAZYVIM_DIR"; then
    rm -rf "$LAZYVIM_DIR/.git"
  else
    warn "could not fetch lazyvim (offline?); continuing without it"
    rm -rf "$LAZYVIM_DIR"
  fi
fi

if [[ ! -f "$HOME_IMG" ]]; then
  if [[ -f "$BR_DIR/output/images/home.img" ]]; then
    say "migrating existing home.img to $HOME_IMG..."
    cp "$BR_DIR/output/images/home.img" "$HOME_IMG"
  else
    say "creating ${HOME_IMG_MB}M home.img..."
    truncate -s "${HOME_IMG_MB}M" "$HOME_IMG"
    mkfs.ext4 -q -F -L home "$HOME_IMG"
  fi
fi

# buildroot
say "building kernel, U-Boot and rootfs..."
make -C "$BR_DIR" -j"$JOBS"

say "build complete! artifacts are in buildroot/output/images/"