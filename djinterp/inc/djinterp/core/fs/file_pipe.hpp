/******************************************************************************
* djinterp [fs]                                                  file_pipe.hpp
*
*   djinterp::process -- a child process run through the shell, its standard
* input or output connected to this handle by a pipe (the popen model, roadmap
* Phase 8). It is the RAII owner of that pipe: reading or writing talks to the
* command, and closing REAPS the command and yields its exit status.
*
*   >>> SECURITY. d_popen runs its command through a SHELL -- /bin/sh -c on
*   POSIX, cmd.exe /c on Windows -- so EVERY shell metacharacter in the string
*   is interpreted. Building the command from anything a user, a file, or a
*   network peer influenced is a command-injection vulnerability, and nothing in
*   this class prevents it: the constructor hands the string straight to the
*   shell. The only fix is not to do it -- run a fixed, literal command here, and
*   reach for posix_spawn / CreateProcess with an ARGUMENT VECTOR when any part
*   of the command is variable. This is the one hazard of this facility, stated
*   where you cannot miss it. <<<
*
*   D4, OWNERSHIP. A pipe to a live process cannot be shared by copying, so
* process is NON-COPYABLE on every tier and MOVABLE on C++11+ -- the same rule
* file and directory follow, drawing the same spellings (D_DELETED_FN,
* D_NOEXCEPT) from the shared prelude.
*
*   EXIT STATUS IS NOT AN ERROR. A command that runs and exits non-zero has not
* failed as an I/O operation -- it has reported a result, and that result is
* information you asked for. So close() returns whether the pipe CLOSED and the
* process was REAPED (an _ec set only if pclose itself failed), and the command's
* exit code is read separately via exit_status(), meaningful once exited() is
* true. A missing command, a command that dies -- these surface as an exit
* status (127, 128+signal), not as a close failure.
*
*   AVAILABILITY. The whole facility is compiled out where the platform has no
* pipes (D_FILE_PIPE_IS_AVAILABLE == 0); on such a build djinterp::process does
* not exist, and naming it is a compile error rather than a link-time surprise.
*
* 
* path:      /inc/djinterp/core/fs/file_pipe.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#ifndef DJINTERP_FS_FILE_PIPE_
#define DJINTERP_FS_FILE_PIPE_ 1

#include "file_common.hpp"

#include "../../c/fs/file_pipe.h"     // d_popen, d_pclose, D_FILE_PIPE_IS_AVAILABLE

#include <cstdio>                  // FILE, fread, fwrite, feof, ferror, clearerr
#include <cerrno>                  // errno, EBADF, EIO


NS_DJINTERP

#if D_FILE_PIPE_IS_AVAILABLE

// process
//   class: owns a pipe to a shell-run child process. Non-copyable on every
// tier; movable on C++11+. Read or write it like a file; close() reaps the
// child and exit_status() then reports how it ended.
class process
{
public:

    // process
    //   function: an owning-nothing handle. is_open() is false.
    process(void)
        : m_stream(0)
        , m_exit_status(0)
        , m_exited(false)
    {
    }

    // process
    //   function: run _command through the shell, with a pipe to its stdio.
    // _mode is "r" to READ the command's standard output, or "w" to WRITE to
    // its standard input. Check the result with operator bool / is_open().
    //   Read the SECURITY note in the banner before passing any _command that
    // is not a fixed literal.
    explicit process(const char* _command, const char* _mode)
        : m_stream(d_popen(_command, _mode))
        , m_exit_status(0)
        , m_exited(false)
    {
    }

    // ~process
    //   function: reap the child if still open. The exit status cannot be
    // reported from a destructor and is discarded -- call close() first when
    // the command's result matters.
    ~process(void)
    {
        if (m_stream)
        {
            (void)d_pclose(m_stream);
        }
    }

#if (D_MOVE_ENABLED == 1)
    process(process&& _other) D_NOEXCEPT
        : m_stream(_other.m_stream)
        , m_exit_status(_other.m_exit_status)
        , m_exited(_other.m_exited)
    {
        _other.m_stream = 0;
        _other.m_exited = false;
    }

    process& operator=(process&& _other) D_NOEXCEPT
    {
        if (this != &_other)
        {
            if (m_stream)
            {
                (void)d_pclose(m_stream);
            }

            m_stream      = _other.m_stream;
            m_exit_status = _other.m_exit_status;
            m_exited      = _other.m_exited;

            _other.m_stream = 0;
            _other.m_exited = false;
        }

        return *this;
    }
#endif

    // read
    //   function: read up to _n bytes of the command's output into _buf. A
    // short read at end of output is success (the command finished writing),
    // exactly as for a file; only a stream error sets _ec.
    size_t read(void* _buf, size_t _n, error& _ec)
    {
        size_t got;

        if (!m_stream)
        {
            _ec.assign(EBADF);
            return 0;
        }

        errno = 0;
        got   = std::fread(_buf, 1, _n, m_stream);

        if (got < _n && std::ferror(m_stream))
        {
            _ec = (errno != 0) ? error::from_errno() : error(EIO);
            std::clearerr(m_stream);
        }
        else
        {
            _ec.clear();
        }

        return got;
    }

    // write
    //   function: write _n bytes to the command's input. A short write is a
    // failure (there is no benign end-of-input for writing).
    size_t write(const void* _buf, size_t _n, error& _ec)
    {
        size_t put;

        if (!m_stream)
        {
            _ec.assign(EBADF);
            return 0;
        }

        errno = 0;
        put   = std::fwrite(_buf, 1, _n, m_stream);

        if (put < _n)
        {
            _ec = (errno != 0) ? error::from_errno() : error(EIO);
            std::clearerr(m_stream);
        }
        else
        {
            _ec.clear();
        }

        return put;
    }

    // flush
    //   function: push buffered output to the command (meaningful in "w" mode).
    bool flush(error& _ec)
    {
        if (!m_stream)
        {
            _ec.assign(EBADF);
            return false;
        }

        if (std::fflush(m_stream) != 0)
        {
            _ec = error::from_errno();
            return false;
        }

        _ec.clear();
        return true;
    }

    // close
    //   function: close the pipe and REAP the child. Returns true if that
    // succeeded -- after which exited() is true and exit_status() holds the
    // command's exit code. A non-zero exit code is NOT reported through _ec
    // (the command ran; its result is data): _ec is set only if the reap itself
    // failed. Closing an already-closed handle is success.
    bool close(error& _ec)
    {
        int status;

        if (!m_stream)
        {
            _ec.clear();
            return true;
        }

        status   = d_pclose(m_stream);
        m_stream = 0;

        if (status < 0)
        {
            _ec = error::from_errno();
            return false;
        }

        m_exit_status = d_pipe_exit_code(status);
        m_exited      = true;
        _ec.clear();
        return true;
    }

    // is_open
    //   function: whether a pipe is currently held.
    bool is_open(void) const
    {
        return m_stream != 0;
    }

    // operator bool
    //   function: is_open(), for `if (p)`. explicit on C++11+.
    D_EXPLICIT_BOOL operator bool(void) const
    {
        return m_stream != 0;
    }

    // native_handle
    //   function: the underlying pipe FILE*. Ownership does not transfer.
    FILE* native_handle(void) const
    {
        return m_stream;
    }

    // exited
    //   function: whether close() has run and reaped the child, so that
    // exit_status() is meaningful.
    bool exited(void) const
    {
        return m_exited;
    }

    // exit_status
    //   function: the command's exit code, valid once exited() is true. 0 is a
    // clean exit; 127 is the shell's "command not found"; 128+N is death by
    // signal N. Zero before close().
    int exit_status(void) const
    {
        return m_exit_status;
    }

private:

    FILE* m_stream;
    int   m_exit_status;
    bool  m_exited;

    // never copyable -- two owners would each reap the one child.
    D_DELETED_FN(process(const process& _other))
    D_DELETED_FN(process& operator=(const process& _other))
};

#endif // D_FILE_PIPE_IS_AVAILABLE

NS_END  // djinterp

#endif // DJINTERP_FS_FILE_PIPE_
