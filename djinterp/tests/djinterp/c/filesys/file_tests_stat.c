/*******************************************************************************
* djinterp [test]                                              file_tests_stat.c
*
*   VIII. METADATA (file_stat) -- interrogating a path or descriptor. The whole
* d_stat_t is checked for the fields that matter: st_size against the fixture's
* exact length, st_mode through the portable S_ISREG / S_ISDIR macros, and the
* deliberately-renamed st_modified (NOT st_mtime, which glibc and macOS both
* macro-hijack) for a plausible timestamp. d_file_stat_fd and d_file_size_stream are
* descriptor-based and report ENOSYS on the ISO C backend. The boolean queries
* (exists / is_file / is_dir) are checked in all three states -- present file,
* present directory, and absent -- and for their NULL tolerance.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_stat.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.10.21
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * VIII.a  d_file_stat
 *****************************************************************************/

static void
d_fs_tests_stat_stat
(
    dtest_suite* _suite
)
{
    char            path[D_FS_TEST_PATH_BUF];
    struct d_stat_t st;
    size_t          expect;
    bool            described;

    dtest_block(_suite, "d_file_stat");

    expect = strlen(D_FS_TEST_CONTENT);

    dtest_test(_suite, "describes a regular file (size, type, timestamp)");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    memset(&st, 0, sizeof(st));
    described = (d_file_stat(path, &st) == 0);
    D_FS_CHECK(_suite, described);
    D_FS_CHECK(_suite, st.st_size == (uint64_t)expect);
    D_FS_CHECK(_suite, S_ISREG(st.st_mode));
    // the renamed modification-time field is populated for a just-made file
    D_FS_CHECK(_suite, st.st_modified > 0);

    dtest_test(_suite, "a directory is reported as a directory");
    memset(&st, 0, sizeof(st));
    D_FS_CHECK(_suite, d_file_stat(d_fs_test_root(), &st) == 0);
    D_FS_CHECK(_suite, S_ISDIR(st.st_mode));

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_stat(NULL, &st) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_stat(path, NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "a missing path -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_stat.txt");
    D_FS_CHECK(_suite, d_file_stat(path, &st) == -1);
}


/******************************************************************************
 * VIII.b  d_file_stat_nofollow
 *****************************************************************************/

static void
d_fs_tests_stat_lstat
(
    dtest_suite* _suite
)
{
    char            path[D_FS_TEST_PATH_BUF];
    struct d_stat_t st;
    size_t          expect;

    dtest_block(_suite, "d_file_stat_nofollow");

    expect = strlen(D_FS_TEST_CONTENT);

    dtest_test(_suite, "describes a regular file that is not a link");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    memset(&st, 0, sizeof(st));
    D_FS_CHECK(_suite, d_file_stat_nofollow(path, &st) == 0);
    D_FS_CHECK(_suite, st.st_size == (uint64_t)expect);
    D_FS_CHECK(_suite, S_ISREG(st.st_mode));

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_stat_nofollow(NULL, &st) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "a missing path -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_lstat.txt");
    D_FS_CHECK(_suite, d_file_stat_nofollow(path, &st) == -1);
}


/******************************************************************************
 * VIII.c  d_file_stat_fd
 *****************************************************************************/

static void
d_fs_tests_stat_fstat
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char            path[D_FS_TEST_PATH_BUF];
    struct d_stat_t st;
    int             fd;
    size_t          expect;
    bool            described;

    dtest_block(_suite, "d_file_stat_fd");

    expect = strlen(D_FS_TEST_CONTENT);

    dtest_test(_suite, "describes the file behind a descriptor");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    fd = d_file_open(path, O_RDONLY);
    memset(&st, 0, sizeof(st));
    described = (fd >= 0) && (d_file_stat_fd(fd, &st) == 0);
    D_FS_CHECK(_suite, described);
    D_FS_CHECK(_suite, st.st_size == (uint64_t)expect);
    D_FS_CHECK(_suite, S_ISREG(st.st_mode));

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    dtest_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_stat_fd(-1, &st) == -1);
    D_FS_CHECK(_suite, errno == EBADF);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> -1, EINVAL");
    fd    = d_file_open(path, O_RDONLY);
    errno = 0;
    D_FS_CHECK(_suite, (fd >= 0) && (d_file_stat_fd(fd, NULL) == -1));
    D_FS_CHECK(_suite, errno == EINVAL);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }
#endif
#else
    dtest_block(_suite, "d_file_stat_fd");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    {
        struct d_stat_t st;
        D_FS_CHECK(_suite, d_file_stat_fd(0, &st) == -1);
    }
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VIII.d  d_file_access
 *****************************************************************************/

static void
d_fs_tests_stat_access
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_file_access");

    dtest_test(_suite, "an existing file passes F_OK and R_OK");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    D_FS_CHECK(_suite, d_file_access(path, F_OK) == 0);
    D_FS_CHECK(_suite, d_file_access(path, R_OK) == 0);

    dtest_test(_suite, "a missing file fails F_OK");
    d_fs_test_path(path, sizeof(path), "no_such_access.txt");
    D_FS_CHECK(_suite, d_file_access(path, F_OK) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_access(NULL, F_OK) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * VIII.e  d_file_chmod
 *****************************************************************************/

static void
d_fs_tests_stat_chmod
(
    dtest_suite* _suite
)
{
    char            path[D_FS_TEST_PATH_BUF];
    struct d_stat_t st;
    bool            applied;

    dtest_block(_suite, "d_file_chmod");

    dtest_test(_suite, "changes the permission bits of a file");
    d_fs_test_path(path, sizeof(path), "chmod_target.txt");
    d_fs_test_write(path, "x", 1);
    applied = (d_file_chmod(path, 0600) == 0);

    memset(&st, 0, sizeof(st));
    if (d_file_stat(path, &st) == 0)
    {
#if defined(_WIN32)
        // Windows keeps one permission, read-only: 0600 leaves it writable
        applied = applied && ((st.st_mode & 0200) != 0);
#else
        applied = applied && ((st.st_mode & 0777) == 0600);
#endif
    }
    else
    {
        applied = false;
    }

    D_FS_CHECK(_suite, applied);

    // make it deletable again regardless of what we set
    d_file_chmod(path, 0644);
    d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_chmod(NULL, 0644) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "a missing path -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_chmod.txt");
    D_FS_CHECK(_suite, d_file_chmod(path, 0644) == -1);
}


/******************************************************************************
 * VIII.f  d_file_size
 *****************************************************************************/

static void
d_fs_tests_stat_file_size
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_file_size");

    dtest_test(_suite, "reports the exact byte count of the fixture");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    D_FS_CHECK(_suite,
               d_file_size(path) == (int64_t)strlen(D_FS_TEST_CONTENT));

    dtest_test(_suite, "a missing file -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_size.txt");
    D_FS_CHECK(_suite, d_file_size(path) == -1);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_size(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
}


/******************************************************************************
 * VIII.g  d_file_size_stream
 *****************************************************************************/

static void
d_fs_tests_stat_file_size_stream
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  correct;

    dtest_block(_suite, "d_file_size_stream");

    dtest_test(_suite, "reports the size of the file behind a stream");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    file    = d_file_open_stream(path, "rb");
    correct = false;

    if (file)
    {
        correct = (d_file_size_stream(file)
                       == (int64_t)strlen(D_FS_TEST_CONTENT));
        d_file_close_stream(file);
    }

    D_FS_CHECK(_suite, correct);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_size_stream(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
#else
    dtest_block(_suite, "d_file_size_stream");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_size_stream(stdin) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VIII.h  d_file_exists / d_file_is_regular / d_dir_exists
 *****************************************************************************/

static void
d_fs_tests_stat_predicates
(
    dtest_suite* _suite
)
{
    char file_path[D_FS_TEST_PATH_BUF];
    char missing[D_FS_TEST_PATH_BUF];

    d_fs_test_path(file_path, sizeof(file_path), D_FS_TEST_FILE);
    d_fs_test_path(missing, sizeof(missing), "no_such_predicate.txt");

    dtest_block(_suite, "d_file_exists");

    dtest_test(_suite, "true for a present file");
    D_FS_CHECK(_suite, d_file_exists(file_path));

    dtest_test(_suite, "false for an absent path");
    D_FS_CHECK(_suite, !d_file_exists(missing));

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "false (not a crash) for NULL");
    D_FS_CHECK(_suite, !d_file_exists(NULL));
    #endif

    dtest_block(_suite, "d_file_is_regular");

    dtest_test(_suite, "true for a regular file");
    D_FS_CHECK(_suite, d_file_is_regular(file_path));

    dtest_test(_suite, "false for a directory");
    D_FS_CHECK(_suite, !d_file_is_regular(d_fs_test_root()));

    dtest_test(_suite, "false for an absent path");
    D_FS_CHECK(_suite, !d_file_is_regular(missing));

    dtest_block(_suite, "d_dir_exists");

    dtest_test(_suite, "true for a directory");
    D_FS_CHECK(_suite, d_dir_exists(d_fs_test_root()));

    dtest_test(_suite, "false for a regular file");
    D_FS_CHECK(_suite, !d_dir_exists(file_path));

    dtest_test(_suite, "false for an absent path");
    D_FS_CHECK(_suite, !d_dir_exists(missing));
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_stat
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "VIII. file_stat");

    d_fs_tests_stat_stat(_suite);
    d_fs_tests_stat_lstat(_suite);
    d_fs_tests_stat_fstat(_suite);
    d_fs_tests_stat_access(_suite);
    d_fs_tests_stat_chmod(_suite);
    d_fs_tests_stat_file_size(_suite);
    d_fs_tests_stat_file_size_stream(_suite);
    d_fs_tests_stat_predicates(_suite);

    return;
}
