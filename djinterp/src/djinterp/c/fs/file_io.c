/******************************************************************************
* djinterp [c]                                                       file_io.c
*
* path:      /src/djinterp/c/fs/file_io.c
******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/c/fs/file_io.h"


// Internal definitions

/*
d_internal_file_io_clamp
  Clamps a caller's byte count to what one platform call may carry.
  This is not tuning. Windows' _read and _write both take an unsigned int, and
POSIX only guarantees a transfer up to SSIZE_MAX, so a size_t-sized request
has to be broken somewhere no matter what. Clamping here means callers may
pass any size, and a short transfer stays a normal documented outcome rather
than a platform quirk that surfaces on one target.
  Read and write pass different limits because they are tuned separately; the
rule itself is identical, so it lives once.

Parameter(s):
  _count: the caller's requested byte count.
  _limit: the configured per-call ceiling for this direction.
Return:
  The number of bytes to ask the platform for in one call.
*/
static size_t
d_internal_file_io_clamp
(
    size_t _count,
    size_t _limit
)
{
    if (_count > _limit)
    {
        return _limit;
    }

    return _count;
}


/*
d_internal_file_read_hint
  Tells the kernel that what follows is a front-to-back read of the whole
file, so it reads ahead aggressively instead of inferring the pattern one
fault at a time.
  Advisory in the strictest sense: a failure changes nothing about
correctness and is not reported.

Parameter(s):
  _stream: the stream about to be read; must not be NULL.
Return:
  none.
*/
static void
d_internal_file_read_hint
(
    FILE* _stream
)
{
#if (D_INTERNAL_FILE_READ_HINT == 1)
    int fd;

    fd = fileno(_stream);

    if (fd >= 0)
    {
        (void)posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    }
#else
    (void)_stream;
#endif

    return;
}


/*
d_internal_file_read_size_hint
  Asks the platform how many bytes a stream is about to produce.
  It prefers a stat on the descriptor over seeking to the end: seeking
perturbs the stream position, fails outright on a pipe, and on a text-mode
Windows stream reports a length that does not match what will actually be
read.
  Two results are rejected as untrustworthy rather than believed:
    - a non-regular file (pipe, socket, character device), whose size means
      nothing;
    - a regular file reporting zero bytes. This is the subtle one. Every
      entry under /proc and /sys IS a regular file by S_ISREG, and every one
      of them stats as zero bytes and then produces data on read. Believing
      the zero is exactly how d_fread_all("/proc/self/status") returns an
      empty buffer and no error. The cost of the distrust is one wasted
      growth allocation for a genuinely empty file, which is the right trade.

Parameter(s):
  _stream: the stream to measure; must not be NULL.
  _size:   receives the byte count when one could be determined.
Return:
  1 when _size holds a trustworthy length, 0 when the source must be read
until EOF instead.
*/
static int
d_internal_file_read_size_hint
(
    FILE*   _stream,
    size_t* _size
)
{
#if !D_FILE_BACKEND_IS_STDC
    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    struct _stat64 st;
    int            fd;

    fd = _fileno(_stream);

    if (fd < 0)
    {
        return 0;
    }

    if (_fstat64(fd, &st) != 0)
    {
        return 0;
    }
    #else
    struct stat st;
    int         fd;

    fd = fileno(_stream);

    if (fd < 0)
    {
        return 0;
    }

    if (fstat(fd, &st) != 0)
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
#else
    long position;
    long length;

    // ISO C has no descriptors, so seek-and-restore is the only option
    position = ftell(_stream);

    if (position < 0)
    {
        return 0;
    }

    if (fseek(_stream, 0, SEEK_END) != 0)
    {
        return 0;
    }

    length = ftell(_stream);

    if (fseek(_stream, position, SEEK_SET) != 0)
    {
        return 0;
    }

    // same distrust as the stat path above: no length, or an apparently
    // empty remainder, means "read until EOF and find out"
    if (length <= position)
    {
        return 0;
    }

    *_size = (size_t)(length - position);

    return 1;
#endif
}


/*
d_internal_file_read_sized
  Reads a known number of bytes from a stream into a fresh allocation.

Parameter(s):
  _stream: the stream to read; must not be NULL.
  _length: the number of payload bytes to read.
  _size:   receives the number of bytes actually read.
Return:
  The allocated buffer on success, or NULL on failure.
*/
static void*
d_internal_file_read_sized
(
    FILE*   _stream,
    size_t  _length,
    size_t* _size
)
{
    void*  buffer;
    size_t bytes_read;

    // refuse before allocating, not after: the whole point of the ceiling is
    // to not attempt the allocation
#if (D_INTERNAL_FILE_READ_MAX_SIZE > 0)
    if (_length > (size_t)D_INTERNAL_FILE_READ_MAX_SIZE)
    {
        D_INTERNAL_FILE_SET_ERR(EFBIG);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               EFBIG,
                               "d_fread_all",
                               NULL,
                               "file exceeds D_CFG_FILE_READ_MAX_SIZE");

        return NULL;
    }
#endif

    buffer = d_internal_file_alloc(_length +
                                  (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

    if (!buffer)
    {
        return NULL;
    }

    bytes_read = fread(buffer, 1, _length, _stream);

    // a short read here is a real failure: we were told the length, and the
    // file did not deliver it. (A text-mode stream can legitimately return
    // fewer bytes than the file holds -- which is exactly why the whole-file
    // path opens in binary mode.)
    if (bytes_read != _length)
    {
        d_internal_file_free(buffer);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fread_all",
                               NULL,
                               "short read against a known length");

        return NULL;
    }

#if D_CFG_IS_ON(D_CFG_FILE_READ_NUL_TERMINATE)
    ((char*)buffer)[bytes_read] = '\0';
#endif

    if (_size)
    {
        *_size = bytes_read;
    }

    return buffer;
}


#if D_CFG_IS_ON(D_CFG_FILE_READ_GROW_UNSIZED)

/*
d_internal_file_read_grown
  Reads a stream of unknown length by doubling a buffer until EOF.
  This is the path that makes d_fread_all work on a pipe, a character device,
and every entry under /proc and /sys -- all of which report zero bytes and
then hand over data.

Parameter(s):
  _stream: the stream to read; must not be NULL.
  _size:   receives the number of bytes actually read.
Return:
  The allocated buffer on success, or NULL on failure.
*/
static void*
d_internal_file_read_grown
(
    FILE*   _stream,
    size_t* _size
)
{
    char*  buffer;
    char*  grown;
    size_t capacity;
    size_t used;
    size_t chunk;
    size_t bytes_read;

    capacity = (size_t)D_INTERNAL_FILE_READ_GROW_INITIAL;
    used     = 0;

    buffer = (char*)d_internal_file_alloc(
                 capacity + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

    if (!buffer)
    {
        return NULL;
    }

    // read until the source stops producing
    for (;;)
    {
        chunk = d_internal_file_io_clamp(
                    capacity - used,
                    (size_t)D_INTERNAL_FILE_READ_CHUNK_SIZE);
        bytes_read = fread(buffer + used, 1, chunk, _stream);
        used += bytes_read;

        // a short read from an unsized source means EOF or an error, and
        // ferror is the only way to tell those apart
        if (bytes_read < chunk)
        {
            if (ferror(_stream))
            {
                d_internal_file_free(buffer);
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                       errno,
                                       "d_fread_all",
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
            if (capacity >= (size_t)D_INTERNAL_FILE_READ_MAX_SIZE)
            {
                d_internal_file_free(buffer);
                D_INTERNAL_FILE_SET_ERR(EFBIG);
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                       EFBIG,
                                       "d_fread_all",
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
            grown = (char*)d_internal_file_realloc(
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
        grown = (char*)d_internal_file_realloc(
                    buffer,
                    used + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

        if (grown)
        {
            buffer = grown;
        }
    }
#endif

#if D_CFG_IS_ON(D_CFG_FILE_READ_NUL_TERMINATE)
    buffer[used] = '\0';
#endif

    if (_size)
    {
        *_size = used;
    }

    return buffer;
}

#endif  // D_CFG_FILE_READ_GROW_UNSIZED


/*
d_internal_file_write_stream
  Writes a whole buffer to a stream, looping until it is done.
  fwrite is allowed to come up short, and a caller that treats a short write
as fatal without retrying loses data it was told was written.

Parameter(s):
  _stream: the destination stream; must not be NULL.
  _data:   the bytes to write; may be NULL only when _size is 0.
  _size:   the number of bytes to write.
Return:
  0 on success, or -1 on failure with errno set by the platform.
*/
static int
d_internal_file_write_stream
(
    FILE*       _stream,
    const void* _data,
    size_t      _size
)
{
    const char* cursor;
    size_t      remaining;
    size_t      chunk;
    size_t      written;

    // nothing to write is a success, not a special case
    if (_size == 0)
    {
        return 0;
    }

    cursor    = (const char*)_data;
    remaining = _size;

    // keep pushing until the buffer is gone or the stream refuses to move
    while (remaining > 0)
    {
        chunk = d_internal_file_io_clamp(
                    remaining,
                    (size_t)D_INTERNAL_FILE_WRITE_CHUNK_SIZE);
        written = fwrite(cursor, 1, chunk, _stream);

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


/*
d_internal_file_write_sync
  Pushes a stream's bytes all the way to durable storage.
  Two steps, and both are needed: fflush moves stdio's buffer into the
kernel, and only then can fsync move the kernel's copy onto the device.
Calling either one alone is the classic way to believe you have durability
and not have it.

Parameter(s):
  _stream: the stream to flush and sync; must not be NULL.
Return:
  0 on success, or -1 on failure with errno set.
*/
static int
d_internal_file_write_sync
(
    FILE* _stream
)
{
#if (D_INTERNAL_FILE_WRITE_SYNC == 1)
    int fd;

    // stdio first: fsync cannot see what stdio has not handed over
    if (fflush(_stream) != 0)
    {
        return -1;
    }

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    fd = _fileno(_stream);

    if (fd < 0)
    {
        return -1;
    }

    if (_commit(fd) != 0)
    {
        return -1;
    }
    #else
    fd = fileno(_stream);

    if (fd < 0)
    {
        return -1;
    }

    if (fsync(fd) != 0)
    {
        return -1;
    }
    #endif

    return 0;
#else
    (void)_stream;

    return 0;
#endif
}


#if (D_INTERNAL_FILE_WRITE_PREALLOC == 1)

/*
d_internal_file_write_prealloc
  Reserves a file's extents up front, so the filesystem allocates once
instead of growing the file a write at a time.
  Advisory: a refusal costs nothing but fragmentation, so it is not reported.

Parameter(s):
  _stream: the destination stream; must not be NULL.
  _size:   the number of bytes the file will hold.
Return:
  none.
*/
static void
d_internal_file_write_prealloc
(
    FILE*  _stream,
    size_t _size
)
{
    int fd;

    if (_size == 0)
    {
        return;
    }

    fd = fileno(_stream);

    if (fd >= 0)
    {
        (void)posix_fallocate(fd, 0, (off_t)_size);
    }

    return;
}

#endif  // D_INTERNAL_FILE_WRITE_PREALLOC


#if (D_INTERNAL_FILE_WRITE_ATOMIC == 1)

/*
d_internal_file_write_temp_path
  Builds the sibling temporary path the atomic path writes to.
  A sibling and not the system temp directory, deliberately: rename is only
atomic within a filesystem, and /tmp is very often a different one. Landing
the temporary next to the target is what makes the rename a rename instead of
a copy.

Parameter(s):
  _path:    the target path.
  _buf:     receives the temporary path.
  _bufsize: size of _buf, in bytes.
Return:
  0 on success, or -1 if the name would not fit.
*/
static int
d_internal_file_write_temp_path
(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    size_t path_len;
    size_t suffix_len;

    path_len   = strlen(_path);
    suffix_len = sizeof(D_INTERNAL_FILE_WRITE_TEMP_SUFFIX) - 1;

    if ((path_len + suffix_len + 1) > _bufsize)
    {
        return -1;
    }

    memcpy(_buf, _path, path_len);
    memcpy(_buf + path_len,
           D_INTERNAL_FILE_WRITE_TEMP_SUFFIX,
           suffix_len);
    _buf[path_len + suffix_len] = '\0';

    return 0;
}


/*
d_internal_file_write_replace
  Renames the temporary over the target, replacing whatever was there.
  POSIX rename already replaces atomically. Windows' rename does not -- it
fails outright if the target exists -- so it gets MoveFileEx, which does.

Parameter(s):
  _temp:   the temporary path to promote.
  _target: the path to replace.
Return:
  0 on success, or -1 on failure with errno set.
*/
static int
d_internal_file_write_replace
(
    const char* _temp,
    const char* _target
)
{
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    if (!MoveFileExA(_temp,
                     _target,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        D_INTERNAL_FILE_SET_ERR(EACCES);

        return -1;
    }

    return 0;
#else
    return rename(_temp, _target);
#endif
}

#endif  // D_INTERNAL_FILE_WRITE_ATOMIC


// I.    Descriptor reads

/*
d_read
  Reads from a file descriptor (POSIX read equivalent).
  Like read(2), it may return fewer bytes than requested without that being
an error; use d_read_full when you want the loop written for you.

Parameter(s):
  _fd:    an open file descriptor.
  _buf:   destination buffer; must hold at least _count bytes.
  _count: the maximum number of bytes to read.
Return:
  The number of bytes read, 0 at end of file, or -1 on failure with errno set.
*/
ssize_t
d_read
(
    int    _fd,
    void*  _buf,
    size_t _count
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;
    (void)_buf;
    (void)_count;

    // the ISO C backend has no descriptors; this cannot be emulated
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_read",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    ssize_t result;
    size_t  chunk;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_read",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_read",
                            NULL,
                            "buffer is NULL",
                            -1);

    // a zero-length read is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    chunk = d_internal_file_io_clamp(_count,
                                    (size_t)D_INTERNAL_FILE_READ_CHUNK_SIZE);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)_read(_fd, _buf, (unsigned int)chunk));
    #else
    D_INTERNAL_FILE_RETRY_EINTR(result, (ssize_t)read(_fd, _buf, chunk));
    #endif

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_read",
                               NULL,
                               "read failed");
    }

    return result;
#endif
}


/*
d_read_full
  Reads until _count bytes have been collected or the source ends.
  A plain read is permitted to come up short for reasons that have nothing to
do with the data -- a signal, a pipe boundary, a socket's buffer -- so code
that wants N bytes has to loop. This is that loop, written once.

Parameter(s):
  _fd:    an open file descriptor.
  _buf:   destination buffer; must hold at least _count bytes.
  _count: the number of bytes wanted.
Return:
  The number of bytes read: _count on success, fewer if end of file arrived
first, or -1 on failure with errno set.
*/
ssize_t
d_read_full
(
    int    _fd,
    void*  _buf,
    size_t _count
)
{
    size_t  total;
    ssize_t chunk;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_read_full",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_read_full",
                            NULL,
                            "buffer is NULL",
                            -1);

    total = 0;

    // keep asking until the request is satisfied or the source is done
    while (total < _count)
    {
        chunk = d_read(_fd, (char*)_buf + total, _count - total);

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


/*
d_pread
  Reads from a fixed offset without disturbing the descriptor's position.
  Where the platform provides pread this is atomic with respect to other
users of the same descriptor. Where it does not, it is emulated with
seek-read-seek, which is not: two threads sharing a descriptor can interleave
and read each other's offsets. Check D_FILE_READ_PREAD_IS_ATOMIC at compile
time if that distinction matters to you.

Parameter(s):
  _fd:     an open, seekable file descriptor.
  _buf:    destination buffer; must hold at least _count bytes.
  _count:  the maximum number of bytes to read.
  _offset: the absolute offset to read from.
Return:
  The number of bytes read, 0 at end of file, or -1 on failure with errno set.
*/
ssize_t
d_pread
(
    int     _fd,
    void*   _buf,
    size_t  _count,
    d_off_t _offset
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;
    (void)_buf;
    (void)_count;
    (void)_offset;

    // the ISO C backend has no descriptors; this cannot be emulated
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_pread",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    ssize_t result;
    size_t  chunk;
    #if (D_INTERNAL_FILE_READ_HAS_PREAD == 0)
    d_off_t saved;
    #endif

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_pread",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_pread",
                            NULL,
                            "buffer is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_offset >= 0,
                            EINVAL,
                            "d_pread",
                            NULL,
                            "offset is negative",
                            -1);

    // a zero-length read is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    chunk = d_internal_file_io_clamp(_count,
                                    (size_t)D_INTERNAL_FILE_READ_CHUNK_SIZE);

    #if (D_INTERNAL_FILE_READ_HAS_PREAD == 1)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)pread(_fd,
                                               _buf,
                                               chunk,
                                               (off_t)_offset));

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_pread",
                               NULL,
                               "pread failed");
    }

    return result;
    #else
    // emulation: save, seek, read, restore. Not atomic -- see above.
    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                           0,
                           "d_pread",
                           NULL,
                           "emulated; not atomic on this target");

        #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    saved = (d_off_t)_lseeki64(_fd, 0, SEEK_CUR);

    if (saved < 0)
    {
        return -1;
    }

    if (_lseeki64(_fd, (__int64)_offset, SEEK_SET) < 0)
    {
        return -1;
    }

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)_read(_fd, _buf, (unsigned int)chunk));

    (void)_lseeki64(_fd, (__int64)saved, SEEK_SET);
        #else
    saved = (d_off_t)lseek(_fd, 0, SEEK_CUR);

    if (saved < 0)
    {
        return -1;
    }

    if (lseek(_fd, (off_t)_offset, SEEK_SET) < 0)
    {
        return -1;
    }

    D_INTERNAL_FILE_RETRY_EINTR(result, (ssize_t)read(_fd, _buf, chunk));

    (void)lseek(_fd, (off_t)saved, SEEK_SET);
        #endif

    return result;
    #endif
#endif
}


// II.   Descriptor writes

/*
d_write
  Writes to a file descriptor (POSIX write equivalent).
  Like write(2), it may accept fewer bytes than offered without that being an
error; use d_write_full when you want the loop written for you.

Parameter(s):
  _fd:    an open file descriptor.
  _buf:   the bytes to write; must hold at least _count bytes.
  _count: the number of bytes to offer.
Return:
  The number of bytes written, or -1 on failure with errno set.
*/
ssize_t
d_write
(
    int         _fd,
    const void* _buf,
    size_t      _count
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;
    (void)_buf;
    (void)_count;

    // the ISO C backend has no descriptors; this cannot be emulated
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_write",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    ssize_t result;
    size_t  chunk;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_write",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_write",
                            NULL,
                            "buffer is NULL",
                            -1);

    // a zero-length write is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    chunk = d_internal_file_io_clamp(_count,
                                    (size_t)D_INTERNAL_FILE_WRITE_CHUNK_SIZE);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_INTERNAL_FILE_RETRY_EINTR(
        result,
        (ssize_t)_write(_fd, _buf, (unsigned int)chunk));
    #else
    D_INTERNAL_FILE_RETRY_EINTR(result, (ssize_t)write(_fd, _buf, chunk));
    #endif

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_write",
                               NULL,
                               "write failed");
    }

    return result;
#endif
}


/*
d_write_full
  Writes until every byte has been accepted.
  A short write is normal -- a pipe fills, a signal lands, a disk quota bites
mid-transfer -- and the caller almost always wants the retry rather than the
news. Unlike the read side, running out of room is a failure here, not a
graceful end: a partial write is a corrupt file.

Parameter(s):
  _fd:    an open file descriptor.
  _buf:   the bytes to write; must hold at least _count bytes.
  _count: the number of bytes to write.
Return:
  _count on success, or -1 on failure with errno set. Note that on failure
some bytes may already have been written.
*/
ssize_t
d_write_full
(
    int         _fd,
    const void* _buf,
    size_t      _count
)
{
    size_t  total;
    ssize_t chunk;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_write_full",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_write_full",
                            NULL,
                            "buffer is NULL",
                            -1);

    total = 0;

    // keep offering until everything is accepted
    while (total < _count)
    {
        chunk = d_write(_fd, (const char*)_buf + total, _count - total);

        if (chunk < 0)
        {
            return -1;
        }

        // no progress and no error: the destination is refusing bytes
        // without saying why, and looping on it would hang
        if (chunk == 0)
        {
            D_INTERNAL_FILE_FAIL(EIO,
                                 "d_write_full",
                                 NULL,
                                 "write stalled without an error",
                                 -1);
        }

        total += (size_t)chunk;
    }

    return (ssize_t)total;
}


/*
d_pwrite
  Writes at a fixed offset without disturbing the descriptor's position.
  Atomic against other users of the same descriptor where the platform
provides pwrite; emulated with seek-write-seek where it does not, and not
atomic in that case.

Parameter(s):
  _fd:     an open, seekable file descriptor.
  _buf:    the bytes to write; must hold at least _count bytes.
  _count:  the number of bytes to offer.
  _offset: the absolute offset to write at.
Return:
  The number of bytes written, or -1 on failure with errno set.
*/
ssize_t
d_pwrite
(
    int         _fd,
    const void* _buf,
    size_t      _count,
    d_off_t     _offset
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;
    (void)_buf;
    (void)_count;
    (void)_offset;

    // the ISO C backend has no descriptors; this cannot be emulated
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_pwrite",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    ssize_t result;
    size_t  chunk;
    #if (D_INTERNAL_FILE_WRITE_HAS_PWRITE == 0)
    d_off_t saved;
    #endif

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_pwrite",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_pwrite",
                            NULL,
                            "buffer is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_offset >= 0,
                            EINVAL,
                            "d_pwrite",
                            NULL,
                            "offset is negative",
                            -1);

    // a zero-length write is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    chunk = d_internal_file_io_clamp(_count,
                                    (size_t)D_INTERNAL_FILE_WRITE_CHUNK_SIZE);

    #if (D_INTERNAL_FILE_WRITE_HAS_PWRITE == 1)
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                (ssize_t)pwrite(_fd,
                                                _buf,
                                                chunk,
                                                (off_t)_offset));

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_pwrite",
                               NULL,
                               "pwrite failed");
    }

    return result;
    #else
    // emulation: save, seek, write, restore. Not atomic -- see above.
    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                           0,
                           "d_pwrite",
                           NULL,
                           "emulated; not atomic on this target");

        #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    saved = (d_off_t)_lseeki64(_fd, 0, SEEK_CUR);

    if (saved < 0)
    {
        return -1;
    }

    if (_lseeki64(_fd, (__int64)_offset, SEEK_SET) < 0)
    {
        return -1;
    }

    D_INTERNAL_FILE_RETRY_EINTR(
        result,
        (ssize_t)_write(_fd, _buf, (unsigned int)chunk));

    (void)_lseeki64(_fd, (__int64)saved, SEEK_SET);
        #else
    saved = (d_off_t)lseek(_fd, 0, SEEK_CUR);

    if (saved < 0)
    {
        return -1;
    }

    if (lseek(_fd, (off_t)_offset, SEEK_SET) < 0)
    {
        return -1;
    }

    D_INTERNAL_FILE_RETRY_EINTR(result, (ssize_t)write(_fd, _buf, chunk));

    (void)lseek(_fd, (off_t)saved, SEEK_SET);
        #endif

    return result;
    #endif
#endif
}


// III.  Whole-file reads

/*
d_fread_all_stream
  Reads an open stream to its end into a single fresh allocation.
  Takes the sized path when the source can be measured and the growth path
when it cannot, so a pipe or a /proc entry reads correctly rather than
returning empty.
  The buffer is NUL-terminated when D_CFG_FILE_READ_NUL_TERMINATE is on; the
terminator is not counted in *_size.

Parameter(s):
  _stream: the stream to read from its current position; must not be NULL.
  _size:   receives the number of payload bytes read; may be NULL.
Return:
  A buffer the caller must release with free (or the configured deallocator),
or NULL on failure.
*/
void*
d_fread_all_stream
(
    FILE*   _stream,
    size_t* _size
)
{
    size_t length;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_fread_all_stream",
                            NULL,
                            "stream is NULL",
                            NULL);

    if (_size)
    {
        *_size = 0;
    }

    length = 0;

    d_internal_file_read_hint(_stream);

    // a known length buys one allocation instead of a doubling series
    if (d_internal_file_read_size_hint(_stream, &length))
    {
        return d_internal_file_read_sized(_stream, length, _size);
    }

#if D_CFG_IS_ON(D_CFG_FILE_READ_GROW_UNSIZED)
    return d_internal_file_read_grown(_stream, _size);
#else
    D_INTERNAL_FILE_FAIL(ESPIPE,
                         "d_fread_all_stream",
                         NULL,
                         "unsized source and D_CFG_FILE_READ_GROW_UNSIZED is 0",
                         NULL);
#endif
}


/*
d_fread_all
  Reads an entire file into a single fresh allocation.
  The file is opened in binary mode unconditionally: a text-mode read would
translate line endings and deliver fewer bytes than the file holds, which
makes the returned size disagree with the file's own size on exactly one
platform. Callers who want translation should open the stream themselves and
call d_fread_all_stream.

Parameter(s):
  _path: path to the file.
  _size: receives the number of payload bytes read; may be NULL.
Return:
  A buffer the caller must release with free (or the configured deallocator),
or NULL on failure with errno set.
*/
void*
d_fread_all
(
    const char* _path,
    size_t*     _size
)
{
    FILE*  file;
    void*  result;
    int    saved_errno;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_fread_all",
                            NULL,
                            "path is NULL",
                            NULL);

    if (_size)
    {
        *_size = 0;
    }

    file = d_fopen(_path, "rb");

    if (!file)
    {
        return NULL;
    }

    result = d_fread_all_stream(file, _size);

    // the close must not overwrite the read's errno; that is the code the
    // caller is about to look at
    saved_errno = errno;
    (void)d_fclose(file);
    errno = saved_errno;

    return result;
}


/*
d_fread_all_into
  Reads an entire file into a buffer the caller already owns.
  Exists for the program that cannot or will not allocate: the buffer is
yours, the size is yours, and nothing here calls the allocator. A file larger
than the buffer is a failure (ERANGE), not a truncation -- silently handing
back a prefix is how a caller ends up parsing half a config file.
  The buffer is NUL-terminated when D_CFG_FILE_READ_NUL_TERMINATE is on,
which costs one byte of _bufsize.

Parameter(s):
  _path:    path to the file.
  _buf:     destination buffer.
  _bufsize: size of _buf, in bytes.
  _size:    receives the number of payload bytes read; may be NULL.
Return:
  0 on success, or a non-zero errno-style code on failure -- ERANGE when the
file does not fit.
*/
int
d_fread_all_into
(
    const char* _path,
    void*       _buf,
    size_t      _bufsize,
    size_t*     _size
)
{
    FILE*  file;
    size_t length;
    size_t bytes_read;
    size_t capacity;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_fread_all_into",
                            NULL,
                            "path is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_fread_all_into",
                            _path,
                            "buffer is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA,
                            EINVAL,
                            "d_fread_all_into",
                            _path,
                            "buffer is too small to hold anything",
                            EINVAL);

    if (_size)
    {
        *_size = 0;
    }

    // the terminator, if this build writes one, comes out of the caller's
    // buffer -- not out of a byte past its end
    capacity = _bufsize - (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA;

    file = d_fopen(_path, "rb");

    if (!file)
    {
        return errno ? errno : ENOENT;
    }

    d_internal_file_read_hint(file);

    // refuse a file that will not fit before reading a single byte of it
    if (d_internal_file_read_size_hint(file, &length))
    {
        if (length > capacity)
        {
            (void)d_fclose(file);
            D_INTERNAL_FILE_SET_ERR(ERANGE);
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   ERANGE,
                                   "d_fread_all_into",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                   "file is larger than the caller's buffer");

            return ERANGE;
        }
    }

    bytes_read = fread(_buf, 1, capacity, file);

    if (ferror(file))
    {
        (void)d_fclose(file);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fread_all_into",
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
        (void)d_fclose(file);
        D_INTERNAL_FILE_SET_ERR(ERANGE);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ERANGE,
                               "d_fread_all_into",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "source is larger than the caller's buffer");

        return ERANGE;
    }

    (void)d_fclose(file);

#if D_CFG_IS_ON(D_CFG_FILE_READ_NUL_TERMINATE)
    ((char*)_buf)[bytes_read] = '\0';
#endif

    if (_size)
    {
        *_size = bytes_read;
    }

    return 0;
}


// IV.   Whole-file writes

/*
d_fwrite_all
  Creates or replaces a file with the contents of a buffer.
  What "replaces" means is a build-time decision. With D_CFG_FILE_WRITE_ATOMIC
off, this opens the target and rewrites it, so a reader arriving mid-call sees
a truncated file and a crash mid-call leaves one. With it on, the bytes go to
a sibling temporary that is renamed over the target, so no reader and no crash
ever observes a partial file. D_CFG_FILE_WRITE_SYNC decides whether returning
0 means "the kernel has it" or "the device has it".
  Opened in binary mode unconditionally: _size bytes in must mean _size bytes
on disk, on every platform.

Parameter(s):
  _path: path to the file.
  _data: the bytes to write; may be NULL only when _size is 0.
  _size: the number of bytes to write.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_fwrite_all
(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
#if (D_INTERNAL_FILE_WRITE_ATOMIC == 1)
    char  temp_path[D_FILE_PATH_MAX];
#endif
    FILE* file;
    int   result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_fwrite_all",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE( (_data != NULL) || (_size == 0),
                            EINVAL,
                            "d_fwrite_all",
                            _path,
                            "data is NULL with a non-zero size",
                            -1);

#if (D_INTERNAL_FILE_WRITE_ATOMIC == 1)
    // the temporary is a sibling, so the rename below stays within one
    // filesystem and therefore stays atomic
    if (d_internal_file_write_temp_path(_path,
                                        temp_path,
                                        sizeof(temp_path)) != 0)
    {
        D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                             "d_fwrite_all",
                             _path,
                             "no room for a temporary name",
                             -1);
    }

    file = d_fopen(temp_path, "wb");
#else
    file = d_fopen(_path, "wb");
#endif

    if (!file)
    {
        return -1;
    }

#if (D_INTERNAL_FILE_WRITE_PREALLOC == 1)
    d_internal_file_write_prealloc(file, _size);
#endif

    result = d_internal_file_write_stream(file, _data, _size);

    // durability before promotion: a rename that beats its own data to the
    // device is a rename that can publish an empty file
    if (result == 0)
    {
        result = d_internal_file_write_sync(file);
    }

    // the close is where a buffered write finally reports failure, so its
    // result counts too
    if (d_fclose(file) != 0)
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

    if (d_internal_file_write_replace(temp_path, _path) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fwrite_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "replace failed; target is unchanged");
        (void)remove(temp_path);

        return -1;
    }

    return 0;
#else
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fwrite_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "write failed; file may be truncated");
    }

    return result;
#endif
}


/*
d_fappend_all
  Appends a buffer to a file, creating it if it does not exist.
  Append mode is not merely a seek to the end: on POSIX the offset and the
write are one operation, so two processes appending to the same file
interleave records rather than overwrite each other. That is why this is its
own function instead of a flag on d_fwrite_all.
  Atomic replace does not apply here and is not attempted -- you cannot
replace a file you are trying to extend.

Parameter(s):
  _path: path to the file.
  _data: the bytes to append; may be NULL only when _size is 0.
  _size: the number of bytes to append.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_fappend_all
(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
    FILE* file;
    int   result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_fappend_all",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE( (_data != NULL) || (_size == 0),
                            EINVAL,
                            "d_fappend_all",
                            _path,
                            "data is NULL with a non-zero size",
                            -1);

    file = d_fopen(_path, "ab");

    if (!file)
    {
        return -1;
    }

    result = d_internal_file_write_stream(file, _data, _size);

    if (result == 0)
    {
        result = d_internal_file_write_sync(file);
    }

    // the close is where a buffered write finally reports failure
    if (d_fclose(file) != 0)
    {
        result = -1;
    }

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fappend_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "append failed");
    }

    return result;
}
