/* Shared test helpers: fixture loading + failure macro.
 * Tests may use malloc/stdio freely (collect path may not). */
#ifndef PME_TEST_FIXTURE_H
#define PME_TEST_FIXTURE_H

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef PME_FIXTURE_DIR
#define PME_FIXTURE_DIR "tests/fixtures/linux"
#endif

#define PME_TEST_CHECK(cond, ...)                                            \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s -- ", __FILE__, __LINE__, #cond);\
            fprintf(stderr, __VA_ARGS__);                                    \
            fputc('\n', stderr);                                             \
            exit(1);                                                         \
        }                                                                    \
    } while (0)

static int pmetest_load_file(const char *path, char **out, size_t *len)
{
    FILE *f = fopen(path, "rb");
    long sz;
    char *buf;

    if (f == NULL) {
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (sz = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }
    buf = malloc((size_t)sz + 1u);
    if (buf == NULL) {
        fclose(f);
        return -1;
    }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf);
        fclose(f);
        return -1;
    }
    buf[sz] = '\0';
    fclose(f);
    *out = buf;
    *len = (size_t)sz;
    return 0;
}

/* Invoke cb(path, buf, len, ud) for every committed fixture file
 * <relpath> (e.g. "proc/stat") found under
 * PME_FIXTURE_DIR/<arch>/<kernel>/. Returns how many matched. */
static int pmetest_for_each_fixture(
    const char *relpath,
    void (*cb)(const char *path, const char *buf, size_t len, void *ud),
    void *ud)
{
    DIR *d = opendir(PME_FIXTURE_DIR);
    struct dirent *e;
    int found = 0;

    if (d == NULL) {
        return 0;
    }
    while ((e = readdir(d)) != NULL) {
        char arch_dir[1024];
        DIR *sd;
        struct dirent *e2;

        if (e->d_name[0] == '.') {
            continue;
        }
        if (snprintf(arch_dir, sizeof(arch_dir), "%s/%s", PME_FIXTURE_DIR,
                     e->d_name) >= (int)sizeof(arch_dir)) {
            continue;
        }
        sd = opendir(arch_dir);
        if (sd == NULL) {
            continue;
        }
        while ((e2 = readdir(sd)) != NULL) {
            char path[4096];
            struct stat st;
            char *buf = NULL;
            size_t len = 0;

            if (e2->d_name[0] == '.') {
                continue;
            }
            if (snprintf(path, sizeof(path), "%s/%s/%s", arch_dir,
                         e2->d_name, relpath) >= (int)sizeof(path)) {
                continue;
            }
            if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
                continue;
            }
            if (pmetest_load_file(path, &buf, &len) == 0) {
                cb(path, buf, len, ud);
                free(buf);
                found++;
            }
        }
        closedir(sd);
    }
    closedir(d);
    return found;
}

#endif /* PME_TEST_FIXTURE_H */
