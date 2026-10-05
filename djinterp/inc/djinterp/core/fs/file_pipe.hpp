/*******************************************************************************
* djinterp [core]                                                  file_pipe.hpp
*
* djinterp::process -- a child process run through the shell, its standard
* input or output connected to this handle by a pipe (the popen model,
* roadmap Phase 8).
*   It is the RAII owner of that pipe: reading or writing talks to the
* command, and closing REAPS the command and yields its exit status.
*   >>> SECURITY. d_pipe_open runs its command through a SHELL -- /bin/sh -c
*   on POSIX, cmd.exe /c on Windows -- so EVERY shell metacharacter in the
*   string is interpreted. Building the command from anything a user, a file,
*   or a network peer influenced is a command-injection vulnerability, and
*   nothing in this class prevents it: the constructor hands the string
*   straight to the shell. The only fix is not to do it -- run a fixed,
*   literal command here, and reach for posix_spawn / CreateProcess with an
*   ARGUMENT VECTOR when any part of the command is variable. This is the one
*   hazard of this facility, stated where you cannot miss it. <<<
*   D4, OWNERSHIP. A pipe to a live process cannot be shared by copying, so
* process is NON-COPYABLE on every tier and MOVABLE on C++11+ -- the same rule
* file and directory follow, drawing the same spellings (D_DELETED_FN,
* D_NOEXCEPT) from the shared prelude.
*   EXIT STATUS IS NOT AN ERROR. A command that runs and exits non-zero has not
* failed as an I/O operation -- it has reported a result, and that result is
* information you asked for. So close() returns whether the pipe CLOSED and the
* process was REAPED (an _ec set only if pclose itself failed), and the
* command's exit code is read separately via exit_status(), meaningful once
* exited() is true. A missing command, a command that dies -- these surface as
* an exit status (127, 128+signal), not as a close failure.
*   AVAILABILITY. The whole facility is compiled out where the platform has no
* pipes (D_FILE_PIPE_IS_AVAILABLE == 0); on such a build djinterp::process does
* not exist, and naming it is a compile error rather than a link-time surprise.
*
*
* path:      /inc/djinterp/core/fs/file_pipe.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PROCESS
    -------
    1.  Construction and destruction
    2.  Transfer
    3.  Completion
    4.  Observers
*/

#ifndef DJINTERP_FS_FILE_PIPE_HPP
#define DJINTERP_FS_FILE_PIPE_HPP 1

// std
#include <cerrno>  // errno, EBADF, EIO
#include <cstdio>  // FILE, std::fread, std::fwrite, std::fflush, std::ferror,
                   // std::clearerr
// djinterp
#include "file_common.hpp"         // error, the D_* kit
#include "../../c/fs/file_pipe.h"  // d_pipe_open, d_pipe_close,
#include "../../env/env.h"         // D_ENV_LANG_*
                                   // d_pipe_exit_code
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


NS_DJINTERP

#if D_FILE_PIPE_IS_AVAILABLE


//==============================================================================
// 1.  PROCESS
//==============================================================================


// process
//   class: owns a pipe to a shell-run child process. Non-copyable on every
// tier; movable on C++11+. Read or write it like a file; close() reaps the
// child and exit_status() then reports how it ended.
class process
{
public:
    // 1.1    Construction and destruction
    //--------------------------------------------------------------------------
    /**
     * @brief Constructs a handle that owns nothing; is_open() is false.
     */
    process(void)
        : m_stream(0),
          m_exit_status(0),
          m_exited(false)
    {}

    /**
     * @brief Runs a command through the shell, with a pipe to its standard
     *        input or output.
     *
     * @warning Read the SECURITY note in the banner before passing any
     *          `_command` that is not a fixed literal.
     *
     * @param[in] _command  the command line, handed to the shell unchanged.
     * @param[in] _mode     "r" to READ the command's standard output, or "w"
     *                      to WRITE to its standard input.
     * @post is_open() reports whether the command could be started.
     */
    explicit process(
        const char* _command,
        const char* _mode
    )
        : m_stream(d_pipe_open(_command,
                               _mode)),
          m_exit_status(0),
          m_exited(false)
    {}

    /**
     * @brief Reaps the child, if the pipe is still open.
     *
     * @note A destructor cannot report the exit status, so it is discarded:
     *       call close() first when the command's result matters.
     */
    ~process(void)
    {
        // nothing to reap once close() has run
        if (m_stream)
        {
            (void)d_pipe_close(m_stream);
        }
    }

#if (D_MOVE_ENABLED == 1)
    /**
     * @brief Moves the pipe and its status into a new handle. C++11 and later.
     *
     * @param[in,out] _other  the handle to move from; it owns nothing
     *                        afterwards.
     */
    process(
        process&& _other
    ) D_NOEXCEPT
        : m_stream(_other.m_stream),
          m_exit_status(_other.m_exit_status),
          m_exited(_other.m_exited)
    {
        _other.m_stream = 0;
        _other.m_exited = false;
    }

    /**
     * @brief Move-assigns, reaping the child this handle held first.
     *
     * @param[in,out] _other  the handle to move from; it owns nothing
     *                        afterwards.
     * @return `*this`.
     */
    process& operator=(process&& _other) D_NOEXCEPT
    {
        // a self-move leaves the handle as it was
        if (this != &_other)
        {
            // reap the child this handle held before taking the other's
            if (m_stream)
            {
                (void)d_pipe_close(m_stream);
            }

            m_stream      = _other.m_stream;
            m_exit_status = _other.m_exit_status;
            m_exited      = _other.m_exited;

            _other.m_stream = 0;
            _other.m_exited = false;
        }

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // 1.2    Transfer
    //--------------------------------------------------------------------------
    /**
     * @brief Reads up to `_n` bytes of the command's output.
     *
     * @note A short read at the end of the output is success -- the command
     *       finished writing -- exactly as for a file; only a stream error
     *       sets `_ec`.
     *
     * @param[out] _buf  receives the bytes; holds at least `_n`.
     * @param[in]  _n    the most bytes to read.
     * @param[out] _ec   cleared on success; EBADF on a closed handle, otherwise
     *                   the platform's code, or EIO when it gave none.
     * @return the number of bytes read.
     */
    size_t read(void*  _buf,
                size_t _n,
                error& _ec)
    {
        // a closed handle has nothing to read
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return 0;
        }

        errno = 0;

        const size_t got = std::fread(_buf,
                                      1,
                                      _n,
                                      m_stream);

        // a short read is an error only when the stream says so
        if ( (got < _n) &&
             (std::ferror(m_stream)) )
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

    /**
     * @brief Writes `_n` bytes to the command's input.
     *
     * @note A short write is a failure: there is no benign end of input for
     *       writing.
     *
     * @param[in]  _buf  the bytes to write; holds at least `_n`.
     * @param[in]  _n    the number of bytes to write.
     * @param[out] _ec   cleared on success; EBADF on a closed handle, otherwise
     *                   the platform's code, or EIO when it gave none.
     * @return the number of bytes written.
     */
    size_t write(const void* _buf,
                 size_t      _n,
                 error&      _ec)
    {
        // a closed handle cannot be written
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return 0;
        }

        errno = 0;

        const size_t put = std::fwrite(_buf,
                                       1,
                                       _n,
                                       m_stream);

        // anything short of the whole buffer is a failure
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

    /**
     * @brief Pushes buffered output to the command; meaningful in "w" mode.
     *
     * @param[out] _ec  cleared on success; EBADF on a closed handle,
     *                  otherwise the platform's code.
     * @return true on success.
     */
    bool flush(error& _ec)
    {
        // a closed handle has nothing to flush
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        // stdio reports its own failure through errno
        if (std::fflush(m_stream) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // 1.3    Completion
    //--------------------------------------------------------------------------
    /**
     * @brief Closes the pipe and REAPS the child.
     *
     * @note A non-zero exit code is NOT reported through `_ec` -- the command
     *       ran, and its result is data -- so `_ec` is set only if the reap
     *       itself failed. Closing an already-closed handle is success.
     *
     * @param[out] _ec  cleared on success; set only when the reap failed.
     * @return true when the pipe closed and the child was reaped.
     * @post On success, exited() is true and exit_status() holds the
     *       command's exit code. The pipe is released either way.
     */
    bool close(error& _ec)
    {
        // closing twice is not an error
        if (!m_stream)
        {
            _ec.clear();

            return true;
        }

        const int status = d_pipe_close(m_stream);

        m_stream = 0;

        // only the reap itself can fail here
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

    // 1.4    Observers
    //--------------------------------------------------------------------------
    /**
     * @brief Reports whether a pipe is currently held.
     *
     * @return true while the handle owns a pipe.
     */
    bool is_open(void) const
    {
        return m_stream != 0;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    /**
     * @brief is_open(), for `if (p)`; from C++11, as an explicit
     *        conversion; below, is_open() is the spelling (decision 3.2).
     *
     * @return true while the handle owns a pipe.
     */
    D_EXPLICIT_BOOL operator bool(void) const
    {
        return m_stream != 0;
    }
#endif

    /**
     * @brief Returns the underlying pipe stream.
     *
     * @return the FILE*, or `NULL` when closed; ownership does not transfer.
     */
    FILE* native_handle(void) const
    {
        return m_stream;
    }

    /**
     * @brief Reports whether close() has run and reaped the child, so that
     *        exit_status() is meaningful.
     *
     * @return true after a successful close().
     */
    bool exited(void) const
    {
        return m_exited;
    }

    /**
     * @brief Reports the command's exit code, valid once exited() is true.
     *
     * @return 0 for a clean exit, 127 for the shell's "command not found",
     *         128 + N for death by signal N; 0 before close().
     */
    int exit_status(void) const
    {
        return m_exit_status;
    }

private:
    // never copyable -- two owners would each reap the one child
    D_DELETED_FN(process(const process& _other))
    D_DELETED_FN(process& operator=(const process& _other))

    FILE* m_stream;
    int   m_exit_status;
    bool  m_exited;
};

#endif  // D_FILE_PIPE_IS_AVAILABLE


NS_END  // djinterp

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_PIPE_HPP
