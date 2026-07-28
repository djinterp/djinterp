/******************************************************************************
* djinterp [core]                                         dfile_descriptor.hpp
*
* Raw file-descriptor I/O, locking, and pipes for the djinterp C++ toolkit.
*   This is the C++ counterpart to dfile_descriptor.h. It provides two
* move-only RAII owners: descriptor, which wraps a raw int file descriptor
* (open/read/write/dup/lock/sync/truncate), and pipe, which wraps a process
* pipe opened with popen and closed with pclose. Both forward inline to the
* extern "C" implementation.
*   The fd-level sync and truncate operations are declared by the C stream
* module (where they sit alongside their FILE* variants), so this header also
* includes dfile_stream.h to reach d_fsync and d_ftruncate.
*
*
* path:      /inc/djinterp/cpp/io/file/dfile_descriptor.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    class descriptor
      ---------------
      1.  construction / destruction (open, adopt, move)
      2.  observers (is_open, get)
      3.  ownership (release, reset)
      4.  I/O (read, write, duplicate, duplicate_to, lock, sync, truncate)

II.   class pipe
      ---------
      1.  construction / destruction (popen, adopt, move)
      2.  observers (is_open, get)
      3.  ownership (release, reset, close)
      4.  I/O (read, write)
*/

#ifndef DJINTERP_FILE_DESCRIPTOR_
#define DJINTERP_FILE_DESCRIPTOR_ 1

#include "../../../c/io/file/dfile_descriptor.h"
#include "../../../c/io/file/dfile_stream.h"    // for d_fsync / d_ftruncate
#include "./dfile_common.hpp"


NS_DJINTERP
D_NAMESPACE(file)

// I. class descriptor

// descriptor
//   type: move-only RAII wrapper owning a raw file descriptor; closes on
// destruction. An invalid descriptor holds -1 and tests false.
class descriptor
{
public:
    // construction / destruction

    // construct an invalid (closed) descriptor.
    D_INLINE descriptor() noexcept
        : m_fd(-1)
    {}

    // adopt an already-open descriptor; takes ownership.
    D_INLINE explicit descriptor(int _fd) noexcept
        : m_fd(_fd)
    {}

    descriptor(const descriptor&)            = delete;
    descriptor& operator=(const descriptor&) = delete;

    D_INLINE descriptor(descriptor&& _other) noexcept
        : m_fd(_other.m_fd)
    {
        _other.m_fd = -1;
    }

    D_INLINE descriptor& operator=(descriptor&& _other) noexcept
    {
        // guard against self-move before releasing our own descriptor
        if (this != &_other)
        {
            reset(_other.m_fd);
            _other.m_fd = -1;
        }

        return *this;
    }

    D_INLINE ~descriptor() noexcept
    {
        reset();
    }

    // open _path with the given open(2) _flags (see d_open).
    D_INLINE static descriptor open(const char* _path,
                                    int         _flags) noexcept
    {
        return descriptor(d_open(_path, _flags));
    }

    // open _path with _flags and creation mode _mode (see d_open).
    D_INLINE static descriptor open(const char* _path,
                                    int         _flags,
                                    uint32_t    _mode) noexcept
    {
        return descriptor(d_open(_path, _flags, _mode));
    }

    // observers

    D_NO_DISCARD D_INLINE bool is_open() const noexcept
    {
        return m_fd >= 0;
    }

    D_NO_DISCARD D_INLINE explicit operator bool() const noexcept
    {
        return m_fd >= 0;
    }

    D_NO_DISCARD D_INLINE int get() const noexcept
    {
        return m_fd;
    }

    // ownership

    // relinquish ownership and return the raw descriptor.
    D_NO_DISCARD D_INLINE int release() noexcept
    {
        int fd;

        fd    = m_fd;
        m_fd  = -1;

        return fd;
    }

    // close any current descriptor and adopt _fd (default: invalid).
    D_INLINE void reset(int _fd = -1) noexcept
    {
        // close the existing descriptor before replacing it
        if (m_fd >= 0)
        {
            d_close(m_fd);
        }

        m_fd = _fd;

        return;
    }

    // I/O

    // read up to _count bytes into _buf (see d_read).
    D_INLINE ssize_t read(void*  _buf,
                          size_t _count) noexcept
    {
        return d_read(m_fd, _buf, _count);
    }

    // write _count bytes from _buf (see d_write).
    D_INLINE ssize_t write(const void* _buf,
                           size_t      _count) noexcept
    {
        return d_write(m_fd, _buf, _count);
    }

    // duplicate this descriptor onto the lowest free fd (see d_dup).
    D_NO_DISCARD D_INLINE descriptor duplicate() const noexcept
    {
        return descriptor(d_dup(m_fd));
    }

    // duplicate this descriptor onto _fd2 (see d_dup2).
    D_INLINE int duplicate_to(int _fd2) const noexcept
    {
        return d_dup2(m_fd, _fd2);
    }

    // apply an advisory lock operation (see d_flock).
    D_INLINE int lock(lock_flag _operation) noexcept
    {
        return d_flock(m_fd, to_int(_operation));
    }

    // force the OS to commit this descriptor to disk (see d_fsync).
    D_INLINE int sync() noexcept
    {
        return d_fsync(m_fd);
    }

    // truncate the underlying file to _length bytes (see d_ftruncate).
    D_INLINE int truncate(offset _length) noexcept
    {
        return d_ftruncate(m_fd, _length);
    }

private:
    int m_fd;
};


// II. class pipe

// pipe
//   type: move-only RAII wrapper owning a process pipe opened with popen and
// closed with pclose. An invalid pipe holds nullptr and tests false.
class pipe
{
public:
    // construction / destruction

    // construct an invalid (closed) pipe.
    D_INLINE pipe() noexcept
        : m_pipe(nullptr)
    {}

    // run _command and connect a pipe in mode _mode (see d_popen).
    D_INLINE pipe(const char* _command, const char* _mode) noexcept
        : m_pipe(d_popen(_command, _mode))
    {}

    pipe(const pipe&)            = delete;
    pipe& operator=(const pipe&) = delete;

    D_INLINE pipe(pipe&& _other) noexcept
        : m_pipe(_other.m_pipe)
    {
        _other.m_pipe = nullptr;
    }

    D_INLINE pipe& operator=(pipe&& _other) noexcept
    {
        // guard against self-move before releasing our own pipe
        if (this != &_other)
        {
            reset(_other.m_pipe);
            _other.m_pipe = nullptr;
        }

        return *this;
    }

    D_INLINE ~pipe() noexcept
    {
        reset();
    }

    // observers

    D_NO_DISCARD D_INLINE bool is_open() const noexcept
    {
        return m_pipe != nullptr;
    }

    D_NO_DISCARD D_INLINE explicit operator bool() const noexcept
    {
        return m_pipe != nullptr;
    }

    D_NO_DISCARD D_INLINE FILE* get() const noexcept
    {
        return m_pipe;
    }

    // ownership

    // relinquish ownership and return the raw handle.
    D_NO_DISCARD D_INLINE FILE* release() noexcept
    {
        FILE* handle;

        handle  = m_pipe;
        m_pipe  = nullptr;

        return handle;
    }

    // close any current pipe (discarding status) and adopt _handle.
    D_INLINE void reset(FILE* _handle = nullptr) noexcept
    {
        // close the existing pipe before replacing it
        if (m_pipe)
        {
            d_pclose(m_pipe);
        }

        m_pipe = _handle;

        return;
    }

    // close the pipe and return the command's exit status (see d_pclose).
    D_INLINE int close() noexcept
    {
        int status;

        status  = (m_pipe ? d_pclose(m_pipe) : -1);
        m_pipe  = nullptr;

        return status;
    }

    // I/O

    // read up to _size bytes from the pipe into _buf.
    D_INLINE size_t read(void*  _buf,
                         size_t _size) noexcept
    {
        return fread(_buf, 1, _size, m_pipe);
    }

    // write _size bytes from _buf to the pipe.
    D_INLINE size_t write(const void* _buf,
                          size_t      _size) noexcept
    {
        return fwrite(_buf, 1, _size, m_pipe);
    }

private:
    FILE* m_pipe;
};

NS_END  // file
NS_END  // djinterp


#endif  // DJINTERP_FILE_DESCRIPTOR_