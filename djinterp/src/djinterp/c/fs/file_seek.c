/*******************************************************************************
* djinterp [c]                                                       file_seek.c
*
* Implementation of the 64-bit positioning and truncation declared in
* file_seek.h.
*   Seeking routes to _fseeki64 / _ftelli64 on Windows and fseeko / ftello
* where POSIX has them; a build with neither refuses an offset that does not
* fit a long rather than wrapping it. Truncation works on the descriptor, so
* the stream form flushes stdio's buffer before touching the file.
*
*
* path:      /src/djinterp/c/fs/file_seek.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_seek.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EINVAL, ENOSYS, EOVERFLOW
#include <limits.h>  // LONG_MAX, LONG_MIN
#include <stdio.h>   // FILE, fseek, ftell, fflush, clearerr, fileno
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/fs/cfg_file_seek.h"  // D_INTERNAL_FILE_SEEK_FLUSH


//==============================================================================
// 1.  POSITION
//==============================================================================

/*
d_file_seek_stream
  Not just fseek: on a 32-bit build fseek takes a long, so seeking past 2 GiB
either fails or -- worse -- wraps and silently succeeds at the wrong place.
This routes to _fseeki64 or fseeko, which do not, and a build with neither
refuses an offset that does not fit a long instead of wrapping it.
*/
int
d_file_seek_stream(
    FILE*   _stream,
    d_off_t _offset,
    int     _whence
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_seek_stream",
                            NULL,
                            "stream is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(( (_whence == SEEK_SET) ||
                              (_whence == SEEK_CUR) ||
                              (_whence == SEEK_END) ),
                            EINVAL,
                            "d_file_seek_stream",
                            NULL,
                            "whence is not SEEK_SET / SEEK_CUR / SEEK_END",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int result = _fseeki64(_stream,
                                 (__int64)_offset,
                                 _whence);
#elif (D_INTERNAL_FILE_HAS_FSEEKO == 1)
    const int result = fseeko(_stream,
                              (off_t)_offset,
                              _whence);
#else
    // no 64-bit seek here: refuse an offset that would wrap rather than seek
    // to a plausible wrong place and let the caller find out later
    if ( (_offset > (d_off_t)LONG_MAX) ||
         (_offset < (d_off_t)LONG_MIN) )
    {
        D_INTERNAL_FILE_FAIL(EOVERFLOW,
                             "d_file_seek_stream",
                             NULL,
                             "offset exceeds long on a build without fseeko",
                             -1);
    }

    const int result = fseek(_stream,
                             (long)_offset,
                             _whence);
#endif

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_seek_stream",
                               NULL,
                               "seek failed");

        return -1;
    }

    return 0;
}

/*
d_file_tell_stream
  The same routing as d_file_seek_stream, for the same reason: ftell's long
wraps at 2 GiB on a 32-bit build and reports a plausible wrong answer.
*/
d_off_t
d_file_tell_stream(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_tell_stream",
                            NULL,
                            "stream is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const d_off_t result = (d_off_t)_ftelli64(_stream);
#elif (D_INTERNAL_FILE_HAS_FSEEKO == 1)
    const d_off_t result = (d_off_t)ftello(_stream);
#else
    const d_off_t result = (d_off_t)ftell(_stream);
#endif

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_tell_stream",
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
    if (d_file_seek_stream(_stream,
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
d_file_truncate_fd
  The ISO C backend has no descriptors and no truncation primitive, so this
only reports ENOSYS.
*/
int
d_file_truncate_fd(
    int     _fd,
    d_off_t _length
)
{
    (void)_fd;
    (void)_length;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_truncate_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_truncate_fd
  _chsize_s reports the error as its return value rather than through errno,
so on Windows the value is moved into errno to give callers one contract.
Elsewhere ftruncate is retried across interruptions like any other call.
*/
int
d_file_truncate_fd(
    int     _fd,
    d_off_t _length
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_truncate_fd",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_length >= 0,
                            EINVAL,
                            "d_file_truncate_fd",
                            NULL,
                            "length is negative",
                            -1);

    int result = -1;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // _chsize_s reports the errno value rather than setting errno
    result = _chsize_s(_fd,
                       (__int64)_length);

    // move the reported code into errno, where callers look for it
    if (result != 0)
    {
        D_INTERNAL_FILE_SET_ERR(result);
        result = -1;
    }
#else
    D_INTERNAL_FILE_RETRY_EINTR(result,
                                ftruncate(_fd,
                                          (off_t)_length));
#endif

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_truncate_fd",
                               NULL,
                               "truncate failed");

        return -1;
    }

    return 0;
}

#endif  // D_FILE_BACKEND_IS_STDC

#if D_FILE_BACKEND_IS_STDC

/*
d_file_truncate_stream
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_truncate_stream(
    FILE*   _stream,
    d_off_t _length
)
{
    (void)_stream;
    (void)_length;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_truncate_stream",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_truncate_stream
  The flush is the whole point, and it is why this is not a one-line wrapper.
Truncation works on the DESCRIPTOR, and the descriptor knows nothing about
bytes still sitting in stdio's buffer. Truncate to 10 with 200 unflushed bytes
pending and stdio writes them out afterwards: the file ends up 200 bytes long,
the truncation appears to have silently done nothing, and the bug reproduces
only when the buffer happens to be dirty.
*/
int
d_file_truncate_stream(
    FILE*   _stream,
    d_off_t _length
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_truncate_stream",
                            NULL,
                            "stream is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_length >= 0,
                            EINVAL,
                            "d_file_truncate_stream",
                            NULL,
                            "length is negative",
                            -1);

#if (D_INTERNAL_FILE_SEEK_FLUSH == 1)
    // hand stdio's buffer to the kernel before changing the file underneath it
    if (fflush(_stream) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_truncate_stream",
                               NULL,
                               "flush failed; truncating anyway would lose the "
                               "buffer");

        return -1;
    }
#endif

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int fd = _fileno(_stream);
#else
    const int fd = fileno(_stream);
#endif

    // a stream with no descriptor cannot be truncated
    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_file_truncate_stream",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    return d_file_truncate_fd(fd,
                              _length);
}

#endif  // D_FILE_BACKEND_IS_STDC
