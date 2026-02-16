/******************************************************************************
* djinterp [test]                                         dfile_tests_sa_sync.c
*
*   Tests for file synchronization operations (fsync, fflush).
*
*
* path:      \src	est\dfile_tests_sa_sync.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"


/******************************************************************************
 * VI. FILE SYNCHRONIZATION TESTS
 *****************************************************************************/

/*
d_tests_dfile_fsync
  Tests d_fsync for synchronizing files to disk.
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
    fd = d_open(path_buf, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    test_valid_fd = false;

    if (fd >= 0)
    {
        d_write(fd, "test", 4);
        result = d_fsync(fd);
        test_valid_fd = (result == 0);
        d_close(fd);
    }

    // cleanup
    d_remove(path_buf);

    // test 2: invalid fd
    result = d_fsync(-1);
    test_invalid_fd = (result == -1);

    // build result tree
    group = d_test_object_new_interior("d_fsync", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_fd",
                                           test_valid_fd,
                                           "d_fsync returns 0 for valid fd");
    group->elements[idx++] = D_ASSERT_TRUE("invalid_fd",
                                           test_invalid_fd,
                                           "d_fsync returns -1 for invalid fd");

    return group;
}


/*
d_tests_dfile_fsync_stream
  Tests d_fsync_stream for stream synchronization.
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
    file = d_fopen(path_buf, "w");
    test_valid_stream = false;

    if (file)
    {
        fprintf(file, "test data");
        result = d_fsync_stream(file);
        test_valid_stream = (result == 0);
        fclose(file);
    }

    // cleanup
    d_remove(path_buf);

    // test 2: NULL stream
    result = d_fsync_stream(NULL);
    test_null_stream = (result == -1);

    // build result tree
    group = d_test_object_new_interior("d_fsync_stream", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_stream",
                                           test_valid_stream,
                                           "d_fsync_stream returns 0 for valid stream");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_fsync_stream returns -1 for NULL");

    return group;
}


/*
d_tests_dfile_fflush
  Tests d_fflush for flushing streams.
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
    file = d_fopen(path_buf, "w");
    test_valid_stream = false;

    if (file)
    {
        fprintf(file, "test data");
        result = d_fflush(file);
        test_valid_stream = (result == 0);
        fclose(file);
    }

    // cleanup
    d_remove(path_buf);

    // test 2: NULL stream (flushes all streams)
    result = d_fflush(NULL);
    test_null_stream = (result == 0);

    // build result tree
    group = d_test_object_new_interior("d_fflush", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_stream",
                                           test_valid_stream,
                                           "d_fflush returns 0 for valid stream");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_fflush(NULL) flushes all streams");

    return group;
}


/*
d_tests_dfile_synchronization_all
  Runs all file synchronization tests.
  Tests the following:
  - d_fsync
  - d_fsync_stream
  - d_fflush
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


