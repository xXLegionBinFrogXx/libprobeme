#ifndef PROBEME_LINUX_READFD_H
#define PROBEME_LINUX_READFD_H

#include <errno.h>
#include <unistd.h>

/* Re-read an already-open procfs/sysfs fd from offset 0 into buf.
 * Returns 0 and sets *out_len, or -1. Never reads past cap. */
static inline int pme_read_fd(int fd, char *buf, size_t cap, size_t *out_len)
{
    size_t off = 0;

    if (fd < 0 || lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        return -1;
    }
    while (off < cap) {
        ssize_t n = read(fd, buf + off, cap - off);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            break;
        }
        off += (size_t)n;
    }
    *out_len = off;
    return 0;
}

#endif /* PROBEME_LINUX_READFD_H */
