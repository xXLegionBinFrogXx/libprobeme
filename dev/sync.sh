#!/bin/sh
# Sync the tree to a remote host (e.g. DGX Spark), build and test there.
#   dev/sync.sh spark-01 [preset]
# Preset: debug | release | asan | ubsan (default: release)
set -eu

host="${1:?usage: sync.sh <host> [preset]}"
preset="${2:-release}"

rsync -az --delete \
    --exclude '.git' \
    --exclude 'build/' \
    ./ "$host":/tmp/probeme/

ssh "$host" "set -eu
    cd /tmp/probeme
    cmake --preset $preset
    cmake --build --preset $preset -j \$(nproc)
    ctest --preset $preset"
