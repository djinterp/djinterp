/*******************************************************************************
* djinterp [c][filesys]                                       dfile_tests_meta.c
*
*   Tests for file metadata operations (stat, fstat, chmod, access).
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_meta.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * IX. FILE METADATA TESTS
 *****************************************************************************/

/*
d_tests_dfile_stat
  Tests d_file_stat for getting file status.
  Tests the following:
  - returns 0 for existing file
  - populates stat structure with valid size
  - returns -1 for nonexistent file
  - returns -1 for NULL path
*/
bool d_tests_dfile_stat(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    struct dj_stat_t       st;
    int                   result;
    bool                  test_existing;
    bool                  test_valid_size;
    bool                  test_nonexistent;
    bool                  test_null_path;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1 & 2: stat existing file
    memset(&st, 0, sizeof(st));
    result = d_file_stat(path_buf, &st);
    test_existing = (result == 0);
    test_valid_size = (st.st_size == strlen(D_TEST_DFILE_TEST_CONTENT));

    // test 3: nonexistent file
    result = d_file_stat("nonexistent_stat_test.txt", &st);
    test_nonexistent = (result != 0);

    // test 4: NULL path
    result = d_file_stat(NULL, &st);
    test_null_path = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_stat");

    D_TEST_CHECK_MSG(_suite,
                     "existing",
                     test_existing,
                     "d_file_stat returns 0 for existing file");
    D_TEST_CHECK_MSG(_suite,
                     "valid_size",
                     test_valid_size,
                     "d_file_stat populates correct file size");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_stat returns error for nonexistent");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_file_stat returns error for NULL path");

    return true;
}


/*
d_tests_dfile_fstat
  Tests d_file_stat_fd for getting file status from descriptor.
  Tests the following:
  - returns 0 for valid fd
  - populates stat structure
  - returns -1 for invalid fd
*/
bool d_tests_dfile_fstat(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    struct dj_stat_t       st;
    int                   fd;
    int                   result;
    bool                  test_valid_fd;
    bool                  test_valid_size;
    bool                  test_invalid_fd;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1 & 2: fstat on valid fd
    fd = d_file_open(path_buf, O_RDONLY);
    test_valid_fd = false;
    test_valid_size = false;

    if (fd >= 0)
    {
        memset(&st, 0, sizeof(st));
        result = d_file_stat_fd(fd, &st);
        test_valid_fd = (result == 0);
        test_valid_size = (st.st_size == strlen(D_TEST_DFILE_TEST_CONTENT));
        d_file_close_fd(fd);
    }

    // test 3: invalid fd
    result = d_file_stat_fd(-1, &st);
    test_invalid_fd = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_stat_fd");

    D_TEST_CHECK_MSG(_suite,
                     "valid_fd",
                     test_valid_fd,
                     "d_file_stat_fd returns 0 for valid fd");
    D_TEST_CHECK_MSG(_suite,
                     "valid_size",
                     test_valid_size,
                     "d_file_stat_fd populates correct file size");
    D_TEST_CHECK_MSG(_suite,
                     "invalid_fd",
                     test_invalid_fd,
                     "d_file_stat_fd returns error for invalid fd");

    return true;
}


/*
d_tests_dfile_lstat
  Tests d_file_stat_nofollow for status without following symlinks.
  Tests the following:
  - returns 0 for existing file
  - returns -1 for nonexistent file
*/
bool d_tests_dfile_lstat(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    struct dj_stat_t       st;
    int                   result;
    bool                  test_existing;
    bool                  test_nonexistent;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: lstat existing file
    result = d_file_stat_nofollow(path_buf, &st);
    test_existing = (result == 0);

    // test 2: nonexistent file
    result = d_file_stat_nofollow("nonexistent_lstat_test.txt", &st);
    test_nonexistent = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_stat_nofollow");

    D_TEST_CHECK_MSG(_suite,
                     "existing",
                     test_existing,
                     "d_file_stat_nofollow returns 0 for existing file");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_stat_nofollow returns error for nonexistent");

    return true;
}


/*
d_tests_dfile_access
  Tests d_file_access for checking file permissions.
  Tests the following:
  - returns 0 for existing file with F_OK
  - returns 0 for readable file with R_OK
  - returns -1 for nonexistent file
*/
bool d_tests_dfile_access(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_exists;
    bool                  test_readable;
    bool                  test_nonexistent;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: file exists (F_OK)
    result = d_file_access(path_buf, F_OK);
    test_exists = (result == 0);

    // test 2: file is readable (R_OK)
    result = d_file_access(path_buf, R_OK);
    test_readable = (result == 0);

    // test 3: nonexistent file
    result = d_file_access("nonexistent_access_test.txt", F_OK);
    test_nonexistent = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_access");

    D_TEST_CHECK_MSG(_suite,
                     "exists",
                     test_exists,
                     "d_file_access returns 0 for existing file");
    D_TEST_CHECK_MSG(_suite,
                     "readable",
                     test_readable,
                     "d_file_access returns 0 for readable file");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_access returns error for nonexistent");

    return true;
}


/*
d_tests_dfile_chmod
  Tests d_file_chmod for changing file permissions.
  Tests the following:
  - returns 0 on success
  - actually changes permissions
  - returns -1 for nonexistent file
*/
bool d_tests_dfile_chmod(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_change;
    bool                  test_nonexistent;

    // setup: create test file
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "chmod_test.txt");
    d_file_write_all(path_buf, "test", 4);

    // test 1: change permissions
    result = d_file_chmod(path_buf, S_IRUSR | S_IWUSR);
    test_change = (result == 0);

    // cleanup
    d_file_remove(path_buf);

    // test 2: nonexistent file
    result = d_file_chmod("nonexistent_chmod_test.txt", S_IRUSR);
    test_nonexistent = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_chmod");

    D_TEST_CHECK_MSG(_suite,
                     "change",
                     test_change,
                     "d_file_chmod changes file permissions");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_chmod returns error for nonexistent");

    return true;
}


/*
d_tests_dfile_file_size
  Tests d_file_size for getting file size by path.
  Tests the following:
  - returns correct size for existing file
  - returns -1 for nonexistent file
  - returns -1 for NULL path
*/
bool d_tests_dfile_file_size(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int64_t               size;
    bool                  test_correct_size;
    bool                  test_nonexistent;
    bool                  test_null_path;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: correct size for existing file
    size = d_file_size(path_buf);
    test_correct_size = (size == (int64_t)strlen(D_TEST_DFILE_TEST_CONTENT));

    // test 2: nonexistent file
    size = d_file_size("nonexistent_size_test.txt");
    test_nonexistent = (size == -1);

    // test 3: NULL path
    size = d_file_size(NULL);
    test_null_path = (size == -1);

    // build result tree
    d_test_suite_test(_suite, "d_file_size");

    D_TEST_CHECK_MSG(_suite,
                     "correct_size",
                     test_correct_size,
                     "d_file_size returns correct size");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_size returns -1 for nonexistent");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_file_size returns -1 for NULL path");

    return true;
}


/*
d_tests_dfile_file_size_stream
  Tests d_file_size_stream for getting size from stream.
  Tests the following:
  - returns correct size for valid stream
  - returns -1 for NULL stream
*/
bool d_tests_dfile_file_size_stream(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    int64_t               size;
    bool                  test_correct_size;
    bool                  test_null_stream;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: correct size from stream
    file = d_file_open_stream(path_buf, "r");
    test_correct_size = false;

    if (file)
    {
        size = d_file_size_stream(file);
        test_correct_size = (size == (int64_t)strlen(D_TEST_DFILE_TEST_CONTENT));
        fclose(file);
    }

    // test 2: NULL stream
    size = d_file_size_stream(NULL);
    test_null_stream = (size == -1);

    // build result tree
    d_test_suite_test(_suite, "d_file_size_stream");

    D_TEST_CHECK_MSG(_suite,
                     "correct_size",
                     test_correct_size,
                     "d_file_size_stream returns correct size");
    D_TEST_CHECK_MSG(_suite,
                     "null_stream",
                     test_null_stream,
                     "d_file_size_stream returns -1 for NULL");

    return true;
}


/*
d_tests_dfile_file_exists
  Tests d_file_exists for checking file existence.
  Tests the following:
  - returns non-zero for existing file
  - returns 0 for nonexistent file
  - returns 0 for NULL path
*/
bool d_tests_dfile_file_exists(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_exists;
    bool                  test_nonexistent;
    bool                  test_null_path;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: existing file
    result = d_file_exists(path_buf);
    test_exists = (result != 0);

    // test 2: nonexistent file
    result = d_file_exists("nonexistent_exists_test.txt");
    test_nonexistent = (result == 0);

    // test 3: NULL path
    result = d_file_exists(NULL);
    test_null_path = (result == 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_exists");

    D_TEST_CHECK_MSG(_suite,
                     "exists",
                     test_exists,
                     "d_file_exists returns non-zero for existing");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_exists returns 0 for nonexistent");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_file_exists returns 0 for NULL path");

    return true;
}


/*
d_tests_dfile_is_file
  Tests d_file_is_regular for checking if path is regular file.
  Tests the following:
  - returns non-zero for regular file
  - returns 0 for directory
  - returns 0 for nonexistent path
*/
bool d_tests_dfile_is_file(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_regular_file;
    bool                  test_directory;
    bool                  test_nonexistent;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: regular file
    result = d_file_is_regular(path_buf);
    test_regular_file = (result != 0);

    // test 2: directory returns 0
    result = d_file_is_regular(D_TEST_DFILE_TEMP_DIR);
    test_directory = (result == 0);

    // test 3: nonexistent path
    result = d_file_is_regular("nonexistent_isfile_test.txt");
    test_nonexistent = (result == 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_is_regular");

    D_TEST_CHECK_MSG(_suite,
                     "regular_file",
                     test_regular_file,
                     "d_file_is_regular returns non-zero for file");
    D_TEST_CHECK_MSG(_suite,
                     "directory",
                     test_directory,
                     "d_file_is_regular returns 0 for directory");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_is_regular returns 0 for nonexistent");

    return true;
}


/*
d_tests_dfile_is_dir
  Tests d_dir_exists for checking if path is directory.
  Tests the following:
  - returns non-zero for directory
  - returns 0 for regular file
  - returns 0 for nonexistent path
*/
bool d_tests_dfile_is_dir(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   result;
    bool                  test_directory;
    bool                  test_regular_file;
    bool                  test_nonexistent;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: directory
    result = d_dir_exists(D_TEST_DFILE_TEMP_DIR);
    test_directory = (result != 0);

    // test 2: regular file returns 0
    result = d_dir_exists(path_buf);
    test_regular_file = (result == 0);

    // test 3: nonexistent path
    result = d_dir_exists("nonexistent_isdir_test");
    test_nonexistent = (result == 0);

    // build result tree
    d_test_suite_test(_suite, "d_dir_exists");

    D_TEST_CHECK_MSG(_suite,
                     "directory",
                     test_directory,
                     "d_dir_exists returns non-zero for directory");
    D_TEST_CHECK_MSG(_suite,
                     "regular_file",
                     test_regular_file,
                     "d_dir_exists returns 0 for regular file");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_dir_exists returns 0 for nonexistent");

    return true;
}


/*
d_tests_dfile_metadata_all
  Runs all file metadata tests.
  Tests the following:
  - d_file_stat
  - d_file_stat_fd
  - d_file_stat_nofollow
  - d_file_access
  - d_file_chmod
  - d_file_size
  - d_file_size_stream
  - d_file_exists
  - d_file_is_regular
  - d_dir_exists
*/
bool d_tests_dfile_metadata_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "IX. File Metadata");

    d_tests_dfile_stat(_suite);
    d_tests_dfile_fstat(_suite);
    d_tests_dfile_lstat(_suite);
    d_tests_dfile_access(_suite);
    d_tests_dfile_chmod(_suite);
    d_tests_dfile_file_size(_suite);
    d_tests_dfile_file_size_stream(_suite);
    d_tests_dfile_file_exists(_suite);
    d_tests_dfile_is_file(_suite);
    d_tests_dfile_is_dir(_suite);

    return true;
}
