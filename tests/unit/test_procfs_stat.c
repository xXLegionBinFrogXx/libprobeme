#include "fixture.h"
#include "procfs.h"

#include <string.h>

static void check_fixture(const char *path, const char *buf, size_t len, void *ud)
{
    struct pme_cpu cpu;

    (void)ud;
    memset(&cpu, 0, sizeof(cpu));
    PME_TEST_CHECK(pme_parse_proc_stat(buf, len, &cpu) == PME_OK, "%s", path);
    PME_TEST_CHECK(cpu.n >= 2u, "%s: n=%u", path, cpu.n);
    PME_TEST_CHECK(cpu.cpu[0].user + cpu.cpu[0].idle > 0u, "%s: aggregate", path);
}

int main(void)
{
    /* valid: aggregate + two cores + non-cpu line ends the block */
    {
        static const char t[] =
            "cpu  100 10 200 3000 0 40 0 0 0 0\n"
            "cpu0 50 5 100 1500 0 20 0 0 0 0\n"
            "cpu1 50 5 100 1500 0 20 0 0 0 0\n"
            "intr 12345\n";
        struct pme_cpu c;
        memset(&c, 0, sizeof(c));
        PME_TEST_CHECK(pme_parse_proc_stat(t, sizeof(t) - 1u, &c) == PME_OK, "valid");
        PME_TEST_CHECK(c.n == 3u, "n=%u", c.n);
        PME_TEST_CHECK(c.cpu[0].user == 100u && c.cpu[0].steal == 0u, "aggregate");
        PME_TEST_CHECK(c.cpu[1].user == 50u && c.cpu[2].idle == 1500u, "cores");
    }
    /* exactly 8 fields, no trailing extras */
    {
        static const char t[] = "cpu 1 2 3 4 5 6 7 8\ncpu0 9 8 7 6 5 4 3 2\n";
        struct pme_cpu c;
        memset(&c, 0, sizeof(c));
        PME_TEST_CHECK(pme_parse_proc_stat(t, sizeof(t) - 1u, &c) == PME_OK, "8field");
        PME_TEST_CHECK(c.n == 2u && c.cpu[0].steal == 8u && c.cpu[1].user == 9u, "8field vals");
    }
    /* malformed aggregate: non-numeric field */
    {
        static const char t[] = "cpu x 1 2 3 4 5 6 7 8\n";
        struct pme_cpu c;
        memset(&c, 0, sizeof(c));
        PME_TEST_CHECK(pme_parse_proc_stat(t, sizeof(t) - 1u, &c) == PME_EIO, "bad agg");
    }
    /* truncated final line, no newline, fewer than 8 fields */
    {
        static const char t[] = "cpu 1 2";
        struct pme_cpu c;
        memset(&c, 0, sizeof(c));
        PME_TEST_CHECK(pme_parse_proc_stat(t, sizeof(t) - 1u, &c) == PME_OK, "trunc");
        PME_TEST_CHECK(c.n == 1u && c.cpu[0].user == 1u && c.cpu[0].softirq == 0u, "trunc vals");
    }
    /* empty */
    {
        struct pme_cpu c;
        PME_TEST_CHECK(pme_parse_proc_stat("", 0, &c) == PME_EIO, "empty");
    }

    pmetest_for_each_fixture("proc/stat", check_fixture, NULL);
    printf("ok %s\n", __FILE__);
    return 0;
}
