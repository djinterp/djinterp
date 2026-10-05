/*******************************************************************************
* djinterp [core]                                                file_stream.hpp
*
* djinterp::file -- an open file, owned.
*   This is the RAII core of the C++ file layer (roadmap Phase 3), and it makes
* the two decisions the roadmap reserved for exactly this class.
*   D4, OWNERSHIP. A file owns a FILE*, and a FILE* cannot be shared by copying
* -- two owners would each close it. So `file` is NON-COPYABLE on every tier,
* and MOVABLE on C++11+. The move is ADDED on C++11 (D1), never substituted: a
* C++98 caller constructs in place (`file f(p, "rb");`) and passes by
* reference, and that same source compiles on C++11 unchanged. The copy
* operations are deleted, not merely private-and-undefined-with-a-comment --
* see D_DELETED_FN.
*   D3, ERROR CHANNEL. Every operation reports through an `error& _ec`
* out-parameter and a bool/size_t return -- the return says whether it worked,
* _ec says why not. There is no throwing overload here yet; that is a later,
* additive tier (D1), and valid()-style state plus the return value already
* carry the news on every standard. The constructor is the one operation that
* cannot take an _ec (constructors have no return), so a failed open leaves
* is_open() false and the reason unreadable -- use open(p, mode, ec) on a
* default-constructed file when you need the reason.
*   ONE BUFFERING MODEL. `file` is the stdio wrapper: it owns a FILE* and does
* byte I/O through stdio (fread/fwrite), because mixing buffered stdio with
* raw-fd reads on the same descriptor desynchronizes the two. The fd-based c/fs
* byte functions (d_file_read_fd/d_file_write_fd) are for a caller who opened
* a raw fd -- a different tool. seek/tell/truncate/sync here call the
* FILE*-taking c/fs functions (d_file_seek_stream / d_file_tell_stream /
* d_file_truncate_stream / d_file_sync_stream), which stay on the same side of
* the buffer.
*   NO OS ANYWHERE. There is not one `#if defined(_WIN32)` in this file. Every
* platform decision was already made, once, in c/fs. Each method is the same
* three steps: reject a bad handle, call the C function, translate errno.
*
*
* path:      /inc/djinterp/core/fs/file_stream.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FILE
    ----
    1.  Construction and destruction
    2.  Transfer
    3.  Positioning
    4.  Durability
    5.  Advisory locking
    6.  Lifetime
    7.  Observers
    8.  Implementation
*/

#ifndef DJINTERP_FS_FILE_STREAM_HPP
#define DJINTERP_FS_FILE_STREAM_HPP 1

// std
#include <cerrno>  // errno, EBADF, EINVAL, EIO
#include <cstdio>  // FILE, std::fread, std::fwrite, std::ferror,
                   // std::clearerr
// djinterp
#include "file_path.hpp"           // path
#include "file_common.hpp"         // error, the D_* kit
#include "file_stat.hpp"           // file_status
#include "../../c/fs/file_open.h"  // d_file_open_stream, d_file_close_stream
#include "../../c/fs/file_io.h"    // the whole-file helpers; byte I/O here
#include "../../env/env.h"         // D_ENV_LANG_*
                                   // is stdio
#include "../../c/fs/file_seek.h"  // d_file_seek_stream, d_file_tell_stream,
                                   // d_file_truncate_stream
#include "../../c/fs/file_sync.h"  // d_file_sync_stream, d_file_flush_stream
#include "../../c/fs/file_lock.h"  // d_file_lock_stream
#include "../../c/fs/file_desc.h"  // d_file_descriptor_stream
#include "../../c/fs/file_stat.h"  // d_file_stat_fd
#include "../../c/fs/file_temp.h"  // d_file_temp_stream
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


NS_DJINTERP


//==============================================================================
// 1.  FILE
//==============================================================================
// The move/noexcept/explicit/deleted spellings -- D_MOVE_ENABLED, D_NOEXCEPT,
// D_EXPLICIT_BOOL, D_DELETED_FN -- come from djinterp.hpp, through
// file_common.hpp. file was one of the three headers that carried a private
// copy of that kit (D_FILE_*) as a stopgap; this is the promoted,
// single-source version, one spelling every C++ module shares rather than a
// fourth re-derivation waiting to drift.


// file
//   class: owns an open file. Non-copyable on every tier; movable on C++11+.
class file
{
public:
    // 1.1    Construction and destruction
    //--------------------------------------------------------------------------
    /**
     * @brief Constructs a file that owns nothing; is_open() is false until
     *        open().
     */
    file(void)
        : m_stream(0)
    {}

    /**
     * @brief Opens a file: the convenient form.
     *
     * @note It cannot report WHY an open failed -- a constructor has no return
     *       and this takes no `_ec` -- so when the reason matters,
     *       default-construct and call open(p, mode, ec). explicit, because a
     *       path and a mode string do not add up to a file by accident, and an
     *       implicit conversion that OPENED something is exactly the surprise
     *       explicit exists to stop.
     *
     * @param[in] _p     the file to open.
     * @param[in] _mode  an fopen mode.
     * @post is_open() reports whether the open succeeded.
     */
    explicit file(
        const path& _p,
        const char* _mode
    )
        : m_stream(_p.valid() ? d_file_open_stream(_p.c_str(),
                                                   _mode)
                              : 0)
    {}

    /**
     * @brief Closes what is owned.
     *
     * @note A destructor cannot report a close error, so it is dropped. A
     *       caller who needs to KNOW the final flush succeeded calls close(ec)
     *       before the object dies -- the whole reason close() is also a named
     *       method.
     */
    ~file(void)
    {
        // nothing to close once close() has run
        if (m_stream)
        {
            (void)d_file_close_stream(m_stream);
        }
    }

#if (D_MOVE_ENABLED == 1)
    /**
     * @brief Moves the stream into a new file, so exactly one object closes
     *        it. C++11 and later, ADDITIVE.
     *
     * @param[in,out] _other  the file to move from; it owns nothing
     *                        afterwards.
     */
    file(
        file&& _other
    ) D_NOEXCEPT
        : m_stream(_other.m_stream)
    {
        _other.m_stream = 0;
    }

    /**
     * @brief Move-assigns: closes what this held first -- dropping it would
     *        leak -- then takes the other's stream.
     *
     * @param[in,out] _other  the file to move from; it owns nothing
     *                        afterwards.
     * @return `*this`.
     */
    file& operator=(file&& _other) D_NOEXCEPT
    {
        // a self-move leaves the file as it was
        if (this != &_other)
        {
            // close what this file held before taking the other's stream
            if (m_stream)
            {
                (void)d_file_close_stream(m_stream);
            }

            m_stream        = _other.m_stream;
            _other.m_stream = 0;
        }

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // 1.2    Transfer
    //--------------------------------------------------------------------------
    /**
     * @brief Reads up to `_n` bytes through stdio.
     *
     * @note Fewer than `_n` at end of file is success: `_ec` stays clear and
     *       the short count is the news. Only a stream error sets `_ec`, and
     *       since stdio does not promise errno on ferror, EIO stands in when
     *       errno is not live.
     *
     * @param[out] _buf  receives the bytes; holds at least `_n`.
     * @param[in]  _n    the most bytes to read.
     * @param[out] _ec   cleared on success; EBADF on a closed file, otherwise
     *                   the platform's code or EIO.
     * @return the number of bytes read.
     */
    size_t read(void*  _buf,
                size_t _n,
                error& _ec)
    {
        // a closed file has nothing to read
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
     * @brief Writes `_n` bytes through stdio.
     *
     * @note A short write is always a failure: unlike reading, there is no
     *       benign end of file to hit.
     *
     * @param[in]  _buf  the bytes to write; holds at least `_n`.
     * @param[in]  _n    the number of bytes to write.
     * @param[out] _ec   cleared on success; EBADF on a closed file, otherwise
     *                   the platform's code or EIO.
     * @return the number of bytes written.
     */
    size_t write(const void* _buf,
                 size_t      _n,
                 error&      _ec)
    {
        // a closed file cannot be written
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

    // 1.3    Positioning
    //--------------------------------------------------------------------------
    /**
     * @brief Reports the current offset.
     *
     * @param[out] _ec  cleared on success; EBADF on a closed file, otherwise
     *                  the platform's code.
     * @return the offset, or -1 on failure.
     */
    d_off_t tell(error& _ec) const
    {
        // a closed file has no position
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return (d_off_t)-1;
        }

        const d_off_t pos = d_file_tell_stream(m_stream);

        // c/fs reports a failed tell through errno
        if (pos < 0)
        {
            _ec = error::from_errno();
        }
        else
        {
            _ec.clear();
        }

        return pos;
    }

    /**
     * @brief Moves to an offset relative to an origin.
     *
     * @param[in]  _off     the offset.
     * @param[in]  _whence  SEEK_SET, SEEK_CUR or SEEK_END.
     * @param[out] _ec      cleared on success; EBADF on a closed file,
     *                      otherwise the platform's code.
     * @return true on success.
     */
    bool seek(d_off_t _off,
              int     _whence,
              error&  _ec)
    {
        // a closed file cannot move
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        // c/fs reports a failed seek through errno
        if (d_file_seek_stream(m_stream,
                               _off,
                               _whence) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    /**
     * @brief Sets the file's length.
     *
     * @param[in]  _length  the new length, in bytes.
     * @param[out] _ec      cleared on success; EBADF on a closed file,
     *                      otherwise the platform's code.
     * @return true on success.
     */
    bool truncate(d_off_t _length,
                  error&  _ec)
    {
        // a closed file cannot be truncated
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        // c/fs flushes stdio first, then truncates
        if (d_file_truncate_stream(m_stream,
                                   _length) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // 1.4    Durability
    //--------------------------------------------------------------------------
    /**
     * @brief Forces this file's data to the storage device.
     *
     * @note Stronger than flush: flush pushes stdio's buffer to the OS, sync
     *       pushes the OS's buffer to the disk.
     *
     * @param[out] _ec  cleared on success; EBADF on a closed file, otherwise
     *                  the platform's code.
     * @return true on success.
     */
    bool sync(error& _ec)
    {
        // a closed file has nothing to sync
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        // c/fs reports a failed sync through errno
        if (d_file_sync_stream(m_stream) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    /**
     * @brief Pushes stdio's buffer to the OS; it does not reach the disk --
     *        see sync().
     *
     * @param[out] _ec  cleared on success; EBADF on a closed file, otherwise
     *                  the platform's code.
     * @return true on success.
     */
    bool flush(error& _ec)
    {
        // a closed file has nothing to flush
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        // c/fs reports a failed flush through errno
        if (d_file_flush_stream(m_stream) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // 1.5    Advisory locking
    //--------------------------------------------------------------------------
    //   Advisory: these locks bind only processes that ALSO call them. They do
    // not stop a process that never locks from reading or writing the file --
    // that is what "advisory" means, and it is the only kind POSIX offers
    // portably. A lock is released by unlock() or when the file closes.
    //   The blocking forms wait for the lock; the try_ forms do not -- they
    // return false immediately when the lock is held elsewhere, and in THAT
    // case _ec is EWOULDBLOCK or EAGAIN, which is how a caller tells an honest
    // contention from a real error. Every form returns true on success, and
    // sets _ec to EBADF on a closed file.

    /**
     * @brief Takes a shared (read) lock, blocking until it is available; many
     *        holders may share it at once.
     *
     * @param[out] _ec  cleared on success; set otherwise.
     * @return true on success.
     */
    bool lock_shared(error& _ec)
    {
        return lock_op(D_LOCK_SH,
                       _ec);
    }

    /**
     * @brief Takes an exclusive (write) lock, blocking until it is available;
     *        no other holder, shared or exclusive, may coexist with it.
     *
     * @param[out] _ec  cleared on success; set otherwise.
     * @return true on success.
     */
    bool lock_exclusive(error& _ec)
    {
        return lock_op(D_LOCK_EX,
                       _ec);
    }

    /**
     * @brief Takes a shared lock without waiting.
     *
     * @param[out] _ec  cleared on success; EWOULDBLOCK or EAGAIN when the lock
     *                  is held elsewhere, which is contention, not breakage.
     * @return true on success.
     */
    bool try_lock_shared(error& _ec)
    {
        return lock_op(D_LOCK_SH | D_LOCK_NB,
                       _ec);
    }

    /**
     * @brief Takes an exclusive lock without waiting.
     *
     * @param[out] _ec  cleared on success; EWOULDBLOCK or EAGAIN when the lock
     *                  is held elsewhere, which is contention, not breakage.
     * @return true on success.
     */
    bool try_lock_exclusive(error& _ec)
    {
        return lock_op(D_LOCK_EX | D_LOCK_NB,
                       _ec);
    }

    /**
     * @brief Releases a lock this file holds.
     *
     * @param[out] _ec  cleared on success; set otherwise.
     * @return true on success.
     */
    bool unlock(error& _ec)
    {
        return lock_op(D_LOCK_UN,
                       _ec);
    }

    // 1.6    Lifetime
    //--------------------------------------------------------------------------
    /**
     * @brief Opens a file, reporting the reason on failure.
     *
     * @note Re-opening a file that is already open drops the old stream first,
     *       without surfacing its close error -- a caller who cares closes
     *       explicitly before re-opening. An invalid path is rejected as
     *       EINVAL rather than handed to d_file_open_stream as "".
     *
     * @param[in]  _p     the file to open.
     * @param[in]  _mode  an fopen mode.
     * @param[out] _ec    cleared on success; EINVAL for an invalid path,
     *                    otherwise the platform's code.
     * @return true on success.
     * @post Any stream this file held before is closed, whatever the result.
     */
    bool open(const path& _p,
              const char* _mode,
              error&      _ec)
    {
        // an invalid path names nothing to open
        if (!_p.valid())
        {
            _ec.assign(EINVAL);

            return false;
        }

        // drop the stream this file already holds
        if (m_stream)
        {
            (void)d_file_close_stream(m_stream);
            m_stream = 0;
        }

        m_stream = d_file_open_stream(_p.c_str(),
                                      _mode);

        // c/fs reports why the open failed through errno
        if (!m_stream)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    /**
     * @brief Acquires an ANONYMOUS temporary file into this handle.
     *
     * @note It has no name in the filesystem and is deleted automatically the
     *       moment it closes, so it leaves nothing behind and no other process
     *       can open it by name -- the safe kind of scratch space. Like open(),
     *       re-acquiring drops any file already held.
     *
     * @param[out] _ec  cleared on success; the platform's code otherwise.
     * @return true on success.
     */
    bool open_temp(error& _ec)
    {
        FILE* const stream = d_file_temp_stream();

        // c/fs reports why no temporary could be made through errno
        if (!stream)
        {
            _ec = error::from_errno();

            return false;
        }

        // drop the stream this file already holds
        if (m_stream)
        {
            (void)d_file_close_stream(m_stream);
        }

        m_stream = stream;
        _ec.clear();

        return true;
    }

    /**
     * @brief Closes what is owned.
     *
     * @note Closing a file that is not open is success -- the postcondition,
     *       nothing owned, already holds.
     *
     * @param[out] _ec  cleared on success; set when the final flush or close
     *                  failed.
     * @return true on success.
     * @post is_open() is false whether or not the close reported a failure,
     *       because the stream is gone either way.
     */
    bool close(error& _ec)
    {
        // closing twice is not an error
        if (!m_stream)
        {
            _ec.clear();

            return true;
        }

        const int rc = d_file_close_stream(m_stream);

        m_stream = 0;

        // the close is where a buffered write finally reports failure
        if (rc != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // 1.7    Observers
    //--------------------------------------------------------------------------
    /**
     * @brief Reports whether this owns an open stream: the spelling that works
     *        on every tier.
     *
     * @return true while a stream is owned.
     */
    bool is_open(void) const
    {
        return m_stream != 0;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    /**
     * @brief is_open(), for `if (f)`.
     *
     * @note From C++11, as an explicit conversion, so a file does not
     *       silently become an int in arithmetic; below, is_open() is the
     *       spelling (decision 3.2).
     *
     * @return true while a stream is owned.
     */
    D_EXPLICIT_BOOL operator bool(void) const
    {
        return m_stream != 0;
    }
#endif

    /**
     * @brief Returns the underlying FILE*, for a C API this class does not
     *        wrap.
     *
     * @return the stream, or `NULL` when closed; ownership does not transfer,
     *         and the file still closes it.
     */
    FILE* native_handle(void) const
    {
        return m_stream;
    }

    /**
     * @brief BORROWS the file's descriptor, for an fd-level C call this class
     *        does not wrap -- fstat, an fd-based lock, fcntl.
     *
     * @warning The file still owns it: do NOT close the returned descriptor,
     *          and do NOT do raw read/write on it while the stream holds
     *          buffered data, or the two views desynchronize -- the reason this
     *          class does its own I/O through stdio.
     *
     * @param[out] _ec  cleared on success; EBADF on a closed file, otherwise
     *                  the platform's code.
     * @return the descriptor, or -1 on failure.
     */
    int descriptor(error& _ec) const
    {
        // a closed file has no descriptor
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return -1;
        }

        const int fd = d_file_descriptor_stream(m_stream);

        // c/fs reports a failed lookup through errno
        if (fd < 0)
        {
            _ec = error::from_errno();
        }
        else
        {
            _ec.clear();
        }

        return fd;
    }

    /**
     * @brief Retrieves the metadata of THIS open file, via fstat on its
     *        descriptor.
     *
     * @note Unlike the free status(path), this cannot be raced: the descriptor
     *       names one file for its whole lifetime, so what comes back
     *       describes the very bytes you are reading, not whatever the path
     *       resolves to a moment later -- the TOCTOU-free query file_stat.h
     *       points to.
     *
     * @param[out] _ec  cleared on success; EBADF on a closed file, otherwise
     *                  the platform's code.
     * @return the snapshot, or an empty (type_none) status on failure.
     */
    file_status status(error& _ec) const
    {
        // a closed file has nothing to describe
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return file_status();
        }

        struct d_stat_t buf;

        // c/fs reports a failed fstat through errno
        if (d_file_stat_fd(d_file_descriptor_stream(m_stream),
                           &buf) != 0)
        {
            _ec = error::from_errno();

            return file_status();
        }

        _ec.clear();

        return file_status(buf);
    }

private:
    // 1.8    Implementation
    //--------------------------------------------------------------------------
    /**
     * @brief The shared body of the five lock methods: reject a closed
     *        handle, apply the operation, translate errno -- written once, not
     *        five times.
     *
     * @param[in]  _operation  a D_LOCK_* operation, optionally with D_LOCK_NB.
     * @param[out] _ec         cleared on success; EBADF on a closed file,
     *                         otherwise the platform's code.
     * @return true on success.
     */
    bool lock_op(int    _operation,
                 error& _ec)
    {
        // a closed file cannot be locked
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        // c/fs reports a refused or failed lock through errno
        if (d_file_lock_stream(m_stream,
                               _operation) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // never copyable -- two owners would double-close; see D_DELETED_FN
    D_DELETED_FN(file(const file& _other))
    D_DELETED_FN(file& operator=(const file& _other))

    FILE* m_stream;
};


NS_END  // djinterp

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_STREAM_HPP
