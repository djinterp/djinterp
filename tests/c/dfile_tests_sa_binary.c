#include "dfile_tests_sa.h"


/******************************************************************************
 * XV. BINARY I/O HELPERS TESTS
 *****************************************************************************/

/*
d_tests_dfile_fread_all
  Tests d_fread_all for reading entire files.
  Tests the following:
  - reads entire file content
  - returns correct size
  - returns NULL for nonexistent file
  - returns NULL for NULL path
*/
struct d_test_object*
d_tests_dfile_fread_all
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    void*                 content;
    size_t                size;
    bool                  test_read;
    bool                  test_size;
    bool                  test_content;
    bool                  test_nonexistent;
    bool                  test_null_path;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1, 2, 3: read file, check size, verify content
    content = d_fread_all(path_buf, &size);
    test_read = (content != NULL);
    test_size = (size == strlen(D_TEST_DFILE_TEST_CONTENT));
    test_content = test_read && (strcmp((char*)content, D_TEST_DFILE_TEST_CONTENT) == 0);

    if (content)
    {
        free(content);
    }

    // test 4: nonexistent file
    content = d_fread_all("nonexistent_fread_test.txt", &size);
    test_nonexistent = (content == NULL);

    // test 5: NULL path
    content = d_fread_all(NULL, &size);
    test_null_path = (content == NULL);

    // build result tree
    group = d_test_object_new_interior("d_fread_all", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("read",
                                           test_read,
                                           "d_fread_all reads file");
    group->elements[idx++] = D_ASSERT_TRUE("size",
                                           test_size,
                                           "d_fread_all returns correct size");
    group->elements[idx++] = D_ASSERT_TRUE("content",
                                           test_content,
                                           "d_fread_all content is correct");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_fread_all returns NULL for nonexistent");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_fread_all returns NULL for NULL path");

    return group;
}


/*
d_tests_dfile_fwrite_all
  Tests d_fwrite_all for writing entire files.
  Tests the following:
  - writes data to new file
  - overwrites existing file
  - returns error for NULL path
*/
struct d_test_object*
d_tests_dfile_fwrite_all
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char*                 read_content;
    size_t                read_size;
    int                   result;
    bool                  test_write;
    bool                  test_content;
    bool                  test_overwrite;
    bool                  test_null_path;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "fwrite_all_test.txt");

    // test 1 & 2: write to new file and verify
    result = d_fwrite_all(path_buf, "test content", 12);
    test_write = (result == 0);

    read_content = (char*)d_fread_all(path_buf, &read_size);
    test_content = (read_content != NULL) && (strcmp(read_content, "test content") == 0);

    if (read_content)
    {
        free(read_content);
    }

    // test 3: overwrite existing
    result = d_fwrite_all(path_buf, "new", 3);
    read_content = (char*)d_fread_all(path_buf, &read_size);
    test_overwrite = (result == 0) && 
                     (read_content != NULL) && 
                     (strcmp(read_content, "new") == 0);

    if (read_content)
    {
        free(read_content);
    }

    // cleanup
    d_remove(path_buf);

    // test 4: NULL path
    result = d_fwrite_all(NULL, "data", 4);
    test_null_path = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_fwrite_all", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("write",
                                           test_write,
                                           "d_fwrite_all writes file");
    group->elements[idx++] = D_ASSERT_TRUE("content",
                                           test_content,
                                           "written content is correct");
    group->elements[idx++] = D_ASSERT_TRUE("overwrite",
                                           test_overwrite,
                                           "d_fwrite_all overwrites existing");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_fwrite_all returns error for NULL");

    return group;
}


/*
d_tests_dfile_fappend_all
  Tests d_fappend_all for appending to files.
  Tests the following:
  - appends to existing file
  - creates new file if not exists
  - returns error for NULL path
*/
struct d_test_object*
d_tests_dfile_fappend_all
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char*                 read_content;
    size_t                read_size;
    int                   result;
    bool                  test_append;
    bool                  test_content;
    bool                  test_null_path;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "fappend_all_test.txt");

    // write initial content
    d_fwrite_all(path_buf, "Hello", 5);

    // test 1 & 2: append and verify
    result = d_fappend_all(path_buf, " World", 6);
    test_append = (result == 0);

    read_content = (char*)d_fread_all(path_buf, &read_size);
    test_content = (read_content != NULL) && (strcmp(read_content, "Hello World") == 0);

    if (read_content)
    {
        free(read_content);
    }

    // cleanup
    d_remove(path_buf);

    // test 3: NULL path
    result = d_fappend_all(NULL, "data", 4);
    test_null_path = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_fappend_all", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("append",
                                           test_append,
                                           "d_fappend_all appends to file");
    group->elements[idx++] = D_ASSERT_TRUE("content",
                                           test_content,
                                           "appended content is correct");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_fappend_all returns error for NULL");

    return group;
}


/*
d_tests_dfile_binary_io_all
  Runs all binary I/O helper tests.
  Tests the following:
  - d_fread_all
  - d_fwrite_all
  - d_fappend_all
*/
struct d_test_object*
d_tests_dfile_binary_io_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("XV. Binary I/O Helpers", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_fread_all();
    group->elements[idx++] = d_tests_dfile_fwrite_all();
    group->elements[idx++] = d_tests_dfile_fappend_all();

    return group;
}


