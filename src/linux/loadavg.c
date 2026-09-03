#include "sections.h"

#include "procfs.h"
#include "readfd.h"

int pme_collect_loadavg_section(int fd, struct pme_loadavg *out)
{
    char buf[4096];
    size_t len;

    if (pme_read_fd(fd, buf, sizeof(buf), &len) != 0) {
        return PME_EIO;
    }
    if (pme_parse_loadavg(buf, len, out) != PME_OK) {
        return PME_EIO;
    }
    out->read_at_ns = pme_now_ns();
    return PME_OK;
}
