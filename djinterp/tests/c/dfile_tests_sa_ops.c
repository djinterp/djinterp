/******************************************************************************
* djinterp [test]                                          dfile_tests_sa_ops.c
*
*   Tests for file operations (remove, unlink, rename, copy).
*
*
* path:      \src	est\dfile_tests_sa_ops.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"


/******************************************************************************
 * XI. FILE OPERATIONS TESTS
 *****************************************************************************/

/*
d_tests_dfile_remove
  Tests d_remove for removing files.
  Tests the following:
  - removes existing file
  - returns error for nonexistent file
  - returns error for NULL path
*/
struct d_test_object*
d_tests_dfile_remove
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

    // setup: create file to remove
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "remove_test.txt");
    d_fwrite_all(path_buf, "test", 4);

    // test 1: remove existing file
    result = d_remove(path_buf);
    test_remove = (result == 0) && (!d_file_exists(path_buf));

    // test 2: nonexistent file
    result = d_remove("nonexistent_remove_test.txt");
    test_nonexistent = (result != 0);

    // test 3: NULL path
    result = d_remove(NULL);
    test_null_path = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_remove", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("remove",
                                           test_remove,
                                           "d_remove removes existing file");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_remove returns error for nonexistent");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_remove returns error for NULL path");

    return group;
}


/*
d_tests_dfile_unlink
  Tests d_unlink for unlinking files.
  Tests the following:
  - unlinks existing file
  - returns error for nonexistent file
*/
struct d_test_object*
d_tests_dfile_unlink
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_unlink;
    bool                  test_nonexistent;
    size_t                idx;

    // setup: create file to unlink
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "unlink_test.txt");
    d_fwrite_all(path_buf, "test", 4);

    // test 1: unlink existing file
    result = d_unlink(path_buf);
    test_unlink = (result == 0) && (!d_file_exists(path_buf));

    // test 2: nonexistent file
    result = d_unlink("nonexistent_unlink_test.txt");
    test_nonexistent = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_unlink", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("unlink",
                                           test_unlink,
                                           "d_unlink unlinks existing file");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_unlink returns error for nonexistent");

    return group;
}


/*
d_tests_dfile_rename
  Tests d_rename for renaming/moving files.
  Tests the following:
  - renames file to new name
  - overwrites existing with overwrite flag
  - fails to overwrite without flag
*/
struct d_test_object*
d_tests_dfile_rename
(
    void
)
{
    struct d_test_object* group;
    char                  src_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  dst_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_rename;
    bool                  test_overwrite;
    bool                  test_no_overwrite;
    size_t                idx;

    // setup paths
    d_tests_dfile_get_test_path(src_buf,
                               sizeof(src_buf),
                               "rename_src.txt");
    d_tests_dfile_get_test_path(dst_buf,
                               sizeof(dst_buf),
                               "rename_dst.txt");

    // test 1: simple rename
    d_fwrite_all(src_buf, "source", 6);
    result = d_rename(src_buf, dst_buf, 0);
    test_rename = (result == 0) && 
                  d_file_exists(dst_buf) && 
                  (!d_file_exists(src_buf));

    // cleanup
    d_remove(dst_buf);

    // test 2: rename with overwrite
    d_fwrite_all(src_buf, "new source", 10);
    d_fwrite_all(dst_buf, "old dest", 8);
    result = d_rename(src_buf, dst_buf, 1);  // overwrite = 1
    test_overwrite = (result == 0) && 
                     d_file_exists(dst_buf) && 
                     (!d_file_exists(src_buf));

    // cleanup
    d_remove(dst_buf);

    // test 3: rename without overwrite should fail
    d_fwrite_all(src_buf, "source", 6);
    d_fwrite_all(dst_buf, "existing", 8);
    result = d_rename(src_buf, dst_buf, 0);  // overwrite = 0
    test_no_overwrite = (result != 0);

    // cleanup
    d_remove(src_buf);
    d_remove(dst_buf);

    // build result tree
    group = d_test_object_new_interior("d_rename", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("rename",
                                           test_rename,
                                           "d_rename renames file");
    group->elements[idx++] = D_ASSERT_TRUE("overwrite",
                                           test_overwrite,
                                           "d_rename overwrites with flag");
    group->elements[idx++] = D_ASSERT_TRUE("no_overwrite",
                                           test_no_overwrite,
                                           "d_rename fails without overwrite flag");

    return group;
}


/*
d_tests_dfile_copy_file
  Tests d_copy_file for copying file contents.
  Tests the following:
  - copies file successfully
  - copied content matches original
  - returns error for nonexistent source
*/
struct d_test_object*
d_tests_dfile_copy_file
(
    void
)
{
    struct d_test_object* group;
    char                  src_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  dst_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char*                 content;
    size_t                size;
    int                   result;
    bool                  test_copy;
    bool                  test_content;
    bool                  test_nonexistent;
    size_t                idx;

    // setup paths
    d_tests_dfile_get_test_path(src_buf,
                               sizeof(src_buf),
                               D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(dst_buf,
                               sizeof(dst_buf),
                               "copy_dst.txt");

    // test 1: copy file
    result = d_copy_file(src_buf, dst_buf);
    test_copy = (result == 0) && d_file_exists(dst_buf);

    // test 2: verify content
    test_content = false;

    if (test_copy)
    {
        content = (char*)d_fread_all(dst_buf, &size);

        if (content)
        {
            test_content = (strcmp(content, D_TEST_DFILE_TEST_CONTENT) == 0);
            free(content);
        }
    }

    // cleanup
    d_remove(dst_buf);

    // test 3: nonexistent source
    result = d_copy_file("nonexistent_copy_source.txt", dst_buf);
    test_nonexistent = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_copy_file", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("copy",
                                           test_copy,
                                           "d_copy_file copies file");
    group->elements[idx++] = D_ASSERT_TRUE("content",
                                           test_content,
                                           "copied content matches original");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_copy_file fails for nonexistent source");

    return group;
}


/*
d_tests_dfile_file_operations_all
  Runs all file operation tests.
  Tests the following:
  - d_remove
  - d_unlink
  - d_rename
  - d_copy_file
*/
struct d_test_object*
d_tests_dfile_file_operations_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("XI. File Operations", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_remove();
    group->elements[idx++] = d_tests_dfile_unlink();
    group->elements[idx++] = d_tests_dfile_rename();
    group->elements[idx++] = d_tests_dfile_copy_file();

    return group;
}


