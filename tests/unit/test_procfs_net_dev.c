#include "fixture.h"
#include "procfs.h"

#include <string.h>

static void check_fixture(const char *path, const char *buf, size_t len, void *ud)
{
    struct pme_netdev nd;
    uint32_t i;

    (void)ud;
    memset(&nd, 0, sizeof(nd));
    PME_TEST_CHECK(pme_parse_net_dev(buf, len, &nd) == PME_OK, "%s", path);
    PME_TEST_CHECK(nd.n >= 1u, "%s: n=%u", path, nd.n);
    for (i = 0; i < nd.n; i++) {
        PME_TEST_CHECK(nd.ifaces[i].name[0] != '\0', "%s: name[%u]", path, i);
    }
}

int main(void)
{
    /* valid: two header lines + two interfaces */
    {
        static const char t[] =
            "Inter-|   Receive                                                "
            "|  Transmit\n"
            " face |bytes    packets errs drop fifo frame compressed multicast"
            "|bytes    packets errs drop fifo colls carrier compressed\n"
            "    lo: 100 10 1 2 3 4 5 6 200 20 2 3 4 5 6 7\n"
            "  eth0: 1000 100 0 0 0 0 0 0 2000 200 0 0 0 0 0 0\n";
        struct pme_netdev nd;
        memset(&nd, 0, sizeof(nd));
        PME_TEST_CHECK(pme_parse_net_dev(t, sizeof(t) - 1u, &nd) == PME_OK, "valid");
        PME_TEST_CHECK(nd.n == 2u, "n=%u", nd.n);
        PME_TEST_CHECK(strcmp(nd.ifaces[0].name, "lo") == 0, "name0=%s",
                       nd.ifaces[0].name);
        PME_TEST_CHECK(nd.ifaces[0].rx_bytes == 100u && nd.ifaces[0].tx_bytes == 200u,
                       "lo counters");
        PME_TEST_CHECK(nd.ifaces[0].rx_drop == 2u && nd.ifaces[0].tx_errs == 2u,
                       "lo drop/errs");
        PME_TEST_CHECK(strcmp(nd.ifaces[1].name, "eth0") == 0, "name1");
        PME_TEST_CHECK(nd.ifaces[1].rx_bytes == 1000u && nd.ifaces[1].tx_packets == 200u,
                       "eth0 counters");
    }
    /* malformed: 11 counters only -> skipped */
    {
        static const char t[] = "  x0: 1 2 3 4 5 6 7 8 9 10 11\n";
        struct pme_netdev nd;
        PME_TEST_CHECK(pme_parse_net_dev(t, sizeof(t) - 1u, &nd) == PME_EIO, "short");
    }
    /* malformed: name too long for the fixed field */
    {
        static const char t[] =
            "  abcdefghijklmnop0: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16\n";
        struct pme_netdev nd;
        PME_TEST_CHECK(pme_parse_net_dev(t, sizeof(t) - 1u, &nd) == PME_EIO, "longname");
    }
    /* no data lines at all */
    {
        static const char t[] =
            "Inter-|   Receive |  Transmit\n"
            " face |bytes|bytes\n";
        struct pme_netdev nd;
        PME_TEST_CHECK(pme_parse_net_dev(t, sizeof(t) - 1u, &nd) == PME_EIO, "headers");
    }
    /* empty */
    {
        struct pme_netdev nd;
        PME_TEST_CHECK(pme_parse_net_dev("", 0, &nd) == PME_EIO, "empty");
    }

    pmetest_for_each_fixture("proc/net_dev", check_fixture, NULL);
    printf("ok %s\n", __FILE__);
    return 0;
}
