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
*                                                            revised: 2026.11.06
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
static
bool d_fs_file_has(
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

static
void d_fs_tests_ops_remove(
    struct dj_test_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_file_remove");

    d_test_suite_test(_suite, "removes a regular file");
    d_fs_test_path(path, sizeof(path), "remove_file.txt");
    d_fs_test_write(path, "x", 1);
    D_FS_CHECK(_suite, d_file_remove(path) == 0);
    D_FS_CHECK(_suite, !d_file_exists(path));

    d_test_suite_test(_suite, "removes an empty directory");
    d_fs_test_path(path, sizeof(path), "remove_dir");
    d_dir_create(path, S_IRWXU);
    D_FS_CHECK(_suite, d_file_remove(path) == 0);
    D_FS_CHECK(_suite, !d_dir_exists(path));

    d_test_suite_test(_suite, "a missing path -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_remove");
    D_FS_CHECK(_suite, d_file_remove(path) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_remove(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * XI.b  d_file_unlink
 *****************************************************************************/

static
void d_fs_tests_ops_unlink(
    struct dj_test_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_file_unlink");

    d_test_suite_test(_suite, "removes a regular file");
    d_fs_test_path(path, sizeof(path), "unlink_file.txt");
    d_fs_test_write(path, "x", 1);
    D_FS_CHECK(_suite, d_file_unlink(path) == 0);
    D_FS_CHECK(_suite, !d_file_exists(path));

    d_test_suite_test(_suite, "a missing file -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_unlink.txt");
    D_FS_CHECK(_suite, d_file_unlink(path) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_unlink(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * XI.c  d_file_rename
 *****************************************************************************/

static
void d_fs_tests_ops_rename(
    struct dj_test_suite* _suite
)
{
    char src[D_FS_TEST_PATH_BUF];
    char dst[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_file_rename");

    d_fs_test_path(src, sizeof(src), "rename_src.txt");
    d_fs_test_path(dst, sizeof(dst), "rename_dst.txt");

    d_test_suite_test(_suite, "moves a file to a free destination");
    d_fs_test_write(src, "payload", 7);
    d_file_remove(dst);
    D_FS_CHECK(_suite, d_file_rename(src, dst, 0) == 0);
    D_FS_CHECK(_suite, !d_file_exists(src));
    D_FS_CHECK(_suite, d_fs_file_has(dst, "payload"));

    d_test_suite_test(_suite,
                      "without overwrite an existing destination -> -1, "
                      "EEXIST");
    d_fs_test_write(src, "again", 5);
    // dst still exists from the previous case
    errno = 0;
    D_FS_CHECK(_suite, d_file_rename(src, dst, 0) == -1);
    D_FS_CHECK(_suite, errno == EEXIST);
    // the source was left untouched by the refused rename
    D_FS_CHECK(_suite, d_file_exists(src));

    d_test_suite_test(_suite,
                      "with overwrite the destination is replaced -> 0");
    D_FS_CHECK(_suite, d_file_rename(src, dst, 1) == 0);
    D_FS_CHECK(_suite, !d_file_exists(src));
    D_FS_CHECK(_suite, d_fs_file_has(dst, "again"));

    d_file_remove(dst);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL source -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_rename(NULL, dst, 0) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL destination -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_rename(src, NULL, 0) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
}


/******************************************************************************
 * XI.d  d_file_copy
 *****************************************************************************/

static
void d_fs_tests_ops_copy_file(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char src[D_FS_TEST_PATH_BUF];
    char dst[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_file_copy");

    d_fs_test_path(src, sizeof(src), "copy_src.txt");
    d_fs_test_path(dst, sizeof(dst), "copy_dst.txt");

    d_test_suite_test(_suite, "copies contents to a new destination");
    d_fs_test_write(src, "copy me exactly", 15);
    d_file_remove(dst);
    D_FS_CHECK(_suite, d_file_copy(src, dst) == 0);
    D_FS_CHECK(_suite, d_fs_file_has(dst, "copy me exactly"));
    // the source is left in place
    D_FS_CHECK(_suite, d_file_exists(src));

    d_test_suite_test(_suite, "replaces an existing destination by default");
    d_fs_test_write(dst, "stale", 5);
    D_FS_CHECK(_suite, d_file_copy(src, dst) == 0);
    D_FS_CHECK(_suite, d_fs_file_has(dst, "copy me exactly"));

    d_file_remove(src);
    d_file_remove(dst);

    d_test_suite_test(_suite,
                      "a directory source -> -1, EINVAL (not a regular file)");
    errno = 0;
    D_FS_CHECK(_suite, d_file_copy(d_fs_test_root(), dst) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    d_test_suite_test(_suite, "a missing source -> -1");
    d_fs_test_path(src, sizeof(src), "no_such_copy_src.txt");
    D_FS_CHECK(_suite, d_file_copy(src, dst) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL source -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_copy(NULL, dst) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL destination -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_copy(src, NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
#else
    d_test_suite_block(_suite, "d_file_copy");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_copy("a", "b") == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void d_fs_tests_ops(
    struct dj_test_suite* _suite
)
{
    d_test_suite_module(_suite, "XI. file_ops");

    d_fs_tests_ops_remove(_suite);
    d_fs_tests_ops_unlink(_suite);
    d_fs_tests_ops_rename(_suite);
    d_fs_tests_ops_copy_file(_suite);

    return;
}

/******************************************************************************
 * d_tests_file_ops_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * XI.  file_ops  --  REMOVE / RENAME / COPY
 *****************************************************************************/

/*
d_tests_file_remove_unlink
  Tests d_file_remove and d_file_unlink, and the one thing that distinguishes them.
*/
static
bool d_tests_file_remove_unlink(struct dj_test_suite* _suite)
{
    char                  path[D_TEST_FILE_PATH_BUF];
    char                  dir[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite, "d_file_remove / d_file_unlink");

    d_tests_file_path(path, sizeof(path), "remove_test.txt");
    d_tests_file_path(dir, sizeof(dir), "remove_dir");

    (void)d_file_write_all(path, "test", 4);
    D_TEST_CHECK_MSG(_suite,
                     "remove",
                     (d_file_remove(path)) == (0),
                     "d_file_remove removes a file");
    D_TEST_CHECK_MSG(_suite,
                     "removed",
                     !(d_file_exists(path)),
                     "...and it is gone");
    D_TEST_CHECK_MSG(_suite,
                     "remove_missing",
                     (d_file_remove("no/such/file")) == (-1),
                     "d_file_remove fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "remove_null",
                     (d_file_remove(NULL)) == (-1),
                     "d_file_remove rejects NULL");
    #endif

    // this is the difference: remove takes directories, unlink does not
    (void)d_dir_create(dir, S_IRWXU);
    D_TEST_CHECK_MSG(_suite,
                     "remove_dir",
                     (d_file_remove(dir)) == (0),
                     "d_file_remove accepts an empty directory");

    (void)d_file_write_all(path, "test", 4);
    D_TEST_CHECK_MSG(_suite,
                     "unlink",
                     (d_file_unlink(path)) == (0),
                     "d_file_unlink removes a file");
    D_TEST_CHECK_MSG(_suite,
                     "unlink_missing",
                     (d_file_unlink("no/such/file")) == (-1),
                     "d_file_unlink fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "unlink_null",
                     (d_file_unlink(NULL)) == (-1),
                     "d_file_unlink rejects NULL");
    #endif

#if D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    (void)d_dir_create(dir, S_IRWXU);
    D_TEST_CHECK_MSG(_suite,
                     "unlink_refuses_dir",
                     (d_file_unlink(dir)) == (-1),
                     "d_file_unlink refuses a directory");
    (void)d_dir_remove(dir);
#endif

    return true;
}


/*
d_tests_file_rename
  Tests d_file_rename.
*/
static
bool d_tests_file_rename(struct dj_test_suite* _suite)
{
    char                  src[D_TEST_FILE_PATH_BUF];
    char                  dst[D_TEST_FILE_PATH_BUF];
    char*                 back;
    size_t                size;

    d_test_suite_test(_suite,
                      D_FILE_OPS_RENAME_IS_ATOMIC
                          ? "d_file_rename (always atomic)"
                          : "d_file_rename (cross-device fallback is NOT atomic)");

    d_tests_file_path(src, sizeof(src), "rename_src.txt");
    d_tests_file_path(dst, sizeof(dst), "rename_dst.txt");

    (void)d_file_write_all(src, "source", 6);
    D_TEST_CHECK_MSG(_suite,
                     "rename",
                     (d_file_rename(src, dst, 0)) == (0),
                     "d_file_rename moves a file");
    D_TEST_CHECK_MSG(_suite,
                     "moved",
                     d_file_exists(dst) && !d_file_exists(src),
                     "...and the old name is gone");
    (void)d_file_remove(dst);

    (void)d_file_write_all(src, "new source", 10);
    (void)d_file_write_all(dst, "old dest", 8);
    D_TEST_CHECK_MSG(_suite,
                     "overwrite",
                     (d_file_rename(src, dst, 1)) == (0),
                     "overwrite=1 replaces the destination");

    back = (char*)d_file_read_all(dst, &size);
    D_TEST_CHECK_MSG(_suite,
                     "overwrite_content",
                     back && size == 10 && memcmp(back, "new source", 10) == 0,
                     "...with the source's contents");

    if (back)
    {
        free(back);
    }

    // overwrite is a PARAMETER, not a knob, because neither platform's
    // default is portable: POSIX rename always clobbers, Win32 never does
    (void)d_file_write_all(src, "source", 6);
    D_TEST_CHECK_MSG(_suite,
                     "no_overwrite",
                     (d_file_rename(src, dst, 0)) == (-1),
                     "overwrite=0 refuses an existing destination");
    D_TEST_CHECK_MSG(_suite,
                     "source_survives",
                     d_file_exists(src),
                     "...and leaves the source alone");

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_src",
                     (d_file_rename(NULL, dst, 0)) == (-1),
                     "d_file_rename rejects a NULL source");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_dst",
                     (d_file_rename(src, NULL, 0)) == (-1),
                     "d_file_rename rejects a NULL destination");
    #endif

    (void)d_file_remove(src);
    (void)d_file_remove(dst);

    return true;
}


/*
d_tests_file_copy
  Tests d_file_copy.
*/
static
bool d_tests_file_copy(struct dj_test_suite* _suite)
{
    struct dj_stat_t       st;
    char                  src[D_TEST_FILE_PATH_BUF];
    char                  dst[D_TEST_FILE_PATH_BUF];
    char*                 back;
    size_t                size;

    d_test_suite_test(_suite,
                      D_FILE_OPS_COPY_IS_NATIVE
                          ? "d_file_copy (native engine)"
                          : "d_file_copy (portable buffer)");

    d_tests_file_path(src, sizeof(src), D_TEST_FILE_TEST_FILENAME);
    d_tests_file_path(dst, sizeof(dst), "copy_dst.txt");

    D_TEST_CHECK_MSG(_suite,
                     "copy",
                     (d_file_copy(src, dst)) == (0),
                     "d_file_copy copies a file");

    back = (char*)d_file_read_all(dst, &size);
    D_TEST_CHECK_MSG(_suite,
                     "content",
                     back && size == strlen(D_TEST_FILE_TEST_CONTENT) && memcmp(back, D_TEST_FILE_TEST_CONTENT, size) == 0,
                     "the copy is byte-identical");

    if (back)
    {
        free(back);
    }

    (void)d_file_remove(dst);

#if ( D_CFG_IS_ON(D_CFG_FILE_OPS_COPY_PRESERVE_MODE) &&                       \
      !D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32) )
    // copying a 0600 private key into a 0644 file is a security bug, and it
    // is what the umask does by default if nobody puts the bits back
    (void)d_file_chmod(src, S_IRUSR | S_IWUSR);
    (void)d_file_copy(src, dst);
    (void)d_file_stat(dst, &st);
    D_TEST_CHECK_MSG(_suite,
                     "preserve_mode",
                     (st.st_mode & 0777) == (0600u),
                     "a copy of a 0600 file is not world-readable");
    (void)d_file_chmod(src, 0644);
    (void)d_file_remove(dst);
#else
    (void)st;
#endif

    D_TEST_CHECK_MSG(_suite,
                     "missing_source",
                     (d_file_copy("no/such/file", dst)) == (-1),
                     "d_file_copy fails for a missing source");

    // a failed copy must not leave a partial file claiming to be one
    D_TEST_CHECK_MSG(_suite,
                     "no_debris",
                     !(d_file_exists(dst)),
                     "a failed copy leaves no destination behind");

    D_TEST_CHECK_MSG(_suite,
                     "dir_source",
                     (d_file_copy(D_TEST_FILE_TEMP_DIR, dst)) == (-1),
                     "d_file_copy refuses a directory");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_src",
                     (d_file_copy(NULL, dst)) == (-1),
                     "d_file_copy rejects a NULL source");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_dst",
                     (d_file_copy(src, NULL)) == (-1),
                     "d_file_copy rejects a NULL destination");
    #endif

    return true;
}


/*
d_tests_file_ops_all
  Runs every file_ops test.
*/
bool d_tests_file_ops_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "XI. file_ops");

    d_tests_file_remove_unlink(_suite);
    d_tests_file_rename(_suite);
    d_tests_file_copy(_suite);

    return true;
}
