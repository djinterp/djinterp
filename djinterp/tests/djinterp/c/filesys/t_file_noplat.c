/*******************************************************************************
* djinterp [c]                                                   t_file_noplat.c
*
*   Runtime harness for c/filesys built with neither POSIX nor Windows --
* D_CFG_FILE_HAS_POSIX=0 on a POSIX host, or a target env counts as neither,
* as WASI is -- which puts every module on the ISO C backend. It checks that
* every call needing the platform fails with ENOSYS, the allocating ones
* without allocating; that the calls that are only text still work; that the
* predicates answer no rather than fail; and that streams still do what ISO C
* can. It is ISO C itself, and works in the current directory.
*   ci/check_filesys_platforms.sh runs it natively at 64 and 32 bits, and on
* WASI under a runtime; by hand, from the repo root, in a scratch directory:
*     cc -std=c99 -D_XOPEN_SOURCE=700 -DD_CFG_FILE_HAS_POSIX=0 -Wall -Wextra  \
*        -Werror src/djinterp/c/filesys/file_*.c                               \
*        tests/djinterp/c/filesys/t_file_noplat.c -o t_file_noplat
*
*
* path:      /tests/djinterp/c/filesys/t_file_noplat.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.19
*******************************************************************************/
// std
#include <errno.h>   // errno, ENOSYS
#include <stdio.h>   // printf, remove
#include <stdlib.h>  // free
#include <string.h>  // memcmp, strcmp
// djinterp
#include "../../../../inc/djinterp/c/filesys/dfile.h"  // unit under test
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


static int g_checks   = 0;
static int g_failures = 0;

// CHECK
//   macro: count a check, and report it when it fails.
#define CHECK(_cond, _what)                                                    \
    do                                                                         \
    {                                                                          \
        ++g_checks;                                                            \
                                                                               \
        if (!(_cond))                                                          \
        {                                                                      \
            ++g_failures;                                                      \
            printf("FAIL %s (line %d, errno %d)\n",                            \
                   (_what),                                                    \
                   __LINE__,                                                   \
                   errno);                                                     \
        }                                                                      \
    } while (0)

// CHECK_ENOSYS
//   macro: a call that must fail, with ENOSYS.
#define CHECK_ENOSYS(_failed, _what)                                           \
    do                                                                         \
    {                                                                          \
        errno = 0;                                                             \
        CHECK( (_failed) && (errno == ENOSYS),                                 \
               _what);                                                         \
    } while (0)


/*
t_noplat_directories
  Directories and resolution need the platform; joining names does not.
*/
static void
t_noplat_directories(
    void
)
{
    char buf[256] = {0};

    CHECK_ENOSYS(d_dir_create("nd", 0755) == -1,
                 "d_dir_create is ENOSYS");
    CHECK_ENOSYS(d_dir_create_parents("nd/sub", 0755) == -1,
                 "d_dir_create_parents is ENOSYS");
    CHECK_ENOSYS(d_dir_remove("nd") == -1,
                 "d_dir_remove is ENOSYS");
    CHECK_ENOSYS(d_dir_open(".") == NULL,
                 "d_dir_open is ENOSYS");
    CHECK_ENOSYS(d_dir_get_current(buf, sizeof(buf)) == NULL,
                 "d_dir_get_current is ENOSYS");
    CHECK_ENOSYS(d_dir_set_current(".") == -1,
                 "d_dir_set_current is ENOSYS");
    CHECK_ENOSYS(d_path_resolve(".", buf, sizeof(buf)) == NULL,
                 "d_path_resolve is ENOSYS");
    CHECK_ENOSYS(d_path_resolve(".", NULL, 0) == NULL,
                 "d_path_resolve allocating is ENOSYS");
    CHECK( (d_dir_entry_path(buf, sizeof(buf), "dir", "name") != NULL) &&
           (strcmp(buf, "dir/name") == 0),
           "d_dir_entry_path, which is only text, works");

    return;
}

/*
t_noplat_metadata
  Metadata needs the platform; the predicates answer no instead of failing.
*/
static void
t_noplat_metadata(
    void
)
{
    struct d_stat64_t st   = {0};
    uint32_t          mode = 0;

    CHECK_ENOSYS(d_file_stat64("t.txt", &st) == -1,
                 "d_file_stat64 is ENOSYS");
    CHECK_ENOSYS(d_file_stat_nofollow64("t.txt", &st) == -1,
                 "d_file_stat_nofollow64 is ENOSYS");
    CHECK_ENOSYS(d_file_mode("t.txt", &mode) == -1,
                 "d_file_mode is ENOSYS");
    CHECK_ENOSYS(d_file_access("t.txt", F_OK) == -1,
                 "d_file_access is ENOSYS");
    CHECK_ENOSYS(d_file_chmod("t.txt", 0644) == -1,
                 "d_file_chmod is ENOSYS");
    CHECK_ENOSYS(d_file_size64("t.txt") == -1,
                 "d_file_size64 is ENOSYS");
    CHECK( (d_file_exists("t.txt") == 0) &&
           (d_dir_exists(".") == 0)      &&
           (d_file_is_regular("t.txt") == 0),
           "the predicates answer no");

    return;
}

/*
t_noplat_streams
  What ISO C can do, a whole-file write and read, still works.
*/
static void
t_noplat_streams(
    void
)
{
    size_t length = 0;
    char*  data   = NULL;

    CHECK(d_file_write_all("t.txt", "hi", 2) == 0,
          "a stream write works");

    data = d_file_read_all("t.txt", &length);
    CHECK( (data != NULL) &&
           (length == 2)  &&
           (memcmp(data, "hi", 2) == 0),
           "  and so does a stream read");
    free(data);
    CHECK(remove("t.txt") == 0,
          "  and the file is removed");

    return;
}


int
main(
    void
)
{
    CHECK(D_FILE_BACKEND_IS_STDC,
          "the build is on the ISO C backend");
    t_noplat_directories();
    t_noplat_metadata();
    t_noplat_streams();

    printf("t_file_noplat: %d checks, %d failed\n",
           g_checks,
           g_failures);

    return (g_failures == 0) ? 0 : 1;
}
