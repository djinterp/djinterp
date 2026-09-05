/******************************************************************************
* djinterp [core]                                              file_stream.hpp
*
*   djinterp::file -- an open file, owned. This is the RAII core of the C++
* file layer (roadmap Phase 3), and it makes the two decisions the roadmap
* reserved for exactly this class.
*
*   D4, OWNERSHIP. A file owns a FILE*, and a FILE* cannot be shared by
* copying -- two owners would each close it. So `file` is NON-COPYABLE on
* every tier, and MOVABLE on C++11+. The move is ADDED on C++11 (D1), never
* substituted: a C++98 caller constructs in place (`file f(p, "rb");`) and
* passes by reference, and that same source compiles on C++11 unchanged. The
* copy operations are deleted, not merely private-and-undefined-with-a-comment
* -- see D_DELETED_FN.
*
*   D3, ERROR CHANNEL. Every operation reports through an `error& _ec`
* out-parameter and a bool/size_t return -- the return says whether it worked,
* _ec says why not. There is no throwing overload here yet; that is a later,
* additive tier (D1), and valid()-style state plus the return value already
* carry the news on every standard. The constructor is the one operation that
* cannot take an _ec (constructors have no return), so a failed open leaves
* is_open() false and the reason unreadable -- use open(p, mode, ec) on a
* default-constructed file when you need the reason.
*
*   ONE BUFFERING MODEL. `file` is the stdio wrapper: it owns a FILE* and does
* byte I/O through stdio (fread/fwrite), because mixing buffered stdio with
* raw-fd reads on the same descriptor desynchronizes the two. The fd-based
* c/fs byte functions (d_read/d_write) are for a caller who opened a raw fd --
* a different tool. seek/tell/truncate/sync here call the FILE*-taking c/fs
* functions (d_fseeko / d_ftello / d_ftruncate_stream / d_fsync_stream), which
* stay on the same side of the buffer.
*
*   NO OS ANYWHERE. There is not one `#if defined(_WIN32)` in this file. Every
* platform decision was already made, once, in c/fs. Each method is the same
* three steps: reject a bad handle, call the C function, translate errno.
*
*
* path:      /inc/djinterp/core/fs/file_stream.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    LANGUAGE SUPPORT       D_MOVE_ENABLED / _NOEXCEPT /
                             _EXPLICIT_BOOL / _DELETED_FN
I.    CONSTRUCTION / MOVE    ctor, open-ctor, dtor, move (11+)
II.   I/O                    read / write   (stdio; EOF is not an error)
III.  POSITIONING            tell / seek / truncate
IV.   DURABILITY             sync / flush
V.    ADVISORY LOCKING       lock_shared / lock_exclusive / try_* / unlock
VI.   LIFETIME               open / open_temp / close
VII.  OBSERVERS              is_open / operator bool / native_handle /
                             descriptor / status
      non-copyable declarations
*/

#ifndef DJINTERP_FS_FILE_STREAM_
#define DJINTERP_FS_FILE_STREAM_ 1

// std
#include <cerrno>                  // errno, EBADF, EINVAL, EIO
// FILE, fread, fwrite, feof, ferror, clearerr
#include <cstdio>
// djinterp
#include "file_path.hpp"
#include "file_common.hpp"
#include "file_stat.hpp"
#include "../../c/fs/file_open.h"     // d_fopen, d_fclose
#include "../../c/fs/file_io.h"       // (whole-file helpers; byte I/O is stdio)
#include "../../c/fs/file_seek.h"     // d_fseeko, d_ftello, d_ftruncate_stream
#include "../../c/fs/file_sync.h"     // d_fsync_stream, d_fflush
#include "../../c/fs/file_lock.h"     // d_flock_stream  (advisory locking)
#include "../../c/fs/file_desc.h"     // d_fileno        (borrow the descriptor)
#include "../../c/fs/file_stat.h"     // d_fstat         (metadata for this fd)
#include "../../c/fs/file_temp.h"     // d_tmpfile       (anonymous temp file)


// 0.    Language support
//
//   The move/noexcept/explicit/deleted spellings come from djinterp.hpp
// (included via file_path.hpp above, and directly here for clarity):
// D_MOVE_ENABLED,
// D_NOEXCEPT, D_EXPLICIT_BOOL, D_DELETED_FN. file was one of the three headers
// that carried a private copy of this kit (D_FILE_*) as a stopgap; this is the
// promoted, single-source version -- one spelling every C++ module shares,
// rather than a fourth re-derivation waiting to drift.



NS_DJINTERP

// file
//   class: owns an open file. Non-copyable on every tier; movable on C++11+.
class file
{
public:

    // -----------------------------------------------------------------------
    // I.   CONSTRUCTION / MOVE
    // -----------------------------------------------------------------------

    // file
    //   function: a file that owns nothing. is_open() is false until open().
    file(void)
        : m_stream(0)
    {}

    // file
    //   function: open _p with mode _mode. The convenient form -- check the
    // result with operator bool or is_open(). It cannot report WHY an open
    // failed (a constructor has no return and this takes no _ec); when the
    // reason matters, default-construct and call open(p, mode, ec).
    //
    //   explicit: a path and a mode string do not add up to a file by
    // accident, and an implicit conversion from `path` to `file` (which would
    // OPEN it) is exactly the kind of surprise `explicit` exists to stop.
    explicit file(const path& _p, const char* _mode)
        : m_stream(_p.valid() ? d_fopen(_p.c_str(), _mode) : 0)
    {}

    // ~file
    //   function: close what is owned. A close error cannot be reported from a
    // destructor, so it is dropped -- a caller who needs to KNOW the final
    // flush succeeded calls close(ec) explicitly before the object dies, which
    // is the whole reason close() is also a named method.
    ~file(void)
    {
        if (m_stream)
        {
            (void)d_fclose(m_stream);
        }
    }

#if (D_MOVE_ENABLED == 1)
    // file
    //   function: move. C++11+ only, ADDITIVE. Takes the other's stream and
    // leaves it owning nothing, so exactly one object closes the file.
    file(file&& _other) D_NOEXCEPT
        : m_stream(_other.m_stream)
    {
        _other.m_stream = 0;
    }

    // operator=
    //   function: move-assign. Closes what this held first -- dropping it
    // would leak -- then takes the other's stream.
    file& operator=(file&& _other) D_NOEXCEPT
    {
        if (this != &_other)
        {
            if (m_stream)
            {
                (void)d_fclose(m_stream);
            }

            m_stream        = _other.m_stream;
            _other.m_stream = 0;
        }

        return *this;
    }
#endif

    // -----------------------------------------------------------------------
    // II.  I/O   (stdio; a short read at end-of-file is NOT an error)
    // -----------------------------------------------------------------------

    // read
    //   function: read up to _n bytes into _buf, returning the count. Fewer
    // than _n at end of file is success -- _ec stays clear and the short count
    // is the news. Only a stream error sets _ec. (stdio does not promise errno
    // on ferror, so a generic EIO stands in when errno is not live.)
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

    // write
    //   function: write _n bytes from _buf, returning the count written. A
    // short write is always a failure -- unlike reading, there is no benign
    // end-of-file to hit.
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

    // -----------------------------------------------------------------------
    // III. POSITIONING
    // -----------------------------------------------------------------------

    // tell
    //   function: the current offset, or -1 on failure with _ec set.
    d_off_t tell(error& _ec) const
    {
        d_off_t pos;

        if (!m_stream)
        {
            _ec.assign(EBADF);

            return (d_off_t)-1;
        }

        pos = d_ftello(m_stream);

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

    // seek
    //   function: move to _off relative to _whence (SEEK_SET/CUR/END).
    bool seek(d_off_t _off, int _whence, error& _ec)
    {
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        if (d_fseeko(m_stream, _off, _whence) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // truncate
    //   function: set the file's length to _length bytes.
    bool truncate(d_off_t _length, error& _ec)
    {
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        if (d_ftruncate_stream(m_stream, _length) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // -----------------------------------------------------------------------
    // IV.  DURABILITY
    // -----------------------------------------------------------------------

    // sync
    //   function: force this file's data to the storage device. Stronger than
    // flush -- flush pushes stdio's buffer to the OS, sync pushes the OS's
    // buffer to the disk.
    bool sync(error& _ec)
    {
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        if (d_fsync_stream(m_stream) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // flush
    //   function: push stdio's buffer to the OS. Does not reach the disk -- see
    // sync.
    bool flush(error& _ec)
    {
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        if (d_fflush(m_stream) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // -----------------------------------------------------------------------
    // V.   ADVISORY LOCKING
    // -----------------------------------------------------------------------
    //
    //   Advisory: these locks bind only processes that ALSO call them. They do
    // not stop a process that never locks from reading or writing the file --
    // that is what "advisory" means, and it is the only kind POSIX offers
    // portably. A lock is released by unlock() or when the file closes.
    //   The blocking forms wait for the lock; the try_ forms do not -- they
    // return false immediately when the lock is held elsewhere, and in THAT
    // case _ec is EWOULDBLOCK/EAGAIN, which is how a caller tells an honest
    // contention from a real error.

    // lock_shared
    //   function: take a shared (read) lock, blocking until it is available.
    // Many holders may share it at once.
    bool lock_shared(error& _ec)
    {
        return lock_op(D_LOCK_SH, _ec);
    }

    // lock_exclusive
    //   function: take an exclusive (write) lock, blocking until it is
    // available. No other holder, shared or exclusive, may coexist with it.
    bool lock_exclusive(error& _ec)
    {
        return lock_op(D_LOCK_EX, _ec);
    }

    // try_lock_shared
    //   function: the non-blocking shared lock. false + EWOULDBLOCK means held
    // elsewhere, not broken.
    bool try_lock_shared(error& _ec)
    {
        return lock_op(D_LOCK_SH | D_LOCK_NB, _ec);
    }

    // try_lock_exclusive
    //   function: the non-blocking exclusive lock. false + EWOULDBLOCK means
    // held elsewhere, not broken.
    bool try_lock_exclusive(error& _ec)
    {
        return lock_op(D_LOCK_EX | D_LOCK_NB, _ec);
    }

    // unlock
    //   function: release a lock this file holds.
    bool unlock(error& _ec)
    {
        return lock_op(D_LOCK_UN, _ec);
    }

    // -----------------------------------------------------------------------
    // VI.  LIFETIME
    // -----------------------------------------------------------------------

    // open
    //   function: open _p with mode _mode, reporting the reason on failure.
    // Re-opening a file that is already open drops the old stream first (its
    // close error is not surfaced -- a caller who cares closes explicitly
    // before re-opening). An invalid path is rejected as EINVAL rather than
    // handed to d_fopen as "".
    bool open(const path& _p, const char* _mode, error& _ec)
    {
        if (!_p.valid())
        {
            _ec.assign(EINVAL);

            return false;
        }

        if (m_stream)
        {
            (void)d_fclose(m_stream);
            m_stream = 0;
        }

        m_stream = d_fopen(_p.c_str(), _mode);

        if (!m_stream)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // open_temp
    //   function: acquire an ANONYMOUS temporary file into this handle. It has
    // no name in the filesystem and is deleted automatically the moment it
    // closes (via close() or the destructor), so it leaves nothing behind and
    // there is no window in which another process could open it by name -- the
    // safe kind of scratch space. Re-acquiring drops any file already held,
    // like open().
    bool open_temp(error& _ec)
    {
        FILE* stream = d_tmpfile();

        if (!stream)
        {
            _ec = error::from_errno();

            return false;
        }

        if (m_stream)
        {
            (void)d_fclose(m_stream);
        }

        m_stream = stream;
        _ec.clear();

        return true;
    }

    // close
    //   function: close what is owned. Closing a file that is not open is
    // success, not an error -- the postcondition (nothing owned) already
    // holds. After this, is_open() is false whether or not the close reported
    // a flush failure, because the stream is gone either way.
    bool close(error& _ec)
    {
        int rc;

        if (!m_stream)
        {
            _ec.clear();

            return true;
        }

        rc       = d_fclose(m_stream);
        m_stream = 0;

        if (rc != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    // -----------------------------------------------------------------------
    // VII. OBSERVERS
    // -----------------------------------------------------------------------

    // is_open
    //   function: whether this owns an open stream. The spelling that works on
    // every tier; operator bool is the C++11+ sugar for it.
    bool is_open(void) const
    {
        return m_stream != 0;
    }

    // operator bool
    //   function: is_open(), for `if (f)`. explicit on C++11+ so a file does
    // not silently become an int in arithmetic; on C++98 the keyword is empty
    // and is_open() is the safe form to prefer.
    D_EXPLICIT_BOOL operator bool(void) const
    {
        return m_stream != 0;
    }

    // native_handle
    //   function: the underlying FILE*, for the caller who must reach a C API
    // this class does not wrap. Ownership does not transfer -- the file still
    // closes it. (Phase 4 adds a descriptor accessor for the fd-based C side.)
    FILE* native_handle(void) const
    {
        return m_stream;
    }

    // descriptor
    //   function: BORROW the file's descriptor, for an fd-level C call this
    // class does not wrap -- fstat, an fd-based lock, fcntl. The file still
    // owns it: do NOT close the returned fd, and do NOT do raw read/write on it
    // while the stream holds buffered data, or the two views desynchronize
    // (the reason this class does its own I/O through stdio). Returns -1 with
    // _ec set on a closed file.
    int descriptor(error& _ec) const
    {
        int fd;

        if (!m_stream)
        {
            _ec.assign(EBADF);

            return -1;
        }

        fd = d_fileno(m_stream);

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

    // status
    //   function: metadata for THIS open file, via fstat on its descriptor.
    // Unlike the free status(path), this cannot be raced -- the descriptor
    // names one file for its whole lifetime, so what comes back describes the
    // very bytes you are reading, not whatever the path resolves to a moment
    // later. The TOCTOU-free query file_stat.h points to. Returns an empty
    // (type_none) status with _ec set on a closed file.
    file_status status(error& _ec) const
    {
        struct d_stat_t buf;

        if (!m_stream)
        {
            _ec.assign(EBADF);

            return file_status();
        }

        if (d_fstat(d_fileno(m_stream), &buf) != 0)
        {
            _ec = error::from_errno();

            return file_status();
        }

        _ec.clear();

        return file_status(buf);
    }

private:

    // lock_op
    //   function: the shared body of the five lock methods -- reject a closed
    // handle, apply the operation, translate errno. One place so the validate/
    // call/translate shape is written once, not five times.
    bool lock_op(int _operation, error& _ec)
    {
        if (!m_stream)
        {
            _ec.assign(EBADF);

            return false;
        }

        if (d_flock_stream(m_stream, _operation) != 0)
        {
            _ec = error::from_errno();

            return false;
        }

        _ec.clear();

        return true;
    }

    FILE* m_stream;

    // never copyable -- two owners would double-close. See D_DELETED_FN.
    D_DELETED_FN(file(const file& _other))
    D_DELETED_FN(file& operator=(const file& _other))
};

NS_END  // djinterp

#endif  // DJINTERP_FS_FILE_STREAM_
