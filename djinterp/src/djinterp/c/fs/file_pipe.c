#include "../../../../inc/djinterp/c/fs/file_pipe.h"


#if (D_INTERNAL_FILE_HAS_PIPES == 1)

#if !D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #include <sys/wait.h>   /* WIFEXITED / WEXITSTATUS / WTERMSIG */
#endif

///////////////////////////////////////////////////////////////////////////////
///             I.  PIPES                                                   ///
///////////////////////////////////////////////////////////////////////////////

/*
d_popen
  Runs a command through the system shell and connects a stream to its
standard input or output.
  THE COMMAND GOES THROUGH A SHELL. /bin/sh -c on POSIX, cmd.exe /c on
Windows. Every metacharacter is interpreted, so a command built from anything
a user influenced -- a filename, a config value, an argument -- is a
command-injection vulnerability. Quoting does not fix it; there is no portable
quoting that survives both shells. Use posix_spawn / CreateProcess with an
argument vector when the command is not a literal.
  The child inherits this process's descriptors that are not close-on-exec.
D_CFG_FILE_CLOEXEC_DEFAULT is on precisely so that inheritance is something
you ask for rather than something that happens to you.

Parameter(s):
  _command: the command line, passed to the shell.
  _mode:    "r" to read the child's stdout, "w" to write its stdin.
Return:
  An open stream on success, or NULL on failure with errno set. Close it with
d_pclose -- never fclose, which does not reap the child.
*/
FILE*
d_popen
(
    const char* _command,
    const char* _mode
)
{
    FILE* result;
#if (D_INTERNAL_FILE_PIPE_BINARY == 1)
    char  mode_buf[8];
    size_t length;
#endif

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_command != NULL,
                            EINVAL,
                            "d_popen",
                            NULL,
                            "command is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_popen",
                            NULL,
                            "mode is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE( (_mode[0] == 'r') ||
                             (_mode[0] == 'w'),
                            EINVAL,
                            "d_popen",
                            NULL,
                            "mode must begin with 'r' or 'w'",
                            NULL);

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_TRACE,
                           0,
                           "d_popen",
                           NULL,
                           "running a command through the system shell");

#if (D_INTERNAL_FILE_PIPE_BINARY == 1)
    length = strlen(_mode);

    if ((length + 2) > sizeof(mode_buf))
    {
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_popen",
                             NULL,
                             "mode string is malformed",
                             NULL);
    }

    memcpy(mode_buf, _mode, length);
    mode_buf[length]     = 'b';
    mode_buf[length + 1] = '\0';
    result = _popen(_command, mode_buf);
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _popen(_command, _mode);
#else
    result = popen(_command, _mode);
#endif

    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_popen",
                               NULL,
                               "popen failed");
    }

    return result;
}


/*
d_pclose
  Closes a pipe and waits for the child to finish.
  The return value is the child's EXIT STATUS, not a success flag, and that is
the trap: 0 means the command ran AND succeeded, non-zero may mean the command
ran and failed, and -1 means the wait itself failed. A caller testing
`d_pclose(p) != 0` for "did close work" reports every command that exits
non-zero as a close failure.
  On POSIX the value is a wait(2) status -- decode it with WIFEXITED /
WEXITSTATUS. On Windows it is the exit code directly. That difference is
inherent to the platforms and is not smoothed over here, because doing so
would discard the signal information POSIX carries.

Parameter(s):
  _stream: a stream from d_popen.
Return:
  The child's status, or -1 if the wait failed.
*/
int
d_pclose
(
    FILE* _stream
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_pclose",
                            NULL,
                            "stream is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _pclose(_stream);
#else
    result = pclose(_stream);
#endif

    // only -1 is OUR failure; anything else is the child's business
    if (result == -1)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_pclose",
                               NULL,
                               "could not wait for the child");
    }

    return result;
}


/*
d_pipe_exit_code
  Decodes what d_pclose returned into the command's exit code.
  d_pclose deliberately hands back the platform's own value, because smoothing
that over would discard the signal information a POSIX wait status carries.
This is the other half of that decision: the caller who wants a single number
in the shell's vocabulary -- N for a normal exit, 128+N for death by signal N,
127 for "command not found" -- gets it here, and the caller who wants the raw
status still has it from d_pclose.
  The 128+N convention does fold `exit 143` and SIGTERM onto one value. That is
the trade the convention makes, and it is made HERE rather than in each caller,
so that every caller makes the same one. Anything the platform reports that is
neither a normal exit nor a signal is returned unchanged rather than mapped
onto a plausible-looking number.
  This lives in C because decoding needs <sys/wait.h>, and no C++ header in
this framework may read an OS header.

Parameter(s):
  _status: the value d_pclose returned.
Return:
  The exit code, or -1 when _status reported a failed wait.
*/
int
d_pipe_exit_code
(
    int _status
)
{
    if (_status < 0)
    {
        return -1;
    }

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    return _status;
#else
    if (WIFEXITED(_status))
    {
        return WEXITSTATUS(_status);
    }

    if (WIFSIGNALED(_status))
    {
        return 128 + WTERMSIG(_status);
    }

    return _status;
#endif
}


#endif  // D_INTERNAL_FILE_HAS_PIPES
