#include "sections.h"

#include <dirent.h>
#include <string.h>

#include "procfs.h"
#include "readfd.h"

/* /sys/block lists whole disks; anything else in diskstats is a partition. */
static int keep_only_block_devices(struct pme_disk_io *io)
{
    static const char sys_block[] = "/sys/block";
    char names[256][32];
    uint32_t nnames = 0;
    uint32_t kept = 0;
    uint32_t i;
    DIR *d;
    struct dirent *e;

    d = opendir(sys_block);
    if (d == NULL) {
        return -1;
    }
    while ((e = readdir(d)) != NULL && nnames < 256u) {
        size_t n = strlen(e->d_name);
        if (e->d_name[0] == '.' || n == 0 || n >= sizeof(names[0])) {
            continue;
        }
        memcpy(names[nnames], e->d_name, n + 1u);
        nnames++;
    }
    closedir(d);

    for (i = 0; i < io->n; i++) {
        uint32_t j;
        int match = 0;
        for (j = 0; j < nnames; j++) {
            if (strcmp(io->disks[i].name, names[j]) == 0) {
                match = 1;
                break;
            }
        }
        if (match) {
            if (kept != i) {
                io->disks[kept] = io->disks[i];
            }
            kept++;
        }
    }
    io->n = (nnames != 0u) ? kept : io->n;
    return 0;
}

int pme_collect_disk_io_section(int fd, struct pme_disk_io *out)
{
    char buf[65536];
    size_t len;

    if (pme_read_fd(fd, buf, sizeof(buf), &len) != 0) {
        return PME_EIO;
    }
    if (pme_parse_diskstats(buf, len, out) != PME_OK) {
        return PME_EIO;
    }
    (void)keep_only_block_devices(out);
    out->read_at_ns = pme_now_ns();
    return PME_OK;
}
