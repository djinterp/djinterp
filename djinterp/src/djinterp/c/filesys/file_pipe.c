/*******************************************************************************
* djinterp [c]                                                       file_pipe.c
*
* Implementation of the process pipes declared in file_pipe.h.
*   popen and pclose on POSIX, with the parent's end of the pipe kept from
* the children started after it; _popen and _pclose on Windows, where the mode
* gains a 'b' when the build asks for binary pipes and the caller has not
* chosen. One helper per platform. The whole file compiles to nothing when
* this build has no pipes.
*
*
* path:      /src/djinterp/c/filesys/file_pipe.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_pipe.h"  // corresponding header
// std
#include <errno.h>   // errno, EINVAL
#include <stdio.h>   // FILE, popen, pclose, fileno
#include <string.h>  // memcpy, strchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_pipe.h"  // D_INTERNAL_FILE_PIPE_*
// posix
#if ( (D_INTERNAL_FILE_HAS_PIPES == 1) &&                                      \
      (!D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    #include <sys/wait.h>  // WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
#endif


#if (D_INTERNAL_FILE_HAS_PIPES == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_PIPE_POPEN
//   constant: the platform's popen -- _popen on Windows.
// D_INTERNAL_PIPE_PCLOSE
//   constant: the platform's pclose -- _pclose on Windows.
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_INTERNAL_PIPE_POPEN  _popen
    #define D_INTERNAL_PIPE_PCLOSE _pclose
#else
    #define D_INTERNAL_PIPE_POPEN  popen
    #define D_INTERNAL_PIPE_PCLOSE pclose
#endif  // D_CFG_FILE_HAS_WIN32

#if (D_INTERNAL_FILE_PIPE_MODE_CHAR != 0)

// D_INTERNAL_PIPE_MODE_MAX
//   constant: room for any mode the platforms accept -- "r" or "w", the
// letters after it, the one this build adds -- and the terminator.
#define D_INTERNAL_PIPE_MODE_MAX 8

/*
d_internal_pipe_mode
  The caller's mode in _buf, with this build's letter appended unless the
caller has made that choice already: 'b', for binary pipes on Windows, stands
aside for a 'b' or a 't'; 'e', glibc's close-on-exec, for an 'e'. An explicit
choice is never overruled, as in file_open. A mode too long to be one is
malformed.
*/
static int
d_internal_pipe_mode(
    const char* _mode,
    char*       _buf
)
{
    const char   letter = (char)D_INTERNAL_FILE_PIPE_MODE_CHAR;
    const size_t length = strlen(_mode);
    const int    chosen = ( (strchr(_mode,
                                    letter) != NULL) ||
                            ( (letter == 'b') &&
                              (strchr(_mode,
                                      't') != NULL) ) );

    // the mode, the letter and the terminator must fit
    if ((length + 2) > D_INTERNAL_PIPE_MODE_MAX)
    {
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_pipe_open",
                             NULL,
                             "mode string is malformed",
                             0);
    }

    memcpy(_buf,
           _mode,
           length);
    _buf[length]     = chosen ? '\0' : letter;
    _buf[length + 1] = '\0';

    return 1;
}

/*
d_internal_pipe_raw
  The platform's popen, given the mode with this build's letter
(d_internal_pipe_mode): on glibc that is 'e', which makes the parent's end
close-on-exec atomically.
*/
static FILE*
d_internal_pipe_raw(
    const char* _command,
    const char* _mode
)
{
    char mode[D_INTERNAL_PIPE_MODE_MAX] = {0};

    // d_internal_pipe_mode has already reported a malformed mode
    if (!d_internal_pipe_mode(_mode,
                              mode))
    {
        return NULL;
    }

    return D_INTERNAL_PIPE_POPEN(_command,
                                 mode);
}

#elif (D_INTERNAL_FILE_PIPE_CLOEXEC == 1)

/*
d_internal_pipe_raw
  popen, then close-on-exec for the parent's end, which this platform's popen
takes no letter for. Without it every child started later inherits the pipe,
and a "w" pipe's command, waiting for an end of input that the other child's
copy keeps from coming, outlives the close. The child popen started already
has its own end and is unaffected. A refusal is reported, not fatal: the pipe
works.
*/
static FILE*
d_internal_pipe_raw(
    const char* _command,
    const char* _mode
)
{
    FILE* const stream = popen(_command,
                               _mode);
    const int   fd     = (stream != NULL) ? fileno(stream) : -1;
    const int   flags  = (fd >= 0) ? fcntl(fd,
                                           F_GETFD)
                                   : -1;

    // kept from later children, or said that it is not
    if ( (stream != NULL) &&
         ( (flags < 0) ||
           (fcntl(fd,
                  F_SETFD,
                  flags | FD_CLOEXEC) != 0) ) )
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                               errno,
                               "d_pipe_open",
                               NULL,
                               "the pipe could not be made close-on-exec; "
                               "children started later inherit it");
    }

    return stream;
}

#else

/*
d_internal_pipe_raw
  The platform's popen as it is: on Windows the CRT keeps the parent's end
from the child itself, and this build adds no letter to the mode.
*/
static FILE*
d_internal_pipe_raw(
    const char* _command,
    const char* _mode
)
{
    return D_INTERNAL_PIPE_POPEN(_command,
                                 _mode);
}

#endif  // D_INTERNAL_FILE_PIPE_MODE_CHAR, D_INTERNAL_FILE_PIPE_CLOEXEC

//==============================================================================
// 1.  PIPES
//==============================================================================

/*
d_pipe_open
  The command goes through the system shell, with everything the declaration
warns of. The child inherits every descriptor that is not close-on-exec, which
is why D_CFG_FILE_CLOEXEC_DEFAULT is on: inheritance becomes something asked
for rather than something that happens -- and the pipe's own end, which popen
leaves inheritable, is kept back too (d_internal_pipe_raw).
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

    FILE* const result = d_internal_pipe_raw(_command,
                                             _mode);

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

    const int result = D_INTERNAL_PIPE_PCLOSE(_stream);

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

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_pipe_exit_code
  Windows' _pclose returns the command's exit code itself, so the decode is
the identity, for every value: a crash ends a process with an NTSTATUS such as
0xC0000005, negative as an int, which must not be mistaken for the -1 of a
failed wait. An exit code of 0xFFFFFFFF is that -1 as well, and nothing can
tell those two apart.
*/
int
d_pipe_exit_code(
    int _status
)
{
    return _status;
}

#else

/*
d_pipe_exit_code
  The shell's vocabulary: N for a normal exit, 128+N for death by signal N.
That convention folds `exit 143` and SIGTERM onto one value; the trade is made
here rather than in each caller, so that every caller makes the same one. A
status that is neither a normal exit nor a signal comes back unchanged rather
than mapped onto a plausible-looking number.
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
}

#endif  // D_CFG_FILE_HAS_WIN32

#endif  // D_INTERNAL_FILE_HAS_PIPES
