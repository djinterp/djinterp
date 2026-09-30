/*******************************************************************************
* djinterp [c]                                                       file_pipe.c
*
* Implementation of the process pipes declared in file_pipe.h.
*   popen and pclose on POSIX; _popen and _pclose on Windows, where the mode
* gains a 'b' when the build asks for binary pipes. The whole file compiles to
* nothing when this build has no pipes.
*
*
* path:      /src/djinterp/c/fs/file_pipe.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_pipe.h"  // corresponding header
// std
#include <errno.h>   // errno, EINVAL
#include <stdio.h>   // FILE, popen, pclose
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/fs/cfg_file_pipe.h"  // D_INTERNAL_FILE_PIPE_BINARY
// posix
#if ( (D_INTERNAL_FILE_HAS_PIPES == 1) &&                                      \
      (!D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    #include <sys/wait.h>  // WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
#endif


#if (D_INTERNAL_FILE_HAS_PIPES == 1)

//==============================================================================
// 1.  PIPES
//==============================================================================

/*
d_pipe_open
  The command goes through the system shell, with everything the declaration
warns of. The child inherits every descriptor that is not close-on-exec, which
is why D_CFG_FILE_CLOEXEC_DEFAULT is on: inheritance becomes something asked
for rather than something that happens. With binary pipes on, the mode is
copied into a small buffer so a 'b' can be appended; a mode too long for that
buffer is malformed.
*/
FILE*
d_pipe_open(
    const char* _command,
    const char* _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_command != NULL,
                            EINVAL,
                            "d_pipe_open",
                            NULL,
                            "command is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_pipe_open",
                            NULL,
                            "mode is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(( (_mode[0] == 'r') ||
                              (_mode[0] == 'w') ),
                            EINVAL,
                            "d_pipe_open",
                            NULL,
                            "mode must begin with 'r' or 'w'",
                            NULL);

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_TRACE,
                           0,
                           "d_pipe_open",
                           NULL,
                           "running a command through the system shell");

    FILE* result = NULL;

#if (D_INTERNAL_FILE_PIPE_BINARY == 1)
    char         mode_buf[8];
    const size_t length = strlen(_mode);

    // the buffer must hold the mode, the 'b' and the terminator
    if ((length + 2) > sizeof(mode_buf))
    {
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_pipe_open",
                             NULL,
                             "mode string is malformed",
                             NULL);
    }

    memcpy(mode_buf,
           _mode,
           length);
    mode_buf[length]     = 'b';
    mode_buf[length + 1] = '\0';
    result = _popen(_command,
                    mode_buf);
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _popen(_command,
                    _mode);
#else
    result = popen(_command,
                   _mode);
#endif

    // report the failure; errno is the platform's
    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_pipe_open",
                               NULL,
                               "popen failed");
    }

    return result;
}

/*
d_pipe_close
  The platform's status is passed through untouched. Smoothing a POSIX wait
status into an exit code here would discard the signal information it
carries; d_pipe_exit_code does that decoding for the callers who want it.
*/
int
d_pipe_close(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_pipe_close",
                            NULL,
                            "stream is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int result = _pclose(_stream);
#else
    const int result = pclose(_stream);
#endif

    // only -1 is OUR failure; anything else is the child's business
    if (result == -1)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_pipe_close",
                               NULL,
                               "could not wait for the child");
    }

    return result;
}

/*
d_pipe_exit_code
  The shell's vocabulary: N for a normal exit, 128+N for death by signal N.
That convention folds `exit 143` and SIGTERM onto one value; the trade is made
here rather than in each caller, so that every caller makes the same one. A
status that is neither a normal exit nor a signal comes back unchanged rather
than mapped onto a plausible-looking number. Windows' _pclose already returns
the exit code, so there the decode is the identity.
*/
int
d_pipe_exit_code(
    int _status
)
{
    // a negative status reports a failed wait, not an exit code
    if (_status < 0)
    {
        return -1;
    }

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    return _status;
#else
    // a normal exit reports its own code
    if (WIFEXITED(_status))
    {
        return WEXITSTATUS(_status);
    }

    // death by signal N reports 128 + N, as the shell does
    if (WIFSIGNALED(_status))
    {
        return 128 + WTERMSIG(_status);
    }

    return _status;
#endif
}

#endif  // D_INTERNAL_FILE_HAS_PIPES
