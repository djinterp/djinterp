/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_pipe.c
*
*
* path:      /tests/djinterp/c/fs/file_tests_sa_pipe.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

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
static struct d_test_object*
d_tests_file_popen
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    FILE*                 pipe_stream;
    char                  buf[256];
    int                   status;
    size_t                idx;

    idx = 0;

    pipe_stream = d_pipe_open("echo test", "r");
    items[idx++] = D_ASSERT_NOT_NULL("open", pipe_stream,
                                     "d_pipe_open runs a command");

    if (pipe_stream)
    {
        items[idx++] = D_ASSERT_TRUE("read",
                                     (fgets(buf, sizeof(buf), pipe_stream) != NULL) &&
                                     (strlen(buf) > 0),
                                     "the child's output is readable");
        items[idx++] = D_ASSERT_EQUAL("close_success",
                                      d_pipe_close(pipe_stream), 0,
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
        items[idx++] = D_ASSERT_TRUE("close_not_our_failure", status != -1,
                                     "a failing command is not a d_pipe_close failure");
        items[idx++] = D_ASSERT_TRUE("child_status_carried", status != 0,
                                     "d_pipe_close carries the child's exit status");
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_command", d_pipe_open(NULL, "r"),
                                 "d_pipe_open rejects a NULL command");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_mode", d_pipe_open("echo x", NULL),
                                 "d_pipe_open rejects a NULL mode");
    #endif
    items[idx++] = D_ASSERT_NULL("bad_mode", d_pipe_open("echo x", "z"),
                                 "d_pipe_open rejects a mode that is not r or w");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("close_null", d_pipe_close(NULL), -1,
                                  "d_pipe_close rejects NULL");
    #endif

    return d_tests_file_group("d_pipe_open / d_pipe_close", items, idx);
}


/*
d_tests_file_pipe_all
  Runs every file_pipe test.
*/
struct d_test_object*
d_tests_file_pipe_all
(
    void
)
{
    struct d_test_object* items[2];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_popen();

    return d_tests_file_group("XIV. file_pipe", items, idx);
}

#endif  // D_FILE_PIPE_IS_AVAILABLE
