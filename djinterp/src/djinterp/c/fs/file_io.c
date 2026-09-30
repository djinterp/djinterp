/*******************************************************************************
* djinterp [c]                                                         file_io.c
*
* Implementation of the byte transfer declared in file_io.h.
*   Descriptor transfers clamp each request to what one platform call may
* carry and retry interrupted calls; the positional ones use pread and pwrite
* where they exist and an explicit, non-atomic seek emulation where they do
* not. The whole-file readers measure the source when its size can be trusted
* and grow a buffer when it cannot; the whole-file writers loop to completion,
* sync when the build asks, and -- when configured -- rename a sibling
* temporary into place so the replacement is atomic.
*
*
* path:      /src/djinterp/c/fs/file_io.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
//   enable POSIX features for fileno, pread, pwrite, posix_fadvise and
// posix_fallocate. 200809L rather than 200112L because glibc gates pread and
// pwrite on it. A feature-test macro has to precede every include, so it
// sits ahead of the corresponding header rather than with the other defines.
#if ( (!defined(_WIN32)) &&                                                    \
      (!defined(_WIN64)) )
    #define _POSIX_C_SOURCE 200809L
#endif
#include "../../../../inc/djinterp/c/fs/file_io.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EFBIG, EINVAL, EIO, ENOSYS, ERANGE ...
#include <stddef.h>  // NULL, size_t
#include <stdint.h>  // SIZE_MAX, uint64_t
#include <stdio.h>   // FILE, fread, fwrite, fflush, fseek, ftell, remove
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*, d_internal_file_alloc
#include "../../../../inc/djinterp/c/fs/file_open.h"    // d_file_open_stream, d_file_close_stream
#include "../../../../inc/djinterp/config/c/fs/cfg_file_io.h"  // D_INTERNAL_FILE_READ_*, _WRITE_*


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/*
d_internal_file_io_clamp
  Not tuning. Windows' _read and _write both take an unsigned int, and POSIX
only guarantees a transfer up to SSIZE_MAX, so a size_t-sized request has to
be broken somewhere no matter what. Clamping here means callers may pass any
size, and a short transfer stays a normal documented outcome rather than a
platform quirk that surfaces on one target. Read and write pass different
limits because they are tuned separately; the rule itself lives once.
*/
static size_t
d_internal_file_io_clamp(
    size_t _count,
    size_t _limit
)
{
    // one platform call carries at most _limit bytes
    if (_count > _limit)
    {
        return _limit;
    }

    return _count;
}

#if (D_INTERNAL_FILE_READ_HINT == 1)

/*
d_internal_file_read_hint
  Tells the kernel that what follows is a front-to-back read of the whole
file, so it reads ahead aggressively instead of inferring the pattern one
fault at a time. Advisory in the strictest sense: a failure changes nothing
about correctness and is not reported.
*/
static void
d_internal_file_read_hint(
    FILE* _stream
)
{
    const int fd = fileno(_stream);

    // a stream with no descriptor cannot be advised
    if (fd >= 0)
    {
        (void)posix_fadvise(fd,
                            0,
                            0,
                            POSIX_FADV_SEQUENTIAL);
    }

    return;
}

#else

/*
d_internal_file_read_hint
  This build gives the kernel no read-ahead advice.
*/
static void
d_internal_file_read_hint(
    FILE* _stream
)
{
    (void)_stream;

    return;
}

#endif  // D_INTERNAL_FILE_READ_HINT == 1

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_file_read_size_hint
  Prefers a stat on the descriptor to seeking to the end: seeking perturbs the
stream position, fails outright on a pipe, and on a text-mode Windows stream
reports a length that does not match what will actually be read. Two results
are rejected as untrustworthy rather than believed: a non-regular file (pipe,
socket, character device), whose size means nothing, and a regular file
reporting zero bytes. That second one is the subtle case. Every entry under
/proc and /sys IS a regular file by S_ISREG, and every one of them stats as
zero bytes and then produces data on read; believing the zero is exactly how
d_file_read_all("/proc/self/status") returns an empty buffer and no error.
The cost of the distrust is one wasted growth allocation for a genuinely
empty file, which is the right trade. The result is 1 when *_size holds a
length to trust, and 0 when the source must be read until EOF instead.
*/
static int
d_internal_file_read_size_hint(
    FILE*   _stream,
    size_t* _size
)
{
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int fd = _fileno(_stream);

    // a stream with no descriptor cannot be measured
    if (fd < 0)
    {
        return 0;
    }

    struct _stat64 st;

    // an unmeasurable source is read until EOF instead
    if (_fstat64(fd,
                 &st) != 0)
    {
        return 0;
    }
#else
    const int fd = fileno(_stream);

    // a stream with no descriptor cannot be measured
    if (fd < 0)
    {
        return 0;
    }

    struct stat st;

    // an unmeasurable source is read until EOF instead
    if (fstat(fd,
              &st) != 0)
    {
        return 0;
    }
#endif

    // only a regular file's reported size can be believed at all
    if (!S_ISREG(st.st_mode))
    {
        return 0;
    }

    // ...and not even then, if it says zero: /proc and /sys entries are
    // regular files that stat as empty and then hand over kilobytes
    if (st.st_size <= 0)
    {
        return 0;
    }

    // a 64-bit file on a 32-bit host is a real case, and truncating the
    // length here would produce a short read the caller could not detect
    if ((uint64_t)st.st_size > (uint64_t)SIZE_MAX)
    {
        return 0;
    }

    *_size = (size_t)st.st_size;

    return 1;
}

#else

/*
d_internal_file_read_size_hint
  ISO C has no descriptors, so seek-and-restore is the only way to measure,
and the same distrust applies: no length, or an apparently empty remainder,
means "read until EOF and find out". The result is 1 when *_size holds a
length to trust, and 0 otherwise.
*/
static int
d_internal_file_read_size_hint(
    FILE*   _stream,
    size_t* _size
)
{
    const long position = ftell(_stream);

    // an unmeasurable source is read until EOF instead
    if (position < 0)
    {
        return 0;
    }

    // measure by seeking to the end
    if (fseek(_stream,
              0,
              SEEK_END) != 0)
    {
        return 0;
    }

    const long length = ftell(_stream);

    // put the position back before judging the answer
    if (fseek(_stream,
              position,
              SEEK_SET) != 0)
    {
        return 0;
    }

    // same distrust as the stat path: no length, or an apparently empty
    // remainder, means "read until EOF and find out"
    if (length <= position)
    {
        return 0;
    }

    *_size = (size_t)(length - position);

    return 1;
}

#endif  // !D_FILE_BACKEND_IS_STDC

/*
d_internal_file_read_sized
  Reads a known number of bytes into a fresh allocation. The ceiling is
checked before allocating -- the whole point of it is not to attempt the
allocation. A short read is a real failure here: the length was known, and
the file did not deliver it. (A text-mode stream can legitimately return
fewer bytes than the file holds, which is exactly why the whole-file path
opens in binary mode.)
*/
static void*
d_internal_file_read_sized(
    FILE*   _stream,
    size_t  _length,
    size_t* _size
)
{
#if (D_INTERNAL_FILE_READ_MAX_SIZE > 0)
    // refuse before allocating, not after: the whole point of the ceiling is
    // to not attempt the allocation
    if (_length > (size_t)D_INTERNAL_FILE_READ_MAX_SIZE)
    {
        D_INTERNAL_FILE_SET_ERR(EFBIG);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               EFBIG,
                               "d_file_read_all",
                               NULL,
                               "file exceeds D_CFG_FILE_READ_MAX_SIZE");

        return NULL;
    }
#endif

    void* const buffer =
        d_internal_file_alloc(_length + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

    // d_internal_file_alloc has already reported the failure
    if (!buffer)
    {
        return NULL;
    }

    const size_t bytes_read = fread(buffer,
                                    1,
                                    _length,
                                    _stream);

    // a short read here is a real failure: we were told the length, and the
    // file did not deliver it. (A text-mode stream can legitimately return
    // fewer bytes than the file holds -- which is exactly why the whole-file
    // path opens in binary mode.)
    if (bytes_read != _length)
    {
        d_internal_file_free(buffer);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_read_all",
                               NULL,
                               "short read against a known length");

        return NULL;
    }

#if D_CFG_IS_ON(D_CFG_FILE_READ_NUL_TERMINATE)
    ((char*)buffer)[bytes_read] = '\0';
#endif

    // the size out-parameter is optional
    if (_size)
    {
        *_size = bytes_read;
    }

    return buffer;
}

#if D_CFG_IS_ON(D_CFG_FILE_READ_GROW_UNSIZED)

/*
d_internal_file_read_grown
  Reads a stream of unknown length by doubling a buffer until EOF -- the path
that makes d_file_read_all work on a pipe, a character device, and every
entry under /proc and /sys, all of which report zero bytes and then hand over
data. A short read from an unsized source means EOF or an error, and ferror
is the only way to tell them apart. Each doubling is checked against the read
ceiling and against overflow before it is made, and a failed realloc frees
the block it left valid. With D_CFG_FILE_READ_SHRINK_TO_FIT the result is
trimmed to what was read; a declined shrink does not fail a read that has
already succeeded.
*/
static void*
d_internal_file_read_grown(
    FILE*   _stream,
    size_t* _size
)
{
    size_t capacity = (size_t)D_INTERNAL_FILE_READ_GROW_INITIAL;
    size_t used     = 0;
    char*  buffer   = (char*)d_internal_file_alloc(
                          capacity + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

    // d_internal_file_alloc has already reported the failure
    if (!buffer)
    {
        return NULL;
    }

    // read until the source stops producing
    for (;;)
    {
        const size_t chunk = d_internal_file_io_clamp(
                                 capacity - used,
                                 (size_t)D_INTERNAL_FILE_READ_CHUNK_SIZE);
        const size_t bytes_read = fread(buffer + used,
                                        1,
                                        chunk,
                                        _stream);

        used += bytes_read;

        // a short read from an unsized source means EOF or an error, and
        // ferror is the only way to tell those apart
        if (bytes_read < chunk)
        {
            // an error loses everything read so far
            if (ferror(_stream))
            {
                d_internal_file_free(buffer);
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                       errno,
                                       "d_file_read_all",
                                       NULL,
                                       "read error on an unsized source");

                return NULL;
            }

            break;
        }

        // full buffer: double it and keep going
        if (used == capacity)
        {
#if (D_INTERNAL_FILE_READ_MAX_SIZE > 0)
            // the ceiling applies to a source of unknown size too
            if (capacity >= (size_t)D_INTERNAL_FILE_READ_MAX_SIZE)
            {
                d_internal_file_free(buffer);
                D_INTERNAL_FILE_SET_ERR(EFBIG);
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                       EFBIG,
                                       "d_file_read_all",
                                       NULL,
                                       "source exceeds the read ceiling");

                return NULL;
            }
#endif

            // check the doubling for overflow before performing it
            if (capacity > (SIZE_MAX / 2))
            {
                d_internal_file_free(buffer);
                D_INTERNAL_FILE_SET_ERR(EOVERFLOW);

                return NULL;
            }

            capacity *= 2;

            char* const grown =
                (char*)d_internal_file_realloc(
                    buffer,
                    capacity + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

            // realloc leaves the original block valid on failure, so free
            // the pointer we still hold rather than the NULL we just got
            if (!grown)
            {
                d_internal_file_free(buffer);

                return NULL;
            }

            buffer = grown;
        }
    }

#if D_CFG_IS_ON(D_CFG_FILE_READ_SHRINK_TO_FIT)
    // hand back what was read, not what was reserved; a declined shrink is
    // not a reason to fail a read that already succeeded
    if (used < capacity)
    {
        char* const shrunk = (char*)d_internal_file_realloc(
                                 buffer,
                                 used + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

        // a declined shrink keeps the original, larger block
        if (shrunk)
        {
            buffer = shrunk;
        }
    }
#endif

#if D_CFG_IS_ON(D_CFG_FILE_READ_NUL_TERMINATE)
    buffer[used] = '\0';
#endif

    // the size out-parameter is optional
    if (_size)
    {
        *_size = used;
    }

    return buffer;
}

#endif  // D_CFG_FILE_READ_GROW_UNSIZED

/*
d_internal_file_write_stream
  fwrite is allowed to come up short, and a caller that treats a short write
as fatal without retrying loses data it was told was written -- so this
loops, a clamped chunk at a time. A pass that moves nothing is a real error;
without that check a stream that keeps returning 0 would spin forever. errno
is left as the platform set it.
*/
static int
d_internal_file_write_stream(
    FILE*       _stream,
    const void* _data,
    size_t      _size
)
{
    // nothing to write is a success, not a special case
    if (_size == 0)
    {
        return 0;
    }

    const char* cursor    = (const char*)_data;
    size_t      remaining = _size;

    // keep pushing until the buffer is gone or the stream refuses to move
    while (remaining > 0)
    {
        const size_t chunk = d_internal_file_io_clamp(
                                 remaining,
                                 (size_t)D_INTERNAL_FILE_WRITE_CHUNK_SIZE);
        const size_t written = fwrite(cursor,
                                      1,
                                      chunk,
                                      _stream);

        // no forward progress means a real error; without this check a
        // stream that keeps returning 0 spins forever
        if (written == 0)
        {
            return -1;
        }

        cursor    += written;
        remaining -= written;
    }

    return 0;
}

#if (D_INTERNAL_FILE_WRITE_SYNC == 1)

/*
d_internal_file_write_sync
  Two steps, and both are needed: fflush moves stdio's buffer into the kernel,
and only then can fsync (_commit on Windows) move the kernel's copy onto the
device. Calling either one alone is the classic way to believe you have
durability and not have it.
*/
static int
d_internal_file_write_sync(
    FILE* _stream
)
{
    // stdio first: fsync cannot see what stdio has not handed over
    if (fflush(_stream) != 0)
    {
        return -1;
    }

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int fd = _fileno(_stream);

    // a stream with no descriptor cannot be synced
    if (fd < 0)
    {
        return -1;
    }

    // then the kernel's copy onto the device
    if (_commit(fd) != 0)
    {
        return -1;
    }
#else
    const int fd = fileno(_stream);

    // a stream with no descriptor cannot be synced
    if (fd < 0)
    {
        return -1;
    }

    // then the kernel's copy onto the device
    if (fsync(fd) != 0)
    {
        return -1;
    }
#endif

    return 0;
}

#else

/*
d_internal_file_write_sync
  This build does not wait for durable storage, so there is nothing to do:
success means the kernel has the data.
*/
static int
d_internal_file_write_sync(
    FILE* _stream
)
{
    (void)_stream;

    return 0;
}

#endif  // D_INTERNAL_FILE_WRITE_SYNC == 1

#if (D_INTERNAL_FILE_WRITE_PREALLOC == 1)

/*
d_internal_file_write_prealloc
  Reserves the file's extents up front, so the filesystem allocates once
instead of growing the file a write at a time. Advisory: a refusal costs
nothing but fragmentation, so it is not reported.
*/
static void
d_internal_file_write_prealloc(
    FILE*  _stream,
    size_t _size
)
{
    // an empty file has nothing to reserve
    if (_size == 0)
    {
        return;
    }

    const int fd = fileno(_stream);

    // a stream with no descriptor cannot reserve space
    if (fd >= 0)
    {
        (void)posix_fallocate(fd,
                              0,
                              (off_t)_size);
    }

    return;
}

#endif  // D_INTERNAL_FILE_WRITE_PREALLOC == 1

#if (D_INTERNAL_FILE_WRITE_ATOMIC == 1)

/*
d_internal_file_write_temp_path
  A sibling of the target and not the system temp directory, deliberately:
rename is only atomic within a filesystem, and /tmp is very often a different
one. Landing the temporary next to the target is what makes the rename a
rename instead of a copy. A name that would not fit fails rather than being
truncated.
*/
static int
d_internal_file_write_temp_path(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    const size_t path_len   = strlen(_path);
    const size_t suffix_len = sizeof(D_INTERNAL_FILE_WRITE_TEMP_SUFFIX) - 1;

    // the target, the suffix and the terminator must all fit
    if ((path_len + suffix_len + 1) > _bufsize)
    {
        return -1;
    }

    memcpy(_buf,
           _path,
           path_len);
    memcpy(_buf + path_len,
           D_INTERNAL_FILE_WRITE_TEMP_SUFFIX,
           suffix_len);
    _buf[path_len + suffix_len] = '\0';

    return 0;
}

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_file_write_replace
  Windows' rename fails outright when the target exists, so the promotion goes
through MoveFileEx, which replaces -- and, with MOVEFILE_WRITE_THROUGH, does
not return until the move is on disk. It sets no errno, so a failure is
recorded as EACCES.
*/
static int
d_internal_file_write_replace(
    const char* _temp,
    const char* _target
)
{
    // replace the target in one step
    if (!MoveFileExA(_temp,
                     _target,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        D_INTERNAL_FILE_SET_ERR(EACCES);

        return -1;
    }

    return 0;
}

#else

/*
d_internal_file_write_replace
  POSIX rename already replaces the target atomically.
*/
static int
d_internal_file_write_replace(
    const char* _temp,
    const char* _target
)
{
    return rename(_temp,
                  _target);
}

#endif  // D_CFG_FILE_HAS_WIN32

#endif  // D_INTERNAL_FILE_WRITE_ATOMIC == 1

//==============================================================================
// 1.  TRANSFER
//==============================================================================

#if D_FILE_BACKEND_IS_STDC

/*
d_file_read_fd
  The ISO C backend has no descriptors, and none can be emulated, so this only
reports ENOSYS.
*/
ssize_t
d_file_read_fd(
    int    _fd,
    void*  _buf,
    size_t _count
)
{
    (void)_fd;
    (void)_buf;
    (void)_count;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_read_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_read_fd
  One clamped read, retried across interruptions. A zero-length request
returns 0 without calling the platform.
*/
ssize_t
d_file_read_fd(
    int    _fd,
    void*  _buf,
    size_t _count
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_read_fd",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_read_fd",
                            NULL,
                            "buffer is NULL",
                            -1);

    // a zero-length read is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    const size_t chunk = d_internal_file_io_clamp(
                             _count,
                             (size_t)D_INTERNAL_FILE_READ_CHUNK_SIZE);
    ssize_t      result = -1;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)_read(_fd,
                                               _buf,
                                               (unsigned int)chunk));
#else
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)read(_fd,
                                              _buf,
                                              chunk));
#endif

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_read_fd",
                               NULL,
                               "read failed");
    }

    return result;
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_read_full_fd
  A plain read may come up short for reasons that have nothing to do with the
data -- a signal, a pipe boundary, a socket's buffer -- so code that wants N
bytes has to loop. This is that loop, written once; end of file ends it
early with whatever was collected.
*/
ssize_t
d_file_read_full_fd(
    int    _fd,
    void*  _buf,
    size_t _count
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_read_full_fd",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_read_full_fd",
                            NULL,
                            "buffer is NULL",
                            -1);

    size_t total = 0;

    // keep asking until the request is satisfied or the source is done
    while (total < _count)
    {
        const ssize_t chunk = d_file_read_fd(_fd,
                                             (char*)_buf + total,
                                             _count - total);

        // d_file_read_fd has already reported the failure
        if (chunk < 0)
        {
            return -1;
        }

        // end of file: report what was actually collected
        if (chunk == 0)
        {
            break;
        }

        total += (size_t)chunk;
    }

    return (ssize_t)total;
}

#if D_FILE_BACKEND_IS_STDC

/*
d_file_pread_fd
  The ISO C backend has no descriptors, and none can be emulated, so this only
reports ENOSYS.
*/
ssize_t
d_file_pread_fd(
    int     _fd,
    void*   _buf,
    size_t  _count,
    d_off_t _offset
)
{
    (void)_fd;
    (void)_buf;
    (void)_count;
    (void)_offset;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_pread_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_pread_fd
  pread where the platform has it, which is atomic with respect to other users
of the same descriptor. Where it does not, the read is emulated by saving the
position, seeking, reading and seeking back -- which is not atomic: two
threads sharing the descriptor can interleave and read at each other's
offsets. An emulated call says so at info severity.
*/
ssize_t
d_file_pread_fd(
    int     _fd,
    void*   _buf,
    size_t  _count,
    d_off_t _offset
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_pread_fd",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_pread_fd",
                            NULL,
                            "buffer is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_offset >= 0,
                            EINVAL,
                            "d_file_pread_fd",
                            NULL,
                            "offset is negative",
                            -1);

    // a zero-length read is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    const size_t chunk = d_internal_file_io_clamp(
                             _count,
                             (size_t)D_INTERNAL_FILE_READ_CHUNK_SIZE);
    ssize_t      result = -1;

#if (D_INTERNAL_FILE_READ_HAS_PREAD == 1)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)pread(_fd,
                                               _buf,
                                               chunk,
                                               (off_t)_offset));

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_pread_fd",
                               NULL,
                               "pread failed");
    }

    return result;
#else
    // emulation: save, seek, read, restore -- not atomic
    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                           0,
                           "d_file_pread_fd",
                           NULL,
                           "emulated; not atomic on this target");

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const d_off_t saved = (d_off_t)_lseeki64(_fd,
                                             0,
                                             SEEK_CUR);

    // a position that cannot be saved cannot be restored
    if (saved < 0)
    {
        return -1;
    }

    // move to the requested offset
    if (_lseeki64(_fd,
                  (__int64)_offset,
                  SEEK_SET) < 0)
    {
        return -1;
    }

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)_read(_fd,
                                               _buf,
                                               (unsigned int)chunk));
    (void)_lseeki64(_fd,
                    (__int64)saved,
                    SEEK_SET);
    #else
    const d_off_t saved = (d_off_t)lseek(_fd,
                                         0,
                                         SEEK_CUR);

    // a position that cannot be saved cannot be restored
    if (saved < 0)
    {
        return -1;
    }

    // move to the requested offset
    if (lseek(_fd,
              (off_t)_offset,
              SEEK_SET) < 0)
    {
        return -1;
    }

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)read(_fd,
                                              _buf,
                                              chunk));
    (void)lseek(_fd,
                (off_t)saved,
                SEEK_SET);
    #endif

    return result;
#endif
}

#endif  // D_FILE_BACKEND_IS_STDC

#if D_FILE_BACKEND_IS_STDC

/*
d_file_write_fd
  The ISO C backend has no descriptors, and none can be emulated, so this only
reports ENOSYS.
*/
ssize_t
d_file_write_fd(
    int         _fd,
    const void* _buf,
    size_t      _count
)
{
    (void)_fd;
    (void)_buf;
    (void)_count;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_write_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_write_fd
  One clamped write, retried across interruptions. A zero-length request
returns 0 without calling the platform.
*/
ssize_t
d_file_write_fd(
    int         _fd,
    const void* _buf,
    size_t      _count
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_write_fd",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_write_fd",
                            NULL,
                            "buffer is NULL",
                            -1);

    // a zero-length write is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    const size_t chunk = d_internal_file_io_clamp(
                             _count,
                             (size_t)D_INTERNAL_FILE_WRITE_CHUNK_SIZE);
    ssize_t      result = -1;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)_write(_fd,
                                                _buf,
                                                (unsigned int)chunk));
#else
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)write(_fd,
                                               _buf,
                                               chunk));
#endif

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_write_fd",
                               NULL,
                               "write failed");
    }

    return result;
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_write_full_fd
  A short write is normal -- a pipe fills, a signal lands, a disk quota bites
mid-transfer -- and the caller almost always wants the retry rather than the
news. Unlike the read side, running out of room is a failure here, not a
graceful end: a partial write is a corrupt file. A pass that moves nothing
without an error fails with EIO, because looping on it would hang.
*/
ssize_t
d_file_write_full_fd(
    int         _fd,
    const void* _buf,
    size_t      _count
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_write_full_fd",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_write_full_fd",
                            NULL,
                            "buffer is NULL",
                            -1);

    size_t total = 0;

    // keep offering until everything is accepted
    while (total < _count)
    {
        const ssize_t chunk = d_file_write_fd(_fd,
                                              (const char*)_buf + total,
                                              _count - total);

        // d_file_write_fd has already reported the failure
        if (chunk < 0)
        {
            return -1;
        }

        // no progress and no error: the destination is refusing bytes
        // without saying why, and looping on it would hang
        if (chunk == 0)
        {
            D_INTERNAL_FILE_FAIL(EIO,
                                 "d_file_write_full_fd",
                                 NULL,
                                 "write stalled without an error",
                                 -1);
        }

        total += (size_t)chunk;
    }

    return (ssize_t)total;
}

#if D_FILE_BACKEND_IS_STDC

/*
d_file_pwrite_fd
  The ISO C backend has no descriptors, and none can be emulated, so this only
reports ENOSYS.
*/
ssize_t
d_file_pwrite_fd(
    int         _fd,
    const void* _buf,
    size_t      _count,
    d_off_t     _offset
)
{
    (void)_fd;
    (void)_buf;
    (void)_count;
    (void)_offset;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_pwrite_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_pwrite_fd
  pwrite where the platform has it, which is atomic with respect to other
users of the same descriptor. Where it does not, the write is emulated by
saving the position, seeking, writing and seeking back -- which is not atomic.
An emulated call says so at info severity.
*/
ssize_t
d_file_pwrite_fd(
    int         _fd,
    const void* _buf,
    size_t      _count,
    d_off_t     _offset
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_pwrite_fd",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_pwrite_fd",
                            NULL,
                            "buffer is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_offset >= 0,
                            EINVAL,
                            "d_file_pwrite_fd",
                            NULL,
                            "offset is negative",
                            -1);

    // a zero-length write is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    const size_t chunk = d_internal_file_io_clamp(
                             _count,
                             (size_t)D_INTERNAL_FILE_WRITE_CHUNK_SIZE);
    ssize_t      result = -1;

#if (D_INTERNAL_FILE_WRITE_HAS_PWRITE == 1)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)pwrite(_fd,
                                                _buf,
                                                chunk,
                                                (off_t)_offset));

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_pwrite_fd",
                               NULL,
                               "pwrite failed");
    }

    return result;
#else
    // emulation: save, seek, write, restore -- not atomic
    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                           0,
                           "d_file_pwrite_fd",
                           NULL,
                           "emulated; not atomic on this target");

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const d_off_t saved = (d_off_t)_lseeki64(_fd,
                                             0,
                                             SEEK_CUR);

    // a position that cannot be saved cannot be restored
    if (saved < 0)
    {
        return -1;
    }

    // move to the requested offset
    if (_lseeki64(_fd,
                  (__int64)_offset,
                  SEEK_SET) < 0)
    {
        return -1;
    }

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)_write(_fd,
                                                _buf,
                                                (unsigned int)chunk));
    (void)_lseeki64(_fd,
                    (__int64)saved,
                    SEEK_SET);
    #else
    const d_off_t saved = (d_off_t)lseek(_fd,
                                         0,
                                         SEEK_CUR);

    // a position that cannot be saved cannot be restored
    if (saved < 0)
    {
        return -1;
    }

    // move to the requested offset
    if (lseek(_fd,
              (off_t)_offset,
              SEEK_SET) < 0)
    {
        return -1;
    }

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)write(_fd,
                                               _buf,
                                               chunk));
    (void)lseek(_fd,
                (off_t)saved,
                SEEK_SET);
    #endif

    return result;
#endif
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_read_all_stream
  Takes the sized path when the source can be measured and trusted, and the
growth path when it cannot, so a pipe or a /proc entry reads correctly rather
than returning empty. The read-ahead hint goes first; it is advisory and
never fails the read.
*/
void*
d_file_read_all_stream(
    FILE*   _stream,
    size_t* _size
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_read_all_stream",
                            NULL,
                            "stream is NULL",
                            NULL);

    // the size out-parameter is optional, and reads 0 until a read succeeds
    if (_size)
    {
        *_size = 0;
    }

    size_t length = 0;

    d_internal_file_read_hint(_stream);

    // a known length buys one allocation instead of a doubling series
    if (d_internal_file_read_size_hint(_stream,
                                       &length))
    {
        return d_internal_file_read_sized(_stream,
                                          length,
                                          _size);
    }

#if D_CFG_IS_ON(D_CFG_FILE_READ_GROW_UNSIZED)
    return d_internal_file_read_grown(_stream,
                                      _size);
#else
    D_INTERNAL_FILE_FAIL(ESPIPE,
                         "d_file_read_all_stream",
                         NULL,
                         "unsized source and "
                         "D_CFG_FILE_READ_GROW_UNSIZED is 0",
                         NULL);
#endif
}

/*
d_file_read_all
  Opens the file in binary mode unconditionally -- a text-mode read would
translate line endings and deliver fewer bytes than the file holds, making
the returned size disagree with the file's own on exactly one platform -- and
hands the stream to d_file_read_all_stream. The close must not overwrite the
read's errno, which is the code the caller is about to look at.
*/
void*
d_file_read_all(
    const char* _path,
    size_t*     _size
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_read_all",
                            NULL,
                            "path is NULL",
                            NULL);

    // the size out-parameter is optional, and reads 0 until a read succeeds
    if (_size)
    {
        *_size = 0;
    }

    FILE* const file = d_file_open_stream(_path,
                                          "rb");

    // d_file_open_stream has already reported the failure
    if (!file)
    {
        return NULL;
    }

    void* const result = d_file_read_all_stream(file,
                                                _size);

    // the close must not overwrite the read's errno; that is the code the
    // caller is about to look at
    const int saved_errno = errno;

    (void)d_file_close_stream(file);
    errno = saved_errno;

    return result;
}

/*
d_file_read_all_into
  For the program that cannot or will not allocate: the buffer and its size
are the caller's, and nothing here calls the allocator. A file larger than
the buffer fails with ERANGE rather than being truncated -- silently handing
back a prefix is how a caller ends up parsing half a config file. A
measurable source is refused before a byte is read; an unmeasurable one is
read to capacity, and filling the buffer exactly is accepted only when the
next read finds end of file.
*/
int
d_file_read_all_into(
    const char* _path,
    void*       _buf,
    size_t      _bufsize,
    size_t*     _size
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_read_all_into",
                            NULL,
                            "path is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_read_all_into",
                            _path,
                            "buffer is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA,
                            EINVAL,
                            "d_file_read_all_into",
                            _path,
                            "buffer is too small to hold anything",
                            EINVAL);

    // the size out-parameter is optional, and reads 0 until a read succeeds
    if (_size)
    {
        *_size = 0;
    }

    // the terminator, if this build writes one, comes out of the caller's
    // buffer -- not out of a byte past its end
    const size_t capacity = _bufsize - (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA;
    FILE* const  file     = d_file_open_stream(_path,
                                               "rb");

    // d_file_open_stream has already reported the failure
    if (!file)
    {
        return errno ? errno : ENOENT;
    }

    d_internal_file_read_hint(file);

    size_t length;

    // refuse a file that will not fit before reading a single byte of it
    if (d_internal_file_read_size_hint(file,
                                       &length))
    {
        // a measured file larger than the buffer is refused outright
        if (length > capacity)
        {
            (void)d_file_close_stream(file);
            D_INTERNAL_FILE_SET_ERR(ERANGE);
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   ERANGE,
                                   "d_file_read_all_into",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                   "file is larger than the caller's buffer");

            return ERANGE;
        }
    }

    const size_t bytes_read = fread(_buf,
                                    1,
                                    capacity,
                                    file);

    // a failed read closes the stream and reports the platform's code
    if (ferror(file))
    {
        (void)d_file_close_stream(file);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_read_all_into",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "read failed");

        return errno ? errno : EIO;
    }

    // an unsized source could not be pre-checked, so check it now: filling
    // the buffer exactly means there may be more, and we cannot prove there
    // is not
    if ( (bytes_read == capacity) &&
         (fgetc(file) != EOF) )
    {
        (void)d_file_close_stream(file);
        D_INTERNAL_FILE_SET_ERR(ERANGE);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ERANGE,
                               "d_file_read_all_into",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "source is larger than the caller's buffer");

        return ERANGE;
    }

    (void)d_file_close_stream(file);

#if D_CFG_IS_ON(D_CFG_FILE_READ_NUL_TERMINATE)
    ((char*)_buf)[bytes_read] = '\0';
#endif

    // the size out-parameter is optional
    if (_size)
    {
        *_size = bytes_read;
    }

    return 0;
}

/*
d_file_write_all
  Without D_CFG_FILE_WRITE_ATOMIC the target is opened and rewritten, so a
reader arriving mid-call sees a truncated file and a crash leaves one. With
it, the bytes go to a sibling temporary -- a sibling so the rename stays
within one filesystem and therefore stays atomic -- that is synced, closed,
and only then renamed over the target; a temporary that failed is removed and
never promoted, leaving the old file intact. Durability comes before
promotion because a rename that beats its own data to the device can publish
an empty file, and the close counts because it is where a buffered write
finally reports failure. The file is opened in binary mode so that _size
bytes in means _size bytes on disk.
*/
int
d_file_write_all(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_write_all",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(( (_data != NULL) ||
                              (_size == 0) ),
                            EINVAL,
                            "d_file_write_all",
                            _path,
                            "data is NULL with a non-zero size",
                            -1);

#if (D_INTERNAL_FILE_WRITE_ATOMIC == 1)
    char temp_path[D_FILE_PATH_MAX];

    // the temporary is a sibling, so the rename below stays within one
    // filesystem and therefore stays atomic
    if (d_internal_file_write_temp_path(_path,
                                        temp_path,
                                        sizeof(temp_path)) != 0)
    {
        D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                             "d_file_write_all",
                             _path,
                             "no room for a temporary name",
                             -1);
    }

    FILE* const file = d_file_open_stream(temp_path,
                                          "wb");
#else
    FILE* const file = d_file_open_stream(_path,
                                          "wb");
#endif

    // d_file_open_stream has already reported the failure
    if (!file)
    {
        return -1;
    }

#if (D_INTERNAL_FILE_WRITE_PREALLOC == 1)
    d_internal_file_write_prealloc(file,
                                   _size);
#endif

    int result = d_internal_file_write_stream(file,
                                              _data,
                                              _size);

    // durability before promotion: a rename that beats its own data to the
    // device is a rename that can publish an empty file
    if (result == 0)
    {
        result = d_internal_file_write_sync(file);
    }

    // the close is where a buffered write finally reports failure, so its
    // result counts too
    if (d_file_close_stream(file) != 0)
    {
        result = -1;
    }

#if (D_INTERNAL_FILE_WRITE_ATOMIC == 1)
    // never promote a temporary we failed to write; leaving the old file
    // intact is the entire point
    if (result != 0)
    {
        (void)remove(temp_path);

        return -1;
    }

    // promote the temporary over the target in one step
    if (d_internal_file_write_replace(temp_path,
                                      _path) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_write_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "replace failed; target is unchanged");
        (void)remove(temp_path);

        return -1;
    }

    return 0;
#else
    // report the failure; the file may be left truncated
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_write_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "write failed; file may be truncated");
    }

    return result;
#endif
}

/*
d_file_append_all
  Append mode is not merely a seek to the end: on POSIX the offset and the
write are one operation, which is why this is its own function instead of a
flag on d_file_write_all. Atomic replacement is not attempted -- a file being
extended cannot be replaced. Otherwise the same loop, sync and counted close
as d_file_write_all.
*/
int
d_file_append_all(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_append_all",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(( (_data != NULL) ||
                              (_size == 0) ),
                            EINVAL,
                            "d_file_append_all",
                            _path,
                            "data is NULL with a non-zero size",
                            -1);

    FILE* const file = d_file_open_stream(_path,
                                          "ab");

    // d_file_open_stream has already reported the failure
    if (!file)
    {
        return -1;
    }

    int result = d_internal_file_write_stream(file,
                                              _data,
                                              _size);

    // the data must be durable before the append is reported done
    if (result == 0)
    {
        result = d_internal_file_write_sync(file);
    }

    // the close is where a buffered write finally reports failure
    if (d_file_close_stream(file) != 0)
    {
        result = -1;
    }

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_append_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "append failed");
    }

    return result;
}
