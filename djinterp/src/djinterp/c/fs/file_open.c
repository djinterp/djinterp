/*******************************************************************************
* djinterp [c]                                                       file_open.c
*
* Implementation of the stream opening and closing declared in file_open.h.
*   Three file-local steps shape every open: the caller's mode is decorated
* with the build's policy, the path is opened through the one backend call
* this build selected (_fsopen, fopen_s or fopen), and the new stream is given
* the configured buffering. Every entry point here funnels through them, so
* the policy is applied once rather than re-litigated per caller.
*
*
* path:      /src/djinterp/c/fs/file_open.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_open.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EINVAL, ENOENT, ENOSYS
#include <stddef.h>  // NULL, size_t
#include <stdio.h>   // FILE, EOF, fopen, freopen, fclose, fdopen, setvbuf
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/fs/cfg_file_open.h"  // D_INTERNAL_FILE_OPEN_*


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if (D_INTERNAL_FILE_OPEN_DECORATE == 1)

/*
d_internal_file_open_mode
  Rewrites a caller's mode string so it carries this build's open policy: a
'b' when D_CFG_FILE_OPEN_BINARY_DEFAULT is set and the caller named neither
'b' nor 't', and the target's close-on-exec character when the platform has
one and D_CFG_FILE_OPEN_CLOEXEC is set. Anything the caller already asked for
is left alone -- an explicit "rt" is an explicit request for text mode, and
the job here is to supply a default, not to overrule.
  If the rewrite would not fit, the caller's string is returned untouched: a
mode string longer than the buffer is malformed, and fopen rejects it with a
better error than could be invented here. So the result is _buf or _mode,
and never NULL.
*/
static const char*
d_internal_file_open_mode(
    const char* _mode,
    char*       _buf,
    size_t      _bufsize
)
{
    const size_t length = strlen(_mode);

    // leave a malformed or oversized mode to the platform to reject
    if ((length + 3) > _bufsize)
    {
        return _mode;
    }

    int has_binary  = 0;
    int has_text    = 0;
    int has_cloexec = 0;

    // find out what the caller already asked for
    for (size_t idx = 0; idx < length; ++idx)
    {
        if (_mode[idx] == 'b')
        {
            has_binary = 1;
        }
        else if (_mode[idx] == 't')
        {
            has_text = 1;
        }
#if (D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR != 0)
        else if (_mode[idx] == D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR)
        {
            has_cloexec = 1;
        }
#endif
    }

    memcpy(_buf,
           _mode,
           length);

    size_t out = length;

#if D_CFG_IS_ON(D_CFG_FILE_OPEN_BINARY_DEFAULT)
    // supply binary only where the caller expressed no preference
    if ( (!has_binary) &&
         (!has_text) )
    {
        _buf[out++] = 'b';
    }
#endif

#if (D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR != 0)
    // supply close-on-exec unless it is already there
    if (!has_cloexec)
    {
        _buf[out++] = (char)D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR;
    }
#endif

    _buf[out] = '\0';

    // a configuration may leave any of these unread
    (void)has_binary;
    (void)has_text;
    (void)has_cloexec;

    return _buf;
}

#endif  // D_INTERNAL_FILE_OPEN_DECORATE == 1

/*
d_internal_file_open_configure
  Applies this build's post-open policy to a freshly opened stream -- at
present, the configured stdio buffering. A setvbuf failure is deliberately not
fatal: the stream is open and usable, and refusing to hand it back because the
platform declined a buffer size would turn a tuning preference into an outage.
The argument is returned as given, NULL included, so that an open expression
can be wrapped in it.
*/
static FILE*
d_internal_file_open_configure(
    FILE* _stream
)
{
#if (D_INTERNAL_FILE_OPEN_SETVBUF == 1)
    // only a stream that exists can be buffered
    if (_stream)
    {
        // a declined buffer size is a warning, never a failure
        if (setvbuf(_stream,
                    NULL,
                    D_CFG_FILE_OPEN_BUFFER_MODE,
                    (size_t)D_CFG_FILE_OPEN_BUFFER_SIZE) != 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                                   0,
                                   "d_file_open_stream",
                                   NULL,
                                   "setvbuf declined; using stdio's buffer");
        }
    }
#endif

    return _stream;
}

#if (D_INTERNAL_FILE_OPEN_USE_FSOPEN == 1)

/*
d_internal_file_open_raw
  The one place this subframework asks the platform to open a path, so the
backend choice is made once rather than per entry point. This build shares the
file through _fsopen, with the sharing mode it was configured for.
*/
static FILE*
d_internal_file_open_raw(
    const char* _filename,
    const char* _mode
)
{
    return _fsopen(_filename,
                   _mode,
                   D_INTERNAL_FILE_OPEN_SHARE_FLAG);
}

#elif (D_INTERNAL_FILE_OPEN_FOPEN_S == 1)

/*
d_internal_file_open_raw
  The one place this subframework asks the platform to open a path, so the
backend choice is made once rather than per entry point. This build uses
fopen_s, which reports through its return value rather than errno; that is
normalized to the fopen contract so every caller has one failure shape.
*/
static FILE*
d_internal_file_open_raw(
    const char* _filename,
    const char* _mode
)
{
    FILE* result = NULL;

    // fopen_s reports through its return value, not errno; normalize to the
    // fopen contract so every caller has one failure shape to handle
    if (fopen_s(&result,
                _filename,
                _mode) != 0)
    {
        return NULL;
    }

    return result;
}

#else

/*
d_internal_file_open_raw
  The one place this subframework asks the platform to open a path, so the
backend choice is made once rather than per entry point. This build uses
plain fopen.
*/
static FILE*
d_internal_file_open_raw(
    const char* _filename,
    const char* _mode
)
{
    return fopen(_filename,
                 _mode);
}

#endif  // D_INTERNAL_FILE_OPEN_USE_FSOPEN, D_INTERNAL_FILE_OPEN_FOPEN_S

//==============================================================================
// 1.  STREAMS
//==============================================================================

/*
d_file_open_stream
  Decorate, open, configure: the mode gains this build's policy, the path goes
through the backend call this build selected, and the new stream is given the
configured buffering.
*/
FILE*
d_file_open_stream(
    const char* _filename,
    const char* _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_filename != NULL,
                            EINVAL,
                            "d_file_open_stream",
                            NULL,
                            "filename is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_file_open_stream",
                            _filename,
                            "mode is NULL",
                            NULL);

#if (D_INTERNAL_FILE_OPEN_DECORATE == 1)
    char              mode_buf[D_INTERNAL_FILE_OPEN_MODE_MAX];
    const char* const mode = d_internal_file_open_mode(_mode,
                                                       mode_buf,
                                                       sizeof(mode_buf));
#else
    const char* const mode = _mode;
#endif

    FILE* const result = d_internal_file_open_raw(_filename,
                                                  mode);

    // report the failure; the caller only learns that it got NULL
    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_open_stream",
                               D_INTERNAL_FILE_NOTIFY_PATH(_filename),
                               "open failed");

        return NULL;
    }

    return d_internal_file_open_configure(result);
}

/*
d_file_open_stream_s
  *_stream is cleared as soon as it is known to be writable, before anything
else can fail. A failed open that left errno clear is reported as ENOENT
rather than as success.
*/
int
d_file_open_stream_s(
    FILE**      _stream,
    const char* _filename,
    const char* _mode
)
{
    // parameter validation: _stream first, since everything else writes
    // through it
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_open_stream_s",
                            NULL,
                            "stream out-parameter is NULL",
                            EINVAL);

    *_stream = NULL;

    D_INTERNAL_FILE_REQUIRE(_filename != NULL,
                            EINVAL,
                            "d_file_open_stream_s",
                            NULL,
                            "filename is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_file_open_stream_s",
                            _filename,
                            "mode is NULL",
                            EINVAL);

    *_stream = d_file_open_stream(_filename,
                                  _mode);

    // translate a failure into a code the caller can return
    if (!*_stream)
    {
        return errno ? errno : ENOENT;
    }

    return 0;
}

/*
d_file_reopen_stream
  The same mode decoration as d_file_open_stream, then freopen or freopen_s.
The C contract makes failure destructive -- the original stream is closed
whether or not the reopen succeeds -- and the failure notice says so.
*/
FILE*
d_file_reopen_stream(
    const char* _filename,
    const char* _mode,
    FILE*       _stream
)
{
    // parameter validation: _filename may legitimately be NULL here
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_file_reopen_stream",
                            _filename,
                            "mode is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_reopen_stream",
                            _filename,
                            "stream is NULL",
                            NULL);

#if (D_INTERNAL_FILE_OPEN_DECORATE == 1)
    char              mode_buf[D_INTERNAL_FILE_OPEN_MODE_MAX];
    const char* const mode = d_internal_file_open_mode(_mode,
                                                       mode_buf,
                                                       sizeof(mode_buf));
#else
    const char* const mode = _mode;
#endif

#if (D_INTERNAL_FILE_OPEN_FOPEN_S == 1)
    FILE* result = NULL;

    // freopen_s reports through its return value; normalize to NULL
    if (freopen_s(&result,
                  _filename,
                  mode,
                  _stream) != 0)
    {
        return NULL;
    }
#else
    FILE* const result = freopen(_filename,
                                 mode,
                                 _stream);
#endif

    // report the failure; note the stream is gone either way
    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_reopen_stream",
                               D_INTERNAL_FILE_NOTIFY_PATH(_filename),
                               "reopen failed; stream is now closed");

        return NULL;
    }

    return d_internal_file_open_configure(result);
}

/*
d_file_reopen_stream_s
  *_newstream is cleared as soon as it is known to be writable, before
anything else can fail. A failed reopen that left errno clear is reported as
ENOENT rather than as success.
*/
int
d_file_reopen_stream_s(
    FILE**      _newstream,
    const char* _filename,
    const char* _mode,
    FILE*       _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_newstream != NULL,
                            EINVAL,
                            "d_file_reopen_stream_s",
                            NULL,
                            "stream out-parameter is NULL",
                            EINVAL);

    *_newstream = NULL;

    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_file_reopen_stream_s",
                            _filename,
                            "mode is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_reopen_stream_s",
                            _filename,
                            "stream is NULL",
                            EINVAL);

    *_newstream = d_file_reopen_stream(_filename,
                                       _mode,
                                       _stream);

    // translate a failure into a code the caller can return
    if (!*_newstream)
    {
        return errno ? errno : ENOENT;
    }

    return 0;
}

#if D_FILE_BACKEND_IS_STDC

/*
d_file_open_stream_fd
  The ISO C backend has no descriptors to adopt, so this cannot be emulated --
only reported, as ENOSYS.
*/
FILE*
d_file_open_stream_fd(
    int         _fd,
    const char* _mode
)
{
    (void)_fd;
    (void)_mode;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_open_stream_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         NULL);
}

#else

/*
d_file_open_stream_fd
  fdopen adopts the descriptor rather than duplicating it, so from here on the
stream owns _fd. The new stream is given the same configured buffering as any
other open.
*/
FILE*
d_file_open_stream_fd(
    int         _fd,
    const char* _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_open_stream_fd",
                            NULL,
                            "descriptor is negative",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_file_open_stream_fd",
                            NULL,
                            "mode is NULL",
                            NULL);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    FILE* const result = _fdopen(_fd,
                                 _mode);
#else
    FILE* const result = fdopen(_fd,
                                _mode);
#endif

    // report the failure; errno is the platform's
    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_open_stream_fd",
                               NULL,
                               "fdopen failed");

        return NULL;
    }

    return d_internal_file_open_configure(result);
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_close_stream
  Closing is a decision this subframework owns rather than one scattered
across callers -- and fclose's failure mode is worth surfacing: a buffered
write that could not be flushed is reported here, at close, and nowhere else.
*/
int
d_file_close_stream(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_close_stream",
                            NULL,
                            "stream is NULL",
                            EOF);

    const int result = fclose(_stream);

    // a failure here means buffered data never reached the file
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_close_stream",
                               NULL,
                               "close failed; buffered data may be lost");
    }

    return result;
}
