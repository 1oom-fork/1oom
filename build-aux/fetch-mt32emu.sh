#!/bin/sh
set -eu

git_cmd="${GIT:-git}"
cmake_cmd="${CMAKE:-cmake}"
url="${MT32EMU_GIT_URL:-https://github.com/munt/munt.git}"
ref="${MT32EMU_GIT_TAG:-libmt32emu_2_7_3}"

workdir="$1"
prefix="$2"

case "$workdir" in
  /*) ;;
  *) workdir="`pwd`/$workdir" ;;
esac
case "$prefix" in
  /*) ;;
  *) prefix="`pwd`/$prefix" ;;
esac

srcdir="$workdir/munt-src"
builddir="$workdir/build"

mkdir -p "$workdir" "$prefix"

if [ ! -d "$srcdir/.git" ]; then
    rm -rf "$srcdir"
    "$git_cmd" clone --depth 1 --branch "$ref" "$url" "$srcdir"
fi

"$cmake_cmd" -S "$srcdir/mt32emu" -B "$builddir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -Dlibmt32emu_SHARED=OFF \
    -Dlibmt32emu_C_INTERFACE=ON \
    -DCMAKE_INSTALL_PREFIX="$prefix"

"$cmake_cmd" --build "$builddir" --config Release
"$cmake_cmd" --install "$builddir" --config Release

if [ ! -f "$prefix/lib/libmt32emu.a" ] && [ -f "$prefix/lib64/libmt32emu.a" ]; then
    mkdir -p "$prefix/lib"
    ln -sf ../lib64/libmt32emu.a "$prefix/lib/libmt32emu.a"
fi
