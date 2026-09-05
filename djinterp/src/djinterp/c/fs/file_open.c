/******************************************************************************
* djinterp [c]                                                     file_open.c
*
* path:      /src/djinterp/c/fs/file_open.c
******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/c/fs/file_open.h"


// Internal definitions

#if (D_INTERNAL_FILE_OPEN_DECORATE == 1)

/*
d_internal_file_open_mode
  Rewrites a caller's mode string so it carries this build's open policy: a
'b' when D_CFG_FILE_OPEN_BINARY_DEFAULT is set and the caller named neither
'b' nor 't', and the target's close-on-exec character when the platform has
one and D_CFG_FILE_OPEN_CLOEXEC is set.
  Anything the caller already asked for is left alone -- an explicit "rt" is
an explicit request for text mode, and this function's job is to supply a
default, not to overrule.
  If the rewrite would not fit, the caller's string is returned untouched: a
mode string longer than the buffer is malformed, and fopen rejects it with a
better error than we could invent.

Parameter(s):
  _mode:    the caller's mode string; must not be NULL.
  _buf:     scratch space for the rewrite.
  _bufsize: size of _buf, in bytes.
Return:
  A pointer to the mode string to pass on -- either _buf or _mode itself.
Never NULL.
*/
static const char*
d_internal_file_open_mode
(
    const char* _mode,
    char*       _buf,
    size_t      _bufsize
)
{
    size_t length;
    size_t idx;
    size_t out;
    int    has_binary;
    int    has_text;
    int    has_cloexec;

    length = strlen(_mode);

    // leave a malformed or oversized mode to the platform to reject
    if ((length + 3) > _bufsize)
    {
        return _mode;
    }

    has_binary  = 0;
    has_text    = 0;
    has_cloexec = 0;

    // find out what the caller already asked for
    for (idx = 0; idx < length; ++idx)
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

    memcpy(_buf, _mode, length);
    out = length;

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

    (void)has_binary;
    (void)has_text;
    (void)has_cloexec;

    return _buf;
}

#endif  // D_INTERNAL_FILE_OPEN_DECORATE


/*
d_internal_file_open_configure
  Applies this build's post-open policy to a freshly opened stream. Currently
that means the configured stdio buffering, if any.
  A setvbuf failure is deliberately not fatal: the stream is open and usable,
and refusing to hand it back because the platform declined a buffer size would
turn a tuning preference into an outage.

Parameter(s):
  _stream: the newly opened stream; may be NULL, in which case nothing is done.
Return:
  _stream, unchanged, so callers can wrap an open expression in it.
*/
static FILE*
d_internal_file_open_configure
(
    FILE* _stream
)
{
#if (D_INTERNAL_FILE_OPEN_SETVBUF == 1)
    if (_stream)
    {
        if (setvbuf(_stream,
                    NULL,
                    D_CFG_FILE_OPEN_BUFFER_MODE,
                    (size_t)D_CFG_FILE_OPEN_BUFFER_SIZE) != 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                                   0,
                                   "d_fopen",
                                   NULL,
                                   "setvbuf declined; using stdio's buffer");
        }
    }
#endif

    return _stream;
}


/*
d_internal_file_open_raw
  The one place this subframework actually asks the platform to open a path.
Everything above it -- d_fopen, d_fopen_s, and the whole-file helpers in
file_read / file_write -- funnels through here, so the backend choice is made
once rather than re-litigated per entry point.

Parameter(s):
  _filename: path to open; must not be NULL.
  _mode:     already-decorated mode string; must not be NULL.
Return:
  An open stream, or NULL on failure with errno set by the platform.
*/
static FILE*
d_internal_file_open_raw
(
    const char* _filename,
    const char* _mode
)
{
#if (D_INTERNAL_FILE_OPEN_USE_FSOPEN == 1)
    return _fsopen(_filename, _mode, D_INTERNAL_FILE_OPEN_SHARE_FLAG);
#elif (D_INTERNAL_FILE_OPEN_FOPEN_S == 1)
    FILE* result;

    result = NULL;

    // fopen_s reports through its return value, not errno; normalize to the
    // fopen contract so every caller has one failure shape to handle.
    if (fopen_s(&result, _filename, _mode) != 0)
    {
        return NULL;
    }

    return result;
#else
    return fopen(_filename, _mode);
#endif
}


// I.    Stream opening

/*
d_fopen
  Portable file open with consistent behaviour across platforms.

Parameter(s):
  _filename: path to file.
  _mode:     file open mode ("r", "w", "a", ...), optionally with 'b', '+'.
Return:
  A file pointer on success, or NULL on failure with errno set.
*/
FILE*
d_fopen
(
    const char* _filename,
    const char* _mode
)
{
#if (D_INTERNAL_FILE_OPEN_DECORATE == 1)
    char        mode_buf[D_INTERNAL_FILE_OPEN_MODE_MAX];
    const char* mode;
#endif
    FILE*       result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_filename != NULL,
                            EINVAL,
                            "d_fopen",
                            NULL,
                            "filename is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_fopen",
                            _filename,
                            "mode is NULL",
                            NULL);

#if (D_INTERNAL_FILE_OPEN_DECORATE == 1)
    mode = d_internal_file_open_mode(_mode, mode_buf, sizeof(mode_buf));
    result = d_internal_file_open_raw(_filename, mode);
#else
    result = d_internal_file_open_raw(_filename, _mode);
#endif

    // report the failure; the caller only learns that it got NULL
    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fopen",
                               D_INTERNAL_FILE_NOTIFY_PATH(_filename),
                               "open failed");

        return NULL;
    }

    return d_internal_file_open_configure(result);
}


/*
d_fopen_s
  Secure file open (C11 Annex K fopen_s equivalent).
  Unlike d_fopen this reports through the return value, and it clears
*_stream before doing anything else, so a caller that ignores the return code
is left holding NULL rather than an indeterminate pointer.

Parameter(s):
  _stream:   receives the file pointer; must not be NULL.
  _filename: path to file.
  _mode:     file open mode.
Return:
  0 on success, or a non-zero error code (EINVAL for a bad argument,
otherwise the platform's errno) on failure.
*/
int
d_fopen_s
(
    FILE**      _stream,
    const char* _filename,
    const char* _mode
)
{
    // parameter validation: _stream first, since everything else writes
    // through it
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_fopen_s",
                            NULL,
                            "stream out-parameter is NULL",
                            EINVAL);

    *_stream = NULL;

    D_INTERNAL_FILE_REQUIRE(_filename != NULL,
                            EINVAL,
                            "d_fopen_s",
                            NULL,
                            "filename is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_fopen_s",
                            _filename,
                            "mode is NULL",
                            EINVAL);

    *_stream = d_fopen(_filename, _mode);

    if (!*_stream)
    {
        return errno ? errno : ENOENT;
    }

    return 0;
}


/*
d_freopen
  Portable file reopen.

Parameter(s):
  _filename: path to file, or NULL to change the mode of _stream in place.
  _mode:     new file open mode.
  _stream:   the existing stream to reopen.
Return:
  A file pointer on success, or NULL on failure. Note the C contract: on
failure _stream is closed regardless, so the caller must not reuse it.
*/
FILE*
d_freopen
(
    const char* _filename,
    const char* _mode,
    FILE*       _stream
)
{
#if (D_INTERNAL_FILE_OPEN_DECORATE == 1)
    char        mode_buf[D_INTERNAL_FILE_OPEN_MODE_MAX];
    const char* mode;
#endif
    FILE*       result;

    // parameter validation: _filename may legitimately be NULL here
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_freopen",
                            _filename,
                            "mode is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_freopen",
                            _filename,
                            "stream is NULL",
                            NULL);

#if (D_INTERNAL_FILE_OPEN_DECORATE == 1)
    mode = d_internal_file_open_mode(_mode, mode_buf, sizeof(mode_buf));
#endif

#if (D_INTERNAL_FILE_OPEN_FOPEN_S == 1)
    result = NULL;

    #if (D_INTERNAL_FILE_OPEN_DECORATE == 1)
    if (freopen_s(&result, _filename, mode, _stream) != 0)
    #else
    if (freopen_s(&result, _filename, _mode, _stream) != 0)
    #endif
    {
        return NULL;
    }
#else
    #if (D_INTERNAL_FILE_OPEN_DECORATE == 1)
    result = freopen(_filename, mode, _stream);
    #else
    result = freopen(_filename, _mode, _stream);
    #endif
#endif

    // report the failure; note the stream is gone either way
    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_freopen",
                               D_INTERNAL_FILE_NOTIFY_PATH(_filename),
                               "reopen failed; stream is now closed");

        return NULL;
    }

    return d_internal_file_open_configure(result);
}


/*
d_freopen_s
  Secure file reopen (C11 Annex K freopen_s equivalent).

Parameter(s):
  _newstream: receives the reopened file pointer; must not be NULL.
  _filename:  path to file, or NULL to change the mode of _stream in place.
  _mode:      new file open mode.
  _stream:    the existing stream to reopen.
Return:
  0 on success, or a non-zero error code on failure.
*/
int
d_freopen_s
(
    FILE**      _newstream,
    const char* _filename,
    const char* _mode,
    FILE*       _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_newstream != NULL,
                            EINVAL,
                            "d_freopen_s",
                            NULL,
                            "stream out-parameter is NULL",
                            EINVAL);

    *_newstream = NULL;

    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_freopen_s",
                            _filename,
                            "mode is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_freopen_s",
                            _filename,
                            "stream is NULL",
                            EINVAL);

    *_newstream = d_freopen(_filename, _mode, _stream);

    if (!*_newstream)
    {
        return errno ? errno : ENOENT;
    }

    return 0;
}


/*
d_fdopen
  Associates a stream with an already-open file descriptor (POSIX fdopen).
  The descriptor is adopted, not duplicated: closing the returned stream
closes _fd, and closing _fd out from under the stream is undefined. On a pure
ISO C backend there are no descriptors, so this always fails with ENOSYS.

Parameter(s):
  _fd:   an open file descriptor.
  _mode: a mode string compatible with how _fd was opened.
Return:
  A file pointer on success, or NULL on failure with errno set.
*/
FILE*
d_fdopen
(
    int         _fd,
    const char* _mode
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;
    (void)_mode;

    // the ISO C backend has no descriptors to adopt, so this cannot be
    // emulated -- only reported
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_fdopen",
                         NULL,
                         "no descriptors on the ISO C backend",
                         NULL);
#else
    FILE* result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_fdopen",
                            NULL,
                            "descriptor is negative",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_fdopen",
                            NULL,
                            "mode is NULL",
                            NULL);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _fdopen(_fd, _mode);
    #else
    result = fdopen(_fd, _mode);
    #endif

    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fdopen",
                               NULL,
                               "fdopen failed");

        return NULL;
    }

    return d_internal_file_open_configure(result);
#endif
}


// II.   Stream closing

/*
d_fclose
  Closes a stream opened by this module.
  It exists so that closing is a decision this subframework owns rather than
one scattered across callers -- and because fclose's failure mode is worth
surfacing: a buffered write that could not be flushed is reported here, at
close, and nowhere else. A caller that ignores this return value can lose
data it believes it wrote.

Parameter(s):
  _stream: the stream to close; must not be NULL.
Return:
  0 on success, or EOF on failure with errno set.
*/
int
d_fclose
(
    FILE* _stream
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_fclose",
                            NULL,
                            "stream is NULL",
                            EOF);

    result = fclose(_stream);

    // a failure here means buffered data never reached the file
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fclose",
                               NULL,
                               "close failed; buffered data may be lost");
    }

    return result;
}
