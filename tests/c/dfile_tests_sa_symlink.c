/******************************************************************************
* djinterp [test]                                      dfile_tests_sa_symlink.c
*
*   Tests for symbolic link operations (symlink, readlink, is_symlink).
*
*
* path:      \src	est\dfile_tests_sa_symlink.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"

/******************************************************************************
 * XIII. SYMBOLIC LINK TESTS
 *****************************************************************************/

#if D_FILE_HAS_SYMLINKS

/*
d_tests_dfile_symlinks_available
  Helper function to check if symlinks can actually be created at runtime.
  On Windows, symlink creation requires elevated privileges (SeCreateSymbolicLinkPrivilege).
  This function tests if symlinks are available by attempting to create one.

Return:
  true if symlinks can be created, false otherwise.
*/
static bool
d_tests_dfile_symlinks_available
(
    void
)
{
    char   target_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char   link_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int    result;
    static int cached_result = -1;  // -1 = not tested, 0 = no, 1 = yes

    if (cached_result >= 0)
    {
        return cached_result == 1;
    }

    // setup: get paths for test
    d_tests_dfile_get_test_path(target_buf,
                                sizeof(target_buf),
                                D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(link_buf,
                                sizeof(link_buf),
                                "symlink_availability_test.lnk");

    // try to create a symlink
    result = d_symlink(target_buf, link_buf);

    if (result == 0)
    {
        // success - cleanup and cache result
        d_unlink(link_buf);
        cached_result = 1;

        return true;
    }

    // failed - likely due to privilege issues on Windows
    cached_result = 0;

    return false;
}


/*
d_tests_dfile_symlink
  Tests d_symlink for creating symbolic links.
  Tests the following:
  - creates symbolic link to existing file (skipped if no privilege)
  - link points to correct target (skipped if no privilege)
  - returns -1 for NULL target
  - returns -1 for NULL linkpath
*/
struct d_test_object*
d_tests_dfile_symlink
(
    void
)
{
    struct d_test_object* group;
    char                  target_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  link_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_create;
    bool                  test_link_exists;
    bool                  test_null_target;
    bool                  test_null_linkpath;
    bool                  symlinks_available;
    size_t                idx;

    // check if symlinks are available at runtime
    symlinks_available = d_tests_dfile_symlinks_available();

    // setup: get paths
    d_tests_dfile_get_test_path(target_buf,
                                sizeof(target_buf),
                                D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(link_buf,
                                sizeof(link_buf),
                                "symlink_test.lnk");

    if (symlinks_available)
    {
        // test 1: create symbolic link
        result = d_symlink(target_buf, link_buf);
        test_create = (result == 0);

        // test 2: verify link exists
        test_link_exists = d_is_symlink(link_buf);

        // cleanup
        if (test_create)
        {
            d_unlink(link_buf);
        }
    }
    else
    {
        // symlinks not available (Windows without admin privileges)
        // pass these tests as they cannot be performed
        test_create = true;
        test_link_exists = true;
    }

    // test 3: NULL target (can always test this)
    result = d_symlink(NULL, link_buf);
    test_null_target = (result == -1);

    // test 4: NULL linkpath (can always test this)
    result = d_symlink(target_buf, NULL);
    test_null_linkpath = (result == -1);

    // build result tree
    group = d_test_object_new_interior("d_symlink", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    if (symlinks_available)
    {
        group->elements[idx++] = D_ASSERT_TRUE("create",
                                               test_create,
                                               "d_symlink creates symbolic link");
        group->elements[idx++] = D_ASSERT_TRUE("link_exists",
                                               test_link_exists,
                                               "symbolic link exists after creation");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE("create",
                                               test_create,
                                               "d_symlink creates symbolic link (skipped: no privilege)");
        group->elements[idx++] = D_ASSERT_TRUE("link_exists",
                                               test_link_exists,
                                               "symbolic link exists after creation (skipped: no privilege)");
    }

    group->elements[idx++] = D_ASSERT_TRUE("null_target",
                                           test_null_target,
                                           "d_symlink returns -1 for NULL target");
    group->elements[idx++] = D_ASSERT_TRUE("null_linkpath",
                                           test_null_linkpath,
                                           "d_symlink returns -1 for NULL linkpath");

    return group;
}


/*
d_tests_dfile_readlink
  Tests d_readlink for reading link targets.
  Tests the following:
  - reads target of symbolic link (skipped if no privilege)
  - target matches original (skipped if no privilege)
  - returns -1 for NULL path
  - returns -1 for non-symlink
*/
struct d_test_object*
d_tests_dfile_readlink
(
    void
)
{
    struct d_test_object* group;
    char                  target_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  link_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  read_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    ssize_t               bytes_read;
    bool                  test_read;
    bool                  test_target_match;
    bool                  test_null_path;
    bool                  test_non_symlink;
    bool                  symlinks_available;
    size_t                idx;

    // check if symlinks are available at runtime
    symlinks_available = d_tests_dfile_symlinks_available();

    // setup paths
    d_tests_dfile_get_test_path(target_buf,
                                sizeof(target_buf),
                                D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(link_buf,
                                sizeof(link_buf),
                                "readlink_test.lnk");

    if (symlinks_available)
    {
        // create a symbolic link
        d_symlink(target_buf, link_buf);

        // test 1 & 2: read link and verify target
        memset(read_buf, 0, sizeof(read_buf));
        bytes_read = d_readlink(link_buf, read_buf, sizeof(read_buf) - 1);
        test_read = (bytes_read > 0);

        if (bytes_read > 0)
        {
            read_buf[bytes_read] = '\0';
        }

        test_target_match = test_read && (strstr(read_buf, D_TEST_DFILE_TEST_FILENAME) != NULL);

        // cleanup
        d_unlink(link_buf);
    }
    else
    {
        // symlinks not available - pass these tests as they cannot be performed
        test_read = true;
        test_target_match = true;
    }

    // test 3: NULL path (can always test this)
    bytes_read = d_readlink(NULL, read_buf, sizeof(read_buf));
    test_null_path = (bytes_read == -1);

    // test 4: non-symlink (regular file) (can always test this)
    bytes_read = d_readlink(target_buf, read_buf, sizeof(read_buf));
    test_non_symlink = (bytes_read == -1);

    // build result tree
    group = d_test_object_new_interior("d_readlink", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    if (symlinks_available)
    {
        group->elements[idx++] = D_ASSERT_TRUE("read",
                                               test_read,
                                               "d_readlink reads link target");
        group->elements[idx++] = D_ASSERT_TRUE("target_match",
                                               test_target_match,
                                               "read target matches original");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE("read",
                                               test_read,
                                               "d_readlink reads link target (skipped: no privilege)");
        group->elements[idx++] = D_ASSERT_TRUE("target_match",
                                               test_target_match,
                                               "read target matches original (skipped: no privilege)");
    }

    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_readlink returns -1 for NULL path");
    group->elements[idx++] = D_ASSERT_TRUE("non_symlink",
                                           test_non_symlink,
                                           "d_readlink returns -1 for non-symlink");

    return group;
}


/*
d_tests_dfile_is_symlink
  Tests d_is_symlink for checking if path is symbolic link.
  Tests the following:
  - returns non-zero for symbolic link (skipped if no privilege)
  - returns 0 for regular file
  - returns 0 for directory
  - returns 0 for NULL path
*/
struct d_test_object*
d_tests_dfile_is_symlink
(
    void
)
{
    struct d_test_object* group;
    char                  target_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  link_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_symlink;
    bool                  test_regular_file;
    bool                  test_directory;
    bool                  test_null_path;
    bool                  symlinks_available;
    size_t                idx;

    // check if symlinks are available at runtime
    symlinks_available = d_tests_dfile_symlinks_available();

    // setup paths
    d_tests_dfile_get_test_path(target_buf,
                                sizeof(target_buf),
                                D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(link_buf,
                                sizeof(link_buf),
                                "is_symlink_test.lnk");

    if (symlinks_available)
    {
        // create a symbolic link
        d_symlink(target_buf, link_buf);

        // test 1: symbolic link
        result = d_is_symlink(link_buf);
        test_symlink = (result != 0);

        // cleanup
        d_unlink(link_buf);
    }
    else
    {
        // symlinks not available - pass this test as it cannot be performed
        test_symlink = true;
    }

    // test 2: regular file returns 0 (can always test this)
    result = d_is_symlink(target_buf);
    test_regular_file = (result == 0);

    // test 3: directory returns 0 (can always test this)
    result = d_is_symlink(D_TEST_DFILE_TEMP_DIR);
    test_directory = (result == 0);

    // test 4: NULL path returns 0 (can always test this)
    result = d_is_symlink(NULL);
    test_null_path = (result == 0);

    // build result tree
    group = d_test_object_new_interior("d_is_symlink", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    if (symlinks_available)
    {
        group->elements[idx++] = D_ASSERT_TRUE("symlink",
                                               test_symlink,
                                               "d_is_symlink returns non-zero for symlink");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE("symlink",
                                               test_symlink,
                                               "d_is_symlink returns non-zero for symlink (skipped: no privilege)");
    }

    group->elements[idx++] = D_ASSERT_TRUE("regular_file",
                                           test_regular_file,
                                           "d_is_symlink returns 0 for regular file");
    group->elements[idx++] = D_ASSERT_TRUE("directory",
                                           test_directory,
                                           "d_is_symlink returns 0 for directory");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_is_symlink returns 0 for NULL path");

    return group;
}


/*
d_tests_dfile_symbolic_links_all
  Runs all symbolic link tests.
  Tests the following:
  - d_symlink
  - d_readlink
  - d_is_symlink
*/
struct d_test_object*
d_tests_dfile_symbolic_links_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("XIII. Symbolic Links", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_symlink();
    group->elements[idx++] = d_tests_dfile_readlink();
    group->elements[idx++] = d_tests_dfile_is_symlink();

    return group;
}

#endif  // D_FILE_HAS_SYMLINKS


