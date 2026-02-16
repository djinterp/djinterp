/******************************************************************************
* djinterp [test]                                          dfile_tests_sa_dir.c
*
*   Tests for directory operations (mkdir, rmdir, opendir, readdir).
*
*
* path:      \src	est\dfile_tests_sa_dir.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"


/******************************************************************************
 * X. DIRECTORY OPERATIONS TESTS
 *****************************************************************************/

/*
d_tests_dfile_mkdir
  Tests d_mkdir for creating directories.
  Tests the following:
  - creates new directory
  - returns error for existing directory
  - returns error for NULL path
*/
struct d_test_object*
d_tests_dfile_mkdir
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_create;
    bool                  test_existing;
    bool                  test_null_path;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "mkdir_test");

    // test 1: create new directory
    result = d_mkdir(path_buf, S_IRWXU);
    test_create = (result == 0) && d_is_dir(path_buf);

    // test 2: existing directory returns error
    result = d_mkdir(path_buf, S_IRWXU);
    test_existing = (result != 0);

    // cleanup
    d_rmdir(path_buf);

    // test 3: NULL path
    result = d_mkdir(NULL, S_IRWXU);
    test_null_path = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_mkdir", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("create",
                                           test_create,
                                           "d_mkdir creates new directory");
    group->elements[idx++] = D_ASSERT_TRUE("existing",
                                           test_existing,
                                           "d_mkdir returns error for existing");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_mkdir returns error for NULL path");

    return group;
}


/*
d_tests_dfile_mkdir_p
  Tests d_mkdir_p for creating nested directories.
  Tests the following:
  - creates nested directories
  - succeeds for existing directory
  - returns error for NULL path
*/
struct d_test_object*
d_tests_dfile_mkdir_p
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  parent_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_nested;
    bool                  test_existing;
    bool                  test_null_path;
    size_t                idx;

    // setup nested path
    d_tests_dfile_get_test_path(parent_buf,
                               sizeof(parent_buf),
                               "mkdir_p_parent");
    d_path_join(path_buf,
                sizeof(path_buf),
                parent_buf,
                "child");

    // test 1: create nested directories
    result = d_mkdir_p(path_buf, S_IRWXU);
    test_nested = (result == 0) && d_is_dir(path_buf);

    // test 2: existing directory succeeds
    result = d_mkdir_p(path_buf, S_IRWXU);
    test_existing = (result == 0);

    // cleanup: remove child first, then parent
    d_rmdir(path_buf);
    d_rmdir(parent_buf);

    // test 3: NULL path
    result = d_mkdir_p(NULL, S_IRWXU);
    test_null_path = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_mkdir_p", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("nested",
                                           test_nested,
                                           "d_mkdir_p creates nested directories");
    group->elements[idx++] = D_ASSERT_TRUE("existing",
                                           test_existing,
                                           "d_mkdir_p succeeds for existing dir");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_mkdir_p returns error for NULL path");

    return group;
}


/*
d_tests_dfile_rmdir
  Tests d_rmdir for removing directories.
  Tests the following:
  - removes empty directory
  - returns error for nonexistent directory
  - returns error for NULL path
*/
struct d_test_object*
d_tests_dfile_rmdir
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_remove;
    bool                  test_nonexistent;
    bool                  test_null_path;
    size_t                idx;

    // setup: create directory to remove
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "rmdir_test");
    d_mkdir(path_buf, S_IRWXU);

    // test 1: remove empty directory
    result = d_rmdir(path_buf);
    test_remove = (result == 0) && (!d_is_dir(path_buf));

    // test 2: nonexistent directory
    result = d_rmdir("nonexistent_rmdir_test");
    test_nonexistent = (result != 0);

    // test 3: NULL path
    result = d_rmdir(NULL);
    test_null_path = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_rmdir", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("remove",
                                           test_remove,
                                           "d_rmdir removes empty directory");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_rmdir returns error for nonexistent");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_rmdir returns error for NULL path");

    return group;
}


/*
d_tests_dfile_opendir_readdir_closedir
  Tests d_opendir, d_readdir, d_closedir for directory traversal.
  Tests the following:
  - opens directory successfully
  - reads directory entries
  - finds expected file in directory
  - closes directory
  - returns NULL for nonexistent directory
*/
struct d_test_object*
d_tests_dfile_opendir_readdir_closedir
(
    void
)
{
    struct d_test_object* group;
    struct d_dir_t*       dir;
    struct d_dirent_t*    entry;
    int                   close_result;
    bool                  test_open;
    bool                  test_read;
    bool                  test_found_file;
    bool                  test_close;
    bool                  test_nonexistent;
    size_t                idx;
    size_t                entry_count;

    // test 1: open test directory
    dir = d_opendir(D_TEST_DFILE_TEMP_DIR);
    test_open = (dir != NULL);
    test_read = false;
    test_found_file = false;
    test_close = false;
    entry_count = 0;

    if (dir)
    {
        // test 2 & 3: read entries and look for test file
        while ((entry = d_readdir(dir)) != NULL)
        {
            entry_count++;

            if (strcmp(entry->d_name, D_TEST_DFILE_TEST_FILENAME) == 0)
            {
                test_found_file = true;
            }
        }

        test_read = (entry_count >= 1);  // at least . and ..

        // test 4: close directory
        close_result = d_closedir(dir);
        test_close = (close_result == 0);
    }

    // test 5: nonexistent directory
    dir = d_opendir("nonexistent_opendir_test");
    test_nonexistent = (dir == NULL);

    // build result tree
    group = d_test_object_new_interior("d_opendir/d_readdir/d_closedir", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("open",
                                           test_open,
                                           "d_opendir opens directory");
    group->elements[idx++] = D_ASSERT_TRUE("read",
                                           test_read,
                                           "d_readdir reads entries");
    group->elements[idx++] = D_ASSERT_TRUE("found_file",
                                           test_found_file,
                                           "d_readdir finds expected file");
    group->elements[idx++] = D_ASSERT_TRUE("close",
                                           test_close,
                                           "d_closedir closes directory");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_opendir returns NULL for nonexistent");

    return group;
}


/*
d_tests_dfile_rewinddir
  Tests d_rewinddir for resetting directory position.
  Tests the following:
  - rewinds and can re-read first entries
*/
struct d_test_object*
d_tests_dfile_rewinddir
(
    void
)
{
    struct d_test_object* group;
    struct d_dir_t*       dir;
    struct d_dirent_t*    entry;
    char                  first_name[260];
    bool                  test_rewind;
    size_t                idx;

    // open and read first entry
    dir = d_opendir(D_TEST_DFILE_TEMP_DIR);
    test_rewind = false;

    if (dir)
    {
        entry = d_readdir(dir);

        if (entry)
        {
            d_strcpy_s(first_name, sizeof(first_name), entry->d_name);

            // read a few more entries
            d_readdir(dir);
            d_readdir(dir);

            // rewind
            d_rewinddir(dir);

            // read first entry again
            entry = d_readdir(dir);

            if (entry)
            {
                test_rewind = (strcmp(entry->d_name, first_name) == 0);
            }
        }

        d_closedir(dir);
    }

    // build result tree
    group = d_test_object_new_interior("d_rewinddir", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("rewind",
                                           test_rewind,
                                           "d_rewinddir resets position");

    return group;
}


/*
d_tests_dfile_directory_operations_all
  Runs all directory operation tests.
  Tests the following:
  - d_mkdir
  - d_mkdir_p
  - d_rmdir
  - d_opendir/d_readdir/d_closedir
  - d_rewinddir
*/
struct d_test_object*
d_tests_dfile_directory_operations_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("X. Directory Operations", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_mkdir();
    group->elements[idx++] = d_tests_dfile_mkdir_p();
    group->elements[idx++] = d_tests_dfile_rmdir();
    group->elements[idx++] = d_tests_dfile_opendir_readdir_closedir();
    group->elements[idx++] = d_tests_dfile_rewinddir();

    return group;
}


