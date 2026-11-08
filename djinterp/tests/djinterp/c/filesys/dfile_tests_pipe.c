/*******************************************************************************
* djinterp [test]                                             dfile_tests_pipe.c
*
*   Tests for pipe operations (popen, pclose).
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_pipe.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * XIV. PIPE OPERATIONS TESTS
 *****************************************************************************/

/*
d_tests_dfile_popen_pclose
  Tests d_pipe_open and d_pipe_close for process pipes.
  Tests the following:
  - opens pipe for reading
  - can read output from command
  - closes pipe successfully
  - returns NULL for NULL command
*/
bool d_tests_dfile_popen_pclose(struct dj_test_suite* _suite)
{
    FILE*                 pipe_stream;
    char                  buf[256];
    int                   close_result;
    bool                  test_open;
    bool                  test_read;
    bool                  test_close;
    bool                  test_null_command;

    // test 1, 2, 3: open pipe, read, and close
#if defined(_WIN32)
    pipe_stream = d_pipe_open("echo test", "r");
#else
    pipe_stream = d_pipe_open("echo test", "r");
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
        close_result = d_pipe_close(pipe_stream);
        test_close = (close_result != -1);
    }

    // test 4: NULL command
    pipe_stream = d_pipe_open(NULL, "r");
    test_null_command = (pipe_stream == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_pipe_open/d_pipe_close");

    D_TEST_CHECK_MSG(_suite, "open", test_open, "d_pipe_open opens pipe");
    D_TEST_CHECK_MSG(_suite, "read", test_read, "can read from pipe");
    D_TEST_CHECK_MSG(_suite, "close", test_close, "d_pipe_close closes pipe");
    D_TEST_CHECK_MSG(_suite,
                     "null_command",
                     test_null_command,
                     "d_pipe_open returns NULL for NULL command");

    return true;
}


/*
d_tests_dfile_pipe_operations_all
  Runs all pipe operation tests.
  Tests the following:
  - d_pipe_open/d_pipe_close
*/
bool d_tests_dfile_pipe_operations_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "XIV. Pipe Operations");

    d_tests_dfile_popen_pclose(_suite);

    return true;
}
