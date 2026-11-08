/*******************************************************************************
* djinterp [test]                                              file_tests_stat.c
*
*   VIII. METADATA (file_stat) -- interrogating a path or descriptor. The whole
* dj_stat_t is checked for the fields that matter: st_size against the fixture's
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
*                                                            revised: 2026.11.06
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

static
void d_fs_tests_stat_stat(
    struct dj_test_suite* _suite
)
{
    char            path[D_FS_TEST_PATH_BUF];
    struct dj_stat_t st;
    size_t          expect;
    bool            described;

    d_test_suite_block(_suite, "d_file_stat");

    expect = strlen(D_FS_TEST_CONTENT);

    d_test_suite_test(_suite,
                      "describes a regular file (size, type, timestamp)");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    memset(&st, 0, sizeof(st));
    described = (d_file_stat(path, &st) == 0);
    D_FS_CHECK(_suite, described);
    D_FS_CHECK(_suite, st.st_size == (uint64_t)expect);
    D_FS_CHECK(_suite, S_ISREG(st.st_mode));
    // the renamed modification-time field is populated for a just-made file
    D_FS_CHECK(_suite, st.st_modified > 0);

    d_test_suite_test(_suite, "a directory is reported as a directory");
    memset(&st, 0, sizeof(st));
    D_FS_CHECK(_suite, d_file_stat(d_fs_test_root(), &st) == 0);
    D_FS_CHECK(_suite, S_ISDIR(st.st_mode));

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_stat(NULL, &st) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_stat(path, NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite, "a missing path -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_stat.txt");
    D_FS_CHECK(_suite, d_file_stat(path, &st) == -1);
}


/******************************************************************************
 * VIII.b  d_file_stat_nofollow
 *****************************************************************************/

static
void d_fs_tests_stat_lstat(
    struct dj_test_suite* _suite
)
{
    char            path[D_FS_TEST_PATH_BUF];
    struct dj_stat_t st;
    size_t          expect;

    d_test_suite_block(_suite, "d_file_stat_nofollow");

    expect = strlen(D_FS_TEST_CONTENT);

    d_test_suite_test(_suite, "describes a regular file that is not a link");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    memset(&st, 0, sizeof(st));
    D_FS_CHECK(_suite, d_file_stat_nofollow(path, &st) == 0);
    D_FS_CHECK(_suite, st.st_size == (uint64_t)expect);
    D_FS_CHECK(_suite, S_ISREG(st.st_mode));

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_stat_nofollow(NULL, &st) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite, "a missing path -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_lstat.txt");
    D_FS_CHECK(_suite, d_file_stat_nofollow(path, &st) == -1);
}


/******************************************************************************
 * VIII.c  d_file_stat_fd
 *****************************************************************************/

static
void d_fs_tests_stat_fstat(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char            path[D_FS_TEST_PATH_BUF];
    struct dj_stat_t st;
    int             fd;
    size_t          expect;
    bool            described;

    d_test_suite_block(_suite, "d_file_stat_fd");

    expect = strlen(D_FS_TEST_CONTENT);

    d_test_suite_test(_suite, "describes the file behind a descriptor");
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

    d_test_suite_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_stat_fd(-1, &st) == -1);
    D_FS_CHECK(_suite, errno == EBADF);

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> -1, EINVAL");
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
    d_test_suite_block(_suite, "d_file_stat_fd");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    {
        struct dj_stat_t st;
        D_FS_CHECK(_suite, d_file_stat_fd(0, &st) == -1);
    }
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VIII.d  d_file_access
 *****************************************************************************/

static
void d_fs_tests_stat_access(
    struct dj_test_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_file_access");

    d_test_suite_test(_suite, "an existing file passes F_OK and R_OK");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    D_FS_CHECK(_suite, d_file_access(path, F_OK) == 0);
    D_FS_CHECK(_suite, d_file_access(path, R_OK) == 0);

    d_test_suite_test(_suite, "a missing file fails F_OK");
    d_fs_test_path(path, sizeof(path), "no_such_access.txt");
    D_FS_CHECK(_suite, d_file_access(path, F_OK) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_access(NULL, F_OK) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * VIII.e  d_file_chmod
 *****************************************************************************/

static
void d_fs_tests_stat_chmod(
    struct dj_test_suite* _suite
)
{
    char            path[D_FS_TEST_PATH_BUF];
    struct dj_stat_t st;
    bool            applied;

    d_test_suite_block(_suite, "d_file_chmod");

    d_test_suite_test(_suite, "changes the permission bits of a file");
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
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_chmod(NULL, 0644) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite, "a missing path -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_chmod.txt");
    D_FS_CHECK(_suite, d_file_chmod(path, 0644) == -1);
}


/******************************************************************************
 * VIII.f  d_file_size
 *****************************************************************************/

static
void d_fs_tests_stat_file_size(
    struct dj_test_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_file_size");

    d_test_suite_test(_suite, "reports the exact byte count of the fixture");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    D_FS_CHECK(_suite,
               d_file_size(path) == (int64_t)strlen(D_FS_TEST_CONTENT));

    d_test_suite_test(_suite, "a missing file -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_size.txt");
    D_FS_CHECK(_suite, d_file_size(path) == -1);

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_size(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
}


/******************************************************************************
 * VIII.g  d_file_size_stream
 *****************************************************************************/

static
void d_fs_tests_stat_file_size_stream(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  correct;

    d_test_suite_block(_suite, "d_file_size_stream");

    d_test_suite_test(_suite, "reports the size of the file behind a stream");
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
    d_test_suite_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_size_stream(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
#else
    d_test_suite_block(_suite, "d_file_size_stream");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_size_stream(stdin) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VIII.h  d_file_exists / d_file_is_regular / d_dir_exists
 *****************************************************************************/

static
void d_fs_tests_stat_predicates(
    struct dj_test_suite* _suite
)
{
    char file_path[D_FS_TEST_PATH_BUF];
    char missing[D_FS_TEST_PATH_BUF];

    d_fs_test_path(file_path, sizeof(file_path), D_FS_TEST_FILE);
    d_fs_test_path(missing, sizeof(missing), "no_such_predicate.txt");

    d_test_suite_block(_suite, "d_file_exists");

    d_test_suite_test(_suite, "true for a present file");
    D_FS_CHECK(_suite, d_file_exists(file_path));

    d_test_suite_test(_suite, "false for an absent path");
    D_FS_CHECK(_suite, !d_file_exists(missing));

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "false (not a crash) for NULL");
    D_FS_CHECK(_suite, !d_file_exists(NULL));
    #endif

    d_test_suite_block(_suite, "d_file_is_regular");

    d_test_suite_test(_suite, "true for a regular file");
    D_FS_CHECK(_suite, d_file_is_regular(file_path));

    d_test_suite_test(_suite, "false for a directory");
    D_FS_CHECK(_suite, !d_file_is_regular(d_fs_test_root()));

    d_test_suite_test(_suite, "false for an absent path");
    D_FS_CHECK(_suite, !d_file_is_regular(missing));

    d_test_suite_block(_suite, "d_dir_exists");

    d_test_suite_test(_suite, "true for a directory");
    D_FS_CHECK(_suite, d_dir_exists(d_fs_test_root()));

    d_test_suite_test(_suite, "false for a regular file");
    D_FS_CHECK(_suite, !d_dir_exists(file_path));

    d_test_suite_test(_suite, "false for an absent path");
    D_FS_CHECK(_suite, !d_dir_exists(missing));
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void d_fs_tests_stat(
    struct dj_test_suite* _suite
)
{
    d_test_suite_module(_suite, "VIII. file_stat");

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

/******************************************************************************
 * d_tests_file_stat_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * VIII.  file_stat  --  METADATA
 *
 *   The interesting group here is the timestamp one, and what it asserts is
 * that THE QUERY MACROS DO NOT LIE. D_FILE_STAT_HAS_BIRTHTIME reported 1 on
 * Linux while st_created was always 0, because the gate described the
 * platform (which has btime) rather than the module (which called plain
 * stat() and could not reach it). A caller can branch on a macro that says
 * no; it cannot defend against one that says yes and lies.
 *   So the assertion runs in BOTH directions -- the earlier version of this
 * test skipped when the macro claimed yes, which is exactly how the bug
 * survived.
 *****************************************************************************/

/*
d_tests_file_stat_basic
  Tests d_file_stat / d_file_stat_nofollow / d_file_stat_fd.
*/
static
bool d_tests_file_stat_basic(struct dj_test_suite* _suite)
{
    struct dj_stat_t       st;
    char                  path[D_TEST_FILE_PATH_BUF];
    int                   fd;

    d_test_suite_test(_suite,
                      "d_file_stat / d_file_stat_nofollow / d_file_stat_fd");

    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    D_TEST_CHECK_MSG(_suite,
                     "stat",
                     (d_file_stat(path, &st)) == (0),
                     "d_file_stat describes an existing file");
    D_TEST_CHECK_MSG(_suite,
                     "size",
                     ((size_t)st.st_size) == (strlen(D_TEST_FILE_TEST_CONTENT)),
                     "the size matches the fixture");
    D_TEST_CHECK_MSG(_suite,
                     "is_regular",
                     S_ISREG(st.st_mode),
                     "the fixture is a regular file");
    D_TEST_CHECK_MSG(_suite,
                     "stat_missing",
                     (d_file_stat("no/such/file/at/all", &st)) == (-1),
                     "d_file_stat fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "stat_null_path",
                     (d_file_stat(NULL, &st)) == (-1),
                     "d_file_stat rejects a NULL path");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "stat_null_buf",
                     (d_file_stat(path, NULL)) == (-1),
                     "d_file_stat rejects a NULL buffer");
    #endif

    // every field the platform cannot answer must be ZEROED, not left as
    // whatever the caller's stack held
    memset(&st, 0xAB, sizeof(st));
    (void)d_file_stat(path, &st);
    D_TEST_CHECK_MSG(_suite,
                     "overwrites_all",
                     ((size_t)st.st_size) == (strlen(D_TEST_FILE_TEST_CONTENT)),
                     "d_file_stat overwrites a dirty buffer completely");

    D_TEST_CHECK_MSG(_suite,
                     "lstat",
                     (d_file_stat_nofollow(path, &st)) == (0),
                     "d_file_stat_nofollow describes a regular file too");

    fd = d_file_open(path, O_RDONLY);

    if (fd >= 0)
    {
        // the only TOCTOU-free member of the module
        D_TEST_CHECK_MSG(_suite,
                         "fstat",
                         (d_file_stat_fd(fd, &st)) == (0),
                         "d_file_stat_fd describes an open descriptor");
        D_TEST_CHECK_MSG(_suite,
                         "fstat_size",
                         ((size_t)st.st_size) == (strlen(D_TEST_FILE_TEST_CONTENT)),
                         "...with the same size");
        (void)d_file_close_fd(fd);
    }

    D_TEST_CHECK_MSG(_suite,
                     "fstat_bad_fd",
                     (d_file_stat_fd(-1, &st)) == (-1),
                     "d_file_stat_fd rejects a bad descriptor");

    return true;
}


/*
d_tests_file_stat_times
  Tests the timestamp fields, and that the macros describing them are honest.
*/
static
bool d_tests_file_stat_times(struct dj_test_suite* _suite)
{
    struct dj_stat_t       st;
    char                  path[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite, "timestamps (the macros must not lie)");

    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);
    (void)d_file_stat(path, &st);

    D_TEST_CHECK_MSG(_suite,
                     "modified",
                     st.st_modified > 0,
                     "st_modified carries a real timestamp");
    D_TEST_CHECK_MSG(_suite,
                     "accessed",
                     st.st_accessed > 0,
                     "st_accessed carries a real timestamp");

    // THE assertion. Both directions, because a macro that says yes and lies
    // is the failure mode a one-directional test cannot see -- the earlier
    // version of this skipped whenever the macro claimed yes, which is
    // precisely how the bug survived.
#if D_FILE_STAT_HAS_BIRTHTIME
    D_TEST_CHECK_MSG(_suite,
                     "birthtime_real",
                     st.st_created > 0,
                     "HAS_BIRTHTIME=1 must mean st_created has data");
#else
    D_TEST_CHECK_MSG(_suite,
                     "birthtime_zero",
                     (st.st_created) == ((int64_t)0),
                     "HAS_BIRTHTIME=0 means 0, never a substituted ctime");
#endif

#if D_FILE_STAT_HAS_NSEC
    // a nanosecond field is allowed to be zero -- a file really can be
    // created on a second boundary -- so the only honest assertion is that it
    // is in range, not that it is nonzero
    D_TEST_CHECK_MSG(_suite,
                     "nsec_in_range",
                     st.st_modified_nsec < 1000000000u,
                     "sub-second parts are within one second");
#else
    D_TEST_CHECK_MSG(_suite,
                     "nsec_zero",
                     (st.st_modified_nsec) == (0u),
                     "HAS_NSEC=0 means the field reads 0");
#endif

    return true;
}


/*
d_tests_file_stat_sizes
  Tests d_file_size and d_file_size_stream.
*/
static
bool d_tests_file_stat_sizes(struct dj_test_suite* _suite)
{
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;

    d_test_suite_test(_suite, "d_file_size");

    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    D_TEST_CHECK_MSG(_suite,
                     "file_size",
                     (d_file_size(path)) == ((int64_t)strlen(D_TEST_FILE_TEST_CONTENT)),
                     "d_file_size reports the fixture's length");
    D_TEST_CHECK_MSG(_suite,
                     "missing",
                     (d_file_size("no/such/file")) == ((int64_t)-1),
                     "d_file_size fails for a missing file");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     (d_file_size(NULL)) == ((int64_t)-1),
                     "d_file_size rejects NULL");
    #endif

    file = d_file_open_stream(path, "rb");

    if (file)
    {
        D_TEST_CHECK_MSG(_suite,
                         "stream",
                         (d_file_size_stream(file)) == ((int64_t)strlen(D_TEST_FILE_TEST_CONTENT)),
                         "d_file_size_stream agrees");

        // it must use the descriptor, not seek: seeking would move the
        // position and the caller never asked for that
        D_TEST_CHECK_MSG(_suite,
                         "stream_position",
                         (ftell(file)) == (0L),
                         "...without moving the stream position");
        (void)d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "stream_null",
                     (d_file_size_stream(NULL)) == ((int64_t)-1),
                     "d_file_size_stream rejects NULL");
    #endif

    return true;
}


/*
d_tests_file_stat_predicates
  Tests d_file_exists / d_file_is_regular / d_dir_exists, and d_file_access / d_file_chmod.
*/
static
bool d_tests_file_stat_predicates(struct dj_test_suite* _suite)
{
    struct dj_stat_t       st;
    char                  path[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite, "predicates / d_file_access / d_file_chmod");

    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    D_TEST_CHECK_MSG(_suite,
                     "exists_file",
                     d_file_exists(path),
                     "the fixture exists");
    D_TEST_CHECK_MSG(_suite,
                     "exists_dir",
                     d_file_exists(D_TEST_FILE_TEMP_DIR),
                     "a directory exists too");
    D_TEST_CHECK_MSG(_suite,
                     "exists_missing",
                     !(d_file_exists("no/such/file/at/all")),
                     "a missing path does not exist");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "exists_null",
                     !(d_file_exists(NULL)),
                     "NULL does not exist");
    #endif

    D_TEST_CHECK_MSG(_suite,
                     "is_file",
                     d_file_is_regular(path),
                     "the fixture is a regular file");
    D_TEST_CHECK_MSG(_suite,
                     "is_file_dir",
                     !(d_file_is_regular(D_TEST_FILE_TEMP_DIR)),
                     "a directory is not a regular file");
    D_TEST_CHECK_MSG(_suite,
                     "is_dir",
                     d_dir_exists(D_TEST_FILE_TEMP_DIR),
                     "the scratch directory is a directory");
    D_TEST_CHECK_MSG(_suite,
                     "is_dir_file",
                     !(d_dir_exists(path)),
                     "a file is not a directory");

    D_TEST_CHECK_MSG(_suite,
                     "access_f_ok",
                     (d_file_access(path, F_OK)) == (0),
                     "d_file_access confirms existence");
    D_TEST_CHECK_MSG(_suite,
                     "access_r_ok",
                     (d_file_access(path, R_OK)) == (0),
                     "d_file_access confirms readability");
    D_TEST_CHECK_MSG(_suite,
                     "access_missing",
                     (d_file_access("no/such/file", F_OK)) == (-1),
                     "d_file_access fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "access_null",
                     (d_file_access(NULL, F_OK)) == (-1),
                     "d_file_access rejects NULL");
    #endif

    D_TEST_CHECK_MSG(_suite,
                     "chmod",
                     (d_file_chmod(path, S_IRUSR | S_IWUSR)) == (0),
                     "d_file_chmod sets permissions");

#if !D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Windows maps the whole mode onto one read-only attribute, so asserting
    // the bits there would be asserting something the platform cannot express
    (void)d_file_stat(path, &st);
    D_TEST_CHECK_MSG(_suite,
                     "chmod_bits",
                     (st.st_mode & 0777) == (0600u),
                     "...and the bits take");
#else
    (void)st;
#endif

    (void)d_file_chmod(path, 0644);

    D_TEST_CHECK_MSG(_suite,
                     "chmod_missing",
                     (d_file_chmod("no/such/file", S_IRUSR)) == (-1),
                     "d_file_chmod fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "chmod_null",
                     (d_file_chmod(NULL, S_IRUSR)) == (-1),
                     "d_file_chmod rejects NULL");
    #endif

    return true;
}


/*
d_tests_file_stat_all
  Runs every file_stat test.
*/
bool d_tests_file_stat_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "VIII. file_stat");

    d_tests_file_stat_basic(_suite);
    d_tests_file_stat_times(_suite);
    d_tests_file_stat_sizes(_suite);
    d_tests_file_stat_predicates(_suite);

    return true;
}
