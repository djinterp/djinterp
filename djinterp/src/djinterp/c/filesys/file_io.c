/*******************************************************************************
* djinterp [c]                                                         file_io.c
*
* Implementation of the byte transfer declared in file_io.h.
*   Descriptor transfers clamp each request to the build's chunk size and to
* what one platform call may carry, and retry interrupted calls; the
* positional ones use pread and pwrite where they exist and an explicit,
* non-atomic seek emulation where they do not -- Windows among them. The
* whole-file readers measure the source when its size can be trusted -- what
* lies past the stream's position -- and grow a buffer when it cannot, never
* past the read ceiling. The whole-file writers go through a descriptor
* where the backend has one, so a file they create has the configured mode;
* they sync the file and its directory when the build asks, and -- when
* configured -- replace the file with a sibling, made exclusively, that is
* renamed into place.
*   The positional calls are implemented once, as the 64-bit twins; the
* 32-bit ones widen the offset and call them.
*
*
* path:      /src/djinterp/c/filesys/file_io.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_io.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EFBIG, EINVAL, EIO, ENOSYS, ERANGE ...
#include <limits.h>  // INT_MAX, SSIZE_MAX
#include <stddef.h>  // NULL, size_t
#include <stdio.h>   // FILE, fread, fwrite, fseek, ftell, rename, remove
#include <string.h>  // memcpy, memset, strlen
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*, d_internal_file_alloc
#include "../../../../inc/djinterp/c/filesys/file_desc.h"    // d_file_open, d_file_close_fd
#include "../../../../inc/djinterp/c/filesys/file_open.h"    // d_file_open_stream, d_file_close_stream
#include "../../../../inc/djinterp/c/filesys/file_path.h"    // d_path_dirname
#include "../../../../inc/djinterp/c/filesys/file_seek.h"    // d_file_tell_stream64
#include "../../../../inc/djinterp/c/filesys/file_stat.h"    // d_file_stat_fd64
#include "../../../../inc/djinterp/c/filesys/file_sync.h"    // d_internal_file_sync_raw
#include "../../../../inc/djinterp/c/filesys/file_temp.h"    // d_internal_file_temp_create
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_io.h"  // D_INTERNAL_FILE_READ_*, _WRITE_*
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint64_t, SIZE_MAX


// the large-file interface this source needs (file_common.h, 6.4.1)
D_INTERNAL_FILE_ASSERT_LARGE_FILES;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_IO_READ_CEILING
//   constant: D_INTERNAL_FILE_READ_MAX_SIZE as a size_t, or 0 for no ceiling.
// A ceiling past what a size_t holds admits every length a size_t can name,
// so it is none -- not the remainder a cast would leave, which on a 32-bit
// build makes a ceiling of 4 GiB and 1 MiB one of 1 MiB. A ceiling of 0 is
// settled here, where comparing it would draw a warning.
#if (D_INTERNAL_FILE_READ_MAX_SIZE == 0)
    #define D_INTERNAL_IO_READ_CEILING ((size_t)0)
#else
    #define D_INTERNAL_IO_READ_CEILING                                         \
        ( ((D_INTERNAL_FILE_READ_MAX_SIZE) > (size_t)-1)                       \
              ? (size_t)0                                                      \
              : (size_t)(D_INTERNAL_FILE_READ_MAX_SIZE) )
#endif  // D_INTERNAL_FILE_READ_MAX_SIZE == 0

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

#if !D_FILE_BACKEND_IS_STDC

// D_INTERNAL_IO_EMULATES
//   constant: 1 when this build emulates a positional call with seeks, and so
// needs d_internal_io_seek.
#define D_INTERNAL_IO_EMULATES                                                 \
    ( (D_INTERNAL_FILE_READ_HAS_PREAD == 0) ||                                 \
      (D_INTERNAL_FILE_WRITE_HAS_PWRITE == 0) )

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

// D_INTERNAL_IO_CALL_MAX
//   constant: the most bytes one CRT _read or _write carries. Both count in
// an int, and the UCRT refuses a count past INT_MAX as an invalid parameter
// -- which, by default, ends the process.
#define D_INTERNAL_IO_CALL_MAX ((size_t)INT_MAX)

/*
d_internal_io_read
  The CRT's _read, asked for no more than it can carry.
*/
static ssize_t
d_internal_io_read(
    int    _fd,
    void*  _buf,
    size_t _count
)
{
    return (ssize_t)_read(_fd,
                          _buf,
                          (unsigned int)d_internal_file_io_clamp(
                              _count,
                              D_INTERNAL_IO_CALL_MAX));
}

/*
d_internal_io_write
  The CRT's _write, offered no more than it can carry.
*/
static ssize_t
d_internal_io_write(
    int         _fd,
    const void* _buf,
    size_t      _count
)
{
    return (ssize_t)_write(_fd,
                           _buf,
                           (unsigned int)d_internal_file_io_clamp(
                               _count,
                               D_INTERNAL_IO_CALL_MAX));
}

#if D_INTERNAL_IO_EMULATES

/*
d_internal_io_seek
  The CRT's 64-bit seek.
*/
static d_off64_t
d_internal_io_seek(
    int       _fd,
    d_off64_t _offset,
    int       _whence
)
{
    return (d_off64_t)_lseeki64(_fd,
                                (__int64)_offset,
                                _whence);
}

#endif  // D_INTERNAL_IO_EMULATES

#else

// D_INTERNAL_IO_CALL_MAX
//   constant: the most bytes one read or write is defined for: POSIX leaves
// a count past SSIZE_MAX to the implementation.
#define D_INTERNAL_IO_CALL_MAX ((size_t)SSIZE_MAX)

/*
d_internal_io_read
  read(2), asked for no more than it is defined for.
*/
static ssize_t
d_internal_io_read(
    int    _fd,
    void*  _buf,
    size_t _count
)
{
    return read(_fd,
                _buf,
                d_internal_file_io_clamp(_count,
                                         D_INTERNAL_IO_CALL_MAX));
}

/*
d_internal_io_write
  write(2), offered no more than it is defined for.
*/
static ssize_t
d_internal_io_write(
    int         _fd,
    const void* _buf,
    size_t      _count
)
{
    return write(_fd,
                 _buf,
                 d_internal_file_io_clamp(_count,
                                          D_INTERNAL_IO_CALL_MAX));
}

#if D_INTERNAL_IO_EMULATES

/*
d_internal_io_seek
  lseek(2), whose off_t is 64-bit in this build (file_common.h, 6.4.1).
*/
static d_off64_t
d_internal_io_seek(
    int       _fd,
    d_off64_t _offset,
    int       _whence
)
{
    return (d_off64_t)lseek(_fd,
                            (off_t)_offset,
                            _whence);
}

#endif  // D_INTERNAL_IO_EMULATES

#endif  // D_CFG_FILE_HAS_WIN32

#if (D_INTERNAL_FILE_READ_HAS_PREAD == 1)

/*
d_internal_io_pread
  pread(2), which reads at the offset without touching the shared position --
atomic against every other user of the descriptor.
*/
static ssize_t
d_internal_io_pread(
    int       _fd,
    void*     _buf,
    size_t    _count,
    d_off64_t _offset
)
{
    return pread(_fd,
                 _buf,
                 d_internal_file_io_clamp(_count,
                                          D_INTERNAL_IO_CALL_MAX),
                 (off_t)_offset);
}

#else

/*
d_internal_io_pread
  pread emulated: save the position, seek, read, and seek back -- not atomic,
since another user of the descriptor can move it in between, and said so at
info severity. The read's errno survives the seek back.
*/
static ssize_t
d_internal_io_pread(
    int       _fd,
    void*     _buf,
    size_t    _count,
    d_off64_t _offset
)
{
    const d_off64_t saved = d_internal_io_seek(_fd,
                                               0,
                                               SEEK_CUR);

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                           0,
                           "d_file_pread_fd64",
                           NULL,
                           "emulated; not atomic on this target");

    // a position that cannot be saved, or reached, cannot be read from
    if ( (saved < 0) ||
         (d_internal_io_seek(_fd,
                             _offset,
                             SEEK_SET) < 0) )
    {
        return -1;
    }

    const ssize_t result      = d_internal_io_read(_fd,
                                                   _buf,
                                                   _count);
    const int     saved_errno = errno;

    (void)d_internal_io_seek(_fd,
                             saved,
                             SEEK_SET);
    errno = saved_errno;

    return result;
}

#endif  // D_INTERNAL_FILE_READ_HAS_PREAD == 1

#if (D_INTERNAL_FILE_WRITE_HAS_PWRITE == 1)

/*
d_internal_io_pwrite
  pwrite(2), which writes at the offset without touching the shared position.
*/
static ssize_t
d_internal_io_pwrite(
    int         _fd,
    const void* _buf,
    size_t      _count,
    d_off64_t   _offset
)
{
    return pwrite(_fd,
                  _buf,
                  d_internal_file_io_clamp(_count,
                                           D_INTERNAL_IO_CALL_MAX),
                  (off_t)_offset);
}

#else

/*
d_internal_io_pwrite
  pwrite emulated as d_internal_io_pread emulates pread: not atomic, and said
so at info severity.
*/
static ssize_t
d_internal_io_pwrite(
    int         _fd,
    const void* _buf,
    size_t      _count,
    d_off64_t   _offset
)
{
    const d_off64_t saved = d_internal_io_seek(_fd,
                                               0,
                                               SEEK_CUR);

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                           0,
                           "d_file_pwrite_fd64",
                           NULL,
                           "emulated; not atomic on this target");

    // a position that cannot be saved, or reached, cannot be written at
    if ( (saved < 0) ||
         (d_internal_io_seek(_fd,
                             _offset,
                             SEEK_SET) < 0) )
    {
        return -1;
    }

    const ssize_t result      = d_internal_io_write(_fd,
                                                    _buf,
                                                    _count);
    const int     saved_errno = errno;

    (void)d_internal_io_seek(_fd,
                             saved,
                             SEEK_SET);
    errno = saved_errno;

    return result;
}

#endif  // D_INTERNAL_FILE_WRITE_HAS_PWRITE == 1

#endif  // !D_FILE_BACKEND_IS_STDC

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
  Prefers a stat on the stream's descriptor to seeking to the end: seeking
perturbs the stream, fails outright on a pipe, and on a text-mode Windows
stream reports a length that does not match what will actually be read. Two
results are rejected as untrustworthy rather than believed: a non-regular file
(pipe, socket, character device), whose size means nothing, and a regular file
reporting zero bytes. That second one is the subtle case. Every entry under
/proc and /sys IS a regular file by S_ISREG, and every one of them stats as
zero bytes and then produces data on read; believing the zero is exactly how
d_file_read_all("/proc/self/status") returns an empty buffer and no error.
The cost of the distrust is one wasted growth allocation for a genuinely
empty file, which is the right trade.
  A stream reads from where it is, so the length is what lies past its
position, and a remainder that seems empty is distrusted the same way. The
result is 1 when *_size holds a length to trust, and 0 when the source must
be read until EOF instead.
*/
static int
d_internal_file_read_size_hint(
    FILE*   _stream,
    size_t* _size
)
{
    const int         fd = d_file_descriptor_stream(_stream);
    struct d_stat64_t st = {0};

    // a stream with no descriptor, or one that cannot be measured, is read
    // until EOF instead
    if ( (fd < 0) ||
         (d_file_stat_fd64(fd,
                           &st) != 0) )
    {
        return 0;
    }

    // only a regular file's reported size can be believed at all, and not
    // even then if it says zero: /proc and /sys entries are regular files
    // that stat as empty and then hand over kilobytes
    if ( (!S_ISREG(st.st_mode)) ||
         (st.st_size == 0) )
    {
        return 0;
    }

    const d_off64_t position = d_file_tell_stream64(_stream);

    // what lies past the position is the length; nothing past it is as
    // little to be believed as a size of zero
    if ( (position < 0) ||
         ((uint64_t)position >= st.st_size) )
    {
        return 0;
    }

    const uint64_t left = st.st_size - (uint64_t)position;

    // a 64-bit file on a 32-bit host is a real case, and truncating the
    // length here would produce a short read the caller could not detect
    if (left > (uint64_t)SIZE_MAX)
    {
        return 0;
    }

    *_size = (size_t)left;

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
d_internal_file_read_ends
  Whether a stream that has just filled a buffer is at its end: 1 when the
next read finds end of file, 0 when there is more -- the byte that said so
is pushed back, so the stream is as it was -- and -1 when that read fails,
which fgetc's EOF alone cannot tell from the end.
*/
static int
d_internal_file_read_ends(
    FILE* _stream
)
{
    const int next = fgetc(_stream);

    // more: put it back for whoever reads next
    if (next != EOF)
    {
        (void)ungetc(next,
                     _stream);

        return 0;
    }

    // EOF is either the end or an error, and ferror is the only way to tell
    // those apart
    return ferror(_stream) ? -1 : 1;
}

/*
d_internal_file_read_fail
  Ends a whole-file read that failed: frees the buffer, if there is one, and
reports the failure -- with _code, the module's own, or with 0 for the
platform's errno; or EIO, when a stream's error indicator is set and errno is
not, so that a failure never reads as errno 0.
*/
static void*
d_internal_file_read_fail(
    void*       _buffer,
    int         _code,
    const char* _message
)
{
    const int code = (_code != 0)  ? _code
                   : (errno != 0) ? errno
                                  : EIO;

    d_internal_file_free(_buffer);
    D_INTERNAL_FILE_SET_ERR(code);
    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                           code,
                           "d_file_read_all",
                           NULL,
                           _message);
    (void)code;
    (void)_message;

    return NULL;
}

/*
d_internal_file_read_finish
  Ends a whole-file read that succeeded: terminates the payload when the build
terminates -- the buffer was allocated with the byte for it -- and reports
its size.
*/
static void*
d_internal_file_read_finish(
    char*   _buffer,
    size_t  _used,
    size_t* _size
)
{
    // the terminator has its own byte, past the payload
    if (D_INTERNAL_FILE_READ_NUL_EXTRA == 1)
    {
        _buffer[_used] = '\0';
    }

    // the size out-parameter is optional
    if (_size)
    {
        *_size = _used;
    }

    return _buffer;
}

/*
d_internal_file_read_sized
  Reads a source of known length into a fresh allocation. The ceiling is
checked before allocating -- the whole point of it is not to attempt the
allocation. Fewer bytes than measured are not a failure: a text-mode stream
translates line endings and delivers fewer, and a file that shrank since it
was measured has fewer to give; only an error is. A file that grew is read
to the length it had.
*/
static void*
d_internal_file_read_sized(
    FILE*   _stream,
    size_t  _length,
    size_t* _size
)
{
    const size_t ceiling = D_INTERNAL_IO_READ_CEILING;

    // refuse before allocating, not after: the whole point of the ceiling
    // is to not attempt the allocation
    if ( (ceiling > 0) &&
         (_length > ceiling) )
    {
        D_INTERNAL_FILE_SET_ERR(EFBIG);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               EFBIG,
                               "d_file_read_all",
                               NULL,
                               "file exceeds the read ceiling");

        return NULL;
    }

    char* const buffer = (char*)d_internal_file_alloc(
                             _length + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

    // d_internal_file_alloc has already reported the failure
    if (!buffer)
    {
        return NULL;
    }

    const size_t bytes_read = fread(buffer,
                                    1,
                                    _length,
                                    _stream);

    // a short read is the end of what there was, unless it is an error
    if ( (bytes_read < _length) &&
         (ferror(_stream)) )
    {
        return d_internal_file_read_fail(buffer,
                                         0,
                                         "read failed");
    }

    return d_internal_file_read_finish(buffer,
                                       bytes_read,
                                       _size);
}

#if (D_INTERNAL_FILE_READ_GROW == 1)

/*
d_internal_file_read_next
  The capacity a full buffer grows to: twice what it was, but never past the
read ceiling -- the whole point of the ceiling is not to attempt the
allocation -- so a source the ceiling admits always fits. Without a ceiling
a doubling that would overflow, terminator included, is refused with 0.
*/
static size_t
d_internal_file_read_next(
    size_t _capacity
)
{
    const size_t ceiling = D_INTERNAL_IO_READ_CEILING;

    // a ceiling stops the doubling, which then cannot overflow
    if (ceiling > 0)
    {
        return (_capacity > (ceiling / 2)) ? ceiling : (_capacity * 2);
    }

    return (_capacity > ((SIZE_MAX - (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA) /
                         2))
               ? 0
               : (_capacity * 2);
}

/*
d_internal_file_read_full
  A full buffer at the read ceiling has no room to make, and one more read
decides (d_internal_file_read_ends): a source that ends there fits exactly,
and one that does not fails with EFBIG, as a measured file does. The result
is 0 when the source ended, and -1 on failure, when the buffer is freed and
the failure reported.
*/
static int
d_internal_file_read_full(
    FILE* _stream,
    char* _buffer
)
{
    const int ends = d_internal_file_read_ends(_stream);

    // the source ends at the ceiling, and so fits exactly
    if (ends > 0)
    {
        return 0;
    }

    // more than the ceiling admits is refused; a read error is the
    // platform's failure
    (void)d_internal_file_read_fail(
              _buffer,
              (ends == 0) ? EFBIG : 0,
              (ends == 0) ? "source exceeds the read ceiling"
                          : "read error on an unsized source");

    return -1;
}

/*
d_internal_file_read_grow
  Makes room in a full buffer (d_internal_file_read_next), or at the ceiling
lets one more read decide (d_internal_file_read_full). The result is 1 when
the buffer grew, 0 when the source ended at the ceiling, and -1 on failure,
when the buffer is freed and the failure reported. A failed realloc frees the
block it left valid.
*/
static int
d_internal_file_read_grow(
    FILE*   _stream,
    char**  _buffer,
    size_t* _capacity
)
{
    const size_t ceiling = D_INTERNAL_IO_READ_CEILING;

    // at the ceiling there is no room to make: the source fits only if it
    // ends here
    if ( (ceiling > 0) &&
         (*_capacity >= ceiling) )
    {
        return d_internal_file_read_full(_stream,
                                         *_buffer);
    }

    const size_t next = d_internal_file_read_next(*_capacity);

    // a doubling past what size_t can hold is refused before it is made
    if (next == 0)
    {
        (void)d_internal_file_read_fail(*_buffer,
                                        EOVERFLOW,
                                        "source exceeds what size_t holds");

        return -1;
    }

    char* const grown = (char*)d_internal_file_realloc(
                            *_buffer,
                            next + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

    // realloc leaves the original block valid on failure, so free the
    // pointer still held rather than the NULL just returned
    if (!grown)
    {
        d_internal_file_free(*_buffer);

        return -1;
    }

    *_buffer   = grown;
    *_capacity = next;

    return 1;
}

/*
d_internal_file_read_shrink
  With D_CFG_FILE_READ_SHRINK_TO_FIT, hands back what was read rather than what
was reserved. A declined shrink keeps the larger block, since it is no reason
to fail a read that has succeeded -- and so does an empty read with no
terminator, which would ask the allocator for zero bytes: realloc may free
the block for that and return NULL, which is not a declined shrink.
*/
static char*
d_internal_file_read_shrink(
    char*  _buffer,
    size_t _used,
    size_t _capacity
)
{
    const size_t keep = _used + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA;

    // nothing to hand back, or nothing a shrink may safely ask for
    if ( (D_INTERNAL_FILE_READ_SHRINK == 0) ||
         (_used == _capacity)               ||
         (keep == 0) )
    {
        return _buffer;
    }

    char* const shrunk = (char*)d_internal_file_realloc(_buffer,
                                                        keep);

    // a declined shrink keeps the original, larger block
    return shrunk ? shrunk : _buffer;
}

/*
d_internal_file_read_first
  The first allocation for a source that cannot be measured:
D_CFG_FILE_READ_GROW_INITIAL, or the read ceiling when that is smaller.
*/
static size_t
d_internal_file_read_first(
    void
)
{
    const size_t initial = (size_t)D_INTERNAL_FILE_READ_GROW_INITIAL;
    const size_t ceiling = D_INTERNAL_IO_READ_CEILING;

    // the first allocation already respects the ceiling
    if ( (ceiling > 0) &&
         (initial > ceiling) )
    {
        return ceiling;
    }

    return initial;
}

/*
d_internal_file_read_some
  One read, of at most a chunk, into the room left in the buffer. The result is
1 when it filled what it asked for, so there may be more; 0 at end of file;
and -1 on a read error, when the buffer is freed and the failure reported. A
short read from an unsized source means EOF or an error, and ferror is the
only way to tell them apart.
*/
static int
d_internal_file_read_some(
    FILE*   _stream,
    char*   _buffer,
    size_t  _capacity,
    size_t* _used
)
{
    const size_t chunk      = d_internal_file_io_clamp(
                                  _capacity - *_used,
                                  (size_t)D_INTERNAL_FILE_READ_CHUNK_SIZE);
    const size_t bytes_read = fread(_buffer + *_used,
                                    1,
                                    chunk,
                                    _stream);

    *_used += bytes_read;

    // all it asked for: there may be more
    if (bytes_read == chunk)
    {
        return 1;
    }

    // an error loses everything read so far
    if (ferror(_stream))
    {
        (void)d_internal_file_read_fail(_buffer,
                                        0,
                                        "read error on an unsized source");

        return -1;
    }

    return 0;
}

/*
d_internal_file_read_unsized
  Reads a stream of unknown length by doubling a buffer until EOF -- the path
that makes d_file_read_all work on a pipe, a character device, and every
entry under /proc and /sys, all of which report zero bytes and then hand over
data. The buffer starts at d_internal_file_read_first and grows by
d_internal_file_read_grow; each read (d_internal_file_read_some) says whether
there may be more. The helpers free the buffer when they fail.
*/
static void*
d_internal_file_read_unsized(
    FILE*   _stream,
    size_t* _size
)
{
    size_t capacity = d_internal_file_read_first();
    size_t used     = 0;
    int    more     = 1;
    char*  buffer   = (char*)d_internal_file_alloc(
                          capacity + (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA);

    // d_internal_file_alloc has already reported the failure
    if (!buffer)
    {
        return NULL;
    }

    // read until the source stops producing
    while (more > 0)
    {
        // a full buffer grows -- or, at the ceiling, may hold all there is
        if (used == capacity)
        {
            more = d_internal_file_read_grow(_stream,
                                             &buffer,
                                             &capacity);
        }
        else
        {
            more = d_internal_file_read_some(_stream,
                                             buffer,
                                             capacity,
                                             &used);
        }
    }

    // the helpers have freed the buffer and reported a failure
    if (more < 0)
    {
        return NULL;
    }

    return d_internal_file_read_finish(d_internal_file_read_shrink(buffer,
                                                                   used,
                                                                   capacity),
                                       used,
                                       _size);
}

#else

/*
d_internal_file_read_unsized
  Without D_CFG_FILE_READ_GROW_UNSIZED a source that cannot be measured is
refused with ESPIPE -- unless one read finds it empty
(d_internal_file_read_ends): an empty file stats as unmeasurably as a /proc
entry does, and is no reason to fail. It reads as the empty buffer every
empty source reads as.
*/
static void*
d_internal_file_read_unsized(
    FILE*   _stream,
    size_t* _size
)
{
    const int ends = d_internal_file_read_ends(_stream);

    // a read error is a failure
    if (ends < 0)
    {
        return d_internal_file_read_fail(NULL,
                                         0,
                                         "read failed");
    }

    // bytes this build cannot measure are refused
    if (ends == 0)
    {
        D_INTERNAL_FILE_FAIL(ESPIPE,
                             "d_file_read_all_stream",
                             NULL,
                             "unsized source and "
                             "D_CFG_FILE_READ_GROW_UNSIZED is 0",
                             NULL);
    }

    // an empty buffer, with room for its terminator; never a request for
    // zero bytes, which may legitimately return NULL
    char* const buffer = (char*)d_internal_file_alloc(1);

    // d_internal_file_alloc has already reported the failure
    if (!buffer)
    {
        return NULL;
    }

    return d_internal_file_read_finish(buffer,
                                       0,
                                       _size);
}

#endif  // D_INTERNAL_FILE_READ_GROW == 1

/*
d_internal_file_read_refuse
  Reports d_file_read_all_into's failure and hands back its code: _code, the
module's own, or with 0 the platform's errno -- or EIO, so that a failure
never reads as success.
*/
static int
d_internal_file_read_refuse(
    int         _code,
    const char* _path,
    const char* _message
)
{
    const int code = (_code != 0)  ? _code
                   : (errno != 0) ? errno
                                  : EIO;

    D_INTERNAL_FILE_SET_ERR(code);
    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                           code,
                           "d_file_read_all_into",
                           D_INTERNAL_FILE_NOTIFY_PATH(_path),
                           _message);
    (void)_path;
    (void)_message;

    return code;
}

/*
d_internal_file_read_into
  d_file_read_all_into's read, on the stream it opened: a measured source
larger than the buffer is refused before a byte is read; an unmeasured one is
read to capacity, and filling the buffer exactly is accepted only when the
next read finds end of file (d_internal_file_read_ends). The result is 0 or
an errno-style code: ERANGE when the source does not fit.
*/
static int
d_internal_file_read_into(
    FILE*       _stream,
    char*       _buf,
    size_t      _capacity,
    size_t*     _size,
    const char* _path
)
{
    size_t length = 0;

    d_internal_file_read_hint(_stream);

    // refuse a file that will not fit before reading a single byte of it
    if ( (d_internal_file_read_size_hint(_stream,
                                         &length)) &&
         (length > _capacity) )
    {
        return d_internal_file_read_refuse(ERANGE,
                                           _path,
                                           "file is larger than the caller's "
                                           "buffer");
    }

    const size_t bytes_read = fread(_buf,
                                    1,
                                    _capacity,
                                    _stream);
    // a full buffer may not be all there is, and only the next read can say
    const int    ends       = (bytes_read < _capacity)
                                  ? 1
                                  : d_internal_file_read_ends(_stream);

    // a failed read reports the platform's code; more than fits is ERANGE
    if ( (ferror(_stream)) ||
         (ends <= 0) )
    {
        return d_internal_file_read_refuse(
                   (ends == 0) ? ERANGE : 0,
                   _path,
                   (ends == 0) ? "source is larger than the caller's buffer"
                               : "read failed");
    }

    (void)d_internal_file_read_finish(_buf,
                                      bytes_read,
                                      _size);

    return 0;
}

#if D_FILE_BACKEND_IS_STDC

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

/*
d_internal_io_store
  Writes a file through its own name, rewriting it or appending to it. The ISO
C backend has only streams: fopen's mode creates a missing file with the
platform's default permissions -- ISO C cannot choose them -- and neither the
sync nor the reservation is available. The close counts, since it is where a
buffered write finally reports failure; but the first failure is the one
reported.
*/
static int
d_internal_io_store(
    const char* _path,
    const void* _data,
    size_t      _size,
    int         _append
)
{
    FILE* const file = d_file_open_stream(_path,
                                          _append ? "ab" : "wb");

    // d_file_open_stream has already reported the failure
    if (!file)
    {
        return -1;
    }

    int       result      = d_internal_file_write_stream(file,
                                                         _data,
                                                         _size);
    const int saved_errno = errno;
    const int closed      = d_file_close_stream(file);

    // the first failure is the one reported
    if (result != 0)
    {
        errno = saved_errno;
    }
    else if (closed != 0)
    {
        result = -1;
    }

    return result;
}

#else

#if (D_INTERNAL_FILE_WRITE_SYNC == 1)

/*
d_internal_io_sync_fd
  Moves the kernel's copy of a file onto the device, as d_file_sync_fd does on
this platform (d_internal_file_sync_raw): F_FULLFSYNC on Apple's systems,
whose fsync stops short of the drive's cache, under cfg_file_sync.h's
choices.
*/
static int
d_internal_io_sync_fd(
    int _fd
)
{
    return d_internal_file_sync_raw(_fd);
}

#else

/*
d_internal_io_sync_fd
  This build does not wait for durable storage, so there is nothing to do:
success means the kernel has the data.
*/
static int
d_internal_io_sync_fd(
    int _fd
)
{
    (void)_fd;

    return 0;
}

#endif  // D_INTERNAL_FILE_WRITE_SYNC == 1

#if (D_INTERNAL_FILE_WRITE_PREALLOC == 1)

/*
d_internal_io_prealloc
  Reserves a file's extents up front, so the filesystem allocates once instead
of growing the file a write at a time. Advisory: a refusal is not reported,
since the writing that follows meets the same shortage and reports it.
*/
static void
d_internal_io_prealloc(
    int    _fd,
    size_t _size
)
{
    // an empty file has nothing to reserve
    if (_size > 0)
    {
        (void)posix_fallocate(_fd,
                              0,
                              (off_t)_size);
    }

    return;
}

#else

/*
d_internal_io_prealloc
  This build reserves nothing up front.
*/
static void
d_internal_io_prealloc(
    int    _fd,
    size_t _size
)
{
    (void)_fd;
    (void)_size;

    return;
}

#endif  // D_INTERNAL_FILE_WRITE_PREALLOC == 1

/*
d_internal_io_fill
  Puts the whole buffer through a descriptor and, when the build waits for
durable storage, onto the device. d_file_write_full_fd moves everything or
fails, so its count is compared in size_t: a buffer past SSIZE_MAX comes back
as the same bits, and -1 is never a size a buffer can have.
*/
static int
d_internal_io_fill(
    int         _fd,
    const void* _data,
    size_t      _size
)
{
    const ssize_t moved = d_file_write_full_fd(_fd,
                                               _data,
                                               _size);

    // d_file_write_full_fd has already reported the failure
    if ((size_t)moved != _size)
    {
        return -1;
    }

    return d_internal_io_sync_fd(_fd);
}

/*
d_internal_io_finish
  Closes a descriptor that was written through and settles the outcome. The
close counts -- some filesystems, NFS among them, report a failed write only
there -- but the first failure is the one reported: the close's only when
everything before it went through.
*/
static int
d_internal_io_finish(
    int _fd,
    int _result
)
{
    const int saved_errno = errno;
    const int closed      = d_file_close_fd(_fd);

    // the earlier failure is the one the caller should see
    if (_result != 0)
    {
        errno = saved_errno;

        return -1;
    }

    return (closed != 0) ? -1 : 0;
}

/*
d_internal_io_store
  Writes a file through its own name, rewriting it or appending to it. A file
this creates gets D_CFG_FILE_WRITE_CREATE_MODE under the umask; one that
exists keeps its permission bits, since it keeps its inode. A known size is
reserved first when the build asks and the file is rewritten -- a reservation
from offset 0 means nothing to an append.
*/
static int
d_internal_io_store(
    const char* _path,
    const void* _data,
    size_t      _size,
    int         _append
)
{
    const int fd = d_file_open(_path,
                               O_WRONLY | O_CREAT |
                                   (_append ? O_APPEND : O_TRUNC),
                               D_INTERNAL_FILE_WRITE_MODE);

    // d_file_open has already reported the failure
    if (fd < 0)
    {
        return -1;
    }

    // a reservation from offset 0 means nothing to an append
    if (!_append)
    {
        d_internal_io_prealloc(fd,
                               _size);
    }

    return d_internal_io_finish(fd,
                                d_internal_io_fill(fd,
                                                   _data,
                                                   _size));
}

#endif  // D_FILE_BACKEND_IS_STDC

#if ( (D_INTERNAL_FILE_WRITE_SYNC == 1) &&                                     \
      D_CFG_IS_OFF(D_CFG_FILE_HAS_WIN32) )

/*
d_internal_io_sync_dir
  A file's name is an entry in its directory, and the entry reaches the device
only when the directory is synced too: without this, a crash can lose a file
the call created, or the rename that replaced one. The directory is the
path's own (d_path_dirname). A platform that cannot sync a directory says so
with EINVAL or EBADF, as PostgreSQL found, and that is taken as done: there is
nothing more to do there.
*/
static int
d_internal_io_sync_dir(
    const char* _path
)
{
    char dir[D_FILE_PATH_MAX] = {0};

    // the directory the entry is in; the path fitted, so its directory does
    if (!d_path_dirname(_path,
                        dir,
                        sizeof(dir)))
    {
        return -1;
    }

    const int fd = d_file_open(dir,
                               O_RDONLY);

    // d_file_open has already reported the failure
    if (fd < 0)
    {
        return -1;
    }

    // synced as the file was (d_internal_file_sync_raw)
    const int result      = d_internal_file_sync_raw(fd);
    const int saved_errno = errno;

    (void)d_file_close_fd(fd);
    errno = saved_errno;

    // a platform that cannot sync a directory has nothing better to offer
    if ( (result != 0) &&
         ( (errno == EINVAL) ||
           (errno == EBADF) ) )
    {
        return 0;
    }

    return result;
}

#else

/*
d_internal_io_sync_dir
  Nothing to do: the build does not wait for durable storage, or this is
Windows, where the C runtime cannot sync a directory and a replacement's
MoveFileEx write-through is all there is.
*/
static int
d_internal_io_sync_dir(
    const char* _path
)
{
    (void)_path;

    return 0;
}

#endif  // D_INTERNAL_FILE_WRITE_SYNC == 1 && !D_CFG_FILE_HAS_WIN32

/*
d_internal_io_settle
  The last step of a whole-file write that went through: its directory is
synced, when the build asks (d_internal_io_sync_dir). A failure there is a
failure of the call, although the file is written: what the caller was
promised is that it would survive a crash, and that is no longer certain.
*/
static int
d_internal_io_settle(
    const char* _path,
    const char* _fn
)
{
    (void)_fn;

    // the file is written; whether its entry is durable is the question
    if (d_internal_io_sync_dir(_path) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               _fn,
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "written, but its directory could not be "
                               "synced");

        return -1;
    }

    return 0;
}

#if (D_INTERNAL_FILE_WRITE_ATOMIC == 1)

/*
d_internal_io_temp_name
  The replacement's template: the target's own path, then
D_INTERNAL_FILE_WRITE_TEMP_SUFFIX, then the placeholder
d_internal_file_temp_create writes a name into. A sibling and not the system
temp directory, deliberately: rename is only atomic within a filesystem, and
/tmp is very often a different one. A name that would not fit fails rather
than being truncated.
*/
static int
d_internal_io_temp_name(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    const size_t path_len   = strlen(_path);
    const size_t suffix_len = sizeof(D_INTERNAL_FILE_WRITE_TEMP_SUFFIX) - 1;
    const size_t length     = path_len + suffix_len +
                              (size_t)D_INTERNAL_FILE_TEMP_SUFFIX_LEN;

    // the target, the suffix, the placeholder and the terminator must fit
    if ((length + 1) > _bufsize)
    {
        return -1;
    }

    memcpy(_buf,
           _path,
           path_len);
    memcpy(_buf + path_len,
           D_INTERNAL_FILE_WRITE_TEMP_SUFFIX,
           suffix_len);
    memset(_buf + path_len + suffix_len,
           'X',
           (size_t)D_INTERNAL_FILE_TEMP_SUFFIX_LEN);
    _buf[length] = '\0';

    return 0;
}

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_io_kept_bits
  Nothing to keep on Windows: its one permission, the read-only attribute,
makes MoveFileEx refuse the replacement anyway. The result is always -1.
*/
static int
d_internal_io_kept_bits(
    const char* _path
)
{
    (void)_path;

    return -1;
}

/*
d_internal_io_restore_bits
  Nothing to restore on Windows (d_internal_io_kept_bits).
*/
static void
d_internal_io_restore_bits(
    int _fd,
    int _bits
)
{
    (void)_fd;
    (void)_bits;

    return;
}

// D_INTERNAL_IO_MOVE
//   macro: MoveFileEx in the form the build's path text takes: the W function
// where the build hands Windows UTF-16 (D_INTERNAL_FILE_WIDE).
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_IO_MOVE MoveFileExW
#else
    #define D_INTERNAL_IO_MOVE MoveFileExA
#endif  // D_INTERNAL_FILE_WIDE == 1

/*
d_internal_io_promote
  Windows' rename fails outright when the target exists, so the promotion goes
through MoveFileEx, which replaces -- and, with MOVEFILE_WRITE_THROUGH, does
not return until the move is on disk. A target in use is refused: one held
open without delete sharing, as the C runtime opens every file, by a reader,
an indexer or a virus scanner, or one another writer is replacing that
moment. So the move is retried first, on git's schedule -- a yield, then 1,
10, 20 and 40 ms -- and a refusal that outlasts it, like every other failure,
is translated as every Win32 failure here is: in use is EACCES.
*/
static int
d_internal_io_promote(
    const char* _temp,
    const char* _target
)
{
    struct d_internal_file_native        temp_held   = {0};
    struct d_internal_file_native        target_held = {0};
    const d_internal_file_nchar_t* const temp =
        d_internal_file_native_of(&temp_held,
                                  _temp);
    const d_internal_file_nchar_t* const target =
        (temp) ? d_internal_file_native_of(&target_held,
                                           _target)
               : NULL;

    // the conversion has set errno
    if (!target)
    {
        d_internal_file_native_release(&temp_held);

        return -1;
    }

    static const DWORD delays[] = { 0, 1, 10, 20, 40 };
    size_t             tries    = 0;
    int                result   = 0;

    // replace the target in one step, waiting out a target briefly in use
    while (!D_INTERNAL_IO_MOVE(temp,
                               target,
                               MOVEFILE_REPLACE_EXISTING |
                                   MOVEFILE_WRITE_THROUGH))
    {
        const DWORD error = GetLastError();

        // only a target in use is worth waiting for, and not for long
        if ( ( (error != ERROR_ACCESS_DENIED) &&
               (error != ERROR_SHARING_VIOLATION) ) ||
             (tries == (sizeof(delays) / sizeof(delays[0]))) )
        {
            const int code = d_internal_file_errno_win32(error);

            D_INTERNAL_FILE_SET_ERR(code);
            (void)code;
            result = -1;

            break;
        }

        Sleep(delays[tries++]);
    }

    d_internal_file_native_release(&temp_held);
    d_internal_file_native_release(&target_held);

    return result;
}

#else

/*
d_internal_io_kept_bits
  The permission bits of the file a replacement takes the place of, or -1 when
there is none to keep: no file, or not a regular one -- a directory fails at
the rename. stat follows a link, so a link at the path lends the replacement
its file's bits.
*/
static int
d_internal_io_kept_bits(
    const char* _path
)
{
    struct stat st = {0};

    // no file, or one whose bits are not a file's
    if ( (stat(_path,
               &st) != 0) ||
         (!S_ISREG(st.st_mode)) )
    {
        return -1;
    }

    return (int)(st.st_mode & 0777);
}

/*
d_internal_io_restore_bits
  A replacement for an existing file is created owner-only, so no one else can
open it while it is not yet theirs to read; here it takes the old file's bits
exactly, since the umask the create went through may not take bits away from
a file that already had them. A filesystem that refuses -- FAT, some network
mounts -- leaves it owner-only: narrower than the file it replaces, never
wider.
*/
static void
d_internal_io_restore_bits(
    int _fd,
    int _bits
)
{
    // a new file keeps the mode it was created with
    if (_bits >= 0)
    {
        (void)fchmod(_fd,
                     (mode_t)_bits);
    }

    return;
}

/*
d_internal_io_promote
  POSIX rename already replaces the target atomically.
*/
static int
d_internal_io_promote(
    const char* _temp,
    const char* _target
)
{
    return rename(_temp,
                  _target);
}

#endif  // D_CFG_FILE_HAS_WIN32

// D_INTERNAL_IO_REMOVE
//   macro: remove, or _wremove where the build hands Windows UTF-16.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_IO_REMOVE _wremove
#else
    #define D_INTERNAL_IO_REMOVE remove
#endif  // D_INTERNAL_FILE_WIDE == 1

/*
d_internal_io_discard
  Removes a replacement that will not be promoted, keeping the errno of the
failure that stopped it. Its name was made by d_file_open in this build's
text, so it converts as it did then.
*/
static void
d_internal_io_discard(
    const char* _temp
)
{
    const int                            saved_errno = errno;
    struct d_internal_file_native        held = {0};
    const d_internal_file_nchar_t* const temp =
        d_internal_file_native_of(&held,
                                  _temp);

    // a name that converted when it was made converts again
    if (temp)
    {
        (void)D_INTERNAL_IO_REMOVE(temp);
    }

    d_internal_file_native_release(&held);
    errno = saved_errno;

    return;
}

/*
d_internal_io_stage
  Writes the replacement at the template _temp: a sibling made exclusively,
under a name no one else can have chosen (d_internal_file_temp_create), so
two writers never share one and nothing planted at a predictable name is
written through. It takes the target's permission bits, as a rewritten file
keeps them, or for a new target D_CFG_FILE_WRITE_CREATE_MODE under the umask;
it is synced before it is closed, when the build asks. A replacement that
failed is removed.
*/
static int
d_internal_io_stage(
    const char* _path,
    char*       _temp,
    const void* _data,
    size_t      _size
)
{
    const int bits = d_internal_io_kept_bits(_path);
    const int fd   = d_internal_file_temp_create(
                         _temp,
                         (bits >= 0) ? 0600 : D_INTERNAL_FILE_WRITE_MODE);

    // d_internal_file_temp_create has already reported the failure
    if (fd < 0)
    {
        return -1;
    }

    d_internal_io_restore_bits(fd,
                               bits);
    d_internal_io_prealloc(fd,
                           _size);

    // a replacement that failed is neither promoted nor left behind
    if (d_internal_io_finish(fd,
                             d_internal_io_fill(fd,
                                                _data,
                                                _size)) != 0)
    {
        d_internal_io_discard(_temp);

        return -1;
    }

    return 0;
}

/*
d_internal_io_write_all
  Replaces the target: the bytes go to a sibling (d_internal_io_stage) that is
renamed over it, so a reader sees the whole old file or the whole new one,
and a failure leaves the old one as it was. Durability comes before
promotion, because a rename that beats its own data to the device can publish
an empty file; and after it the directory is synced, because the rename is
only an entry there.
*/
static int
d_internal_io_write_all_in(
    const char* _path,
    const void* _data,
    size_t      _size,
    char*       _temp_room
)
{
    char* const temp = _temp_room;

    // the template is a sibling, so the rename stays within one filesystem
    if (d_internal_io_temp_name(_path,
                                temp,
                                D_FILE_PATH_MAX) != 0)
    {
        D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                             "d_file_write_all",
                             _path,
                             "no room for a temporary name",
                             -1);
    }

    // the replacement, written in full; a failed one is already gone
    if (d_internal_io_stage(_path,
                            temp,
                            _data,
                            _size) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_write_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "write failed; target is unchanged");

        return -1;
    }

    // promote it over the target in one step
    if (d_internal_io_promote(temp,
                              _path) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_write_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "replace failed; target is unchanged");
        d_internal_io_discard(temp);

        return -1;
    }

    return d_internal_io_settle(_path,
                                "d_file_write_all");
}

/*
d_internal_io_write_all
  d_internal_io_write_all's work is d_internal_io_write_all_in; this keeps the
room it builds a path in, on the heap where that is 32 KiB
(d_internal_file_path).
*/
static int
d_internal_io_write_all(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
    struct d_internal_file_path temp_held = {0};
    char* const temp = d_internal_file_path_room(&temp_held);

    // d_internal_file_path_room has set errno, and reported the failure
    if (!temp)
    {
        d_internal_file_path_release(&temp_held);

        return -1;
    }

    const int result = d_internal_io_write_all_in(_path,
                                                  _data,
                                                  _size,
                                                  temp);

    d_internal_file_path_release(&temp_held);

    return result;
}

#else

/*
d_internal_io_write_all
  Rewrites the target in place, through its own name (d_internal_io_store):
a reader arriving mid-call sees a truncated file, and a crash can leave one.
*/
static int
d_internal_io_write_all(
    const char* _path,
    const void* _data,
    size_t      _size
)
{
    // report the failure; the file may be left truncated
    if (d_internal_io_store(_path,
                            _data,
                            _size,
                            0) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_write_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "write failed; file may be truncated");

        return -1;
    }

    return d_internal_io_settle(_path,
                                "d_file_write_all");
}

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
  One read of at most a chunk -- the build's D_CFG_FILE_READ_CHUNK_SIZE, and no
more than one platform call carries (d_internal_io_read) -- retried across
interruptions. A zero-length request returns 0 without calling the platform,
and needs no buffer, as read(2) needs none.
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
    D_INTERNAL_FILE_REQUIRE( (_buf != NULL) ||
                             (_count == 0),
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

    const size_t chunk  = d_internal_file_io_clamp(
                              _count,
                              (size_t)D_INTERNAL_FILE_READ_CHUNK_SIZE);
    ssize_t      result = -1;

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                d_internal_io_read(_fd,
                                                   _buf,
                                                   chunk));

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
    D_INTERNAL_FILE_REQUIRE( (_buf != NULL) ||
                             (_count == 0),
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
d_file_pread_fd64
  The ISO C backend has no descriptors, and none can be emulated, so this only
reports ENOSYS.
*/
ssize_t
d_file_pread_fd64(
    int       _fd,
    void*     _buf,
    size_t    _count,
    d_off64_t _offset
)
{
    (void)_fd;
    (void)_buf;
    (void)_count;
    (void)_offset;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_pread_fd64",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_pread_fd64
  pread where the platform has it, which is atomic with respect to other users
of the same descriptor. Where it does not -- Windows among them -- the read
is emulated by seeking there and back (d_internal_io_pread), which is not.
*/
ssize_t
d_file_pread_fd64(
    int       _fd,
    void*     _buf,
    size_t    _count,
    d_off64_t _offset
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_pread_fd64",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE( (_buf != NULL) ||
                             (_count == 0),
                            EINVAL,
                            "d_file_pread_fd64",
                            NULL,
                            "buffer is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_offset >= 0,
                            EINVAL,
                            "d_file_pread_fd64",
                            NULL,
                            "offset is negative",
                            -1);

    // a zero-length read is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    const size_t chunk  = d_internal_file_io_clamp(
                              _count,
                              (size_t)D_INTERNAL_FILE_READ_CHUNK_SIZE);
    ssize_t      result = -1;

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                d_internal_io_pread(_fd,
                                                    _buf,
                                                    chunk,
                                                    _offset));

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_pread_fd64",
                               NULL,
                               "pread failed");
    }

    return result;
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
  One write of at most a chunk -- the build's D_CFG_FILE_WRITE_CHUNK_SIZE, and
no more than one platform call carries (d_internal_io_write) -- retried
across interruptions. A zero-length request returns 0 without calling the
platform, and needs no buffer.
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
    D_INTERNAL_FILE_REQUIRE( (_buf != NULL) ||
                             (_count == 0),
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

    const size_t chunk  = d_internal_file_io_clamp(
                              _count,
                              (size_t)D_INTERNAL_FILE_WRITE_CHUNK_SIZE);
    ssize_t      result = -1;

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                d_internal_io_write(_fd,
                                                    _buf,
                                                    chunk));

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
    D_INTERNAL_FILE_REQUIRE( (_buf != NULL) ||
                             (_count == 0),
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
d_file_pwrite_fd64
  The ISO C backend has no descriptors, and none can be emulated, so this only
reports ENOSYS.
*/
ssize_t
d_file_pwrite_fd64(
    int         _fd,
    const void* _buf,
    size_t      _count,
    d_off64_t   _offset
)
{
    (void)_fd;
    (void)_buf;
    (void)_count;
    (void)_offset;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_pwrite_fd64",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_pwrite_fd64
  pwrite where the platform has it, which is atomic with respect to other
users of the same descriptor. Where it does not -- Windows among them -- the
write is emulated by seeking there and back (d_internal_io_pwrite), which is
not.
*/
ssize_t
d_file_pwrite_fd64(
    int         _fd,
    const void* _buf,
    size_t      _count,
    d_off64_t   _offset
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_pwrite_fd64",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE( (_buf != NULL) ||
                             (_count == 0),
                            EINVAL,
                            "d_file_pwrite_fd64",
                            NULL,
                            "buffer is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_offset >= 0,
                            EINVAL,
                            "d_file_pwrite_fd64",
                            NULL,
                            "offset is negative",
                            -1);

    // a zero-length write is a no-op, not an error
    if (_count == 0)
    {
        return 0;
    }

    const size_t chunk  = d_internal_file_io_clamp(
                              _count,
                              (size_t)D_INTERNAL_FILE_WRITE_CHUNK_SIZE);
    ssize_t      result = -1;

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                d_internal_io_pwrite(_fd,
                                                     _buf,
                                                     chunk,
                                                     _offset));

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_pwrite_fd64",
                               NULL,
                               "pwrite failed");
    }

    return result;
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_read_all_stream
  Takes the sized path when the source can be measured and trusted -- what
lies past the stream's position -- and the unsized one when it cannot, so a
pipe or a /proc entry reads correctly rather than returning empty. A stream
reads in its own mode: a text-mode one delivers translated bytes, fewer than
it measured, and that is not a failure. The read-ahead hint goes first; it is
advisory and never fails the read.
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

    return d_internal_file_read_unsized(_stream,
                                        _size);
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
back a prefix is how a caller ends up parsing half a config file
(d_internal_file_read_into). The terminator, when the build writes one, takes
a byte of the caller's buffer, and the close must not overwrite the read's
errno.
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

    // the terminator comes out of the caller's buffer, not a byte past it
    const size_t capacity = _bufsize - (size_t)D_INTERNAL_FILE_READ_NUL_EXTRA;
    FILE* const  file     = d_file_open_stream(_path,
                                               "rb");

    // d_file_open_stream has already reported the failure
    if (!file)
    {
        return errno ? errno : ENOENT;
    }

    const int result      = d_internal_file_read_into(file,
                                                      (char*)_buf,
                                                      capacity,
                                                      _size,
                                                      _path);
    const int saved_errno = errno;

    (void)d_file_close_stream(file);
    errno = saved_errno;

    return result;
}

/*
d_file_write_all
  The target is replaced, or rewritten in place, as the build says
(d_internal_io_write_all; D_CFG_FILE_WRITE_ATOMIC chooses). Either way the
bytes go through a descriptor -- so a new file is created with
D_CFG_FILE_WRITE_CREATE_MODE, and _size bytes in are _size bytes on disk on
every platform -- and are synced, with their directory, when the build asks.
The ISO C backend writes through a stream instead, and creates with the
platform's default mode.
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

    return d_internal_io_write_all(_path,
                                   _data,
                                   _size);
}

/*
d_file_append_all
  Append mode is not merely a seek to the end: on POSIX the offset and the
write are one operation, which is why this is its own function instead of a
flag on d_file_write_all. Atomic replacement is not attempted -- a file being
extended cannot be replaced. Otherwise as d_file_write_all's rewrite: through
the file's own name, a new file created with D_CFG_FILE_WRITE_CREATE_MODE,
and the file and its directory synced when the build asks.
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

    // report the failure; errno is the platform's
    if (d_internal_io_store(_path,
                            _data,
                            _size,
                            1) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_append_all",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "append failed");

        return -1;
    }

    return d_internal_io_settle(_path,
                                "d_file_append_all");
}

//==============================================================================
// 2.  32-BIT TWINS
//==============================================================================

/*
d_file_pread_fd32
  Widens the offset and reads; nothing comes back that needs narrowing.
*/
ssize_t
d_file_pread_fd32(
    int       _fd,
    void*     _buf,
    size_t    _count,
    d_off32_t _offset
)
{
    return d_file_pread_fd64(_fd,
                             _buf,
                             _count,
                             (d_off64_t)_offset);
}

/*
d_file_pwrite_fd32
  Widens the offset and writes; nothing comes back that needs narrowing.
*/
ssize_t
d_file_pwrite_fd32(
    int         _fd,
    const void* _buf,
    size_t      _count,
    d_off32_t   _offset
)
{
    return d_file_pwrite_fd64(_fd,
                              _buf,
                              _count,
                              (d_off64_t)_offset);
}
