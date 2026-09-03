#include "fixture.h"
#include "procfs.h"

#include <string.h>

static void check_fixture(const char *path, const char *buf, size_t len, void *ud)
{
    struct pme_filesystem fs;
    uint32_t i;
    int have_root = 0;

    (void)ud;
    memset(&fs, 0, sizeof(fs));
    PME_TEST_CHECK(pme_parse_self_mounts(buf, len, &fs) == PME_OK, "%s", path);
    PME_TEST_CHECK(fs.n >= 1u, "%s: n=%u", path, fs.n);
    for (i = 0; i < fs.n; i++) {
        PME_TEST_CHECK(fs.mounts[i].device[0] != '\0', "%s: device[%u]", path, i);
        PME_TEST_CHECK(fs.mounts[i].fstype[0] != '\0', "%s: fstype[%u]", path, i);
        if (strcmp(fs.mounts[i].mountpoint, "/") == 0) {
            have_root = 1;
            PME_TEST_CHECK((fs.mounts[i].flags & PME_MOUNT_SKIPPED) == 0u,
                           "%s: root skipped", path);
        }
    }
    PME_TEST_CHECK(have_root, "%s: no / mount", path);
}

int main(void)
{

    {
        static const char t[] =
            "/dev/sda1 / ext4 rw,relatime 0 0\n"
            "proc /proc proc rw,nosuid 0 0\n"
            "host:/export /mnt/nfs nfs rw 0 0\n"
            "//srv/share /mnt/c cifs ro 0 0\n";
        struct pme_filesystem fs;
        memset(&fs, 0, sizeof(fs));
        PME_TEST_CHECK(pme_parse_self_mounts(t, sizeof(t) - 1u, &fs) == PME_OK, "valid");
        PME_TEST_CHECK(fs.n == 4u, "n=%u", fs.n);
        PME_TEST_CHECK(strcmp(fs.mounts[0].device, "/dev/sda1") == 0, "dev0");
        PME_TEST_CHECK(strcmp(fs.mounts[0].mountpoint, "/") == 0, "mnt0");
        PME_TEST_CHECK(strcmp(fs.mounts[0].fstype, "ext4") == 0, "fst0");
        PME_TEST_CHECK(fs.mounts[0].flags == 0u, "flags0=%u", fs.mounts[0].flags);
        PME_TEST_CHECK((fs.mounts[2].flags & PME_MOUNT_SKIPPED) != 0u, "nfs skipped");
        PME_TEST_CHECK((fs.mounts[2].flags & PME_MOUNT_RO) == 0u, "nfs rw");
        PME_TEST_CHECK(fs.mounts[3].flags == (PME_MOUNT_RO | PME_MOUNT_SKIPPED),
                       "cifs ro+skipped flags=%u", fs.mounts[3].flags);
    }

    {
        static const char t[] =
            "server:/v /mnt/v nfs4 rw 0 0\n"
            "host /mnt/g 9p rw 0 0\n";
        struct pme_filesystem fs;
        memset(&fs, 0, sizeof(fs));
        PME_TEST_CHECK(pme_parse_self_mounts(t, sizeof(t) - 1u, &fs) == PME_OK, "ver");
        PME_TEST_CHECK(fs.n == 2u, "n=%u", fs.n);
        PME_TEST_CHECK((fs.mounts[0].flags & PME_MOUNT_SKIPPED) != 0u, "nfs4 skipped");
        PME_TEST_CHECK((fs.mounts[1].flags & PME_MOUNT_SKIPPED) != 0u, "9p skipped");
    }

    {
        static const char t[] = "/dev/sda1 /\n";
        struct pme_filesystem fs;
        PME_TEST_CHECK(pme_parse_self_mounts(t, sizeof(t) - 1u, &fs) == PME_EIO, "short");
    }

    {
        static const char t[] = "/dev/sda1 / ext4 rw";
        struct pme_filesystem fs;
        memset(&fs, 0, sizeof(fs));
        PME_TEST_CHECK(pme_parse_self_mounts(t, sizeof(t) - 1u, &fs) == PME_OK, "eof");
        PME_TEST_CHECK(fs.n == 1u && strcmp(fs.mounts[0].fstype, "ext4") == 0, "eof val");
    }

    {
        struct pme_filesystem fs;
        PME_TEST_CHECK(pme_parse_self_mounts("", 0, &fs) == PME_EIO, "empty");
    }

    PME_TEST_CHECK(pmetest_for_each_fixture("proc/self_mounts", check_fixture, NULL) >= 1,
                   "no fixtures exercised - capture.sh output missing?");
    printf("ok %s\n", __FILE__);
    return 0;
}
