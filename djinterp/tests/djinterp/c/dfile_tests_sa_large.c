/*******************************************************************************
* djinterp [test]                                         dfile_tests_sa_large.c
*
*   Tests for large file support operations (fseeko, ftello, ftruncate).
*
*
* path:      /tests/djinterp/c/dfile_tests_sa_large.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./dfile_tests_sa.h"


/******************************************************************************
 * IV. LARGE FILE SUPPORT TESTS
 *****************************************************************************/

/*
d_tests_dfile_fseeko
  Tests d_file_seek_stream for 64-bit seeking operations.
  Tests the following:
  - seeks from beginning (SEEK_SET)
  - seeks from current position (SEEK_CUR)
  - seeks from end (SEEK_END)
  - returns -1 for NULL stream
*/
struct d_test_object*
d_tests_dfile_fseeko
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    int                   result;
    bool                  test_seek_set;
    bool                  test_seek_cur;
    bool                  test_seek_end;
    bool                  test_null_stream;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    file = d_file_open_stream(path_buf, "r");
    test_seek_set = false;
    test_seek_cur = false;
    test_seek_end = false;

    if (file)
    {
        // test 1: SEEK_SET
        result = d_file_seek_stream(file, 5, SEEK_SET);
        test_seek_set = (result == 0) && (d_file_tell_stream(file) == 5);

        // test 2: SEEK_CUR
        result = d_file_seek_stream(file, 3, SEEK_CUR);
        test_seek_cur = (result == 0) && (d_file_tell_stream(file) == 8);

        // test 3: SEEK_END
        result = d_file_seek_stream(file, -5, SEEK_END);
        test_seek_end = (result == 0);

        fclose(file);
    }

    // test 4: NULL stream
    result = d_file_seek_stream(NULL, 0, SEEK_SET);
    test_null_stream = (result == -1);

    // build result tree
    group = d_test_object_new_interior("d_file_seek_stream", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("seek_set",
                                           test_seek_set,
                                           "d_file_seek_stream seeks from beginning");
    group->elements[idx++] = D_ASSERT_TRUE("seek_cur",
                                           test_seek_cur,
                                           "d_file_seek_stream seeks from current position");
    group->elements[idx++] = D_ASSERT_TRUE("seek_end",
                                           test_seek_end,
                                           "d_file_seek_stream seeks from end");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_file_seek_stream returns -1 for NULL stream");

    return group;
}


/*
d_tests_dfile_ftello
  Tests d_file_tell_stream for 64-bit position reporting.
  Tests the following:
  - reports position after seek
  - reports 0 at file beginning
  - returns -1 for NULL stream
*/
struct d_test_object*
d_tests_dfile_ftello
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    d_off_t               pos;
    bool                  test_after_seek;
    bool                  test_beginning;
    bool                  test_null_stream;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    file = d_file_open_stream(path_buf, "r");
    test_after_seek = false;
    test_beginning = false;

    if (file)
    {
        // test 1: position at beginning
        pos = d_file_tell_stream(file);
        test_beginning = (pos == 0);

        // test 2: position after seek
        d_file_seek_stream(file, 10, SEEK_SET);
        pos = d_file_tell_stream(file);
        test_after_seek = (pos == 10);

        fclose(file);
    }

    // test 3: NULL stream
    pos = d_file_tell_stream(NULL);
    test_null_stream = (pos == -1);

    // build result tree
    group = d_test_object_new_interior("d_file_tell_stream", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("beginning",
                                           test_beginning,
                                           "d_file_tell_stream reports 0 at beginning");
    group->elements[idx++] = D_ASSERT_TRUE("after_seek",
                                           test_after_seek,
                                           "d_file_tell_stream reports position after seek");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_file_tell_stream returns -1 for NULL stream");

    return group;
}


/*
d_tests_dfile_ftruncate
  Tests d_file_truncate_fd for file truncation by descriptor.
  Tests the following:
  - truncates file to smaller size
  - extends file with truncate
  - returns -1 for invalid fd
*/
struct d_test_object*
d_tests_dfile_ftruncate
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    int                   result;
    int64_t               size_before;
    int64_t               size_after;
    bool                  test_truncate_smaller;
    bool                  test_truncate_extend;
    bool                  test_invalid_fd;
    size_t                idx;

    // setup: create a file to truncate
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "truncate_test.txt");

    // create file with some content
    d_file_write_all(path_buf, "Hello World! This is test content.", 34);

    // test 1: truncate to smaller size
    fd = d_file_open(path_buf, O_RDWR);
    test_truncate_smaller = false;

    if (fd >= 0)
    {
        size_before = d_file_size(path_buf);
        result = d_file_truncate_fd(fd, 10);
        d_file_close_fd(fd);
        size_after = d_file_size(path_buf);
        test_truncate_smaller = (result == 0) && (size_after == 10);
    }

    // test 2: extend file with truncate
    fd = d_file_open(path_buf, O_RDWR);
    test_truncate_extend = false;

    if (fd >= 0)
    {
        result = d_file_truncate_fd(fd, 50);
        d_file_close_fd(fd);
        size_after = d_file_size(path_buf);
        test_truncate_extend = (result == 0) && (size_after == 50);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 3: invalid fd
    result = d_file_truncate_fd(-1, 10);
    test_invalid_fd = (result == -1);

    // build result tree
    group = d_test_object_new_interior("d_file_truncate_fd", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("truncate_smaller",
                                           test_truncate_smaller,
                                           "d_file_truncate_fd truncates to smaller size");
    group->elements[idx++] = D_ASSERT_TRUE("truncate_extend",
                                           test_truncate_extend,
                                           "d_file_truncate_fd extends file size");
    group->elements[idx++] = D_ASSERT_TRUE("invalid_fd",
                                           test_invalid_fd,
                                           "d_file_truncate_fd returns -1 for invalid fd");

    return group;
}


/*
d_tests_dfile_ftruncate_stream
  Tests d_file_truncate_stream for stream-based file truncation.
  Tests the following:
  - truncates file via stream
  - returns -1 for NULL stream
*/
struct d_test_object*
d_tests_dfile_ftruncate_stream
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    int                   result;
    int64_t               size_after;
    bool                  test_truncate;
    bool                  test_null_stream;
    size_t                idx;

    // setup: create a file to truncate
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               "truncate_stream_test.txt");

    // create file with content
    d_file_write_all(path_buf, "Hello World! This is test content.", 34);

    // test 1: truncate via stream
    file = d_file_open_stream(path_buf, "r+");
    test_truncate = false;

    if (file)
    {
        result = d_file_truncate_stream(file, 15);
        fclose(file);
        size_after = d_file_size(path_buf);
        test_truncate = (result == 0) && (size_after == 15);
    }

    // cleanup
    d_file_remove(path_buf);

    // test 2: NULL stream
    result = d_file_truncate_stream(NULL, 10);
    test_null_stream = (result == -1);

    // build result tree
    group = d_test_object_new_interior("d_file_truncate_stream", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("truncate",
                                           test_truncate,
                                           "d_file_truncate_stream truncates via stream");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_file_truncate_stream returns -1 for NULL");

    return group;
}


/*
d_tests_dfile_large_file_support_all
  Runs all large file support tests.
  Tests the following:
  - d_file_seek_stream
  - d_file_tell_stream
  - d_file_truncate_fd
  - d_file_truncate_stream
*/
struct d_test_object*
d_tests_dfile_large_file_support_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("IV. Large File Support", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_fseeko();
    group->elements[idx++] = d_tests_dfile_ftello();
    group->elements[idx++] = d_tests_dfile_ftruncate();
    group->elements[idx++] = d_tests_dfile_ftruncate_stream();

    return group;
}
