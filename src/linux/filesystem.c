#include "sections.h"

#include <sys/statvfs.h>
#include <string.h>

#include "procfs.h"
#include "readfd.h"

/* /proc/self/mounts escapes space, tab and backslash as \040, \011, \134. */
static void unescape_path(char *dst, size_t cap, const char *src, size_t n)
{
    size_t o = 0;
    size_t i = 0;

    while (i < n && o + 1u < cap) {
        if (src[i] == '\\' && i + 3u < n + 1u && i + 3u <= n &&
            src[i + 1] >= '0' && src[i + 1] <= '7' &&
            src[i + 2] >= '0' && src[i + 2] <= '7' &&
            src[i + 3] >= '0' && src[i + 3] <= '7') {
            dst[o] = (char)(((src[i + 1] - '0') << 6) |
                            ((src[i + 2] - '0') << 3) |
                            (src[i + 3] - '0'));
            i += 4;
        } else {
            dst[o] = src[i];
            i++;
        }
        o++;
    }
    dst[o] = '\0';
}

int pme_collect_filesystem_section(int fd, struct pme_filesystem *out,
                                   uint64_t cfg_flags)
{
    char buf[65536];
    size_t len;
    uint32_t i;

    if (pme_read_fd(fd, buf, sizeof(buf), &len) != 0) {
        return PME_EIO;
    }
    if (pme_parse_self_mounts(buf, len, out) != PME_OK) {
        return PME_EIO;
    }

    for (i = 0; i < out->n; i++) {
        struct pme_mount *m = &out->mounts[i];
        int skip = (m->flags & PME_MOUNT_SKIPPED) != 0u;
        char path[sizeof(m->mountpoint)];
        struct statvfs st;

        if (skip && (cfg_flags & PME_CFG_FS_INCLUDE_REMOTE) == 0u) {
            continue;
        }
        unescape_path(path, sizeof(path), m->mountpoint,
                      strlen(m->mountpoint));
        if (statvfs(path, &st) != 0) {
            m->flags |= PME_MOUNT_SKIPPED;
            continue;
        }
        m->flags &= (uint32_t)~PME_MOUNT_SKIPPED;
        m->size_bytes = (uint64_t)st.f_frsize * (uint64_t)st.f_blocks;
        m->free_bytes = (uint64_t)st.f_frsize * (uint64_t)st.f_bfree;
        m->avail_bytes = (uint64_t)st.f_frsize * (uint64_t)st.f_bavail;
        m->files = (uint64_t)st.f_files;
        m->files_free = (uint64_t)st.f_ffree;
    }

    out->read_at_ns = pme_now_ns();
    return PME_OK;
}
