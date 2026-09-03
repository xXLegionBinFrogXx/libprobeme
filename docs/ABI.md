# probeme ABI (v1.0)

The consumer-facing contract of `libprobeme`. The header `include/probeme.h`
is the single source of truth; this document describes semantics.

## Versioning

- `PME_ABI_MAJOR` / `PME_ABI_MINOR`. `pme_abi_version()` returns
  `(major << 16) | minor`.
- **Minor**: append-only changes only — new fields at the end of structs
  (before flex-free fixed arrays), new capability bits, new config flags.
  Struct sizes grow; `size`-first versioning keeps old binaries safe.
- **Major**: anything that moves or repurposes existing fields.

`tests/abi/layout.c` pins every field offset and struct size. It must pass on
x86_64 and aarch64, gcc and clang, before any header change is committed.
All snapshot structs are pointer-free, so their layout is identical on both
architectures (LP64).

## Symbol contract

A provider shared object exports exactly two symbols:

- `pme_abi_version()`
- `pme_provider_get()` → `const struct pme_provider *`

Everything else is hidden (`-fvisibility=hidden`).

## Snapshot protocol

1. Caller allocates `struct pme_snapshot` once (~56 KB; static or heap),
   sets `size = sizeof(struct pme_snapshot)`.
2. Optional: compare `provider->abi_version` major with the header's before
   proceeding.
3. `provider->init(&config)` — `config.size = sizeof(struct pme_config)`.
   Returns `PME_OK`, `PME_EINVAL` (size too small or `reserved != 0`), or
   `PME_ENOTSUP` (backend unusable, e.g. no NVIDIA driver).
4. `provider->collect_all(&snap)` fills the snapshot.
5. `provider->destroy()` releases resources. Providers may be dlclosed
   after this.

Multiple providers may fill one snapshot in any order (e.g. `linux` then
`nvml`): each provider **only writes** sections in its `capabilities`, and
only those sections whose bits it sets in `valid`.

## collect_all semantics

- Returns `PME_OK` if **at least one** section succeeded.
- `PME_EINVAL`: `snap->size < sizeof(struct pme_snapshot)`.
- `PME_ENOINIT`: `init` was not called (or `destroy` already ran).
- `PME_EIO`: no section succeeded.
- Per-section failure: section bit stays clear in `valid`, section memory is
  **never written**.
- `abi_version` is stamped on every call that reaches validation.
- `valid`: bit per successfully filled section.
- `truncated`: bit per section where an array cap was hit; in that case the
  section's `n` equals the cap. `truncated` bits are only set for sections
  that also have their `valid` bit set.

## Semantics of the data

- **Clock**: every section carries `read_at_ns` = `CLOCK_MONOTONIC`
  nanoseconds taken just after reading that section's sources.
- **Counters** are raw kernel monotonic totals since boot. The library never
  accumulates, rates, or otherwise post-processes. Consumers compute deltas.
- **cpu**: `cpu[0]` is the kernel aggregate line; `cpu[1..n)` are per-CPU.
  `n == 1 + logical_cpu_count`, capped at 256.
- **memory**: bytes (`/proc/meminfo` kB × 1024). `available == 0` on kernels
  without `MemAvailable`.
- **loadavg**: kernel fixed-point values (two decimals), not full precision.
- **uptime**: `uptime_s` truncated; `boot_time_unix_s = wall_now - uptime_s`.
- **disk_io**: one entry per disk line of `/proc/diskstats`. Partitions are
  filtered out by matching against `/sys/block` names. Bytes = sectors × 512.
- **filesystem**: one entry per `/proc/self/mounts` line. Paths keep kernel
  escaping (`\040` etc.). Remote/userspace filesystems (nfs*, cifs*, 9p,
  ceph, glusterfs, fuse.sshfs, …) are listed but flagged
  `PME_MOUNT_SKIPPED` with zero size fields unless
  `PME_CFG_FS_INCLUDE_REMOTE` is set. `PME_MOUNT_RO` from the `ro` option.
- **thermal**: one entry per `/sys/class/thermal/thermal_zone*`;
  `temp_mc` in milli-degrees Celsius.
- **gpu** (nvml provider): instantaneous values. `pstate` numeric
  (0 = P0/highest). No VRAM series: unified-memory architectures (GB10)
  report meaningless framebuffer numbers.
- Strings are NUL-terminated and silently truncated to their fixed capacity.

## Array caps

| Array | Cap |
|---|---|
| `pme_cpu.cpu` | 256 |
| `pme_disk_io.disks` | 64 |
| `pme_filesystem.mounts` | 64 |
| `pme_netdev.ifaces` | 64 |
| `pme_thermal.zones` | 32 |
| `pme_gpu.gpus` | 8 |

## Return codes

| Code | Meaning |
|---|---|
| `PME_OK` (0) | success (≥1 section) |
| `PME_ENOTSUP` (-1) | backend not supported on this machine |
| `PME_EIO` (-2) | I/O failure (no section succeeded) |
| `PME_EINVAL` (-3) | bad struct size / config |
| `PME_ENOINIT` (-4) | `init` not called |

## Threading

None inside the library. `collect_all` is not reentrant; the caller
serializes.
