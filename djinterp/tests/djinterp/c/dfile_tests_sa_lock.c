/*******************************************************************************
* djinterp [test]                                          dfile_tests_sa_lock.c
*
*   Tests for file locking operations (flock).
*
*
* path:      /tests/djinterp/c/dfile_tests_sa_lock.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./dfile_tests_sa.h"


/******************************************************************************
 * VII. FILE LOCKING TESTS
 *****************************************************************************/

/*
d_tests_dfile_flock
  Tests d_file_lock_fd for advisory file locking.
  Tests the following:
  - acquires shared lock
  - acquires exclusive lock
  - releases lock
  - returns -1 for invalid fd
*/
struct d_test_object*
d_tests_dfile_flock
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    int                   result;
    bool                  test_shared_lock;
    bool                  test_exclusive_lock;
    bool                  test_unlock;
    bool                  test_invalid_fd;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "flock_test.txt");
    d_file_write_all(path_buf, "test", 4);

    fd = d_file_open(path_buf, O_RDWR);
    test_shared_lock = false;
    test_exclusive_lock = false;
    test_unlock = false;

    if (fd >= 0)
    {
        // test 1: shared lock
        result = d_file_lock_fd(fd, D_LOCK_SH);
        test_shared_lock = (result == 0);

        // test 2: unlock
        result = d_file_lock_fd(fd, D_LOCK_UN);
        test_unlock = (result == 0);

        // test 3: exclusive lock
        result = d_file_lock_fd(fd, D_LOCK_EX);
        test_exclusive_lock = (result == 0);

        // cleanup: unlock and close
        d_file_lock_fd(fd, D_LOCK_UN);
        d_file_close_fd(fd);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 4: invalid fd
    result = d_file_lock_fd(-1, D_LOCK_SH);
    test_invalid_fd = (result == -1);

    // build result tree
    group = d_test_object_new_interior("d_file_lock_fd", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("shared_lock",
                                           test_shared_lock,
                                           "d_file_lock_fd acquires shared lock");
    group->elements[idx++] = D_ASSERT_TRUE("exclusive_lock",
                                           test_exclusive_lock,
                                           "d_file_lock_fd acquires exclusive lock");
    group->elements[idx++] = D_ASSERT_TRUE("unlock",
                                           test_unlock,
                                           "d_file_lock_fd releases lock");
    group->elements[idx++] = D_ASSERT_TRUE("invalid_fd",
                                           test_invalid_fd,
                                           "d_file_lock_fd returns -1 for invalid fd");

    return group;
}


/*
d_tests_dfile_flock_stream
  Tests d_file_lock_stream for stream-based locking.
  Tests the following:
  - acquires lock via stream
  - returns -1 for NULL stream
*/
struct d_test_object*
d_tests_dfile_flock_stream
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    int                   result;
    bool                  test_lock_stream;
    bool                  test_null_stream;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "flock_stream_test.txt");
    d_file_write_all(path_buf, "test", 4);

    // test 1: lock via stream
    file = d_file_open_stream(path_buf, "r+");
    test_lock_stream = false;

    if (file)
    {
        result = d_file_lock_stream(file, D_LOCK_SH);
        test_lock_stream = (result == 0);
        d_file_lock_stream(file, D_LOCK_UN);
        fclose(file);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 2: NULL stream
    result = d_file_lock_stream(NULL, D_LOCK_SH);
    test_null_stream = (result == -1);

    // build result tree
    group = d_test_object_new_interior("d_file_lock_stream", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("lock_stream",
                                           test_lock_stream,
                                           "d_file_lock_stream acquires lock");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_file_lock_stream returns -1 for NULL");

    return group;
}


/*
d_tests_dfile_locking_all
  Runs all file locking tests.
  Tests the following:
  - d_file_lock_fd
  - d_file_lock_stream
*/
struct d_test_object*
d_tests_dfile_locking_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("VII. File Locking", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_flock();
    group->elements[idx++] = d_tests_dfile_flock_stream();

    return group;
}
