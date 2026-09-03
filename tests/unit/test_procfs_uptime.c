#include "fixture.h"
#include "procfs.h"

#include <string.h>

static void check_fixture(const char *path, const char *buf, size_t len, void *ud)
{
    struct pme_uptime u;

    (void)ud;
    memset(&u, 0, sizeof(u));
    PME_TEST_CHECK(pme_parse_uptime(buf, len, &u) == PME_OK, "%s", path);
    PME_TEST_CHECK(u.uptime_s > 0u, "%s: uptime=%llu", path,
                   (unsigned long long)u.uptime_s);
}

int main(void)
{

    {
        static const char t[] = "12345.67 8901.23\n";
        struct pme_uptime u;
        memset(&u, 0, sizeof(u));
        PME_TEST_CHECK(pme_parse_uptime(t, sizeof(t) - 1u, &u) == PME_OK, "valid");
        PME_TEST_CHECK(u.uptime_s == 12345u, "uptime=%llu",
                       (unsigned long long)u.uptime_s);
    }

    {
        static const char t[] = "42\n";
        struct pme_uptime u;
        memset(&u, 0, sizeof(u));
        PME_TEST_CHECK(pme_parse_uptime(t, sizeof(t) - 1u, &u) == PME_OK, "int");
        PME_TEST_CHECK(u.uptime_s == 42u, "int val");
    }

    {
        struct pme_uptime u;
        PME_TEST_CHECK(pme_parse_uptime("abc def\n", 8, &u) == PME_EIO, "garbage");
        PME_TEST_CHECK(pme_parse_uptime("", 0, &u) == PME_EIO, "empty");
    }

    PME_TEST_CHECK(pmetest_for_each_fixture("proc/uptime", check_fixture, NULL) >= 1,
                   "no fixtures exercised - capture.sh output missing?");
    printf("ok %s\n", __FILE__);
    return 0;
}
