/*******************************************************************************
* djinterp [fs]                                              file_tests_sa_ops.c
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sa_ops.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.20
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

/******************************************************************************
 * XI.  file_ops  --  REMOVE / RENAME / COPY
 *****************************************************************************/

/*
d_tests_file_remove_unlink
  Tests d_file_remove and d_file_unlink, and the one thing that distinguishes them.
*/
static struct d_test_object*
d_tests_file_remove_unlink
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    char                  dir[D_TEST_FILE_PATH_BUF];
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), "remove_test.txt");
    d_tests_file_path(dir, sizeof(dir), "remove_dir");

    (void)d_file_write_all(path, "test", 4);
    items[idx++] = D_ASSERT_EQUAL("remove", d_file_remove(path), 0,
                                  "d_file_remove removes a file");
    items[idx++] = D_ASSERT_FALSE("removed", d_file_exists(path),
                                  "...and it is gone");
    items[idx++] = D_ASSERT_EQUAL("remove_missing",
                                  d_file_remove("no/such/file"), -1,
                                  "d_file_remove fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("remove_null", d_file_remove(NULL), -1,
                                  "d_file_remove rejects NULL");
    #endif

    // this is the difference: remove takes directories, unlink does not
    (void)d_dir_create(dir, S_IRWXU);
    items[idx++] = D_ASSERT_EQUAL("remove_dir", d_file_remove(dir), 0,
                                  "d_file_remove accepts an empty directory");

    (void)d_file_write_all(path, "test", 4);
    items[idx++] = D_ASSERT_EQUAL("unlink", d_file_unlink(path), 0,
                                  "d_file_unlink removes a file");
    items[idx++] = D_ASSERT_EQUAL("unlink_missing",
                                  d_file_unlink("no/such/file"), -1,
                                  "d_file_unlink fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("unlink_null", d_file_unlink(NULL), -1,
                                  "d_file_unlink rejects NULL");
    #endif

#if D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    (void)d_dir_create(dir, S_IRWXU);
    items[idx++] = D_ASSERT_EQUAL("unlink_refuses_dir", d_file_unlink(dir), -1,
                                  "d_file_unlink refuses a directory");
    (void)d_dir_remove(dir);
#endif

    return d_tests_file_group("d_file_remove / d_file_unlink", items, idx);
}


/*
d_tests_file_rename
  Tests d_file_rename.
*/
static struct d_test_object*
d_tests_file_rename
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  src[D_TEST_FILE_PATH_BUF];
    char                  dst[D_TEST_FILE_PATH_BUF];
    char*                 back;
    size_t                size;
    size_t                idx;

    idx = 0;
    d_tests_file_path(src, sizeof(src), "rename_src.txt");
    d_tests_file_path(dst, sizeof(dst), "rename_dst.txt");

    (void)d_file_write_all(src, "source", 6);
    items[idx++] = D_ASSERT_EQUAL("rename", d_file_rename(src, dst, 0), 0,
                                  "d_file_rename moves a file");
    items[idx++] = D_ASSERT_TRUE("moved",
                                 d_file_exists(dst) && !d_file_exists(src),
                                 "...and the old name is gone");
    (void)d_file_remove(dst);

    (void)d_file_write_all(src, "new source", 10);
    (void)d_file_write_all(dst, "old dest", 8);
    items[idx++] = D_ASSERT_EQUAL("overwrite", d_file_rename(src, dst, 1), 0,
                                  "overwrite=1 replaces the destination");

    back = (char*)d_file_read_all(dst, &size);
    items[idx++] = D_ASSERT_TRUE("overwrite_content",
                                 back && size == 10 &&
                                 memcmp(back, "new source", 10) == 0,
                                 "...with the source's contents");

    if (back)
    {
        free(back);
    }

    // overwrite is a PARAMETER, not a knob, because neither platform's
    // default is portable: POSIX rename always clobbers, Win32 never does
    (void)d_file_write_all(src, "source", 6);
    items[idx++] = D_ASSERT_EQUAL("no_overwrite", d_file_rename(src, dst, 0), -1,
                                  "overwrite=0 refuses an existing destination");
    items[idx++] = D_ASSERT_TRUE("source_survives", d_file_exists(src),
                                 "...and leaves the source alone");

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_src", d_file_rename(NULL, dst, 0), -1,
                                  "d_file_rename rejects a NULL source");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_dst", d_file_rename(src, NULL, 0), -1,
                                  "d_file_rename rejects a NULL destination");
    #endif

    (void)d_file_remove(src);
    (void)d_file_remove(dst);

    return d_tests_file_group(
               D_FILE_OPS_RENAME_IS_ATOMIC ?
                   "d_file_rename (always atomic)" :
                   "d_file_rename (cross-device fallback is NOT atomic)",
               items,
               idx);
}


/*
d_tests_file_copy
  Tests d_file_copy.
*/
static struct d_test_object*
d_tests_file_copy
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    struct d_stat_t       st;
    char                  src[D_TEST_FILE_PATH_BUF];
    char                  dst[D_TEST_FILE_PATH_BUF];
    char*                 back;
    size_t                size;
    size_t                idx;

    idx = 0;
    d_tests_file_path(src, sizeof(src), D_TEST_FILE_TEST_FILENAME);
    d_tests_file_path(dst, sizeof(dst), "copy_dst.txt");

    items[idx++] = D_ASSERT_EQUAL("copy", d_file_copy(src, dst), 0,
                                  "d_file_copy copies a file");

    back = (char*)d_file_read_all(dst, &size);
    items[idx++] = D_ASSERT_TRUE("content",
                                 back &&
                                 size == strlen(D_TEST_FILE_TEST_CONTENT) &&
                                 memcmp(back, D_TEST_FILE_TEST_CONTENT, size) == 0,
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
    items[idx++] = D_ASSERT_EQUAL("preserve_mode", st.st_mode & 0777, 0600u,
                                  "a copy of a 0600 file is not world-readable");
    (void)d_file_chmod(src, 0644);
    (void)d_file_remove(dst);
#else
    (void)st;
#endif

    items[idx++] = D_ASSERT_EQUAL("missing_source",
                                  d_file_copy("no/such/file", dst), -1,
                                  "d_file_copy fails for a missing source");

    // a failed copy must not leave a partial file claiming to be one
    items[idx++] = D_ASSERT_FALSE("no_debris", d_file_exists(dst),
                                  "a failed copy leaves no destination behind");

    items[idx++] = D_ASSERT_EQUAL("dir_source",
                                  d_file_copy(D_TEST_FILE_TEMP_DIR, dst), -1,
                                  "d_file_copy refuses a directory");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_src", d_file_copy(NULL, dst), -1,
                                  "d_file_copy rejects a NULL source");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_dst", d_file_copy(src, NULL), -1,
                                  "d_file_copy rejects a NULL destination");
    #endif

    return d_tests_file_group(
               D_FILE_OPS_COPY_IS_NATIVE ?
                   "d_file_copy (native engine)" :
                   "d_file_copy (portable buffer)",
               items,
               idx);
}


/*
d_tests_file_ops_all
  Runs every file_ops test.
*/
struct d_test_object*
d_tests_file_ops_all
(
    void
)
{
    struct d_test_object* items[4];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_remove_unlink();
    items[idx++] = d_tests_file_rename();
    items[idx++] = d_tests_file_copy();

    return d_tests_file_group("XI. file_ops", items, idx);
}
