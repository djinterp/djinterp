/*******************************************************************************
* djinterp [test]                                               file_tests_ops.c
*
*   XI. OPERATIONS (file_ops) -- removing, moving, and duplicating whole files.
* d_file_rename's overwrite flag is pinned in both directions: with it clear an
* existing destination is refused (EEXIST); with it set the destination is
* replaced. d_file_copy is descriptor-based (ENOSYS on the ISO C backend), and
* by default replaces an existing destination (COPY_OVERWRITE) and refuses a
* directory source (a directory is not a regular file). Copies are verified by
* reading the destination back and comparing bytes.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_ops.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <errno.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

/*
d_fs_file_has
  True iff the file at _path holds exactly the bytes of the NUL-terminated
_want (compared by length and content, independent of the code under test's
own readers where possible).

Parameter(s):
  _path: file to inspect.
  _want: expected contents.
Return:
  true on an exact match, false otherwise.
*/
static bool
d_fs_file_has
(
    const char* _path,
    const char* _want
)
{
    void*  data;
    size_t size;
    size_t want_len;
    bool   match;

    want_len = strlen(_want);
    size     = 0;
    data     = d_file_read_all(_path, &size);
    match    = (data != NULL) &&
               (size == want_len) &&
               (memcmp(data, _want, want_len) == 0);

    if (data)
    {
        free(data);
    }

    return match;
}


/******************************************************************************
 * XI.a  d_file_remove
 *****************************************************************************/

static void
d_fs_tests_ops_remove
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_file_remove");

    dtest_test(_suite, "removes a regular file");
    d_fs_test_path(path, sizeof(path), "remove_file.txt");
    d_fs_test_write(path, "x", 1);
    D_FS_CHECK(_suite, d_file_remove(path) == 0);
    D_FS_CHECK(_suite, !d_file_exists(path));

    dtest_test(_suite, "removes an empty directory");
    d_fs_test_path(path, sizeof(path), "remove_dir");
    d_dir_create(path, S_IRWXU);
    D_FS_CHECK(_suite, d_file_remove(path) == 0);
    D_FS_CHECK(_suite, !d_dir_exists(path));

    dtest_test(_suite, "a missing path -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_remove");
    D_FS_CHECK(_suite, d_file_remove(path) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_remove(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * XI.b  d_file_unlink
 *****************************************************************************/

static void
d_fs_tests_ops_unlink
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_file_unlink");

    dtest_test(_suite, "removes a regular file");
    d_fs_test_path(path, sizeof(path), "unlink_file.txt");
    d_fs_test_write(path, "x", 1);
    D_FS_CHECK(_suite, d_file_unlink(path) == 0);
    D_FS_CHECK(_suite, !d_file_exists(path));

    dtest_test(_suite, "a missing file -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_unlink.txt");
    D_FS_CHECK(_suite, d_file_unlink(path) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_unlink(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * XI.c  d_file_rename
 *****************************************************************************/

static void
d_fs_tests_ops_rename
(
    dtest_suite* _suite
)
{
    char src[D_FS_TEST_PATH_BUF];
    char dst[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_file_rename");

    d_fs_test_path(src, sizeof(src), "rename_src.txt");
    d_fs_test_path(dst, sizeof(dst), "rename_dst.txt");

    dtest_test(_suite, "moves a file to a free destination");
    d_fs_test_write(src, "payload", 7);
    d_file_remove(dst);
    D_FS_CHECK(_suite, d_file_rename(src, dst, 0) == 0);
    D_FS_CHECK(_suite, !d_file_exists(src));
    D_FS_CHECK(_suite, d_fs_file_has(dst, "payload"));

    dtest_test(_suite, "without overwrite an existing destination -> -1, EEXIST");
    d_fs_test_write(src, "again", 5);
    // dst still exists from the previous case
    errno = 0;
    D_FS_CHECK(_suite, d_file_rename(src, dst, 0) == -1);
    D_FS_CHECK(_suite, errno == EEXIST);
    // the source was left untouched by the refused rename
    D_FS_CHECK(_suite, d_file_exists(src));

    dtest_test(_suite, "with overwrite the destination is replaced -> 0");
    D_FS_CHECK(_suite, d_file_rename(src, dst, 1) == 0);
    D_FS_CHECK(_suite, !d_file_exists(src));
    D_FS_CHECK(_suite, d_fs_file_has(dst, "again"));

    d_file_remove(dst);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL source -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_rename(NULL, dst, 0) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL destination -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_rename(src, NULL, 0) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
}


/******************************************************************************
 * XI.d  d_file_copy
 *****************************************************************************/

static void
d_fs_tests_ops_copy_file
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char src[D_FS_TEST_PATH_BUF];
    char dst[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_file_copy");

    d_fs_test_path(src, sizeof(src), "copy_src.txt");
    d_fs_test_path(dst, sizeof(dst), "copy_dst.txt");

    dtest_test(_suite, "copies contents to a new destination");
    d_fs_test_write(src, "copy me exactly", 15);
    d_file_remove(dst);
    D_FS_CHECK(_suite, d_file_copy(src, dst) == 0);
    D_FS_CHECK(_suite, d_fs_file_has(dst, "copy me exactly"));
    // the source is left in place
    D_FS_CHECK(_suite, d_file_exists(src));

    dtest_test(_suite, "replaces an existing destination by default");
    d_fs_test_write(dst, "stale", 5);
    D_FS_CHECK(_suite, d_file_copy(src, dst) == 0);
    D_FS_CHECK(_suite, d_fs_file_has(dst, "copy me exactly"));

    d_file_remove(src);
    d_file_remove(dst);

    dtest_test(_suite, "a directory source -> -1, EINVAL (not a regular file)");
    errno = 0;
    D_FS_CHECK(_suite, d_file_copy(d_fs_test_root(), dst) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    dtest_test(_suite, "a missing source -> -1");
    d_fs_test_path(src, sizeof(src), "no_such_copy_src.txt");
    D_FS_CHECK(_suite, d_file_copy(src, dst) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL source -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_copy(NULL, dst) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL destination -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_copy(src, NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
#else
    dtest_block(_suite, "d_file_copy");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_copy("a", "b") == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_ops
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "XI. file_ops");

    d_fs_tests_ops_remove(_suite);
    d_fs_tests_ops_unlink(_suite);
    d_fs_tests_ops_rename(_suite);
    d_fs_tests_ops_copy_file(_suite);

    return;
}
