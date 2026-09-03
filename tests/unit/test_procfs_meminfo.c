#include "fixture.h"
#include "procfs.h"

#include <string.h>

static void check_fixture(const char *path, const char *buf, size_t len, void *ud)
{
    struct pme_memory m;

    (void)ud;
    memset(&m, 0, sizeof(m));
    PME_TEST_CHECK(pme_parse_meminfo(buf, len, &m) == PME_OK, "%s", path);
    PME_TEST_CHECK(m.total > 0u, "%s: total", path);
    PME_TEST_CHECK(m.free <= m.total, "%s: free>total", path);
    PME_TEST_CHECK(m.available == 0u || m.available <= m.total, "%s: avail", path);
    PME_TEST_CHECK(m.swap_free <= m.swap_total, "%s: swap", path);
}

int main(void)
{

    {
        static const char t[] =
            "MemTotal:        100 kB\n"
            "MemFree:          50 kB\n"
            "MemAvailable:     60 kB\n"
            "Buffers:          10 kB\n"
            "Cached:           20 kB\n"
            "SwapTotal:        30 kB\n"
            "SwapFree:          5 kB\n"
            "VmallocTotal:      0 kB\n"
            "HugePages_Total:   0\n";
        struct pme_memory m;
        memset(&m, 0, sizeof(m));
        PME_TEST_CHECK(pme_parse_meminfo(t, sizeof(t) - 1u, &m) == PME_OK, "valid");
        PME_TEST_CHECK(m.total == 100u * 1024u, "total=%llu", (unsigned long long)m.total);
        PME_TEST_CHECK(m.free == 50u * 1024u, "free");
        PME_TEST_CHECK(m.available == 60u * 1024u, "available");
        PME_TEST_CHECK(m.buffers == 10u * 1024u, "buffers");
        PME_TEST_CHECK(m.cached == 20u * 1024u, "cached");
        PME_TEST_CHECK(m.swap_total == 30u * 1024u, "swap_total");
        PME_TEST_CHECK(m.swap_free == 5u * 1024u, "swap_free");
    }

    {
        static const char t[] = "MemTotal: 100 kB\nMemFree: 50 kB\n";
        struct pme_memory m;
        memset(&m, 0, sizeof(m));
        PME_TEST_CHECK(pme_parse_meminfo(t, sizeof(t) - 1u, &m) == PME_OK, "old");
        PME_TEST_CHECK(m.total == 102400u && m.available == 0u, "old vals");
    }

    {
        static const char t[] = "MemTotal:\n";
        struct pme_memory m;
        PME_TEST_CHECK(pme_parse_meminfo(t, sizeof(t) - 1u, &m) == PME_EIO, "no value");
    }

    {
        static const char t[] = "MemTotal: abc kB\n";
        struct pme_memory m;
        PME_TEST_CHECK(pme_parse_meminfo(t, sizeof(t) - 1u, &m) == PME_EIO, "bad value");
    }

    {
        static const char t[] = "MemTotal: 4";
        struct pme_memory m;
        memset(&m, 0, sizeof(m));
        PME_TEST_CHECK(pme_parse_meminfo(t, sizeof(t) - 1u, &m) == PME_OK, "trunc");
        PME_TEST_CHECK(m.total == 4096u, "trunc val");
    }
    {
        struct pme_memory m;
        PME_TEST_CHECK(pme_parse_meminfo("", 0, &m) == PME_EIO, "empty");
    }

    PME_TEST_CHECK(pmetest_for_each_fixture("proc/meminfo", check_fixture, NULL) >= 1,
                   "no fixtures exercised - capture.sh output missing?");
    printf("ok %s\n", __FILE__);
    return 0;
}
