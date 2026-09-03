#!/bin/sh
# Build a release tree and package a distribution tarball:
#   dev/package.sh [preset]        (default: release)
# Output: libprobeme-<ver>-linux-<arch>.tar.gz with lib/, include/,
# lib/pkgconfig/ - exactly what probeme-exporter consumes.
set -eu

preset="${1:-release}"

cmake --preset "$preset"
cmake --build --preset "$preset" -j "$(nproc)"

rm -rf package/stage
cmake --install "build/$preset" --prefix "$PWD/package/stage"

arch=$(uname -m)
ver=$(sed -n 's/^[[:space:]]*VERSION \([0-9][0-9.]*\)$/\1/p' CMakeLists.txt | head -1)
tarball="libprobeme-${ver}-linux-${arch}.tar.gz"

tar -czf "$tarball" -C package/stage lib include
echo "built $tarball"
