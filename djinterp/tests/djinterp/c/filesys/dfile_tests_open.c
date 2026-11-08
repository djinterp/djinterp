/*******************************************************************************
* djinterp [test]                                             dfile_tests_open.c
*
*   Tests for secure file opening operations (fopen, fopen_s, freopen, fdopen).
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_open.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * III. SECURE FILE OPENING TESTS
 *****************************************************************************/

/*
d_tests_dfile_fopen
  Tests d_file_open_stream for basic file opening operations.
  Tests the following:
  - opening existing file for reading
  - opening file for writing (creates new)
  - opening file for appending
  - returns NULL for invalid mode
  - returns NULL for NULL filename
  - returns NULL for nonexistent file in read mode
*/
bool d_tests_dfile_fopen(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  new_file_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    bool                  test_read_existing;
    bool                  test_write_new;
    bool                  test_append;
    bool                  test_null_filename;
    bool                  test_null_mode;
    bool                  test_nonexistent;

    // setup: get path to test file
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(new_file_buf,
                               sizeof(new_file_buf),
                               "fopen_new.txt");

    // test 1: open existing file for reading
    file = d_file_open_stream(path_buf, "r");
    test_read_existing = (file != NULL);

    if (file)
    {
        fclose(file);
    }

    // test 2: open new file for writing
    file = d_file_open_stream(new_file_buf, "w");
    test_write_new = (file != NULL);

    if (file)
    {
        fclose(file);
    }

    // test 3: open file for appending
    file = d_file_open_stream(new_file_buf, "a");
    test_append = (file != NULL);

    if (file)
    {
        fclose(file);
    }

    // cleanup new file
    d_file_remove(new_file_buf);

    // test 4: NULL filename should return NULL
    file = d_file_open_stream(NULL, "r");
    test_null_filename = (file == NULL);

    // test 5: NULL mode should return NULL
    file = d_file_open_stream(path_buf, NULL);
    test_null_mode = (file == NULL);

    // test 6: nonexistent file in read mode should return NULL
    file = d_file_open_stream("this_file_does_not_exist_12345.txt", "r");
    test_nonexistent = (file == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_file_open_stream");

    D_TEST_CHECK_MSG(_suite,
                     "read_existing",
                     test_read_existing,
                     "d_file_open_stream opens existing file for reading");
    D_TEST_CHECK_MSG(_suite,
                     "write_new",
                     test_write_new,
                     "d_file_open_stream creates new file for writing");
    D_TEST_CHECK_MSG(_suite,
                     "append",
                     test_append,
                     "d_file_open_stream opens file for appending");
    D_TEST_CHECK_MSG(_suite,
                     "null_filename",
                     test_null_filename,
                     "d_file_open_stream returns NULL for NULL filename");
    D_TEST_CHECK_MSG(_suite,
                     "null_mode",
                     test_null_mode,
                     "d_file_open_stream returns NULL for NULL mode");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_open_stream returns NULL for nonexistent file");

    return true;
}


/*
d_tests_dfile_fopen_s
  Tests d_file_open_stream_s for secure file opening with error codes.
  Tests the following:
  - returns 0 and valid stream for existing file
  - returns 0 and valid stream for new file creation
  - returns EINVAL for NULL stream pointer
  - returns EINVAL for NULL filename
  - returns EINVAL for NULL mode
  - returns non-zero for nonexistent file in read mode
*/
bool d_tests_dfile_fopen_s(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  new_file_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 stream;
    int                   result;
    bool                  test_existing;
    bool                  test_new_file;
    bool                  test_null_stream;
    bool                  test_null_filename;
    bool                  test_null_mode;
    bool                  test_nonexistent;

    // setup: get path to test file
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(new_file_buf,
                               sizeof(new_file_buf),
                               "fopen_s_new.txt");

    // test 1: open existing file
    stream = NULL;
    result = d_file_open_stream_s(&stream, path_buf, "r");
    test_existing = (result == 0) && (stream != NULL);

    if (stream)
    {
        fclose(stream);
    }

    // test 2: create new file
    stream = NULL;
    result = d_file_open_stream_s(&stream, new_file_buf, "w");
    test_new_file = (result == 0) && (stream != NULL);

    if (stream)
    {
        fclose(stream);
    }

    // cleanup
    d_file_remove(new_file_buf);

    // test 3: NULL stream pointer
    result = d_file_open_stream_s(NULL, path_buf, "r");
    test_null_stream = (result == EINVAL);

    // test 4: NULL filename
    stream = NULL;
    result = d_file_open_stream_s(&stream, NULL, "r");
    test_null_filename = (result == EINVAL);

    // test 5: NULL mode
    stream = NULL;
    result = d_file_open_stream_s(&stream, path_buf, NULL);
    test_null_mode = (result == EINVAL);

    // test 6: nonexistent file
    stream = NULL;
    result = d_file_open_stream_s(&stream, "nonexistent_file_98765.txt", "r");
    test_nonexistent = (result != 0) && (stream == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_file_open_stream_s");

    D_TEST_CHECK_MSG(_suite,
                     "existing",
                     test_existing,
                     "d_file_open_stream_s returns 0 for existing file");
    D_TEST_CHECK_MSG(_suite,
                     "new_file",
                     test_new_file,
                     "d_file_open_stream_s creates new file successfully");
    D_TEST_CHECK_MSG(_suite,
                     "null_stream",
                     test_null_stream,
                     "d_file_open_stream_s returns EINVAL for NULL stream");
    D_TEST_CHECK_MSG(_suite,
                     "null_filename",
                     test_null_filename,
                     "d_file_open_stream_s returns EINVAL for NULL filename");
    D_TEST_CHECK_MSG(_suite,
                     "null_mode",
                     test_null_mode,
                     "d_file_open_stream_s returns EINVAL for NULL mode");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_open_stream_s returns error for nonexistent file");

    return true;
}


/*
d_tests_dfile_freopen
  Tests d_file_reopen_stream for file reopening operations.
  Tests the following:
  - reopens file with different mode
  - returns NULL for NULL mode
  - returns NULL for NULL stream
*/
bool d_tests_dfile_freopen(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    FILE*                 reopened;
    bool                  test_reopen;
    bool                  test_null_mode;
    bool                  test_null_stream;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: reopen file with different mode
    file = d_file_open_stream(path_buf, "r");
    test_reopen = false;

    if (file)
    {
        reopened = d_file_reopen_stream(path_buf, "r+", file);
        test_reopen = (reopened != NULL);

        if (reopened)
        {
            fclose(reopened);
        }
        else
        {
            fclose(file);
        }
    }

    // test 2: NULL mode should return NULL
    file = d_file_open_stream(path_buf, "r");
    test_null_mode = false;

    if (file)
    {
        reopened = d_file_reopen_stream(path_buf, NULL, file);
        test_null_mode = (reopened == NULL);

        // close original if reopen failed
        if (!reopened)
        {
            fclose(file);
        }
        else
        {
            fclose(reopened);
        }
    }

    // test 3: NULL stream should return NULL
    reopened = d_file_reopen_stream(path_buf, "r", NULL);
    test_null_stream = (reopened == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_file_reopen_stream");

    D_TEST_CHECK_MSG(_suite,
                     "reopen",
                     test_reopen,
                     "d_file_reopen_stream reopens file with new mode");
    D_TEST_CHECK_MSG(_suite,
                     "null_mode",
                     test_null_mode,
                     "d_file_reopen_stream returns NULL for NULL mode");
    D_TEST_CHECK_MSG(_suite,
                     "null_stream",
                     test_null_stream,
                     "d_file_reopen_stream returns NULL for NULL stream");

    return true;
}


/*
d_tests_dfile_freopen_s
  Tests d_file_reopen_stream_s for secure file reopening.
  Tests the following:
  - returns 0 and valid stream for successful reopen
  - returns EINVAL for NULL newstream pointer
  - returns EINVAL for NULL mode
  - returns EINVAL for NULL stream
*/
bool d_tests_dfile_freopen_s(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    FILE*                 newstream;
    int                   result;
    bool                  test_reopen;
    bool                  test_null_newstream;
    bool                  test_null_mode;
    bool                  test_null_stream;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: successful reopen
    file = d_file_open_stream(path_buf, "r");
    test_reopen = false;

    if (file)
    {
        newstream = NULL;
        result = d_file_reopen_stream_s(&newstream, path_buf, "r+", file);
        test_reopen = (result == 0) && (newstream != NULL);

        if (newstream)
        {
            fclose(newstream);
        }
    }

    // test 2: NULL newstream pointer
    file = d_file_open_stream(path_buf, "r");
    test_null_newstream = false;

    if (file)
    {
        result = d_file_reopen_stream_s(NULL, path_buf, "r", file);
        test_null_newstream = (result == EINVAL);
        fclose(file);
    }

    // test 3: NULL mode
    file = d_file_open_stream(path_buf, "r");
    test_null_mode = false;

    if (file)
    {
        newstream = NULL;
        result = d_file_reopen_stream_s(&newstream, path_buf, NULL, file);
        test_null_mode = (result == EINVAL);
        fclose(file);
    }

    // test 4: NULL stream
    newstream = NULL;
    result = d_file_reopen_stream_s(&newstream, path_buf, "r", NULL);
    test_null_stream = (result == EINVAL);

    // build result tree
    d_test_suite_test(_suite, "d_file_reopen_stream_s");

    D_TEST_CHECK_MSG(_suite,
                     "reopen",
                     test_reopen,
                     "d_file_reopen_stream_s returns 0 for successful reopen");
    D_TEST_CHECK_MSG(_suite,
                     "null_newstream",
                     test_null_newstream,
                     "d_file_reopen_stream_s returns EINVAL for NULL newstream");
    D_TEST_CHECK_MSG(_suite,
                     "null_mode",
                     test_null_mode,
                     "d_file_reopen_stream_s returns EINVAL for NULL mode");
    D_TEST_CHECK_MSG(_suite,
                     "null_stream",
                     test_null_stream,
                     "d_file_reopen_stream_s returns EINVAL for NULL stream");

    return true;
}


/*
d_tests_dfile_fdopen
  Tests d_file_open_stream_fd for associating streams with file descriptors.
  Tests the following:
  - associates stream with valid file descriptor
  - returns NULL for invalid file descriptor
  - returns NULL for NULL mode
*/
bool d_tests_dfile_fdopen(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    FILE*                 stream;
    bool                  test_valid_fd;
    bool                  test_invalid_fd;
    bool                  test_null_mode;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: associate stream with valid fd
    fd = d_file_open(path_buf, O_RDONLY);
    test_valid_fd = false;

    if (fd >= 0)
    {
        stream = d_file_open_stream_fd(fd, "r");
        test_valid_fd = (stream != NULL);

        if (stream)
        {
            // fclose will close the fd
            fclose(stream);
        }
        else
        {
            d_file_close_fd(fd);
        }
    }

    // test 2: invalid fd should return NULL
    stream = d_file_open_stream_fd(-1, "r");
    test_invalid_fd = (stream == NULL);

    // test 3: NULL mode should return NULL
    fd = d_file_open(path_buf, O_RDONLY);
    test_null_mode = false;

    if (fd >= 0)
    {
        stream = d_file_open_stream_fd(fd, NULL);
        test_null_mode = (stream == NULL);
        d_file_close_fd(fd);
    }

    // build result tree
    d_test_suite_test(_suite, "d_file_open_stream_fd");

    D_TEST_CHECK_MSG(_suite,
                     "valid_fd",
                     test_valid_fd,
                     "d_file_open_stream_fd associates stream with valid fd");
    D_TEST_CHECK_MSG(_suite,
                     "invalid_fd",
                     test_invalid_fd,
                     "d_file_open_stream_fd returns NULL for invalid fd");
    D_TEST_CHECK_MSG(_suite,
                     "null_mode",
                     test_null_mode,
                     "d_file_open_stream_fd returns NULL for NULL mode");

    return true;
}


/*
d_tests_dfile_secure_file_opening_all
  Runs all secure file opening tests.
  Tests the following:
  - d_file_open_stream
  - d_file_open_stream_s
  - d_file_reopen_stream
  - d_file_reopen_stream_s
  - d_file_open_stream_fd
*/
bool d_tests_dfile_secure_file_opening_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "III. Secure File Opening");

    d_tests_dfile_fopen(_suite);
    d_tests_dfile_fopen_s(_suite);
    d_tests_dfile_freopen(_suite);
    d_tests_dfile_freopen_s(_suite);
    d_tests_dfile_fdopen(_suite);

    return true;
}
