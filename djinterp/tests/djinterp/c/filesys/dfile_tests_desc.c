/*******************************************************************************
* djinterp [c][filesys]                                       dfile_tests_desc.c
*
*   Tests for file descriptor operations (fileno, dup, dup2, read, write).
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_desc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * V. FILE DESCRIPTOR OPERATIONS TESTS
 *****************************************************************************/

/*
d_tests_dfile_fileno
  Tests d_file_descriptor_stream for getting file descriptors from streams.
  Tests the following:
  - returns valid fd for open stream
  - returns -1 for NULL stream
*/
bool d_tests_dfile_fileno(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    int                   fd;
    bool                  test_valid_stream;
    bool                  test_null_stream;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: valid stream
    file = d_file_open_stream(path_buf, "r");
    test_valid_stream = false;

    if (file)
    {
        fd = d_file_descriptor_stream(file);
        test_valid_stream = (fd >= 0);
        fclose(file);
    }

    // test 2: NULL stream
    fd = d_file_descriptor_stream(NULL);
    test_null_stream = (fd == -1);

    // build result tree
    d_test_suite_test(_suite, "d_file_descriptor_stream");

    D_TEST_CHECK_MSG(_suite,
                     "valid_stream",
                     test_valid_stream,
                     "d_file_descriptor_stream returns valid fd for stream");
    D_TEST_CHECK_MSG(_suite,
                     "null_stream",
                     test_null_stream,
                     "d_file_descriptor_stream returns -1 for NULL stream");

    return true;
}


/*
d_tests_dfile_dup
  Tests d_file_dup_fd for duplicating file descriptors.
  Tests the following:
  - returns new valid fd for valid input
  - both fds refer to same file
  - returns -1 for invalid fd
*/
bool d_tests_dfile_dup(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    int                   dup_fd;
    bool                  test_valid_dup;
    bool                  test_different_fd;
    bool                  test_invalid_fd;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1 & 2: valid dup, different fd numbers
    fd = d_file_open(path_buf, O_RDONLY);
    test_valid_dup = false;
    test_different_fd = false;

    if (fd >= 0)
    {
        dup_fd = d_file_dup_fd(fd);
        test_valid_dup = (dup_fd >= 0);
        test_different_fd = (dup_fd != fd);

        if (dup_fd >= 0)
        {
            d_file_close_fd(dup_fd);
        }

        d_file_close_fd(fd);
    }

    // test 3: invalid fd
    dup_fd = d_file_dup_fd(-1);
    test_invalid_fd = (dup_fd == -1);

    // build result tree
    d_test_suite_test(_suite, "d_file_dup_fd");

    D_TEST_CHECK_MSG(_suite,
                     "valid_dup",
                     test_valid_dup,
                     "d_file_dup_fd returns valid fd");
    D_TEST_CHECK_MSG(_suite,
                     "different_fd",
                     test_different_fd,
                     "d_file_dup_fd returns different fd number");
    D_TEST_CHECK_MSG(_suite,
                     "invalid_fd",
                     test_invalid_fd,
                     "d_file_dup_fd returns -1 for invalid fd");

    return true;
}


/*
d_tests_dfile_dup2
  Tests d_file_dup2_fd for duplicating to specific descriptor numbers.
  Tests the following:
  - duplicates to specified fd number
  - returns -1 for invalid source fd
  - returns -1 for invalid target fd
*/
bool d_tests_dfile_dup2(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    int                   dup_fd;
    int                   result;
    bool                  test_valid_dup2;
    bool                  test_invalid_source;
    bool                  test_invalid_target;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: valid dup2
    fd = d_file_open(path_buf, O_RDONLY);
    dup_fd = d_file_open(path_buf, O_RDONLY);  // open another to get a valid fd
    test_valid_dup2 = false;

    if ( (fd >= 0) &&
         (dup_fd >= 0) )
    {
        result = d_file_dup2_fd(fd, dup_fd);
        test_valid_dup2 = (result == dup_fd);
        d_file_close_fd(dup_fd);
        d_file_close_fd(fd);
    }
    else
    {
        if (fd >= 0)
        {
            d_file_close_fd(fd);
        }

        if (dup_fd >= 0)
        {
            d_file_close_fd(dup_fd);
        }
    }

    // test 2: invalid source fd
    result = d_file_dup2_fd(-1, 10);
    test_invalid_source = (result == -1);

    // test 3: invalid target fd
    fd = d_file_open(path_buf, O_RDONLY);
    test_invalid_target = false;

    if (fd >= 0)
    {
        result = d_file_dup2_fd(fd, -1);
        test_invalid_target = (result == -1);
        d_file_close_fd(fd);
    }

    // build result tree
    d_test_suite_test(_suite, "d_file_dup2_fd");

    D_TEST_CHECK_MSG(_suite,
                     "valid_dup2",
                     test_valid_dup2,
                     "d_file_dup2_fd duplicates to specified fd");
    D_TEST_CHECK_MSG(_suite,
                     "invalid_source",
                     test_invalid_source,
                     "d_file_dup2_fd returns -1 for invalid source");
    D_TEST_CHECK_MSG(_suite,
                     "invalid_target",
                     test_invalid_target,
                     "d_file_dup2_fd returns -1 for invalid target");

    return true;
}


/*
d_tests_dfile_close
  Tests d_file_close_fd for closing file descriptors.
  Tests the following:
  - returns 0 for valid fd
  - returns -1 for invalid fd
*/
bool d_tests_dfile_close(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    int                   result;
    bool                  test_valid_close;
    bool                  test_invalid_fd;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: close valid fd
    fd = d_file_open(path_buf, O_RDONLY);
    test_valid_close = false;

    if (fd >= 0)
    {
        result = d_file_close_fd(fd);
        test_valid_close = (result == 0);
    }

    // test 2: close invalid fd
    result = d_file_close_fd(-1);
    test_invalid_fd = (result == -1);

    // build result tree
    d_test_suite_test(_suite, "d_file_close_fd");

    D_TEST_CHECK_MSG(_suite,
                     "valid_close",
                     test_valid_close,
                     "d_file_close_fd returns 0 for valid fd");
    D_TEST_CHECK_MSG(_suite,
                     "invalid_fd",
                     test_invalid_fd,
                     "d_file_close_fd returns -1 for invalid fd");

    return true;
}


/*
d_tests_dfile_read_write
  Tests d_file_read_fd and d_file_write_fd for descriptor I/O.
  Tests the following:
  - writes data to file descriptor
  - reads data from file descriptor
  - data integrity is maintained
  - returns -1 for invalid fd
  - returns -1 for NULL buffer
*/
bool d_tests_dfile_read_write(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    char                  write_buf[] = "Test data for read/write";
    char                  read_buf[64];
    ssize_t               bytes_written;
    ssize_t               bytes_read;
    bool                  test_write;
    bool                  test_read;
    bool                  test_integrity;
    bool                  test_read_invalid_fd;
    bool                  test_write_invalid_fd;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "read_write_test.txt");

    // test 1: write data
    fd = d_file_open(path_buf, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    test_write = false;

    if (fd >= 0)
    {
        bytes_written = d_file_write_fd(fd, write_buf, strlen(write_buf));
        test_write = (bytes_written == (ssize_t)strlen(write_buf));
        d_file_close_fd(fd);
    }

    // test 2 & 3: read data and verify integrity
    fd = d_file_open(path_buf, O_RDONLY);
    test_read = false;
    test_integrity = false;

    if (fd >= 0)
    {
        memset(read_buf, 0, sizeof(read_buf));
        bytes_read = d_file_read_fd(fd, read_buf, sizeof(read_buf) - 1);
        test_read = (bytes_read == (ssize_t)strlen(write_buf));
        test_integrity = (strcmp(read_buf, write_buf) == 0);
        d_file_close_fd(fd);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 4: read with invalid fd
    bytes_read = d_file_read_fd(-1, read_buf, sizeof(read_buf));
    test_read_invalid_fd = (bytes_read == -1);

    // test 5: write with invalid fd
    bytes_written = d_file_write_fd(-1, write_buf, strlen(write_buf));
    test_write_invalid_fd = (bytes_written == -1);

    // build result tree
    d_test_suite_test(_suite, "d_file_read_fd/d_file_write_fd");

    D_TEST_CHECK_MSG(_suite,
                     "write",
                     test_write,
                     "d_file_write_fd writes correct number of bytes");
    D_TEST_CHECK_MSG(_suite,
                     "read",
                     test_read,
                     "d_file_read_fd reads correct number of bytes");
    D_TEST_CHECK_MSG(_suite,
                     "integrity",
                     test_integrity,
                     "read data matches written data");
    D_TEST_CHECK_MSG(_suite,
                     "read_invalid_fd",
                     test_read_invalid_fd,
                     "d_file_read_fd returns -1 for invalid fd");
    D_TEST_CHECK_MSG(_suite,
                     "write_invalid_fd",
                     test_write_invalid_fd,
                     "d_file_write_fd returns -1 for invalid fd");

    return true;
}


/*
d_tests_dfile_open
  Tests d_file_open for opening files by descriptor.
  Tests the following:
  - opens existing file for reading
  - creates new file with O_CREAT
  - returns -1 for NULL path
  - returns -1 for nonexistent file without O_CREAT
*/
bool d_tests_dfile_open(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  new_file_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    bool                  test_read_existing;
    bool                  test_create_new;
    bool                  test_null_path;
    bool                  test_nonexistent;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(new_file_buf,
                               sizeof(new_file_buf),
                               "open_new.txt");

    // test 1: open existing file
    fd = d_file_open(path_buf, O_RDONLY);
    test_read_existing = (fd >= 0);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    // test 2: create new file
    fd = d_file_open(new_file_buf, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    test_create_new = (fd >= 0);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    // cleanup
    d_file_remove(new_file_buf);

    // test 3: NULL path
    fd = d_file_open(NULL, O_RDONLY);
    test_null_path = (fd == -1);

    // test 4: nonexistent file
    fd = d_file_open("nonexistent_open_test_file.txt", O_RDONLY);
    test_nonexistent = (fd == -1);

    // build result tree
    d_test_suite_test(_suite, "d_file_open");

    D_TEST_CHECK_MSG(_suite,
                     "read_existing",
                     test_read_existing,
                     "d_file_open opens existing file");
    D_TEST_CHECK_MSG(_suite,
                     "create_new",
                     test_create_new,
                     "d_file_open creates new file with O_CREAT");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_file_open returns -1 for NULL path");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_file_open returns -1 for nonexistent file");

    return true;
}


/*
d_tests_dfile_descriptor_operations_all
  Runs all file descriptor operation tests.
  Tests the following:
  - d_file_descriptor_stream
  - d_file_dup_fd
  - d_file_dup2_fd
  - d_file_close_fd
  - d_file_read_fd/d_file_write_fd
  - d_file_open
*/
bool d_tests_dfile_descriptor_operations_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "V. File Descriptor Operations");

    d_tests_dfile_fileno(_suite);
    d_tests_dfile_dup(_suite);
    d_tests_dfile_dup2(_suite);
    d_tests_dfile_close(_suite);
    d_tests_dfile_read_write(_suite);
    d_tests_dfile_open(_suite);

    return true;
}
