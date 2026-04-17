/******************************************************************************
* djinterp [test]                                         dfile_tests_sa_pipe.c
*
*   Tests for pipe operations (popen, pclose).
*
*
* path:      \src	est\dfile_tests_sa_pipe.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"


/******************************************************************************
 * XIV. PIPE OPERATIONS TESTS
 *****************************************************************************/

/*
d_tests_dfile_popen_pclose
  Tests d_popen and d_pclose for process pipes.
  Tests the following:
  - opens pipe for reading
  - can read output from command
  - closes pipe successfully
  - returns NULL for NULL command
*/
struct d_test_object*
d_tests_dfile_popen_pclose
(
    void
)
{
    struct d_test_object* group;
    FILE*                 pipe_stream;
    char                  buf[256];
    int                   close_result;
    bool                  test_open;
    bool                  test_read;
    bool                  test_close;
    bool                  test_null_command;
    size_t                idx;

    // test 1, 2, 3: open pipe, read, and close
#if defined(D_FILE_PLATFORM_WINDOWS)
    pipe_stream = d_popen("echo test", "r");
#else
    pipe_stream = d_popen("echo test", "r");
#endif

    test_open = (pipe_stream != NULL);
    test_read = false;
    test_close = false;

    if (pipe_stream)
    {
        // test 2: read output
        if (fgets(buf, sizeof(buf), pipe_stream) != NULL)
        {
            test_read = (strlen(buf) > 0);
        }

        // test 3: close pipe
        close_result = d_pclose(pipe_stream);
        test_close = (close_result != -1);
    }

    // test 4: NULL command
    pipe_stream = d_popen(NULL, "r");
    test_null_command = (pipe_stream == NULL);

    // build result tree
    group = d_test_object_new_interior("d_popen/d_pclose", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("open",
                                           test_open,
                                           "d_popen opens pipe");
    group->elements[idx++] = D_ASSERT_TRUE("read",
                                           test_read,
                                           "can read from pipe");
    group->elements[idx++] = D_ASSERT_TRUE("close",
                                           test_close,
                                           "d_pclose closes pipe");
    group->elements[idx++] = D_ASSERT_TRUE("null_command",
                                           test_null_command,
                                           "d_popen returns NULL for NULL command");

    return group;
}


/*
d_tests_dfile_pipe_operations_all
  Runs all pipe operation tests.
  Tests the following:
  - d_popen/d_pclose
*/
struct d_test_object*
d_tests_dfile_pipe_operations_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("XIV. Pipe Operations", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_popen_pclose();

    return group;
}


