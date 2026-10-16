/*******************************************************************************
* djinterp [test]                                          dfile_tests_sa_sync.c
*
*   Tests for file synchronization operations (fsync, fflush).
*
*
* path:      /tests/djinterp/c/dfile_tests_sa_sync.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./dfile_tests_sa.h"


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
struct d_test_object*
d_tests_dfile_fsync
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    int                   result;
    bool                  test_valid_fd;
    bool                  test_invalid_fd;
    size_t                idx;

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
    group = d_test_object_new_interior("d_file_sync_fd", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_fd",
                                           test_valid_fd,
                                           "d_file_sync_fd returns 0 for valid fd");
    group->elements[idx++] = D_ASSERT_TRUE("invalid_fd",
                                           test_invalid_fd,
                                           "d_file_sync_fd returns -1 for invalid fd");

    return group;
}


/*
d_tests_dfile_fsync_stream
  Tests d_file_sync_stream for stream synchronization.
  Tests the following:
  - returns 0 for valid stream
  - returns -1 for NULL stream
*/
struct d_test_object*
d_tests_dfile_fsync_stream
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    int                   result;
    bool                  test_valid_stream;
    bool                  test_null_stream;
    size_t                idx;

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
    group = d_test_object_new_interior("d_file_sync_stream", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_stream",
                                           test_valid_stream,
                                           "d_file_sync_stream returns 0 for valid stream");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_file_sync_stream returns -1 for NULL");

    return group;
}


/*
d_tests_dfile_fflush
  Tests d_file_flush_stream for flushing streams.
  Tests the following:
  - returns 0 for valid stream
  - handles NULL stream (flushes all)
*/
struct d_test_object*
d_tests_dfile_fflush
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    int                   result;
    bool                  test_valid_stream;
    bool                  test_null_stream;
    size_t                idx;

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
    group = d_test_object_new_interior("d_file_flush_stream", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_stream",
                                           test_valid_stream,
                                           "d_file_flush_stream returns 0 for valid stream");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_file_flush_stream(NULL) flushes all streams");

    return group;
}


/*
d_tests_dfile_synchronization_all
  Runs all file synchronization tests.
  Tests the following:
  - d_file_sync_fd
  - d_file_sync_stream
  - d_file_flush_stream
*/
struct d_test_object*
d_tests_dfile_synchronization_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("VI. File Synchronization", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_fsync();
    group->elements[idx++] = d_tests_dfile_fsync_stream();
    group->elements[idx++] = d_tests_dfile_fflush();

    return group;
}
