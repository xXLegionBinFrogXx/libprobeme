#include "fixture.h"
#include "procfs.h"

#include <math.h>
#include <string.h>

static void check_fixture(const char *path, const char *buf, size_t len, void *ud)
{
    struct pme_loadavg l;

    (void)ud;
    memset(&l, 0, sizeof(l));
    PME_TEST_CHECK(pme_parse_loadavg(buf, len, &l) == PME_OK, "%s", path);
    PME_TEST_CHECK(l.load1 >= 0.0 && l.load1 < 10000.0, "%s: load1=%f", path, l.load1);
    PME_TEST_CHECK(l.load5 >= 0.0 && l.load15 >= 0.0, "%s", path);
    PME_TEST_CHECK(l.total > 0u, "%s: total=%u", path, l.total);
    PME_TEST_CHECK(l.running <= l.total, "%s: running=%u total=%u", path,
                   l.running, l.total);
}

static int feq(double a, double b)
{
    return fabs(a - b) < 1e-9;
}

int main(void)
{
    /* valid */
    {
        static const char t[] = "0.52 0.58 0.59 1/444 12345\n";
        struct pme_loadavg l;
        memset(&l, 0, sizeof(l));
        PME_TEST_CHECK(pme_parse_loadavg(t, sizeof(t) - 1u, &l) == PME_OK, "valid");
        PME_TEST_CHECK(feq(l.load1, 0.52) && feq(l.load5, 0.58) && feq(l.load15, 0.59),
                       "l1=%f l5=%f l15=%f", l.load1, l.load5, l.load15);
        PME_TEST_CHECK(l.running == 1u && l.total == 444u, "run=%u tot=%u",
                       l.running, l.total);
    }
    /* no newline, pid missing */
    {
        static const char t[] = "0.00 0.01 0.02 0/1";
        struct pme_loadavg l;
        memset(&l, 0, sizeof(l));
        PME_TEST_CHECK(pme_parse_loadavg(t, sizeof(t) - 1u, &l) == PME_OK, "trunc");
        PME_TEST_CHECK(l.running == 0u && l.total == 1u, "trunc vals");
    }
    /* malformed: only two loads */
    {
        struct pme_loadavg l;
        PME_TEST_CHECK(pme_parse_loadavg("0.1 0.2\n", 7, &l) == PME_EIO, "short");
    }
    /* malformed: garbage */
    {
        struct pme_loadavg l;
        PME_TEST_CHECK(pme_parse_loadavg("a b c d/e f\n", 12, &l) == PME_EIO, "garbage");
    }
    /* malformed: missing run/total */
    {
        struct pme_loadavg l;
        PME_TEST_CHECK(pme_parse_loadavg("0.1 0.2 0.3\n", 12, &l) == PME_EIO, "no frac");
    }
    {
        struct pme_loadavg l;
        PME_TEST_CHECK(pme_parse_loadavg("", 0, &l) == PME_EIO, "empty");
    }

    PME_TEST_CHECK(pmetest_for_each_fixture("proc/loadavg", check_fixture, NULL) >= 1,
                   "no fixtures exercised - capture.sh output missing?");
    printf("ok %s\n", __FILE__);
    return 0;
}
