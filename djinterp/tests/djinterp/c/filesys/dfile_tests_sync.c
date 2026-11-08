/*******************************************************************************
* djinterp [test]                                             dfile_tests_sync.c
*
*   Tests for file synchronization operations (fsync, fflush).
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_sync.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * VI. FILE SYNCHRONIZATION TESTS
 *****************************************************************************/

/*
d_tests_dfile_fsync
  Tests d_file_sync_fd for synchronizing files to disk.
  Tests the following:
  - returns 0 for valid fd
  - returns -1 for invalid fd
*/
bool d_tests_dfile_fsync(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    int                   result;
    bool                  test_valid_fd;
    bool                  test_invalid_fd;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "fsync_test.txt");

    // test 1: fsync on valid fd
    fd = d_file_open(path_buf, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    test_valid_fd = false;

    if (fd >= 0)
    {
        d_file_write_fd(fd, "test", 4);
        result = d_file_sync_fd(fd);
        test_valid_fd = (result == 0);
        d_file_close_fd(fd);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 2: invalid fd
    result = d_file_sync_fd(-1);
    test_invalid_fd = (result == -1);

    // build result tree
    d_test_suite_test(_suite, "d_file_sync_fd");

    D_TEST_CHECK_MSG(_suite,
                     "valid_fd",
                     test_valid_fd,
                     "d_file_sync_fd returns 0 for valid fd");
    D_TEST_CHECK_MSG(_suite,
                     "invalid_fd",
                     test_invalid_fd,
                     "d_file_sync_fd returns -1 for invalid fd");

    return true;
}


/*
d_tests_dfile_fsync_stream
  Tests d_file_sync_stream for stream synchronization.
  Tests the following:
  - returns 0 for valid stream
  - returns -1 for NULL stream
*/
bool d_tests_dfile_fsync_stream(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    int                   result;
    bool                  test_valid_stream;
    bool                  test_null_stream;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "fsync_stream_test.txt");

    // test 1: fsync on valid stream
    file = d_file_open_stream(path_buf, "w");
    test_valid_stream = false;

    if (file)
    {
        fprintf(file, "test data");
        result = d_file_sync_stream(file);
        test_valid_stream = (result == 0);
        fclose(file);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 2: NULL stream
    result = d_file_sync_stream(NULL);
    test_null_stream = (result == -1);

    // build result tree
    d_test_suite_test(_suite, "d_file_sync_stream");

    D_TEST_CHECK_MSG(_suite,
                     "valid_stream",
                     test_valid_stream,
                     "d_file_sync_stream returns 0 for valid stream");
    D_TEST_CHECK_MSG(_suite,
                     "null_stream",
                     test_null_stream,
                     "d_file_sync_stream returns -1 for NULL");

    return true;
}


/*
d_tests_dfile_fflush
  Tests d_file_flush_stream for flushing streams.
  Tests the following:
  - returns 0 for valid stream
  - handles NULL stream (flushes all)
*/
bool d_tests_dfile_fflush(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    int                   result;
    bool                  test_valid_stream;
    bool                  test_null_stream;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "fflush_test.txt");

    // test 1: flush valid stream
    file = d_file_open_stream(path_buf, "w");
    test_valid_stream = false;

    if (file)
    {
        fprintf(file, "test data");
        result = d_file_flush_stream(file);
        test_valid_stream = (result == 0);
        fclose(file);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 2: NULL stream (flushes all streams)
    result = d_file_flush_stream(NULL);
    test_null_stream = (result == 0);

    // build result tree
    d_test_suite_test(_suite, "d_file_flush_stream");

    D_TEST_CHECK_MSG(_suite,
                     "valid_stream",
                     test_valid_stream,
                     "d_file_flush_stream returns 0 for valid stream");
    D_TEST_CHECK_MSG(_suite,
                     "null_stream",
                     test_null_stream,
                     "d_file_flush_stream(NULL) flushes all streams");

    return true;
}


/*
d_tests_dfile_synchronization_all
  Runs all file synchronization tests.
  Tests the following:
  - d_file_sync_fd
  - d_file_sync_stream
  - d_file_flush_stream
*/
bool d_tests_dfile_synchronization_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "VI. File Synchronization");

    d_tests_dfile_fsync(_suite);
    d_tests_dfile_fsync_stream(_suite);
    d_tests_dfile_fflush(_suite);

    return true;
}
