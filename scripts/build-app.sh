#!/usr/bin/env bash

# build potad and a bootable image via buildroot
#
# usage: ./scripts/build-app.sh

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
  die "project path contains spaces: '$ROOT_DIR' (spaces make buildroot explode)"
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
FDT="${FDT:-}"
PANIC_SECS="${PANIC_SECS:-5}"
DEFAULT_BR_REF="2025.02.14"
ROOTFS_SIZE="${ROOTFS_SIZE:-512M}"
HOME_IMG_MB="${HOME_IMG_MB:-128}"
JOBS="${JOBS:-$(nproc)}"

BR_DIR="$ROOT_DIR/buildroot"
BR_URL="https://gitlab.com/buildroot.org/buildroot.git"
OVERLAY_DIR="$ROOT_DIR/board/$BOARD/rootfs_overlay"
BUILD_DIR="$ROOT_DIR/build"
BR_OUT="$BUILD_DIR/br-$BOARD"
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
for tool in git make gcc g++ ld sed rsync flex bison bc perl python3 cpio unzip file patch tar gzip bzip2 \
            find awk which wget mkfs.ext4 truncate cmp diff; do
  command -v "$tool" &>/dev/null || missing+=("$tool")
done
if (( ${#missing[@]} )); then
  die "missing host tools: ${missing[*]}"
fi

for hdr in openssl/engine.h gnutls/gnutls.h uuid/uuid.h; do
  echo "#include <$hdr>" | gcc -E -x c - &>/dev/null \
    || die "host header <$hdr> not found. On fedora: sudo dnf install openssl-devel openssl-devel-engine gnutls-devel libuuid-devel"
done

[[ -f "$DEFCONFIG" ]] || die "config file '$DEFCONFIG' not found"

latest_release_tag() {
  git ls-remote --tags --refs "$BR_URL" 2>/dev/null \
    | sed 's|.*refs/tags/||' \
    | grep -E '^[0-9]{4}\.[0-9]{2}(\.[0-9]+)?$' \
    | sort -V | tail -n1 || true
}

br_ref="${BUILDROOT_REF:-$DEFAULT_BR_REF}"
[[ "$br_ref" == latest ]] && br_ref="$(latest_release_tag)"

if [[ ! -f "$BR_DIR/Makefile" ]]; then
  rm -rf "$BR_DIR"
  if [[ -n "$br_ref" ]]; then
    say "cloning buildroot $br_ref..."
    git clone --depth 1 --branch "$br_ref" "$BR_URL" "$BR_DIR"
  else
    warn "could not determine a release tag; cloning the development branch"
    git clone --depth 1 "$BR_URL" "$BR_DIR"
  fi
elif [[ -n "$br_ref" ]]; then
  current="$(git -C "$BR_DIR" describe --tags --exact-match 2>/dev/null || true)"
  [[ "$current" == "$br_ref" ]] \
    || warn "buildroot checkout is '${current:-unknown}' but BUILDROOT_REF is '$br_ref' (delete $BR_DIR to re-clone)"
fi

br_make() { make -C "$BR_DIR" O="$BR_OUT" "$@"; }

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
if [[ ! -f "$BR_OUT/.config" ]] || ! cmp -s "$GEN_DEFCONFIG" "$STAMP" 2>/dev/null; then
  say "applying buildroot configuration..."
  br_make defconfig BR2_DEFCONFIG="$GEN_DEFCONFIG"
  cp "$GEN_DEFCONFIG" "$STAMP"
fi

find_cxx() {
  local f
  for f in "$BR_OUT"/host/bin/*-g++; do
    [[ -x "$f" ]] && { printf '%s' "$f"; return 0; }
  done
  return 1
}

CXX_BIN="$(find_cxx || true)"
if [[ -z "$CXX_BIN" ]]; then
  say "building buildroot host-gcc-final (might take a while)..."
  br_make -j"$JOBS" host-gcc-final
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
{
  echo "# /etc/fstab: static file system information"
  echo "# <file system>  <mount point>  <type>  <options>     <dump>  <pass>"
  echo "$ROOT_DEV        /              ext4    rw,relatime   0       1"
  if [[ -n "$HOME_DEV" ]]; then
    echo "$HOME_DEV        /home          ext4    rw,relatime   0       2"
  fi
} | write_if_changed "$OVERLAY_DIR/etc/fstab"

{
  echo "label potad-linux"
  echo "    kernel /boot/Image"
  if [[ -n "$FDT" ]]; then
    echo "    fdt /boot/$FDT"
  fi
  echo "    append console=$CONSOLE root=$ROOT_DEV rw rootwait earlycon init=/sbin/potad_init panic=$PANIC_SECS $KERNEL_ARGS"
} | write_if_changed "$OVERLAY_DIR/boot/extlinux/extlinux.conf"

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

if [[ -n "$HOME_DEV" && ! -f "$HOME_IMG" ]]; then
  if [[ -f "$BR_DIR/output/images/home.img" ]]; then
    say "migrating existing home.img to $HOME_IMG..."
    cp "$BR_DIR/output/images/home.img" "$HOME_IMG"
  else
    say "creating ${HOME_IMG_MB}M home.img..."
    truncate -s "${HOME_IMG_MB}M" "$HOME_IMG"
    mkfs.ext4 -q -F -L home "$HOME_IMG"
  fi
fi

say "building kernel, u-boot and rootfs..."
br_make -j"$JOBS"

say "build complete! artifacts are in ${BR_OUT#"$ROOT_DIR"/}/images/"