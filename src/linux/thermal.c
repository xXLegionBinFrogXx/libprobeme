#include "sections.h"

#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "procfs.h"

#define THERMAL_ROOT "/sys/class/thermal"

static int read_small_file(const char *path, char *buf, size_t cap)
{
    int fd = open(path, O_RDONLY);
    ssize_t n;

    if (fd < 0) {
        return -1;
    }
    n = read(fd, buf, cap - 1u);
    close(fd);
    if (n < 0) {
        return -1;
    }
    buf[n] = '\0';
    return 0;
}

static int thermal_path(char *dst, size_t cap, const char *zone,
                        const char *leaf)
{
    size_t o = 0;
    size_t n = strlen(THERMAL_ROOT);

    if (o + n + 1u >= cap) {
        return -1;
    }
    memcpy(dst, THERMAL_ROOT, n);
    o += n;
    dst[o++] = '/';

    n = strlen(zone);
    if (o + n + 1u >= cap) {
        return -1;
    }
    memcpy(dst + o, zone, n);
    o += n;

    if (leaf != NULL) {
        n = strlen(leaf);
        if (o + n + 1u >= cap) {
            return -1;
        }
        dst[o++] = '/';
        memcpy(dst + o, leaf, n);
        o += n;
    }
    dst[o] = '\0';
    return 0;
}

int pme_collect_thermal_section(struct pme_thermal *out)
{
    DIR *d = opendir(THERMAL_ROOT);
    struct dirent *e;
    uint32_t n = 0;

    if (d == NULL) {
        return PME_EIO;
    }
    while ((e = readdir(d)) != NULL && n < 32u) {
        char path[128];
        char content[64];
        char type_buf[sizeof(out->zones[0].type)];
        size_t len;
        int64_t temp;

        if (strncmp(e->d_name, "thermal_zone", 12) != 0) {
            continue;
        }

        if (thermal_path(path, sizeof(path), e->d_name, "type") != 0) {
            continue;
        }
        if (read_small_file(path, content, sizeof(content)) != 0) {
            continue;
        }
        len = strlen(content);
        while (len > 0u && (content[len - 1u] == '\n' ||
                            content[len - 1u] == ' ')) {
            content[--len] = '\0';
        }
        if (len == 0u || len >= sizeof(type_buf)) {
            continue;
        }
        memcpy(type_buf, content, len + 1u);

        if (thermal_path(path, sizeof(path), e->d_name, "temp") != 0) {
            continue;
        }
        if (read_small_file(path, content, sizeof(content)) != 0) {
            continue;
        }
        {
            const char *p = content;
            const char *pe = content + strlen(content);
            if (pme_parse_i64(&p, pe, &temp) != 0) {
                continue;
            }
        }

        memcpy(out->zones[n].type, type_buf, len + 1u);
        out->zones[n].temp_mc = temp;
        n++;
    }
    closedir(d);

    out->n = n;
    out->read_at_ns = pme_now_ns();
    return PME_OK;
}
