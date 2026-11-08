/*******************************************************************************
* djinterp [test]                                              file_tests_pipe.c
*
*   XIV. PIPES (file_pipe) -- running a command and streaming its input or
* output. Compiled out where the platform has no command pipes
* (D_FILE_PIPE_IS_AVAILABLE is 0). The read path runs a command with known
* output and checks the bytes; the write path streams into a consumer; and the
* guards are checked -- a NULL command and a mode that is neither read nor
* write are both rejected, and d_pipe_close rejects a NULL stream.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_pipe.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "file_tests.h"


#if D_FILE_PIPE_IS_AVAILABLE

void d_fs_tests_pipe(
    struct dj_test_suite* _suite
)
{
    FILE* pipe;
    char  buf[64];
    bool  read_ok;
    bool  wrote_ok;

    d_test_suite_module(_suite, "XIV. file_pipe");

    d_test_suite_block(_suite, "d_pipe_open / d_pipe_close");

    d_test_suite_test(_suite, "reads a command's output");
    pipe    = d_pipe_open(T_PRINT("hello"), "r");
    read_ok = false;

    if (pipe)
    {
        memset(buf, 0, sizeof(buf));

        if (fread(buf, 1, 5, pipe) == 5)
        {
            read_ok = (memcmp(buf, "hello", 5) == 0);
        }

        // pclose reaps the child; a clean exit is not an error
        D_FS_CHECK(_suite, d_pipe_close(pipe) != -1);
    }

    D_FS_CHECK(_suite, read_ok);

    d_test_suite_test(_suite, "streams into a command's input");
    pipe     = d_pipe_open(T_SINK, "w");
    wrote_ok = false;

    if (pipe)
    {
        wrote_ok = (fwrite("data", 1, 4, pipe) == 4);
        D_FS_CHECK(_suite, d_pipe_close(pipe) != -1);
    }

    D_FS_CHECK(_suite, wrote_ok);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL command -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_pipe_open(NULL, "r") == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite,
                      "a mode that is neither read nor write -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_pipe_open(T_PRINT("hi"), "q") == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "d_pipe_close(NULL) -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_pipe_close(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    return;
#endif
}

#endif  // D_FILE_PIPE_IS_AVAILABLE

/******************************************************************************
 * d_tests_file_pipe_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * XIV.  file_pipe  --  PROCESS PIPES
 *****************************************************************************/

#if D_FILE_PIPE_IS_AVAILABLE

/*
d_tests_file_popen
  Tests d_pipe_open / d_pipe_close.
  The interesting assertion is the last one: d_pipe_close returns the CHILD's
status, not a success flag, and a suite that tested `!= 0` for "did it close"
would report every failing command as a broken close.
*/
static
bool d_tests_file_popen(struct dj_test_suite* _suite)
{
    FILE*                 pipe_stream;
    char                  buf[256];
    int                   status;

    d_test_suite_test(_suite, "d_pipe_open / d_pipe_close");

    pipe_stream = d_pipe_open("echo test", "r");
    D_TEST_CHECK_MSG(_suite,
                     "open",
                     (pipe_stream) != NULL,
                     "d_pipe_open runs a command");

    if (pipe_stream)
    {
        D_TEST_CHECK_MSG(_suite,
                         "read",
                         (fgets(buf, sizeof(buf), pipe_stream) != NULL) && (strlen(buf) > 0),
                         "the child's output is readable");
        D_TEST_CHECK_MSG(_suite,
                         "close_success",
                         (d_pipe_close(pipe_stream)) == (0),
                         "d_pipe_close reaps; 0 = ran AND succeeded");
    }

    // the trap. 0 means the command ran and succeeded; non-zero usually means
    // it ran and FAILED; only -1 means the wait itself broke. Testing
    // `d_pipe_close(p) != 0` for "did close work" reports every non-zero exit as
    // a close failure.
    pipe_stream = d_pipe_open("exit 3", "r");

    if (pipe_stream)
    {
        status = d_pipe_close(pipe_stream);
        D_TEST_CHECK_MSG(_suite,
                         "close_not_our_failure",
                         status != -1,
                         "a failing command is not a d_pipe_close failure");
        D_TEST_CHECK_MSG(_suite,
                         "child_status_carried",
                         status != 0,
                         "d_pipe_close carries the child's exit status");
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_command",
                     (d_pipe_open(NULL, "r")) == NULL,
                     "d_pipe_open rejects a NULL command");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_mode",
                     (d_pipe_open("echo x", NULL)) == NULL,
                     "d_pipe_open rejects a NULL mode");
    #endif
    D_TEST_CHECK_MSG(_suite,
                     "bad_mode",
                     (d_pipe_open("echo x", "z")) == NULL,
                     "d_pipe_open rejects a mode that is not r or w");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "close_null",
                     (d_pipe_close(NULL)) == (-1),
                     "d_pipe_close rejects NULL");
    #endif

    return true;
}


/*
d_tests_file_pipe_all
  Runs every file_pipe test.
*/
bool d_tests_file_pipe_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "XIV. file_pipe");

    d_tests_file_popen(_suite);

    return true;
}

#endif  // D_FILE_PIPE_IS_AVAILABLE
