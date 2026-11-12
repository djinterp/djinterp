/*******************************************************************************
* djinterp [c][filesys]                                       dfile_tests_temp.c
*
*   Tests for temporary file operations (tmpfile, mkstemp, tmpnam).
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_temp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * VIII. TEMPORARY FILE TESTS
 *****************************************************************************/

/*
d_tests_dfile_tmpfile
  Tests d_file_temp_stream for creating temporary files.
  Tests the following:
  - creates a temporary file stream
  - stream is readable and writable
*/
bool d_tests_dfile_tmpfile(struct dj_test_suite* _suite)
{
    FILE*                 tmp;
    int                   write_result;
    bool                  test_create;
    bool                  test_writable;

    // test 1: create temporary file
    tmp = d_file_temp_stream();
    test_create = (tmp != NULL);
    test_writable = false;

    if (tmp)
    {
        // test 2: file is writable
        write_result = fprintf(tmp, "test data");
        test_writable = (write_result > 0);
        fclose(tmp);
    }

    // build result tree
    d_test_suite_test(_suite, "d_file_temp_stream");

    D_TEST_CHECK_MSG(_suite,
                     "create",
                     test_create,
                     "d_file_temp_stream creates temporary file");
    D_TEST_CHECK_MSG(_suite,
                     "writable",
                     test_writable,
                     "temporary file is writable");

    return true;
}


/*
d_tests_dfile_tmpfile_s
  Tests d_file_temp_stream_s for secure temporary file creation.
  Tests the following:
  - returns 0 and valid stream on success
  - returns EINVAL for NULL stream pointer
*/
bool d_tests_dfile_tmpfile_s(struct dj_test_suite* _suite)
{
    FILE*                 stream;
    int                   result;
    bool                  test_create;
    bool                  test_null_ptr;

    // test 1: create temporary file
    stream = NULL;
    result = d_file_temp_stream_s(&stream);
    test_create = (result == 0) && (stream != NULL);

    if (stream)
    {
        fclose(stream);
    }

    // test 2: NULL stream pointer
    result = d_file_temp_stream_s(NULL);
    test_null_ptr = (result == EINVAL);

    // build result tree
    d_test_suite_test(_suite, "d_file_temp_stream_s");

    D_TEST_CHECK_MSG(_suite,
                     "create",
                     test_create,
                     "d_file_temp_stream_s creates temporary file");
    D_TEST_CHECK_MSG(_suite,
                     "null_ptr",
                     test_null_ptr,
                     "d_file_temp_stream_s returns EINVAL for NULL");

    return true;
}


/*
d_tests_dfile_mkstemp
  Tests d_file_temp_create for template-based temporary files.
  Tests the following:
  - creates file with modified template
  - returns valid fd
  - template is modified in place
*/
bool d_tests_dfile_mkstemp(struct dj_test_suite* _suite)
{
    char                  template_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  original[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    bool                  test_create;
    bool                  test_template_modified;

    // setup template
    d_tests_dfile_get_test_path(template_buf,
                               sizeof(template_buf),
                               "tmpXXXXXX");
    d_strcpy_s(original, sizeof(original), template_buf);

    // test 1 & 2: create file and check template modification
    fd = d_file_temp_create(template_buf);
    test_create = (fd >= 0);
    test_template_modified = (strcmp(template_buf, original) != 0);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
        d_file_remove(template_buf);
    }

    // build result tree
    d_test_suite_test(_suite, "d_file_temp_create");

    D_TEST_CHECK_MSG(_suite,
                     "create",
                     test_create,
                     "d_file_temp_create creates temporary file");
    D_TEST_CHECK_MSG(_suite,
                     "template_modified",
                     test_template_modified,
                     "d_file_temp_create modifies template");

    return true;
}


/*
d_tests_dfile_tmpnam_s
  Tests d_file_temp_name for generating temporary filenames.
  Tests the following:
  - returns 0 and generates name on success
  - returns error for NULL buffer
  - returns error for buffer too small
*/
bool d_tests_dfile_tmpnam_s(struct dj_test_suite* _suite)
{
    char                  name_buf[D_FILE_PATH_MAX];
    int                   result;
    bool                  test_generate;
    bool                  test_null_buf;
    bool                  test_small_buf;

    // test 1: generate temporary name
    memset(name_buf, 0, sizeof(name_buf));
    result = d_file_temp_name(name_buf, sizeof(name_buf));
    test_generate = (result == 0) && (strlen(name_buf) > 0);

    // test 2: NULL buffer
    result = d_file_temp_name(NULL, 100);
    test_null_buf = (result != 0);

    // test 3: buffer too small
    result = d_file_temp_name(name_buf, 1);
    test_small_buf = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_temp_name");

    D_TEST_CHECK_MSG(_suite,
                     "generate",
                     test_generate,
                     "d_file_temp_name generates filename");
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     test_null_buf,
                     "d_file_temp_name returns error for NULL");
    D_TEST_CHECK_MSG(_suite,
                     "small_buf",
                     test_small_buf,
                     "d_file_temp_name returns error for small buffer");

    return true;
}


/*
d_tests_dfile_tempdir
  Tests d_dir_temp for getting system temp directory.
  Tests the following:
  - returns valid temp directory path
  - returned path exists
  - returns NULL for NULL buffer
*/
bool d_tests_dfile_tempdir(struct dj_test_suite* _suite)
{
    char                  dir_buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_get_dir;
    bool                  test_dir_exists;
    bool                  test_null_buf;

    // test 1 & 2: get temp directory and verify it exists
    result = d_dir_temp(dir_buf, sizeof(dir_buf));
    test_get_dir = (result != NULL) && (strlen(dir_buf) > 0);
    test_dir_exists = test_get_dir && d_dir_exists(dir_buf);

    // test 3: NULL buffer
    result = d_dir_temp(NULL, 100);
    test_null_buf = (result == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_dir_temp");

    D_TEST_CHECK_MSG(_suite,
                     "get_dir",
                     test_get_dir,
                     "d_dir_temp returns temp directory");
    D_TEST_CHECK_MSG(_suite,
                     "dir_exists",
                     test_dir_exists,
                     "returned directory exists");
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     test_null_buf,
                     "d_dir_temp returns NULL for NULL buffer");

    return true;
}


/*
d_tests_dfile_temporary_files_all
  Runs all temporary file tests.
  Tests the following:
  - d_file_temp_stream
  - d_file_temp_stream_s
  - d_file_temp_create
  - d_file_temp_name
  - d_dir_temp
*/
bool d_tests_dfile_temporary_files_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "VIII. Temporary Files");

    d_tests_dfile_tmpfile(_suite);
    d_tests_dfile_tmpfile_s(_suite);
    d_tests_dfile_mkstemp(_suite);
    d_tests_dfile_tmpnam_s(_suite);
    d_tests_dfile_tempdir(_suite);

    return true;
}
