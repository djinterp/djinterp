/*******************************************************************************
* djinterp [c]                                                       file_seek.c
*
* Implementation of the positioning and truncation declared in file_seek.h,
* at both widths.
*   Seeking routes to _fseeki64 / _ftelli64 on Windows and fseeko / ftello
* where POSIX has them; a build with neither refuses an offset that does not
* fit a long rather than wrapping it. Each route is one per-platform helper.
* Truncation works on the descriptor, so the stream form flushes stdio's
* buffer before touching the file.
*   The 64-bit twins are the implementations. The 32-bit ones widen their
* argument and call them, and only d_file_tell_stream32 has anything to
* narrow: a position past 2 GiB - 1 fails with EOVERFLOW.
*
*
* path:      /src/djinterp/c/filesys/file_seek.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_seek.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EINVAL, ENOSYS, EOVERFLOW
#include <limits.h>  // LONG_MAX, LONG_MIN
#include <stdio.h>   // FILE, fseek, ftell, fflush, clearerr
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/filesys/file_desc.h"    // d_file_descriptor_stream
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_seek.h"  // D_INTERNAL_FILE_SEEK_FLUSH
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // INT32_MAX


// the large-file interface this source needs (file_common.h, 6.4.1)
D_INTERNAL_FILE_ASSERT_LARGE_FILES;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_seek_raw
  The CRT's _fseeki64, 64-bit on every Windows target.
*/
static int
d_internal_seek_raw(
    FILE*     _stream,
    d_off64_t _offset,
    int       _whence
)
{
    return _fseeki64(_stream,
                     (__int64)_offset,
                     _whence);
}

/*
d_internal_tell_raw
  The CRT's _ftelli64, 64-bit on every Windows target.
*/
static d_off64_t
d_internal_tell_raw(
    FILE* _stream
)
{
    return (d_off64_t)_ftelli64(_stream);
}

#elif (D_INTERNAL_FILE_HAS_FSEEKO == 1)

/*
d_internal_seek_raw
  POSIX fseeko, whose off_t the large-file interface makes 64 bits wide.
*/
static int
d_internal_seek_raw(
    FILE*     _stream,
    d_off64_t _offset,
    int       _whence
)
{
    return fseeko(_stream,
                  (off_t)_offset,
                  _whence);
}

/*
d_internal_tell_raw
  POSIX ftello.
*/
static d_off64_t
d_internal_tell_raw(
    FILE* _stream
)
{
    return (d_off64_t)ftello(_stream);
}

#else

/*
d_internal_seek_raw
  ISO C's fseek, which takes a long: an offset that does not fit one is
refused with EOVERFLOW rather than wrapped to a plausible wrong place that
the caller finds out about later.
*/
static int
d_internal_seek_raw(
    FILE*     _stream,
    d_off64_t _offset,
    int       _whence
)
{
    // no 64-bit seek here: refuse an offset that would wrap
    if ( (_offset > (d_off64_t)LONG_MAX) ||
         (_offset < (d_off64_t)LONG_MIN) )
    {
        D_INTERNAL_FILE_SET_ERR(EOVERFLOW);

        return -1;
    }

    return fseek(_stream,
                 (long)_offset,
                 _whence);
}

/*
d_internal_tell_raw
  ISO C's ftell, which fails rather than wraps past what a long holds.
*/
static d_off64_t
d_internal_tell_raw(
    FILE* _stream
)
{
    return (d_off64_t)ftell(_stream);
}

#endif  // D_CFG_FILE_HAS_WIN32, D_INTERNAL_FILE_HAS_FSEEKO

#if !D_FILE_BACKEND_IS_STDC

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_truncate_raw
  The CRT's _chsize_s, whose error is its return value; it is put in errno,
where every other call here leaves its error.
*/
static int
d_internal_truncate_raw(
    int       _fd,
    d_off64_t _length
)
{
    const int code = (int)_chsize_s(_fd,
                                    (__int64)_length);

    // the code _chsize_s returned is the errno a POSIX call would have set
    if (code != 0)
    {
        errno = code;

        return -1;
    }

    return 0;
}

#else

/*
d_internal_truncate_raw
  POSIX ftruncate, retried across interruptions like any other call.
*/
static int
d_internal_truncate_raw(
    int       _fd,
    d_off64_t _length
)
{
    int result = -1;

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                ftruncate(_fd,
                                          (off_t)_length));

    return result;
}

#endif  // D_CFG_FILE_HAS_WIN32

#endif  // !D_FILE_BACKEND_IS_STDC

//==============================================================================
// 1.  POSITION
//==============================================================================

/*
d_file_seek_stream64
  Not just fseek: on a 32-bit build fseek takes a long, so seeking past 2 GiB
either fails or -- worse -- wraps and silently succeeds at the wrong place.
This routes to _fseeki64 or fseeko, which do not, and a build with neither
refuses an offset that does not fit a long instead of wrapping it
(d_internal_seek_raw).
*/
int
d_file_seek_stream64(
    FILE*     _stream,
    d_off64_t _offset,
    int       _whence
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_seek_stream64",
                            NULL,
                            "stream is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(( (_whence == SEEK_SET) ||
                              (_whence == SEEK_CUR) ||
                              (_whence == SEEK_END) ),
                            EINVAL,
                            "d_file_seek_stream64",
                            NULL,
                            "whence is not SEEK_SET / SEEK_CUR / SEEK_END",
                            -1);

    // report the failure; errno is the platform's, or EOVERFLOW
    if (d_internal_seek_raw(_stream,
                            _offset,
                            _whence) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_seek_stream64",
                               NULL,
                               "seek failed");

        return -1;
    }

    return 0;
}

/*
d_file_tell_stream64
  The same routing as d_file_seek_stream64, for the same reason: ftell's long
wraps at 2 GiB on a 32-bit build and reports a plausible wrong answer.
*/
d_off64_t
d_file_tell_stream64(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_tell_stream64",
                            NULL,
                            "stream is NULL",
                            -1);

    const d_off64_t result = d_internal_tell_raw(_stream);

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_tell_stream64",
                               NULL,
                               "tell failed");

        return -1;
    }

    return result;
}

/*
d_file_rewind_stream
  rewind() returns void, so a caller cannot tell a rewound stream from one
that refused -- and a stream that refused is usually a pipe, which is exactly
the case worth knowing about. So this seeks, and clears the error flag only
once the seek has succeeded.
*/
int
d_file_rewind_stream(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_rewind_stream",
                            NULL,
                            "stream is NULL",
                            -1);

    // a stream that cannot seek cannot rewind
    if (d_file_seek_stream64(_stream,
                             0,
                             SEEK_SET) != 0)
    {
        return -1;
    }

    clearerr(_stream);

    return 0;
}

#if D_FILE_BACKEND_IS_STDC

/*
d_file_truncate_fd64
  The ISO C backend has no descriptors and no truncation primitive, so this
only reports ENOSYS.
*/
int
d_file_truncate_fd64(
    int       _fd,
    d_off64_t _length
)
{
    (void)_fd;
    (void)_length;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_truncate_fd64",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

/*
d_file_truncate_stream64
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_truncate_stream64(
    FILE*     _stream,
    d_off64_t _length
)
{
    (void)_stream;
    (void)_length;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_truncate_stream64",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_truncate_fd64
  One call to the platform (d_internal_truncate_raw), with its error in
errno on every platform.
*/
int
d_file_truncate_fd64(
    int       _fd,
    d_off64_t _length
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_truncate_fd64",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_length >= 0,
                            EINVAL,
                            "d_file_truncate_fd64",
                            NULL,
                            "length is negative",
                            -1);

    // report the failure; errno is the platform's
    if (d_internal_truncate_raw(_fd,
                                _length) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_truncate_fd64",
                               NULL,
                               "truncate failed");

        return -1;
    }

    return 0;
}

/*
d_file_truncate_stream64
  The flush is the whole point, and it is why this is not a one-line wrapper.
Truncation works on the DESCRIPTOR, and the descriptor knows nothing about
bytes still sitting in stdio's buffer. Truncate to 10 with 200 unflushed bytes
pending and stdio writes them out afterwards: the file ends up 200 bytes long,
the truncation appears to have silently done nothing, and the bug reproduces
only when the buffer happens to be dirty.
*/
int
d_file_truncate_stream64(
    FILE*     _stream,
    d_off64_t _length
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_truncate_stream64",
                            NULL,
                            "stream is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_length >= 0,
                            EINVAL,
                            "d_file_truncate_stream64",
                            NULL,
                            "length is negative",
                            -1);

    // hand stdio's buffer to the kernel before changing the file underneath it
    if ( (D_INTERNAL_FILE_SEEK_FLUSH == 1) &&
         (fflush(_stream) != 0) )
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_truncate_stream64",
                               NULL,
                               "flush failed; truncating anyway would lose the "
                               "buffer");

        return -1;
    }

    const int fd = d_file_descriptor_stream(_stream);

    // a stream with no descriptor cannot be truncated
    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_file_truncate_stream64",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    return d_file_truncate_fd64(fd,
                                _length);
}

#endif  // D_FILE_BACKEND_IS_STDC

//==============================================================================
// 2.  32-BIT TWINS
//==============================================================================

/*
d_file_seek_stream32
  Widens the offset and seeks. The position that results is not narrowed, so
SEEK_CUR and SEEK_END may land past 2 GiB, where d_file_tell_stream32 will
report EOVERFLOW but reading and writing carry on.
*/
int
d_file_seek_stream32(
    FILE*     _stream,
    d_off32_t _offset,
    int       _whence
)
{
    return d_file_seek_stream64(_stream,
                                (d_off64_t)_offset,
                                _whence);
}

/*
d_file_tell_stream32
  The one positioning call with something to narrow: a position past
INT32_MAX fails with EOVERFLOW rather than wrap into a plausible wrong one.
*/
d_off32_t
d_file_tell_stream32(
    FILE* _stream
)
{
    const d_off64_t position = d_file_tell_stream64(_stream);

    // d_file_tell_stream64 has already reported the failure
    if (position < 0)
    {
        return -1;
    }

    // a position that does not fit is refused, not truncated
    if (position > (d_off64_t)INT32_MAX)
    {
        D_INTERNAL_FILE_FAIL(EOVERFLOW,
                             "d_file_tell_stream32",
                             NULL,
                             "position exceeds d_off32_t",
                             -1);
    }

    return (d_off32_t)position;
}

/*
d_file_truncate_fd32
  Widens the length and truncates.
*/
int
d_file_truncate_fd32(
    int       _fd,
    d_off32_t _length
)
{
    return d_file_truncate_fd64(_fd,
                                (d_off64_t)_length);
}

/*
d_file_truncate_stream32
  Widens the length and truncates, flushing first as the 64-bit twin does.
*/
int
d_file_truncate_stream32(
    FILE*     _stream,
    d_off32_t _length
)
{
    return d_file_truncate_stream64(_stream,
                                    (d_off64_t)_length);
}
