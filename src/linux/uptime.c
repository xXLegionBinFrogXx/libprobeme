#include "sections.h"

#include <time.h>

#include "procfs.h"
#include "readfd.h"

int pme_collect_uptime_section(int fd, struct pme_uptime *out)
{
    char buf[4096];
    size_t len;
    time_t wall;

    if (pme_read_fd(fd, buf, sizeof(buf), &len) != 0) {
        return PME_EIO;
    }
    if (pme_parse_uptime(buf, len, out) != PME_OK) {
        return PME_EIO;
    }
    wall = time(NULL);
    out->boot_time_unix_s = ((uint64_t)wall > out->uptime_s)
                                ? (uint64_t)wall - out->uptime_s
                                : 0u;
    out->read_at_ns = pme_now_ns();
    return PME_OK;
}
