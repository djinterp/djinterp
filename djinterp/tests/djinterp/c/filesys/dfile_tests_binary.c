/*******************************************************************************
* djinterp [test]                                           dfile_tests_binary.c
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_binary.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dfile_tests.h"


/******************************************************************************
 * XV. BINARY I/O HELPERS TESTS
 *****************************************************************************/

/*
d_tests_dfile_fread_all
  Tests d_file_read_all for reading entire files.
  Tests the following:
  - reads entire file content
  - returns correct size
  - returns NULL for nonexistent file
  - returns NULL for NULL path
*/
bool d_tests_dfile_fread_all(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    void*                 content;
    size_t                size;
    bool                  test_read;
    bool                  test_size;
    bool                  test_content;
    bool                  test_nonexistent;
    bool                  test_null_path;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1, 2, 3: read file, check size, verify content
    content = d_file_read_all(path_buf, &size);
    test_read = (content != NULL);
    test_size = (size == strlen(D_TEST_DFILE_TEST_CONTENT));
    test_content = test_read && (strcmp((char*)content, D_TEST_DFILE_TEST_CONTENT) == 0);

    if (content)
    {
        free(content);
    }

    // test 4: nonexistent file
    content = d_file_read_all("nonexistent_fread_test.txt", &size);
    test_nonexistent = (content == NULL);

    // test 5: NULL path
    content = d_file_read_all(NULL, &size);
    test_null_path = (content == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_file_read_all");

    D_TEST_CHECK_MSG(_suite, "read", test_read, "d_file_read_all reads file");
    D_TEST_CHECK_MSG(_suite,
                     "size",
                     test_size,
                     "d_file_read_all returns correct size");
    D_TEST_CHECK_MSG(_suite,
                     "content",
                     test_content,
                     "d_file_read_all content is correct");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_read_all returns NULL for nonexistent");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_file_read_all returns NULL for NULL path");

    return true;
}


/*
d_tests_dfile_fwrite_all
  Tests d_file_write_all for writing entire files.
  Tests the following:
  - writes data to new file
  - overwrites existing file
  - returns error for NULL path
*/
bool d_tests_dfile_fwrite_all(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char*                 read_content;
    size_t                read_size;
    int                   result;
    bool                  test_write;
    bool                  test_content;
    bool                  test_overwrite;
    bool                  test_null_path;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "fwrite_all_test.txt");

    // test 1 & 2: write to new file and verify
    result = d_file_write_all(path_buf, "test content", 12);
    test_write = (result == 0);

    read_content = (char*)d_file_read_all(path_buf, &read_size);
    test_content = (read_content != NULL) && (strcmp(read_content, "test content") == 0);

    if (read_content)
    {
        free(read_content);
    }

    // test 3: overwrite existing
    result = d_file_write_all(path_buf, "new", 3);
    read_content = (char*)d_file_read_all(path_buf, &read_size);
    test_overwrite = (result == 0) &&
                     (read_content != NULL) &&
                     (strcmp(read_content, "new") == 0);

    if (read_content)
    {
        free(read_content);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 4: NULL path
    result = d_file_write_all(NULL, "data", 4);
    test_null_path = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_write_all");

    D_TEST_CHECK_MSG(_suite,
                     "write",
                     test_write,
                     "d_file_write_all writes file");
    D_TEST_CHECK_MSG(_suite,
                     "content",
                     test_content,
                     "written content is correct");
    D_TEST_CHECK_MSG(_suite,
                     "overwrite",
                     test_overwrite,
                     "d_file_write_all overwrites existing");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_file_write_all returns error for NULL");

    return true;
}


/*
d_tests_dfile_fappend_all
  Tests d_file_append_all for appending to files.
  Tests the following:
  - appends to existing file
  - creates new file if not exists
  - returns error for NULL path
*/
bool d_tests_dfile_fappend_all(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char*                 read_content;
    size_t                read_size;
    int                   result;
    bool                  test_append;
    bool                  test_content;
    bool                  test_null_path;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "fappend_all_test.txt");

    // write initial content
    d_file_write_all(path_buf, "Hello", 5);

    // test 1 & 2: append and verify
    result = d_file_append_all(path_buf, " World", 6);
    test_append = (result == 0);

    read_content = (char*)d_file_read_all(path_buf, &read_size);
    test_content = (read_content != NULL) && (strcmp(read_content, "Hello World") == 0);

    if (read_content)
    {
        free(read_content);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 3: NULL path
    result = d_file_append_all(NULL, "data", 4);
    test_null_path = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_append_all");

    D_TEST_CHECK_MSG(_suite,
                     "append",
                     test_append,
                     "d_file_append_all appends to file");
    D_TEST_CHECK_MSG(_suite,
                     "content",
                     test_content,
                     "appended content is correct");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_file_append_all returns error for NULL");

    return true;
}


/*
d_tests_dfile_binary_io_all
  Runs all binary I/O helper tests.
  Tests the following:
  - d_file_read_all
  - d_file_write_all
  - d_file_append_all
*/
bool d_tests_dfile_binary_io_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "XV. Binary I/O Helpers");

    d_tests_dfile_fread_all(_suite);
    d_tests_dfile_fwrite_all(_suite);
    d_tests_dfile_fappend_all(_suite);

    return true;
}
