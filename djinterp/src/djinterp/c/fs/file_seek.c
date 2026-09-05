/******************************************************************************
* djinterp [c]                                                     file_seek.c
*
* path:      /src/djinterp/c/fs/file_seek.c
******************************************************************************/
// std
#include <limits.h>
// djinterp
#include "../../../../inc/djinterp/c/fs/file_seek.h"


// I.    Positioning

/*
d_fseeko
  Sets a stream's position, with a 64-bit offset on every target.
  The reason this is not just fseek: on a 32-bit build fseek takes a long, so
seeking past 2 GiB either fails or -- worse -- wraps and silently succeeds at
the wrong place. This routes to fseeko / _fseeki64, which do not.

Parameter(s):
  _stream: an open stream.
  _offset: the offset, interpreted per _whence.
  _whence: SEEK_SET, SEEK_CUR or SEEK_END.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_fseeko
(
    FILE*   _stream,
    d_off_t _offset,
    int     _whence
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_fseeko",
                            NULL,
                            "stream is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE( (_whence == SEEK_SET) ||
                             (_whence == SEEK_CUR) ||
                             (_whence == SEEK_END),
                            EINVAL,
                            "d_fseeko",
                            NULL,
                            "whence is not SEEK_SET / SEEK_CUR / SEEK_END",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _fseeki64(_stream, (__int64)_offset, _whence);
#elif (D_INTERNAL_FILE_HAS_FSEEKO == 1)
    result = fseeko(_stream, (off_t)_offset, _whence);
#else
    // no 64-bit seek here: refuse an offset that would wrap rather than seek
    // to a plausible wrong place and let the caller find out later
    if ( (_offset > (d_off_t)LONG_MAX) ||
         (_offset < (d_off_t)LONG_MIN) )
    {
        D_INTERNAL_FILE_FAIL(EOVERFLOW,
                             "d_fseeko",
                             NULL,
                             "offset exceeds long on a build without fseeko",
                             -1);
    }

    result = fseek(_stream, (long)_offset, _whence);
#endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fseeko",
                               NULL,
                               "seek failed");

        return -1;
    }

    return 0;
}


/*
d_ftello
  Reports a stream's position, 64-bit on every target.

Parameter(s):
  _stream: an open stream.
Return:
  The current offset on success, or -1 on failure with errno set.
*/
d_off_t
d_ftello
(
    FILE* _stream
)
{
    d_off_t result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_ftello",
                            NULL,
                            "stream is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = (d_off_t)_ftelli64(_stream);
#elif (D_INTERNAL_FILE_HAS_FSEEKO == 1)
    result = (d_off_t)ftello(_stream);
#else
    result = (d_off_t)ftell(_stream);
#endif

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_ftello",
                               NULL,
                               "tell failed");

        return -1;
    }

    return result;
}


/*
d_rewind
  Returns a stream to its start and clears its error flag.
  Unlike ISO C's rewind, this reports failure. rewind() returns void, so a
caller cannot tell a rewound stream from one that refused -- and a stream that
refused is usually a pipe, which is exactly the case worth knowing about.

Parameter(s):
  _stream: an open stream.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_rewind
(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_rewind",
                            NULL,
                            "stream is NULL",
                            -1);

    if (d_fseeko(_stream, 0, SEEK_SET) != 0)
    {
        return -1;
    }

    clearerr(_stream);

    return 0;
}


// II.   Truncation

/*
d_ftruncate
  Sets a file's length by descriptor.
  Both directions are legal and neither is a no-op: shrinking discards the
tail, and EXTENDING zero-fills. An extend produces a sparse region on
filesystems that support one, so the file's apparent size grows while its disk
usage does not.
  The descriptor must be open for writing. Position is not changed, and may
legitimately end up past the new end of file.

Parameter(s):
  _fd:     a descriptor open for writing.
  _length: the new length in bytes.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_ftruncate
(
    int     _fd,
    d_off_t _length
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;
    (void)_length;

    // the ISO C backend has no descriptors and no truncation primitive
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_ftruncate",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_ftruncate",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_length >= 0,
                            EINVAL,
                            "d_ftruncate",
                            NULL,
                            "length is negative",
                            -1);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // _chsize_s reports the errno value rather than setting errno
    result = _chsize_s(_fd, (__int64)_length);

    if (result != 0)
    {
        D_INTERNAL_FILE_SET_ERR(result);
        result = -1;
    }
    #else
    D_INTERNAL_FILE_RETRY_EINTR(result, ftruncate(_fd, (off_t)_length));
    #endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_ftruncate",
                               NULL,
                               "truncate failed");

        return -1;
    }

    return 0;
#endif
}


/*
d_ftruncate_stream
  Sets a file's length through the stream that owns it.
  The flush is the whole point, and it is why this is not a one-line wrapper.
Truncation works on the DESCRIPTOR, and the descriptor knows nothing about
bytes still sitting in stdio's buffer. Truncate to 10 with 200 unflushed bytes
pending and stdio writes them out afterwards: the file ends up 200 bytes long,
the truncation appears to have silently done nothing, and the bug reproduces
only when the buffer happens to be dirty.
  Flushing first makes the two layers agree. Disable it with
D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE only if you flush yourself.

Parameter(s):
  _stream: a stream open for writing.
  _length: the new length in bytes.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_ftruncate_stream
(
    FILE*   _stream,
    d_off_t _length
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_stream;
    (void)_length;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_ftruncate_stream",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    int fd;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_ftruncate_stream",
                            NULL,
                            "stream is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_length >= 0,
                            EINVAL,
                            "d_ftruncate_stream",
                            NULL,
                            "length is negative",
                            -1);

    #if (D_INTERNAL_FILE_SEEK_FLUSH == 1)
    // hand stdio's buffer to the kernel before changing the file underneath it
    if (fflush(_stream) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_ftruncate_stream",
                               NULL,
                               "flush failed; truncating anyway would lose the "
                               "buffer");

        return -1;
    }
    #endif

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    fd = _fileno(_stream);
    #else
    fd = fileno(_stream);
    #endif

    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_ftruncate_stream",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    return d_ftruncate(fd, _length);
#endif
}
