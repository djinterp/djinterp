/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_stat.c
*
*
* path:      /tests/djinterp/c/fs/file_tests_sa_stat.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

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
static struct d_test_object*
d_tests_file_stat_basic
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    struct d_stat_t       st;
    char                  path[D_TEST_FILE_PATH_BUF];
    int                   fd;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    items[idx++] = D_ASSERT_EQUAL("stat", d_file_stat(path, &st), 0,
                                  "d_file_stat describes an existing file");
    items[idx++] = D_ASSERT_EQUAL("size", (size_t)st.st_size,
                                  strlen(D_TEST_FILE_TEST_CONTENT),
                                  "the size matches the fixture");
    items[idx++] = D_ASSERT_TRUE("is_regular", S_ISREG(st.st_mode),
                                 "the fixture is a regular file");
    items[idx++] = D_ASSERT_EQUAL("stat_missing",
                                  d_file_stat("no/such/file/at/all", &st), -1,
                                  "d_file_stat fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("stat_null_path", d_file_stat(NULL, &st), -1,
                                  "d_file_stat rejects a NULL path");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("stat_null_buf", d_file_stat(path, NULL), -1,
                                  "d_file_stat rejects a NULL buffer");
    #endif

    // every field the platform cannot answer must be ZEROED, not left as
    // whatever the caller's stack held
    memset(&st, 0xAB, sizeof(st));
    (void)d_file_stat(path, &st);
    items[idx++] = D_ASSERT_EQUAL("overwrites_all", (size_t)st.st_size,
                                  strlen(D_TEST_FILE_TEST_CONTENT),
                                  "d_file_stat overwrites a dirty buffer completely");

    items[idx++] = D_ASSERT_EQUAL("lstat", d_file_stat_nofollow(path, &st), 0,
                                  "d_file_stat_nofollow describes a regular file too");

    fd = d_file_open(path, O_RDONLY);

    if (fd >= 0)
    {
        // the only TOCTOU-free member of the module
        items[idx++] = D_ASSERT_EQUAL("fstat", d_file_stat_fd(fd, &st), 0,
                                      "d_file_stat_fd describes an open descriptor");
        items[idx++] = D_ASSERT_EQUAL("fstat_size", (size_t)st.st_size,
                                      strlen(D_TEST_FILE_TEST_CONTENT),
                                      "...with the same size");
        (void)d_file_close_fd(fd);
    }

    items[idx++] = D_ASSERT_EQUAL("fstat_bad_fd", d_file_stat_fd(-1, &st), -1,
                                  "d_file_stat_fd rejects a bad descriptor");

    return d_tests_file_group("d_file_stat / d_file_stat_nofollow / d_file_stat_fd", items, idx);
}


/*
d_tests_file_stat_times
  Tests the timestamp fields, and that the macros describing them are honest.
*/
static struct d_test_object*
d_tests_file_stat_times
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    struct d_stat_t       st;
    char                  path[D_TEST_FILE_PATH_BUF];
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);
    (void)d_file_stat(path, &st);

    items[idx++] = D_ASSERT_TRUE("modified", st.st_modified > 0,
                                 "st_modified carries a real timestamp");
    items[idx++] = D_ASSERT_TRUE("accessed", st.st_accessed > 0,
                                 "st_accessed carries a real timestamp");

    // THE assertion. Both directions, because a macro that says yes and lies
    // is the failure mode a one-directional test cannot see -- the earlier
    // version of this skipped whenever the macro claimed yes, which is
    // precisely how the bug survived.
#if D_FILE_STAT_HAS_BIRTHTIME
    items[idx++] = D_ASSERT_TRUE("birthtime_real", st.st_created > 0,
                                 "HAS_BIRTHTIME=1 must mean st_created has data");
#else
    items[idx++] = D_ASSERT_EQUAL("birthtime_zero", st.st_created, (int64_t)0,
                                  "HAS_BIRTHTIME=0 means 0, never a substituted ctime");
#endif

#if D_FILE_STAT_HAS_NSEC
    // a nanosecond field is allowed to be zero -- a file really can be
    // created on a second boundary -- so the only honest assertion is that it
    // is in range, not that it is nonzero
    items[idx++] = D_ASSERT_TRUE("nsec_in_range",
                                 st.st_modified_nsec < 1000000000u,
                                 "sub-second parts are within one second");
#else
    items[idx++] = D_ASSERT_EQUAL("nsec_zero", st.st_modified_nsec, 0u,
                                  "HAS_NSEC=0 means the field reads 0");
#endif

    return d_tests_file_group("timestamps (the macros must not lie)",
                              items, idx);
}


/*
d_tests_file_stat_sizes
  Tests d_file_size and d_file_size_stream.
*/
static struct d_test_object*
d_tests_file_stat_sizes
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    items[idx++] = D_ASSERT_EQUAL("file_size",
                                  d_file_size(path),
                                  (int64_t)strlen(D_TEST_FILE_TEST_CONTENT),
                                  "d_file_size reports the fixture's length");
    items[idx++] = D_ASSERT_EQUAL("missing", d_file_size("no/such/file"),
                                  (int64_t)-1,
                                  "d_file_size fails for a missing file");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null", d_file_size(NULL), (int64_t)-1,
                                  "d_file_size rejects NULL");
    #endif

    file = d_file_open_stream(path, "rb");

    if (file)
    {
        items[idx++] = D_ASSERT_EQUAL("stream",
                                      d_file_size_stream(file),
                                      (int64_t)strlen(D_TEST_FILE_TEST_CONTENT),
                                      "d_file_size_stream agrees");

        // it must use the descriptor, not seek: seeking would move the
        // position and the caller never asked for that
        items[idx++] = D_ASSERT_EQUAL("stream_position", ftell(file), 0L,
                                      "...without moving the stream position");
        (void)d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("stream_null", d_file_size_stream(NULL),
                                  (int64_t)-1,
                                  "d_file_size_stream rejects NULL");
    #endif

    return d_tests_file_group("d_file_size", items, idx);
}


/*
d_tests_file_stat_predicates
  Tests d_file_exists / d_file_is_regular / d_dir_exists, and d_file_access / d_file_chmod.
*/
static struct d_test_object*
d_tests_file_stat_predicates
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    struct d_stat_t       st;
    char                  path[D_TEST_FILE_PATH_BUF];
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    items[idx++] = D_ASSERT_TRUE("exists_file", d_file_exists(path),
                                 "the fixture exists");
    items[idx++] = D_ASSERT_TRUE("exists_dir",
                                 d_file_exists(D_TEST_FILE_TEMP_DIR),
                                 "a directory exists too");
    items[idx++] = D_ASSERT_FALSE("exists_missing",
                                  d_file_exists("no/such/file/at/all"),
                                  "a missing path does not exist");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_FALSE("exists_null", d_file_exists(NULL),
                                  "NULL does not exist");
    #endif

    items[idx++] = D_ASSERT_TRUE("is_file", d_file_is_regular(path),
                                 "the fixture is a regular file");
    items[idx++] = D_ASSERT_FALSE("is_file_dir",
                                  d_file_is_regular(D_TEST_FILE_TEMP_DIR),
                                  "a directory is not a regular file");
    items[idx++] = D_ASSERT_TRUE("is_dir", d_dir_exists(D_TEST_FILE_TEMP_DIR),
                                 "the scratch directory is a directory");
    items[idx++] = D_ASSERT_FALSE("is_dir_file", d_dir_exists(path),
                                  "a file is not a directory");

    items[idx++] = D_ASSERT_EQUAL("access_f_ok", d_file_access(path, F_OK), 0,
                                  "d_file_access confirms existence");
    items[idx++] = D_ASSERT_EQUAL("access_r_ok", d_file_access(path, R_OK), 0,
                                  "d_file_access confirms readability");
    items[idx++] = D_ASSERT_EQUAL("access_missing",
                                  d_file_access("no/such/file", F_OK), -1,
                                  "d_file_access fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("access_null", d_file_access(NULL, F_OK), -1,
                                  "d_file_access rejects NULL");
    #endif

    items[idx++] = D_ASSERT_EQUAL("chmod",
                                  d_file_chmod(path, S_IRUSR | S_IWUSR), 0,
                                  "d_file_chmod sets permissions");

#if !D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Windows maps the whole mode onto one read-only attribute, so asserting
    // the bits there would be asserting something the platform cannot express
    (void)d_file_stat(path, &st);
    items[idx++] = D_ASSERT_EQUAL("chmod_bits", st.st_mode & 0777, 0600u,
                                  "...and the bits take");
#else
    (void)st;
#endif

    (void)d_file_chmod(path, 0644);

    items[idx++] = D_ASSERT_EQUAL("chmod_missing",
                                  d_file_chmod("no/such/file", S_IRUSR), -1,
                                  "d_file_chmod fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("chmod_null", d_file_chmod(NULL, S_IRUSR), -1,
                                  "d_file_chmod rejects NULL");
    #endif

    return d_tests_file_group("predicates / d_file_access / d_file_chmod", items, idx);
}


/*
d_tests_file_stat_all
  Runs every file_stat test.
*/
struct d_test_object*
d_tests_file_stat_all
(
    void
)
{
    struct d_test_object* items[8];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_stat_basic();
    items[idx++] = d_tests_file_stat_times();
    items[idx++] = d_tests_file_stat_sizes();
    items[idx++] = d_tests_file_stat_predicates();

    return d_tests_file_group("VIII. file_stat", items, idx);
}
