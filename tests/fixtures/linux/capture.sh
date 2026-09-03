#!/bin/sh
# Capture /proc snapshots for parser fixtures.
# Run once per host (real hardware beats hand-crafted files), then commit.
#   tests/fixtures/linux/<arch>/<kernel>/proc/<file>
set -eu

arch=$(uname -m)
kernel=$(uname -r)
here=$(cd "$(dirname "$0")" && pwd)
out="$here/${arch}/${kernel}"

mkdir -p "$out/proc"
cp /proc/stat        "$out/proc/stat"
cp /proc/meminfo     "$out/proc/meminfo"
cp /proc/loadavg     "$out/proc/loadavg"
cp /proc/uptime      "$out/proc/uptime"
cp /proc/diskstats   "$out/proc/diskstats"
cp /proc/net/dev     "$out/proc/net_dev"
cp /proc/self/mounts "$out/proc/self_mounts"
chmod 644 "$out"/proc/*

echo "captured $(uname -a)"
echo "-> $out"
