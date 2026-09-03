# probeme architecture

## What it is

`libprobeme` reads Linux system counters and fills a caller-owned snapshot
(`struct pme_snapshot`, ~56 KB). The header is a sysinfo API: raw monotonic
kernel counters, no rates, no Prometheus vocabulary, no heap allocation on
the collect path, no threads.

## Provider model

Each backend is its own shared object exporting exactly two symbols:

    pme_abi_version()          (major<<16)|minor
    pme_provider_get()         -> const struct pme_provider *

`struct pme_provider` carries `capabilities`, `init`, `collect_all`,
`destroy`. Consumers dlopen one or more providers and point them at the
same snapshot: `collect_all` writes only sections in its `capabilities`,
only on success, and marks them in `valid`. Sections are never cleared by a
provider that does not own them, so `linux` + `nvml` can fill one snapshot
in any order.

## Repos layout

    include/probeme.h        the ABI; append-only after v1.0
    src/common/              exported symbols, snapshot validation, clock
    src/linux/               provider + pure /proc parsers (one file each)
    src/nvml/                provider + NVML queries (vendored nvml.h)
    tests/abi/               layout lock: every offset/size pinned
    tests/unit/              parser tests (run anywhere, no root)
    tests/fixtures/          committed /proc snapshots per arch/kernel
    tests/live/              dlopen the built .so on real hardware
    dev/                     Dockerfile, remote sync, packaging

## Linux provider data flow

    init()    open /proc/{stat,meminfo,loadavg,uptime,diskstats},
              /proc/net/dev, /proc/self/mounts into an fd table
    collect() for each section: lseek(0)+read into a stack buffer
              (64 KB worst case), pure parser -> struct, stamp read_at_ns,
              set valid bit; set truncated bit when an array cap is hit
    destroy() close fds

Details worth remembering:

- **disk_io** filters `/proc/diskstats` against `/sys/block` so partitions
  disappear; loop/dm devices stay (they are real block devices).
- **filesystem** keeps kernel path escapes in `mountpoint` (\040 etc.) but
  unescapes for `statvfs`. Remote FS types are listed with
  `PME_MOUNT_SKIPPED` and zero sizes unless
  `PME_CFG_FS_INCLUDE_REMOTE`. A failed `statvfs` also shows up as
  SKIPPED - "no data" and "skipped by policy" are the same state for the
  consumer.
- **uptime** derives `boot_time_unix_s` from the wall clock minus uptime;
  everything else is monotonic.
- **thermal** walks `/sys/class/thermal/thermal_zone*` per call (readdir,
  not glob - glob allocates).

## NVML provider

`nvmlInit_v2` once in init, device handles cached, instantaneous queries in
collect (temp, power mW, SM clock, utilization, pstate, uuid, name). No
VRAM series: unified-memory parts (GB10) report meaningless framebuffer
numbers. The vendored `third_party/nvml/nvml.h` lets the .so build anywhere;
without `libnvidia-ml.so.1` at load time, dlopen simply fails - consumers
treat that as "no GPU".

## Rules (why the code looks like this)

- No malloc, no stdio on the collect path: `pread`/`read` only. Stack
  buffers sized for the largest realistic /proc file (256 CPUs ~ 40 KB).
- Parsers are pure `(buf, len, out) -> PME_OK | PME_EIO` and never read
  past `len`; corrupt lines are skipped, not fatal.
- `-Werror` with `-Wconversion -Wshadow`. A warning is a bug.
- ABI changes: minor = append-only (field at struct end, new caps/flags),
  major = anything else. `tests/abi/layout.c` must pass on x86_64 and
  aarch64 before any header change lands.

## Development loop

- Local (any host incl. macOS): `ctest --preset asan` runs the ABI + parser
  tests; provider targets are Linux-gated.
- Remote Linux (DGX Spark): `dev/sync.sh <host> [preset]` - rsync, cmake,
  build, ctest including live tests and NVML.
- x86_64 sanity: `dev/Dockerfile` (--platform linux/amd64).
- Packaging: `dev/package.sh` -> `libprobeme-<ver>-linux-<arch>.tar.gz`
  (`lib/`, `include/`, `lib/pkgconfig/`).
