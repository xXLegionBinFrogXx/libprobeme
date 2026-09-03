<p align="center">
  <img src="docs/ProbeMeLogo.png" alt="ProbeMe logo" width="256">
</p>

# libprobeme

libprobeme is a C17 library for Linux that turns system counters into one
caller owned snapshot: CPU time, memory, load average, uptime, disk I/O,
filesystem usage, network devices, thermal zones and NVIDIA GPU state.

It reads raw kernel counters and hands them over untouched. No rates, no
allocation, no threads on the collect path, no background work. Two small
shared objects (`libprobeme_linux.so.1`, `libprobeme_nvml.so.1`) export a
stable, layout pinned ABI through exactly two symbols each, so any language
that can dlopen can use it. Multiple providers can fill the same snapshot,
which makes GPU stats a pure add on where a driver exists.

See [docs/ABI.md](docs/ABI.md) for the snapshot contract and
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for how it works inside.

## Build

```
cmake --preset release
cmake --build --preset release
ctest --preset release
```

## Quick look

```
./build/release/monitor 5 build/release/libprobeme_linux.so.1 \
                        build/release/libprobeme_nvml.so.1
```

## License

MIT, see [LICENSE](LICENSE).
