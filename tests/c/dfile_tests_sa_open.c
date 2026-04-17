/******************************************************************************
* djinterp [test]                                         dfile_tests_sa_open.c
*
*   Tests for secure file opening operations (fopen, fopen_s, freopen, fdopen).
*
*
* path:      \src	est\dfile_tests_sa_open.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"


/******************************************************************************
 * III. SECURE FILE OPENING TESTS
 *****************************************************************************/

/*
d_tests_dfile_fopen
  Tests d_fopen for basic file opening operations.
  Tests the following:
  - opening existing file for reading
  - opening file for writing (creates new)
  - opening file for appending
  - returns NULL for invalid mode
  - returns NULL for NULL filename
  - returns NULL for nonexistent file in read mode
*/
struct d_test_object*
d_tests_dfile_fopen
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  new_file_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    bool                  test_read_existing;
    bool                  test_write_new;
    bool                  test_append;
    bool                  test_null_filename;
    bool                  test_null_mode;
    bool                  test_nonexistent;
    size_t                idx;

    // setup: get path to test file
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(new_file_buf,
                               sizeof(new_file_buf),
                               "fopen_new.txt");

    // test 1: open existing file for reading
    file = d_fopen(path_buf, "r");
    test_read_existing = (file != NULL);

    if (file)
    {
        fclose(file);
    }

    // test 2: open new file for writing
    file = d_fopen(new_file_buf, "w");
    test_write_new = (file != NULL);

    if (file)
    {
        fclose(file);
    }

    // test 3: open file for appending
    file = d_fopen(new_file_buf, "a");
    test_append = (file != NULL);

    if (file)
    {
        fclose(file);
    }

    // cleanup new file
    d_remove(new_file_buf);

    // test 4: NULL filename should return NULL
    file = d_fopen(NULL, "r");
    test_null_filename = (file == NULL);

    // test 5: NULL mode should return NULL
    file = d_fopen(path_buf, NULL);
    test_null_mode = (file == NULL);

    // test 6: nonexistent file in read mode should return NULL
    file = d_fopen("this_file_does_not_exist_12345.txt", "r");
    test_nonexistent = (file == NULL);

    // build result tree
    group = d_test_object_new_interior("d_fopen", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("read_existing",
                                           test_read_existing,
                                           "d_fopen opens existing file for reading");
    group->elements[idx++] = D_ASSERT_TRUE("write_new",
                                           test_write_new,
                                           "d_fopen creates new file for writing");
    group->elements[idx++] = D_ASSERT_TRUE("append",
                                           test_append,
                                           "d_fopen opens file for appending");
    group->elements[idx++] = D_ASSERT_TRUE("null_filename",
                                           test_null_filename,
                                           "d_fopen returns NULL for NULL filename");
    group->elements[idx++] = D_ASSERT_TRUE("null_mode",
                                           test_null_mode,
                                           "d_fopen returns NULL for NULL mode");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_fopen returns NULL for nonexistent file");

    return group;
}


/*
d_tests_dfile_fopen_s
  Tests d_fopen_s for secure file opening with error codes.
  Tests the following:
  - returns 0 and valid stream for existing file
  - returns 0 and valid stream for new file creation
  - returns EINVAL for NULL stream pointer
  - returns EINVAL for NULL filename
  - returns EINVAL for NULL mode
  - returns non-zero for nonexistent file in read mode
*/
struct d_test_object*
d_tests_dfile_fopen_s
(
    void
)
{
    struct d_test_object* group;
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
    size_t                idx;

    // setup: get path to test file
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);
    d_tests_dfile_get_test_path(new_file_buf,
                               sizeof(new_file_buf),
                               "fopen_s_new.txt");

    // test 1: open existing file
    stream = NULL;
    result = d_fopen_s(&stream, path_buf, "r");
    test_existing = (result == 0) && (stream != NULL);

    if (stream)
    {
        fclose(stream);
    }

    // test 2: create new file
    stream = NULL;
    result = d_fopen_s(&stream, new_file_buf, "w");
    test_new_file = (result == 0) && (stream != NULL);

    if (stream)
    {
        fclose(stream);
    }

    // cleanup
    d_remove(new_file_buf);

    // test 3: NULL stream pointer
    result = d_fopen_s(NULL, path_buf, "r");
    test_null_stream = (result == EINVAL);

    // test 4: NULL filename
    stream = NULL;
    result = d_fopen_s(&stream, NULL, "r");
    test_null_filename = (result == EINVAL);

    // test 5: NULL mode
    stream = NULL;
    result = d_fopen_s(&stream, path_buf, NULL);
    test_null_mode = (result == EINVAL);

    // test 6: nonexistent file
    stream = NULL;
    result = d_fopen_s(&stream, "nonexistent_file_98765.txt", "r");
    test_nonexistent = (result != 0) && (stream == NULL);

    // build result tree
    group = d_test_object_new_interior("d_fopen_s", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("existing",
                                           test_existing,
                                           "d_fopen_s returns 0 for existing file");
    group->elements[idx++] = D_ASSERT_TRUE("new_file",
                                           test_new_file,
                                           "d_fopen_s creates new file successfully");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_fopen_s returns EINVAL for NULL stream");
    group->elements[idx++] = D_ASSERT_TRUE("null_filename",
                                           test_null_filename,
                                           "d_fopen_s returns EINVAL for NULL filename");
    group->elements[idx++] = D_ASSERT_TRUE("null_mode",
                                           test_null_mode,
                                           "d_fopen_s returns EINVAL for NULL mode");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_fopen_s returns error for nonexistent file");

    return group;
}


/*
d_tests_dfile_freopen
  Tests d_freopen for file reopening operations.
  Tests the following:
  - reopens file with different mode
  - returns NULL for NULL mode
  - returns NULL for NULL stream
*/
struct d_test_object*
d_tests_dfile_freopen
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    FILE*                 reopened;
    bool                  test_reopen;
    bool                  test_null_mode;
    bool                  test_null_stream;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: reopen file with different mode
    file = d_fopen(path_buf, "r");
    test_reopen = false;

    if (file)
    {
        reopened = d_freopen(path_buf, "r+", file);
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
    file = d_fopen(path_buf, "r");
    test_null_mode = false;

    if (file)
    {
        reopened = d_freopen(path_buf, NULL, file);
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
    reopened = d_freopen(path_buf, "r", NULL);
    test_null_stream = (reopened == NULL);

    // build result tree
    group = d_test_object_new_interior("d_freopen", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("reopen",
                                           test_reopen,
                                           "d_freopen reopens file with new mode");
    group->elements[idx++] = D_ASSERT_TRUE("null_mode",
                                           test_null_mode,
                                           "d_freopen returns NULL for NULL mode");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_freopen returns NULL for NULL stream");

    return group;
}


/*
d_tests_dfile_freopen_s
  Tests d_freopen_s for secure file reopening.
  Tests the following:
  - returns 0 and valid stream for successful reopen
  - returns EINVAL for NULL newstream pointer
  - returns EINVAL for NULL mode
  - returns EINVAL for NULL stream
*/
struct d_test_object*
d_tests_dfile_freopen_s
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    FILE*                 file;
    FILE*                 newstream;
    int                   result;
    bool                  test_reopen;
    bool                  test_null_newstream;
    bool                  test_null_mode;
    bool                  test_null_stream;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: successful reopen
    file = d_fopen(path_buf, "r");
    test_reopen = false;

    if (file)
    {
        newstream = NULL;
        result = d_freopen_s(&newstream, path_buf, "r+", file);
        test_reopen = (result == 0) && (newstream != NULL);

        if (newstream)
        {
            fclose(newstream);
        }
    }

    // test 2: NULL newstream pointer
    file = d_fopen(path_buf, "r");
    test_null_newstream = false;

    if (file)
    {
        result = d_freopen_s(NULL, path_buf, "r", file);
        test_null_newstream = (result == EINVAL);
        fclose(file);
    }

    // test 3: NULL mode
    file = d_fopen(path_buf, "r");
    test_null_mode = false;

    if (file)
    {
        newstream = NULL;
        result = d_freopen_s(&newstream, path_buf, NULL, file);
        test_null_mode = (result == EINVAL);
        fclose(file);
    }

    // test 4: NULL stream
    newstream = NULL;
    result = d_freopen_s(&newstream, path_buf, "r", NULL);
    test_null_stream = (result == EINVAL);

    // build result tree
    group = d_test_object_new_interior("d_freopen_s", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("reopen",
                                           test_reopen,
                                           "d_freopen_s returns 0 for successful reopen");
    group->elements[idx++] = D_ASSERT_TRUE("null_newstream",
                                           test_null_newstream,
                                           "d_freopen_s returns EINVAL for NULL newstream");
    group->elements[idx++] = D_ASSERT_TRUE("null_mode",
                                           test_null_mode,
                                           "d_freopen_s returns EINVAL for NULL mode");
    group->elements[idx++] = D_ASSERT_TRUE("null_stream",
                                           test_null_stream,
                                           "d_freopen_s returns EINVAL for NULL stream");

    return group;
}


/*
d_tests_dfile_fdopen
  Tests d_fdopen for associating streams with file descriptors.
  Tests the following:
  - associates stream with valid file descriptor
  - returns NULL for invalid file descriptor
  - returns NULL for NULL mode
*/
struct d_test_object*
d_tests_dfile_fdopen
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    FILE*                 stream;
    bool                  test_valid_fd;
    bool                  test_invalid_fd;
    bool                  test_null_mode;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: associate stream with valid fd
    fd = d_open(path_buf, O_RDONLY);
    test_valid_fd = false;

    if (fd >= 0)
    {
        stream = d_fdopen(fd, "r");
        test_valid_fd = (stream != NULL);

        if (stream)
        {
            // fclose will close the fd
            fclose(stream);
        }
        else
        {
            d_close(fd);
        }
    }

    // test 2: invalid fd should return NULL
    stream = d_fdopen(-1, "r");
    test_invalid_fd = (stream == NULL);

    // test 3: NULL mode should return NULL
    fd = d_open(path_buf, O_RDONLY);
    test_null_mode = false;

    if (fd >= 0)
    {
        stream = d_fdopen(fd, NULL);
        test_null_mode = (stream == NULL);
        d_close(fd);
    }

    // build result tree
    group = d_test_object_new_interior("d_fdopen", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_fd",
                                           test_valid_fd,
                                           "d_fdopen associates stream with valid fd");
    group->elements[idx++] = D_ASSERT_TRUE("invalid_fd",
                                           test_invalid_fd,
                                           "d_fdopen returns NULL for invalid fd");
    group->elements[idx++] = D_ASSERT_TRUE("null_mode",
                                           test_null_mode,
                                           "d_fdopen returns NULL for NULL mode");

    return group;
}


/*
d_tests_dfile_secure_file_opening_all
  Runs all secure file opening tests.
  Tests the following:
  - d_fopen
  - d_fopen_s
  - d_freopen
  - d_freopen_s
  - d_fdopen
*/
struct d_test_object*
d_tests_dfile_secure_file_opening_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("III. Secure File Opening", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_fopen();
    group->elements[idx++] = d_tests_dfile_fopen_s();
    group->elements[idx++] = d_tests_dfile_freopen();
    group->elements[idx++] = d_tests_dfile_freopen_s();
    group->elements[idx++] = d_tests_dfile_fdopen();

    return group;
}