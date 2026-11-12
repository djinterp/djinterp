/*******************************************************************************
* djinterp [c][filesys]                                        dfile_tests_ops.c
*
*   Tests for file operations (remove, unlink, rename, copy).
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_ops.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * XI. FILE OPERATIONS TESTS
 *****************************************************************************/

/*
d_tests_dfile_remove
  Tests d_file_remove for removing files.
  Tests the following:
  - removes existing file
  - returns error for nonexistent file
  - returns error for NULL path
*/
bool d_tests_dfile_remove(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_remove;
    bool                  test_nonexistent;
    bool                  test_null_path;

    // setup: create file to remove
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "remove_test.txt");
    d_file_write_all(path_buf, "test", 4);

    // test 1: remove existing file
    result = d_file_remove(path_buf);
    test_remove = (result == 0) && (!d_file_exists(path_buf));

    // test 2: nonexistent file
    result = d_file_remove("nonexistent_remove_test.txt");
    test_nonexistent = (result != 0);

    // test 3: NULL path
    result = d_file_remove(NULL);
    test_null_path = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_remove");

    D_TEST_CHECK_MSG(_suite,
                     "remove",
                     test_remove,
                     "d_file_remove removes existing file");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_remove returns error for nonexistent");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_file_remove returns error for NULL path");

    return true;
}


/*
d_tests_dfile_unlink
  Tests d_file_unlink for unlinking files.
  Tests the following:
  - unlinks existing file
  - returns error for nonexistent file
*/
bool d_tests_dfile_unlink(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_unlink;
    bool                  test_nonexistent;

    // setup: create file to unlink
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "unlink_test.txt");
    d_file_write_all(path_buf, "test", 4);

    // test 1: unlink existing file
    result = d_file_unlink(path_buf);
    test_unlink = (result == 0) && (!d_file_exists(path_buf));

    // test 2: nonexistent file
    result = d_file_unlink("nonexistent_unlink_test.txt");
    test_nonexistent = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_unlink");

    D_TEST_CHECK_MSG(_suite,
                     "unlink",
                     test_unlink,
                     "d_file_unlink unlinks existing file");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_unlink returns error for nonexistent");

    return true;
}


/*
d_tests_dfile_rename
  Tests d_file_rename for renaming/moving files.
  Tests the following:
  - renames file to new name
  - overwrites existing with overwrite flag
  - fails to overwrite without flag
*/
bool d_tests_dfile_rename(struct dj_test_suite* _suite)
{
    char                  src_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  dst_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_rename;
    bool                  test_overwrite;
    bool                  test_no_overwrite;

    // setup paths
    d_tests_dfile_get_test_path(src_buf,
                               sizeof(src_buf),
                               "rename_src.txt");
    d_tests_dfile_get_test_path(dst_buf,
                               sizeof(dst_buf),
                               "rename_dst.txt");

    // test 1: simple rename
    d_file_write_all(src_buf, "source", 6);
    result = d_file_rename(src_buf, dst_buf, 0);
    test_rename = (result == 0) &&
                  d_file_exists(dst_buf) &&
                  (!d_file_exists(src_buf));

    // cleanup
    d_file_remove(dst_buf);

    // test 2: rename with overwrite
    d_file_write_all(src_buf, "new source", 10);
    d_file_write_all(dst_buf, "old dest", 8);
    result = d_file_rename(src_buf, dst_buf, 1);  // overwrite = 1
    test_overwrite = (result == 0) &&
                     d_file_exists(dst_buf) &&
                     (!d_file_exists(src_buf));

    // cleanup
    d_file_remove(dst_buf);

    // test 3: rename without overwrite should fail
    d_file_write_all(src_buf, "source", 6);
    d_file_write_all(dst_buf, "existing", 8);
    result = d_file_rename(src_buf, dst_buf, 0);  // overwrite = 0
    test_no_overwrite = (result != 0);

    // cleanup
    d_file_remove(src_buf);
    d_file_remove(dst_buf);

    // build result tree
    d_test_suite_test(_suite, "d_file_rename");

    D_TEST_CHECK_MSG(_suite,
                     "rename",
                     test_rename,
                     "d_file_rename renames file");
    D_TEST_CHECK_MSG(_suite,
                     "overwrite",
                     test_overwrite,
                     "d_file_rename overwrites with flag");
    D_TEST_CHECK_MSG(_suite,
                     "no_overwrite",
                     test_no_overwrite,
                     "d_file_rename fails without overwrite flag");

    return true;
}


/*
d_tests_dfile_copy_file
  Tests d_file_copy for copying file contents.
  Tests the following:
  - copies file successfully
  - copied content matches original
  - returns error for nonexistent source
*/
bool d_tests_dfile_copy_file(struct dj_test_suite* _suite)
{
    char                  src_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  dst_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char*                 content;
    size_t                size;
    int                   result;
    bool                  test_copy;
    bool                  test_content;
    bool                  test_nonexistent;

    // setup paths
    d_tests_dfile_get_test_path(src_buf,
                               sizeof(src_buf),
                               D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(dst_buf,
                               sizeof(dst_buf),
                               "copy_dst.txt");

    // test 1: copy file
    result = d_file_copy(src_buf, dst_buf);
    test_copy = (result == 0) && d_file_exists(dst_buf);

    // test 2: verify content
    test_content = false;

    if (test_copy)
    {
        content = (char*)d_file_read_all(dst_buf, &size);

        if (content)
        {
            test_content = (strcmp(content, D_TEST_DFILE_TEST_CONTENT) == 0);
            free(content);
        }
    }

    // cleanup
    d_file_remove(dst_buf);

    // test 3: nonexistent source
    result = d_file_copy("nonexistent_copy_source.txt", dst_buf);
    test_nonexistent = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_copy");

    D_TEST_CHECK_MSG(_suite, "copy", test_copy, "d_file_copy copies file");
    D_TEST_CHECK_MSG(_suite,
                     "content",
                     test_content,
                     "copied content matches original");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_copy fails for nonexistent source");

    return true;
}


/*
d_tests_dfile_file_operations_all
  Runs all file operation tests.
  Tests the following:
  - d_file_remove
  - d_file_unlink
  - d_file_rename
  - d_file_copy
*/
bool d_tests_dfile_file_operations_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "XI. File Operations");

    d_tests_dfile_remove(_suite);
    d_tests_dfile_unlink(_suite);
    d_tests_dfile_rename(_suite);
    d_tests_dfile_copy_file(_suite);

    return true;
}
