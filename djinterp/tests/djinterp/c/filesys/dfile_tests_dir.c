/*******************************************************************************
* djinterp [c][filesys]                                        dfile_tests_dir.c
*
*   Tests for directory operations (mkdir, rmdir, opendir, readdir).
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_dir.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * X. DIRECTORY OPERATIONS TESTS
 *****************************************************************************/

/*
d_tests_dfile_mkdir
  Tests d_dir_create for creating directories.
  Tests the following:
  - creates new directory
  - returns error for existing directory
  - returns error for NULL path
*/
bool d_tests_dfile_mkdir(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_create;
    bool                  test_existing;
    bool                  test_null_path;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "mkdir_test");

    // test 1: create new directory
    result = d_dir_create(path_buf, S_IRWXU);
    test_create = (result == 0) && d_dir_exists(path_buf);

    // test 2: existing directory returns error
    result = d_dir_create(path_buf, S_IRWXU);
    test_existing = (result != 0);

    // cleanup
    d_dir_remove(path_buf);

    // test 3: NULL path
    result = d_dir_create(NULL, S_IRWXU);
    test_null_path = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_dir_create");

    D_TEST_CHECK_MSG(_suite,
                     "create",
                     test_create,
                     "d_dir_create creates new directory");
    D_TEST_CHECK_MSG(_suite,
                     "existing",
                     test_existing,
                     "d_dir_create returns error for existing");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_dir_create returns error for NULL path");

    return true;
}


/*
d_tests_dfile_mkdir_p
  Tests d_dir_create_parents for creating nested directories.
  Tests the following:
  - creates nested directories
  - succeeds for existing directory
  - returns error for NULL path
*/
bool d_tests_dfile_mkdir_p(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  parent_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_nested;
    bool                  test_existing;
    bool                  test_null_path;

    // setup nested path
    d_tests_dfile_get_test_path(parent_buf,
                               sizeof(parent_buf),
                               "mkdir_p_parent");
    d_path_join(path_buf,
                sizeof(path_buf),
                parent_buf,
                "child");

    // test 1: create nested directories
    result = d_dir_create_parents(path_buf, S_IRWXU);
    test_nested = (result == 0) && d_dir_exists(path_buf);

    // test 2: existing directory succeeds
    result = d_dir_create_parents(path_buf, S_IRWXU);
    test_existing = (result == 0);

    // cleanup: remove child first, then parent
    d_dir_remove(path_buf);
    d_dir_remove(parent_buf);

    // test 3: NULL path
    result = d_dir_create_parents(NULL, S_IRWXU);
    test_null_path = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_dir_create_parents");

    D_TEST_CHECK_MSG(_suite,
                     "nested",
                     test_nested,
                     "d_dir_create_parents creates nested directories");
    D_TEST_CHECK_MSG(_suite,
                     "existing",
                     test_existing,
                     "d_dir_create_parents succeeds for existing dir");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_dir_create_parents returns error for NULL path");

    return true;
}


/*
d_tests_dfile_rmdir
  Tests d_dir_remove for removing directories.
  Tests the following:
  - removes empty directory
  - returns error for nonexistent directory
  - returns error for NULL path
*/
bool d_tests_dfile_rmdir(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_remove;
    bool                  test_nonexistent;
    bool                  test_null_path;

    // setup: create directory to remove
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "rmdir_test");
    d_dir_create(path_buf, S_IRWXU);

    // test 1: remove empty directory
    result = d_dir_remove(path_buf);
    test_remove = (result == 0) && (!d_dir_exists(path_buf));

    // test 2: nonexistent directory
    result = d_dir_remove("nonexistent_rmdir_test");
    test_nonexistent = (result != 0);

    // test 3: NULL path
    result = d_dir_remove(NULL);
    test_null_path = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_dir_remove");

    D_TEST_CHECK_MSG(_suite,
                     "remove",
                     test_remove,
                     "d_dir_remove removes empty directory");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_dir_remove returns error for nonexistent");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_dir_remove returns error for NULL path");

    return true;
}


/*
d_tests_dfile_opendir_readdir_closedir
  Tests d_dir_open, d_dir_read, d_dir_close for directory traversal.
  Tests the following:
  - opens directory successfully
  - reads directory entries
  - finds expected file in directory
  - closes directory
  - returns NULL for nonexistent directory
*/
bool d_tests_dfile_opendir_readdir_closedir(struct dj_test_suite* _suite)
{
    struct dj_dir_t*       dir;
    struct dj_dirent_t*    entry;
    int                   close_result;
    bool                  test_open;
    bool                  test_read;
    bool                  test_found_file;
    bool                  test_close;
    bool                  test_nonexistent;
    size_t                entry_count;

    // test 1: open test directory
    dir = d_dir_open(D_TEST_DFILE_TEMP_DIR);
    test_open = (dir != NULL);
    test_read = false;
    test_found_file = false;
    test_close = false;
    entry_count = 0;

    if (dir)
    {
        // test 2 & 3: read entries and look for test file
        while ((entry = d_dir_read(dir)) != NULL)
        {
            entry_count++;

            if (strcmp(entry->d_name, D_TEST_DFILE_TEST_FILENAME) == 0)
            {
                test_found_file = true;
            }
        }

        test_read = (entry_count >= 1);  // at least . and ..

        // test 4: close directory
        close_result = d_dir_close(dir);
        test_close = (close_result == 0);
    }

    // test 5: nonexistent directory
    dir = d_dir_open("nonexistent_opendir_test");
    test_nonexistent = (dir == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_dir_open/d_dir_read/d_dir_close");

    D_TEST_CHECK_MSG(_suite, "open", test_open, "d_dir_open opens directory");
    D_TEST_CHECK_MSG(_suite, "read", test_read, "d_dir_read reads entries");
    D_TEST_CHECK_MSG(_suite,
                     "found_file",
                     test_found_file,
                     "d_dir_read finds expected file");
    D_TEST_CHECK_MSG(_suite,
                     "close",
                     test_close,
                     "d_dir_close closes directory");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_dir_open returns NULL for nonexistent");

    return true;
}


/*
d_tests_dfile_rewinddir
  Tests d_dir_rewind for resetting directory position.
  Tests the following:
  - rewinds and can re-read first entries
*/
bool d_tests_dfile_rewinddir(struct dj_test_suite* _suite)
{
    struct dj_dir_t*       dir;
    struct dj_dirent_t*    entry;
    char                  first_name[260];
    bool                  test_rewind;

    // open and read first entry
    dir = d_dir_open(D_TEST_DFILE_TEMP_DIR);
    test_rewind = false;

    if (dir)
    {
        entry = d_dir_read(dir);

        if (entry)
        {
            d_strcpy_s(first_name, sizeof(first_name), entry->d_name);

            // read a few more entries
            d_dir_read(dir);
            d_dir_read(dir);

            // rewind
            d_dir_rewind(dir);

            // read first entry again
            entry = d_dir_read(dir);

            if (entry)
            {
                test_rewind = (strcmp(entry->d_name, first_name) == 0);
            }
        }

        d_dir_close(dir);
    }

    // build result tree
    d_test_suite_test(_suite, "d_dir_rewind");

    D_TEST_CHECK_MSG(_suite,
                     "rewind",
                     test_rewind,
                     "d_dir_rewind resets position");

    return true;
}


/*
d_tests_dfile_directory_operations_all
  Runs all directory operation tests.
  Tests the following:
  - d_dir_create
  - d_dir_create_parents
  - d_dir_remove
  - d_dir_open/d_dir_read/d_dir_close
  - d_dir_rewind
*/
bool d_tests_dfile_directory_operations_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "X. Directory Operations");

    d_tests_dfile_mkdir(_suite);
    d_tests_dfile_mkdir_p(_suite);
    d_tests_dfile_rmdir(_suite);
    d_tests_dfile_opendir_readdir_closedir(_suite);
    d_tests_dfile_rewinddir(_suite);

    return true;
}
