#include "fixture.h"
#include "procfs.h"

#include <string.h>

static void check_fixture(const char *path, const char *buf, size_t len, void *ud)
{
    struct pme_disk_io io;
    uint32_t i;

    (void)ud;
    memset(&io, 0, sizeof(io));
    PME_TEST_CHECK(pme_parse_diskstats(buf, len, &io) == PME_OK, "%s", path);
    PME_TEST_CHECK(io.n >= 1u, "%s: n=%u", path, io.n);
    for (i = 0; i < io.n; i++) {
        PME_TEST_CHECK(io.disks[i].name[0] != '\0', "%s: name[%u]", path, i);
        PME_TEST_CHECK(io.disks[i].read_bytes % 512u == 0u, "%s: read_bytes", path);
        PME_TEST_CHECK(io.disks[i].write_bytes % 512u == 0u, "%s: write_bytes", path);
    }
}

static void check_disk(const struct pme_disk *d, uint64_t reads, uint64_t rb,
                       uint64_t rt, uint64_t writes, uint64_t wb, uint64_t wt,
                       const char *tag)
{
    PME_TEST_CHECK(d->reads == reads && d->read_bytes == rb, "%s reads/rb", tag);
    PME_TEST_CHECK(d->read_time_ms == rt && d->writes == writes, "%s rt/w", tag);
    PME_TEST_CHECK(d->write_bytes == wb && d->write_time_ms == wt, "%s wb/wt", tag);
}

int main(void)
{

    {
        static const char t[] =
            "   8       0 sda 100 0 2000 40 50 0 1000 30 0 70 70\n"
            "   8       1 sda1 10 0 20 4 5 0 10 3 0 7 7\n";
        struct pme_disk_io io;
        memset(&io, 0, sizeof(io));
        PME_TEST_CHECK(pme_parse_diskstats(t, sizeof(t) - 1u, &io) == PME_OK, "14col");
        PME_TEST_CHECK(io.n == 2u, "n=%u", io.n);
        check_disk(&io.disks[0], 100u, 2000u * 512u, 40u, 50u, 1000u * 512u, 30u, "14col d0");
        PME_TEST_CHECK(strcmp(io.disks[0].name, "sda") == 0, "name0");
        PME_TEST_CHECK(strcmp(io.disks[1].name, "sda1") == 0, "name1");
    }

    {
        static const char t[] =
            "   8       0 nvme0n1 100 0 2000 40 50 0 1000 30 0 70 70 0 0 0 0 0 0\n";
        struct pme_disk_io io;
        memset(&io, 0, sizeof(io));
        PME_TEST_CHECK(pme_parse_diskstats(t, sizeof(t) - 1u, &io) == PME_OK, "20col");
        PME_TEST_CHECK(io.n == 1u, "n=%u", io.n);
        check_disk(&io.disks[0], 100u, 2000u * 512u, 40u, 50u, 1000u * 512u, 30u, "20col");
        PME_TEST_CHECK(io.disks[0].io_in_progress == 0u && io.disks[0].io_time_ms == 70u,
                       "20col io");
    }

    {
        static const char t[] = "   8       0 sda 1 2 3\n";
        struct pme_disk_io io;
        PME_TEST_CHECK(pme_parse_diskstats(t, sizeof(t) - 1u, &io) == PME_EIO, "short");
    }

    {
        static const char t[] = "garbage\n";
        struct pme_disk_io io;
        PME_TEST_CHECK(pme_parse_diskstats(t, sizeof(t) - 1u, &io) == PME_EIO, "garbage");
    }

    {
        struct pme_disk_io io;
        PME_TEST_CHECK(pme_parse_diskstats("", 0, &io) == PME_EIO, "empty");
    }

    PME_TEST_CHECK(pmetest_for_each_fixture("proc/diskstats", check_fixture, NULL) >= 1,
                   "no fixtures exercised - capture.sh output missing?");
    printf("ok %s\n", __FILE__);
    return 0;
}
