#include "procfs.h"

#include <string.h>

static const char *skip_spaces(const char *p, const char *end)
{
    while (p < end && (*p == ' ' || *p == '\t')) {
        p++;
    }
    return p;
}

int pme_parse_u64(const char **pp, const char *end, uint64_t *out)
{
    const char *p = *pp;
    uint64_t v = 0;
    int any = 0;

    while (p < end && *p >= '0' && *p <= '9') {
        uint64_t d = (uint64_t)(*p - '0');
        if (v > (UINT64_MAX - d) / 10u) {
            return -1;
        }
        v = v * 10u + d;
        any = 1;
        p++;
    }
    if (!any) {
        return -1;
    }
    *out = v;
    *pp = p;
    return 0;
}

int pme_parse_i64(const char **pp, const char *end, int64_t *out)
{
    const char *p = *pp;
    int neg = 0;
    uint64_t v = 0;

    if (p < end && *p == '-') {
        neg = 1;
        p++;
    }
    if (pme_parse_u64(&p, end, &v) != 0) {
        return -1;
    }
    if (neg) {
        if (v > (uint64_t)INT64_MAX + 1u) {
            return -1;
        }
        *out = (v == (uint64_t)INT64_MAX + 1u) ? INT64_MIN : -(int64_t)v;
    } else {
        if (v > (uint64_t)INT64_MAX) {
            return -1;
        }
        *out = (int64_t)v;
    }
    *pp = p;
    return 0;
}

static int parse_double(const char **pp, const char *end, double *out)
{
    const char *p = *pp;
    int neg = 0;
    int any = 0;
    uint64_t ipart = 0;
    double frac = 0.0;
    double fscale = 1.0;
    double v;

    if (p < end && *p == '-') {
        neg = 1;
        p++;
    }
    while (p < end && *p >= '0' && *p <= '9') {
        uint64_t d = (uint64_t)(*p - '0');
        if (ipart > (UINT64_MAX - d) / 10u) {
            return -1;
        }
        ipart = ipart * 10u + d;
        any = 1;
        p++;
    }
    if (p < end && *p == '.') {
        p++;
        while (p < end && *p >= '0' && *p <= '9') {
            if (fscale < 1e12) {
                frac = frac * 10.0 + (double)(*p - '0');
                fscale *= 10.0;
            }
            any = 1;
            p++;
        }
    }
    if (!any) {
        return -1;
    }
    v = (double)ipart + frac / fscale;
    *out = neg ? -v : v;
    *pp = p;
    return 0;
}

static int match_key(const char **pp, const char *le, const char *key)
{
    size_t klen = strlen(key);
    const char *p = *pp;

    if ((size_t)(le - p) < klen || memcmp(p, key, klen) != 0) {
        return -1;
    }
    *pp = p + klen;
    return 0;
}

static void copy_str(char *dst, size_t cap, const char *src, size_t n)
{
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static uint64_t sectors_to_bytes(uint64_t sectors)
{
    if (sectors > UINT64_MAX / 512u) {
        return UINT64_MAX;
    }
    return sectors * 512u;
}

int pme_parse_proc_stat(const char *buf, size_t len, struct pme_cpu *out)
{
    const char *p;
    const char *end;
    uint32_t n = 0;

    if (buf == NULL || out == NULL || len == 0) {
        return PME_EIO;
    }
    p = buf;
    end = buf + len;

    while (p < end) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        const char *le = (nl != NULL) ? nl : end;
        const char *q = skip_spaces(p, le);

        if ((size_t)(le - q) >= 3u && memcmp(q, "cpu", 3) == 0 &&
            (q + 3 == le || q[3] == ' ' || q[3] == '\t' ||
             (q[3] >= '0' && q[3] <= '9'))) {
            q += 3;

            while (q < le && *q >= '0' && *q <= '9') {
                q++;
            }
            if (n < 256u) {
                uint64_t v[8] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
                int ok = 1;
                int i;

                for (i = 0; i < 8; i++) {
                    q = skip_spaces(q, le);
                    if (q >= le) {
                        break;
                    }
                    if (*q < '0' || *q > '9') {
                        ok = 0;
                        break;
                    }
                    if (pme_parse_u64(&q, le, &v[i]) != 0) {
                        ok = 0;
                        break;
                    }
                }
                if (ok) {
                    struct pme_cpu_core *c = &out->cpu[n];
                    c->user = v[0];
                    c->nice = v[1];
                    c->system = v[2];
                    c->idle = v[3];
                    c->iowait = v[4];
                    c->irq = v[5];
                    c->softirq = v[6];
                    c->steal = v[7];
                    n++;
                }
            }
        } else {
            break;
        }
        p = (nl != NULL) ? nl + 1 : end;
    }

    if (n == 0u) {
        return PME_EIO;
    }
    out->n = n;
    return PME_OK;
}

int pme_parse_meminfo(const char *buf, size_t len, struct pme_memory *out)
{
    const char *p;
    const char *end;
    uint64_t total = 0;
    uint64_t freek = 0;
    uint64_t avail = 0;
    uint64_t buffers = 0;
    uint64_t cached = 0;
    uint64_t swap_total = 0;
    uint64_t swap_free = 0;
    int got = 0;

    if (buf == NULL || out == NULL || len == 0) {
        return PME_EIO;
    }
    p = buf;
    end = buf + len;

    while (p < end) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        const char *le = (nl != NULL) ? nl : end;
        const char *q = skip_spaces(p, le);
        uint64_t v = 0;
        uint64_t *target = NULL;

        if (match_key(&q, le, "MemTotal:") == 0) {
            target = &total;
        } else if (match_key(&q, le, "MemFree:") == 0) {
            target = &freek;
        } else if (match_key(&q, le, "MemAvailable:") == 0) {
            target = &avail;
        } else if (match_key(&q, le, "Buffers:") == 0) {
            target = &buffers;
        } else if (match_key(&q, le, "Cached:") == 0) {
            target = &cached;
        } else if (match_key(&q, le, "SwapTotal:") == 0) {
            target = &swap_total;
        } else if (match_key(&q, le, "SwapFree:") == 0) {
            target = &swap_free;
        }

        if (target != NULL) {
            q = skip_spaces(q, le);
            if (pme_parse_u64(&q, le, &v) == 0) {
                *target = (v > UINT64_MAX / 1024u) ? UINT64_MAX : v * 1024u;
                got = 1;
            }
        }
        p = (nl != NULL) ? nl + 1 : end;
    }

    if (!got) {
        return PME_EIO;
    }
    out->total = total;
    out->free = freek;
    out->available = avail;
    out->buffers = buffers;
    out->cached = cached;
    out->swap_total = swap_total;
    out->swap_free = swap_free;
    return PME_OK;
}

int pme_parse_loadavg(const char *buf, size_t len, struct pme_loadavg *out)
{
    const char *p;
    const char *end;
    const char *nl;
    const char *le;
    double l1 = 0.0;
    double l5 = 0.0;
    double l15 = 0.0;
    uint64_t running = 0;
    uint64_t total = 0;

    if (buf == NULL || out == NULL || len == 0) {
        return PME_EIO;
    }
    p = buf;
    end = buf + len;
    nl = memchr(p, '\n', (size_t)(end - p));
    le = (nl != NULL) ? nl : end;

    p = skip_spaces(p, le);
    if (parse_double(&p, le, &l1) != 0) {
        return PME_EIO;
    }
    p = skip_spaces(p, le);
    if (parse_double(&p, le, &l5) != 0) {
        return PME_EIO;
    }
    p = skip_spaces(p, le);
    if (parse_double(&p, le, &l15) != 0) {
        return PME_EIO;
    }
    p = skip_spaces(p, le);
    if (pme_parse_u64(&p, le, &running) != 0) {
        return PME_EIO;
    }
    if (p >= le || *p != '/') {
        return PME_EIO;
    }
    p++;
    if (pme_parse_u64(&p, le, &total) != 0) {
        return PME_EIO;
    }

    out->load1 = l1;
    out->load5 = l5;
    out->load15 = l15;
    out->running = (running > (uint64_t)UINT32_MAX) ? UINT32_MAX
                                                    : (uint32_t)running;
    out->total = (total > (uint64_t)UINT32_MAX) ? UINT32_MAX : (uint32_t)total;
    return PME_OK;
}

int pme_parse_uptime(const char *buf, size_t len, struct pme_uptime *out)
{
    const char *p;
    const char *end;
    const char *nl;
    const char *le;
    double up = 0.0;

    if (buf == NULL || out == NULL || len == 0) {
        return PME_EIO;
    }
    p = buf;
    end = buf + len;
    nl = memchr(p, '\n', (size_t)(end - p));
    le = (nl != NULL) ? nl : end;

    p = skip_spaces(p, le);
    if (parse_double(&p, le, &up) != 0) {
        return PME_EIO;
    }
    if (up < 0.0) {
        return PME_EIO;
    }
    out->uptime_s = (up >= 1e18) ? (uint64_t)1e18 : (uint64_t)up;
    return PME_OK;
}

int pme_parse_diskstats(const char *buf, size_t len, struct pme_disk_io *out)
{
    const char *p;
    const char *end;
    uint32_t n = 0;

    if (buf == NULL || out == NULL || len == 0) {
        return PME_EIO;
    }
    p = buf;
    end = buf + len;

    while (p < end && n < 64u) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        const char *le = (nl != NULL) ? nl : end;
        const char *q = skip_spaces(p, le);
        uint64_t v[10] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
        uint64_t discard = 0;
        const char *name;
        size_t name_len;
        int i;

        if (pme_parse_u64(&q, le, &discard) != 0) {
            goto next;
        }
        q = skip_spaces(q, le);
        if (pme_parse_u64(&q, le, &discard) != 0) {
            goto next;
        }
        q = skip_spaces(q, le);
        name = q;
        while (q < le && *q != ' ' && *q != '\t') {
            q++;
        }
        name_len = (size_t)(q - name);
        if (name_len == 0 || name_len >= sizeof(out->disks[0].name)) {
            goto next;
        }

        for (i = 0; i < 10; i++) {
            q = skip_spaces(q, le);
            if (pme_parse_u64(&q, le, &v[i]) != 0) {
                goto next;
            }
        }

        {
            struct pme_disk *d = &out->disks[n];
            copy_str(d->name, sizeof(d->name), name, name_len);
            d->reads = v[0];
            d->read_bytes = sectors_to_bytes(v[2]);
            d->read_time_ms = v[3];
            d->writes = v[4];
            d->write_bytes = sectors_to_bytes(v[6]);
            d->write_time_ms = v[7];
            d->io_in_progress = v[8];
            d->io_time_ms = v[9];
            n++;
        }

next:
        p = (nl != NULL) ? nl + 1 : end;
    }

    if (n == 0u) {
        return PME_EIO;
    }
    out->n = n;
    return PME_OK;
}

int pme_parse_net_dev(const char *buf, size_t len, struct pme_netdev *out)
{
    const char *p;
    const char *end;
    uint32_t n = 0;

    if (buf == NULL || out == NULL || len == 0) {
        return PME_EIO;
    }
    p = buf;
    end = buf + len;

    while (p < end && n < 64u) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        const char *le = (nl != NULL) ? nl : end;
        const char *colon = memchr(p, ':', (size_t)(le - p));
        const char *name;
        const char *name_end;
        const char *q;
        size_t name_len;
        uint64_t f[12];
        int i;

        if (colon == NULL) {
            goto next;
        }
        name = skip_spaces(p, colon);
        name_end = colon;
        while (name_end > name && (name_end[-1] == ' ' || name_end[-1] == '\t')) {
            name_end--;
        }
        name_len = (size_t)(name_end - name);
        if (name_len == 0 || name_len >= sizeof(out->ifaces[0].name)) {
            goto next;
        }

        q = colon + 1;
        for (i = 0; i < 12; i++) {
            q = skip_spaces(q, le);
            if (pme_parse_u64(&q, le, &f[i]) != 0) {
                goto next;
            }
        }

        {
            struct pme_iface *ifp = &out->ifaces[n];
            copy_str(ifp->name, sizeof(ifp->name), name, name_len);
            ifp->rx_bytes = f[0];
            ifp->rx_packets = f[1];
            ifp->rx_errs = f[2];
            ifp->rx_drop = f[3];
            ifp->tx_bytes = f[8];
            ifp->tx_packets = f[9];
            ifp->tx_errs = f[10];
            ifp->tx_drop = f[11];
            n++;
        }

next:
        p = (nl != NULL) ? nl + 1 : end;
    }

    if (n == 0u) {
        return PME_EIO;
    }
    out->n = n;
    return PME_OK;
}

static int is_remote_fstype(const char *s, size_t n)
{
    static const char *const remote[] = {
        "nfs", "nfs4", "nfs41", "cifs", "smbfs", "smb2", "afs", "ncpfs",
        "9p", "ceph", "cephfs", "glusterfs", "fuse.sshfs", "fuse.cephfs",
        "davfs", "davfs2", "ocfs2", NULL
    };
    int i;

    for (i = 0; remote[i] != NULL; i++) {
        size_t rlen = strlen(remote[i]);
        if (n == rlen && memcmp(s, remote[i], rlen) == 0) {
            return 1;
        }
    }

    if (n >= 3u && (memcmp(s, "nfs", 3) == 0)) {
        return 1;
    }
    return 0;
}

static int has_opt(const char *p, const char *le, const char *opt)
{
    size_t olen = strlen(opt);
    const char *tok = p;

    while (p <= le) {
        if (p == le || *p == ',' || *p == ' ' || *p == '\t') {
            if ((size_t)(p - tok) == olen && memcmp(tok, opt, olen) == 0) {
                return 1;
            }
            tok = p + 1;
        }
        p++;
    }
    return 0;
}

int pme_parse_self_mounts(const char *buf, size_t len, struct pme_filesystem *out)
{
    const char *p;
    const char *end;
    uint32_t n = 0;

    if (buf == NULL || out == NULL || len == 0) {
        return PME_EIO;
    }
    p = buf;
    end = buf + len;

    while (p < end && n < 64u) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        const char *le = (nl != NULL) ? nl : end;
        const char *q = p;
        const char *dev;        const char *mntp;
        const char *fst;
        size_t dev_len;
        size_t mnt_len;
        size_t fst_len;
        uint32_t flags = 0;
        struct pme_mount *m;

        dev = q;
        while (q < le && *q != ' ' && *q != '\t') {
            q++;
        }
        dev_len = (size_t)(q - dev);
        if (dev_len == 0) {
            goto next;
        }
        q = skip_spaces(q, le);

        mntp = q;
        while (q < le && *q != ' ' && *q != '\t') {
            q++;
        }
        mnt_len = (size_t)(q - mntp);
        if (mnt_len == 0) {
            goto next;
        }
        q = skip_spaces(q, le);

        fst = q;
        while (q < le && *q != ' ' && *q != '\t') {
            q++;
        }
        fst_len = (size_t)(q - fst);
        if (fst_len == 0) {
            goto next;
        }
        q = skip_spaces(q, le);

        if (has_opt(q, le, "ro")) {
            flags |= PME_MOUNT_RO;
        }
        if (is_remote_fstype(fst, fst_len)) {
            flags |= PME_MOUNT_SKIPPED;
        }

        m = &out->mounts[n];
        copy_str(m->device, sizeof(m->device), dev, dev_len);
        copy_str(m->mountpoint, sizeof(m->mountpoint), mntp, mnt_len);
        copy_str(m->fstype, sizeof(m->fstype), fst, fst_len);
        m->flags = flags;
        n++;

next:
        p = (nl != NULL) ? nl + 1 : end;
    }

    if (n == 0u) {
        return PME_EIO;
    }
    out->n = n;
    return PME_OK;
}
