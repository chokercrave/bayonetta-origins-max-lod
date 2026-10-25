#!/bin/sh
# Build the LOD module twice and package both into dist/: the release (every model at LOD0) and a test build that forces
# LOD2 instead, to check on screen that the redirect reaches the renderer. Uses devkitA64 + libnx when DEVKITPRO is
# set, the devkitpro/devkita64 Docker image otherwise. Make cannot handle spaces, so keep this folder on a space-free path.
set -e
P="$(cd "$(dirname "$0")" && pwd)"
build() {
    if [ -n "$DEVKITPRO" ]; then
        make -C "$P" clean >/dev/null
        make -C "$P" -j"$(nproc)" C_FLAGS="$1"
    else
        W="$(cygpath -w "$P" 2>/dev/null || echo "$P")"
        MSYS_NO_PATHCONV=1 docker run --rm -v "$W:/work" -w /work devkitpro/devkita64 \
            sh -c "make clean >/dev/null && make -j8 C_FLAGS='$1'"
    fi
}
rm -rf "$P/dist"
# No main.npdm: the game's own one is used, so this mod never fights another mod that ships its own main.npdm.
build ""
D="$P/dist/Origins Max LOD/exefs"; mkdir -p "$D"
cp "$P/deploy/subsdk7" "$D/"
cp "$P/README.md" "$P/LICENSE" "$D/.."
build "-DLOD_TARGET=2"
D="$P/dist/Origins Max LOD (test - forces LOWEST detail)/exefs"; mkdir -p "$D"
cp "$P/deploy/subsdk7" "$D/"
cp "$P/README.md" "$P/LICENSE" "$D/.."
ls -lR "$P/dist"
