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
*                                                            revised: 2026.10.21
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "file_tests.h"


#if D_FILE_PIPE_IS_AVAILABLE

void
d_fs_tests_pipe
(
    dtest_suite* _suite
)
{
    FILE* pipe;
    char  buf[64];
    bool  read_ok;
    bool  wrote_ok;

    dtest_module(_suite, "XIV. file_pipe");

    dtest_block(_suite, "d_pipe_open / d_pipe_close");

    dtest_test(_suite, "reads a command's output");
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

    dtest_test(_suite, "streams into a command's input");
    pipe     = d_pipe_open(T_SINK, "w");
    wrote_ok = false;

    if (pipe)
    {
        wrote_ok = (fwrite("data", 1, 4, pipe) == 4);
        D_FS_CHECK(_suite, d_pipe_close(pipe) != -1);
    }

    D_FS_CHECK(_suite, wrote_ok);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL command -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_pipe_open(NULL, "r") == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "a mode that is neither read nor write -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_pipe_open(T_PRINT("hi"), "q") == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "d_pipe_close(NULL) -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_pipe_close(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    return;
#endif
}

#endif  // D_FILE_PIPE_IS_AVAILABLE
