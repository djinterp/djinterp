/*******************************************************************************
* djinterp [c]                                                       file_open.c
*
* Implementation of the stream opening and closing declared in file_open.h.
*   Three file-local steps shape every open: the caller's mode is decorated with
* the build's policy, the path is opened through the one backend call this build
* selected (_fsopen, fopen_s or fopen; on Windows their wide twins, given
* UTF-16, where the build reads paths as UTF-8), and the new stream is given the
* configured buffering. Every entry point here funnels through them, so the
* policy is applied once rather than re-litigated per caller. On Windows two CRT
* gaps are closed as well: a reopen without a file changes only the translation,
* which is all the CRT can change in place, and an adopted descriptor takes the
* mode's 'b' or 't', which _fdopen would ignore.
*
*
* path:      /src/djinterp/c/filesys/file_open.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_open.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EINVAL, ENOENT, ENOSYS
#include <stddef.h>  // NULL, size_t
#include <stdio.h>   // FILE, EOF, fopen, freopen, fclose, fdopen, setvbuf
#include <string.h>  // memcpy, strchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_open.h"  // D_INTERNAL_FILE_OPEN_*


// the large-file interface this source needs (file_common.h, 6.4.1)
D_INTERNAL_FILE_ASSERT_LARGE_FILES;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_OPEN_NAMES_BINARY
//   constant: d_internal_file_open_names' bit for a mode that names 'b'.
#define D_INTERNAL_OPEN_NAMES_BINARY  1

// D_INTERNAL_OPEN_NAMES_TEXT
//   constant: d_internal_file_open_names' bit for a mode that names 't'.
#define D_INTERNAL_OPEN_NAMES_TEXT    2

// D_INTERNAL_OPEN_NAMES_CLOEXEC
//   constant: d_internal_file_open_names' bit for a mode that names the
// target's close-on-exec character.
#define D_INTERNAL_OPEN_NAMES_CLOEXEC 4

// D_INTERNAL_OPEN_MODE_NATIVE
//   constant: the characters a mode needs in the platform's text -- room for
// the decorated mode, or for the caller's own where that was too long to
// decorate, where it is converted (file_common.h, 6.6); one, unused, where it
// is handed on as it is.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_OPEN_MODE_NATIVE (2 * D_INTERNAL_FILE_OPEN_MODE_MAX)
#else
    #define D_INTERNAL_OPEN_MODE_NATIVE 1
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_OPEN_FOPEN
//   macro: fopen, in the form the build's path text takes: _wfopen where
// Windows is handed UTF-16. The four below are likewise.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_OPEN_FOPEN _wfopen
#else
    #define D_INTERNAL_OPEN_FOPEN fopen
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_OPEN_FOPEN_S
//   macro: fopen_s, or _wfopen_s.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_OPEN_FOPEN_S _wfopen_s
#else
    #define D_INTERNAL_OPEN_FOPEN_S fopen_s
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_OPEN_FSOPEN
//   macro: _fsopen, or _wfsopen.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_OPEN_FSOPEN _wfsopen
#else
    #define D_INTERNAL_OPEN_FSOPEN _fsopen
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_OPEN_FREOPEN
//   macro: freopen, or _wfreopen.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_OPEN_FREOPEN _wfreopen
#else
    #define D_INTERNAL_OPEN_FREOPEN freopen
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_OPEN_FREOPEN_S
//   macro: freopen_s, or _wfreopen_s.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_OPEN_FREOPEN_S _wfreopen_s
#else
    #define D_INTERNAL_OPEN_FREOPEN_S freopen_s
#endif  // D_INTERNAL_FILE_WIDE == 1

/*
d_internal_file_open_head
  The length of a mode proper: the characters before any encoding, which glibc
and the Windows CRT both spell after a comma (",ccs=UTF-8").
*/
static size_t
d_internal_file_open_head(
    const char* _mode
)
{
    const char* const comma = strchr(_mode,
                                     ',');

    return comma ? (size_t)(comma - _mode) : strlen(_mode);
}

/*
d_internal_file_open_names
  What the first _head characters of a mode already ask for, as
D_INTERNAL_OPEN_NAMES_* bits: 'b', 't', and the target's close-on-exec
character.
*/
static int
d_internal_file_open_names(
    const char* _mode,
    size_t      _head
)
{
    int names = 0;

    // each character the policy might otherwise supply
    for (size_t idx = 0; idx < _head; ++idx)
    {
        names |= (_mode[idx] == 'b') ? D_INTERNAL_OPEN_NAMES_BINARY
               : (_mode[idx] == 't') ? D_INTERNAL_OPEN_NAMES_TEXT
               : ( (D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR != 0) &&
                   (_mode[idx] == (char)D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR) )
                   ? D_INTERNAL_OPEN_NAMES_CLOEXEC
                   : 0;
    }

    return names;
}

/*
d_internal_file_open_mode
  Rewrites a caller's mode so it carries this build's open policy: a 'b' when
D_CFG_FILE_OPEN_BINARY_DEFAULT is set and the caller named neither 'b' nor 't'
-- nor an encoding, which only a text stream may have -- and, when _cloexec
is set, the target's close-on-exec character. Anything the caller asked for
is left alone: an explicit "rt" is an explicit request for text mode, and the
job here is to supply a default, not to overrule. The additions go at the end
of the mode proper (d_internal_file_open_head): after an encoding they would
be read as part of its name, and the open would fail.
  If the rewrite would not fit, the caller's string is returned untouched: a
mode string longer than the buffer is malformed, and fopen rejects it with a
better error than could be invented here. So the result is _buf or _mode,
and never NULL.
*/
static const char*
d_internal_file_open_mode(
    const char* _mode,
    char*       _buf,
    size_t      _bufsize,
    int         _cloexec
)
{
    const size_t length = strlen(_mode);
    const size_t head   = d_internal_file_open_head(_mode);

    // nothing to add, or a malformed or oversized mode for the platform to
    // reject
    if ( (D_INTERNAL_FILE_OPEN_DECORATE == 0) ||
         ((length + 3) > _bufsize) )
    {
        return _mode;
    }

    const int names = d_internal_file_open_names(_mode,
                                                 head);
    size_t    out   = head;

    memcpy(_buf,
           _mode,
           head);

    // binary only where the caller expressed no preference
    if ( (D_INTERNAL_FILE_OPEN_BINARY == 1) &&
         ((names & (D_INTERNAL_OPEN_NAMES_BINARY |
                    D_INTERNAL_OPEN_NAMES_TEXT)) == 0) &&
         (head == length) )
    {
        _buf[out++] = 'b';
    }

    // close-on-exec unless it is already there, or this open does not take it
    if ( (D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR != 0) &&
         (_cloexec) &&
         ((names & D_INTERNAL_OPEN_NAMES_CLOEXEC) == 0) )
    {
        _buf[out++] = (char)D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR;
    }

    // the encoding, if there is one, and the terminator
    memcpy(_buf + out,
           _mode + head,
           (length - head) + 1);

    return _buf;
}

#if (D_INTERNAL_FILE_OPEN_SETVBUF == 1)

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
    // only a stream that exists can be buffered; a declined buffer size is a
    // warning, never a failure
    if ( (_stream) &&
         (setvbuf(_stream,
                  NULL,
                  D_INTERNAL_FILE_OPEN_BUFFER_MODE,
                  (size_t)D_INTERNAL_FILE_OPEN_BUFFER_SIZE) != 0) )
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                               0,
                               "d_file_open_stream",
                               NULL,
                               "setvbuf declined; using stdio's buffer");
    }

    return _stream;
}

#else

/*
d_internal_file_open_configure
  This build leaves a stream's buffering to stdio; the argument is returned as
given.
*/
static FILE*
d_internal_file_open_configure(
    FILE* _stream
)
{
    return _stream;
}

#endif  // D_INTERNAL_FILE_OPEN_SETVBUF == 1

#if (D_INTERNAL_FILE_OPEN_USE_FSOPEN == 1)

/*
d_internal_file_open_raw
  The one place this subframework asks the platform to open a path, so the
backend choice is made once rather than per entry point. This build shares the
file through _fsopen, with the sharing mode it was configured for.
*/
static FILE*
d_internal_file_open_raw(
    const d_internal_file_nchar_t* _filename,
    const d_internal_file_nchar_t* _mode
)
{
    return D_INTERNAL_OPEN_FSOPEN(_filename,
                                  _mode,
                                  D_INTERNAL_FILE_OPEN_SHARE_FLAG);
}

#elif (D_INTERNAL_FILE_OPEN_FOPEN_S == 1)

/*
d_internal_file_open_raw
  The one place this subframework asks the platform to open a path, so the
backend choice is made once rather than per entry point. This build uses
fopen_s, which reports through its return value; that is normalized to the
fopen contract -- NULL, with the code in errno -- so every caller has one
failure shape.
*/
static FILE*
d_internal_file_open_raw(
    const d_internal_file_nchar_t* _filename,
    const d_internal_file_nchar_t* _mode
)
{
    FILE*     result = NULL;
    const int code   = (int)D_INTERNAL_OPEN_FOPEN_S(&result,
                                                    _filename,
                                                    _mode);

    // the code fopen_s returned is the errno fopen would have set
    if (code != 0)
    {
        errno = code;

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
    const d_internal_file_nchar_t* _filename,
    const d_internal_file_nchar_t* _mode
)
{
    return D_INTERNAL_OPEN_FOPEN(_filename,
                                 _mode);
}

#endif  // D_INTERNAL_FILE_OPEN_USE_FSOPEN, D_INTERNAL_FILE_OPEN_FOPEN_S

#if (D_INTERNAL_FILE_OPEN_FOPEN_S == 1)

/*
d_internal_file_open_reraw
  freopen_s, its return value normalized to the freopen contract as
d_internal_file_open_raw normalizes fopen_s.
*/
static FILE*
d_internal_file_open_reraw(
    const d_internal_file_nchar_t* _filename,
    const d_internal_file_nchar_t* _mode,
    FILE*                          _stream
)
{
    FILE*     result = NULL;
    const int code   = (int)D_INTERNAL_OPEN_FREOPEN_S(&result,
                                                      _filename,
                                                      _mode,
                                                      _stream);

    // the code freopen_s returned is the errno freopen would have set
    if (code != 0)
    {
        errno = code;

        return NULL;
    }

    return result;
}

#else

/*
d_internal_file_open_reraw
  Plain freopen.
*/
static FILE*
d_internal_file_open_reraw(
    const d_internal_file_nchar_t* _filename,
    const d_internal_file_nchar_t* _mode,
    FILE*                          _stream
)
{
    return D_INTERNAL_OPEN_FREOPEN(_filename,
                                   _mode,
                                   _stream);
}

#endif  // D_INTERNAL_FILE_OPEN_FOPEN_S == 1

/*
d_internal_file_open_named
  The mode and the path, given the platform's text (d_internal_file_to_native),
then opened (d_internal_file_open_raw). A mode that will not convert is no
mode the platform takes, and fails with EINVAL, as fopen fails a malformed
one; a path that will not convert fails with the conversion's reason.
*/
static FILE*
d_internal_file_open_named(
    const char* _filename,
    const char* _mode
)
{
    d_internal_file_nchar_t mode_buf[D_INTERNAL_OPEN_MODE_NATIVE] = {0};
    struct d_internal_file_native held = {0};
    const d_internal_file_nchar_t* const mode =
        d_internal_file_to_native_text(_mode,
                                       mode_buf,
                                       D_INTERNAL_OPEN_MODE_NATIVE);

    // a mode the platform cannot be given
    if (!mode)
    {
        D_INTERNAL_FILE_SET_ERR(EINVAL);

        return NULL;
    }

    const d_internal_file_nchar_t* const name =
        d_internal_file_native_of(&held,
                                  _filename);

    // the conversion has set errno
    if (!name)
    {
        return NULL;
    }

    FILE* const result = d_internal_file_open_raw(name,
                                                  mode);

    d_internal_file_native_release(&held);

    return result;
}

/*
d_internal_file_open_renamed
  d_internal_file_open_named's conversions, then the reopen
(d_internal_file_open_reraw). A reopen that cannot start closes the stream,
as a failed one does, so the caller meets one contract: failure ends the
stream.
*/
static FILE*
d_internal_file_open_renamed(
    const char* _filename,
    const char* _mode,
    FILE*       _stream
)
{
    d_internal_file_nchar_t mode_buf[D_INTERNAL_OPEN_MODE_NATIVE] = {0};
    struct d_internal_file_native held = {0};
    const d_internal_file_nchar_t* const mode =
        d_internal_file_to_native_text(_mode,
                                       mode_buf,
                                       D_INTERNAL_OPEN_MODE_NATIVE);
    const d_internal_file_nchar_t* const name =
        (mode) ? d_internal_file_native_of(&held,
                                           _filename)
               : NULL;

    // a mode the platform cannot be given
    if (!mode)
    {
        D_INTERNAL_FILE_SET_ERR(EINVAL);
    }

    // the reopen cannot start: the stream ends, its reason kept in errno
    if (!name)
    {
        const int saved_errno = errno;

        (void)fclose(_stream);
        errno = saved_errno;

        return NULL;
    }

    FILE* const result = d_internal_file_open_reraw(name,
                                                    mode,
                                                    _stream);

    d_internal_file_native_release(&held);

    return result;
}

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_file_open_translation
  Puts a descriptor in the translation a mode names -- binary for 'b', text
for 't' -- and leaves it alone when the mode names neither. The result is 0,
or -1 with errno set.
*/
static int
d_internal_file_open_translation(
    int         _fd,
    const char* _mode
)
{
    const int names = d_internal_file_open_names(
                          _mode,
                          d_internal_file_open_head(_mode));

    // a descriptor the CRT does not have cannot be changed
    if (_fd < 0)
    {
        D_INTERNAL_FILE_SET_ERR(EBADF);

        return -1;
    }

    // no choice named, nothing to apply
    if ((names & (D_INTERNAL_OPEN_NAMES_BINARY |
                  D_INTERNAL_OPEN_NAMES_TEXT)) == 0)
    {
        return 0;
    }

    return (_setmode(_fd,
                     ((names & D_INTERNAL_OPEN_NAMES_BINARY) != 0)
                         ? _O_BINARY
                         : _O_TEXT) < 0) ? -1 : 0;
}

/*
d_internal_file_open_in_place
  The CRT cannot reopen a stream on no file -- freopen with a null path is an
invalid parameter, which by default ends the process -- and the one change it
can make in place is the translation. So a reopen without a file is that: the
stream is flushed and made binary when the mode names 'b' -- the build's
default included -- and text otherwise, and keeps everything else. A refusal
closes the stream, as a failed reopen does.
*/
static FILE*
d_internal_file_open_in_place(
    const char* _mode,
    FILE*       _stream
)
{
    const int binary = ( (d_internal_file_open_names(
                              _mode,
                              d_internal_file_open_head(_mode)) &
                          D_INTERNAL_OPEN_NAMES_BINARY) != 0 );

    (void)fflush(_stream);

    // the translation is all that can change; a refusal ends the stream
    if (d_internal_file_open_translation(_fileno(_stream),
                                         binary ? "b" : "t") != 0)
    {
        const int saved_errno = errno;

        (void)fclose(_stream);
        errno = saved_errno;

        return NULL;
    }

    return _stream;
}

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_file_open_adopt
  The CRT's _fdopen, which takes the stream's translation from the descriptor
and ignores the mode's 'b' or 't': a mode that names one -- the build's
default included -- is applied to the descriptor first
(d_internal_file_open_translation), so the stream does what the mode says.
*/
static FILE*
d_internal_file_open_adopt(
    int         _fd,
    const char* _mode
)
{
    // the mode's own translation, which _fdopen would not apply
    if (d_internal_file_open_translation(_fd,
                                         _mode) != 0)
    {
        return NULL;
    }

    return _fdopen(_fd,
                   _mode);
}

#endif  // !D_FILE_BACKEND_IS_STDC

#else

/*
d_internal_file_open_in_place
  freopen with a null path, which changes the stream's mode in place as far as
the platform allows.
*/
static FILE*
d_internal_file_open_in_place(
    const char* _mode,
    FILE*       _stream
)
{
    return d_internal_file_open_reraw(NULL,
                                      _mode,
                                      _stream);
}

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_file_open_adopt
  POSIX fdopen, which has no translation to apply.
*/
static FILE*
d_internal_file_open_adopt(
    int         _fd,
    const char* _mode
)
{
    return fdopen(_fd,
                  _mode);
}

#endif  // !D_FILE_BACKEND_IS_STDC

#endif  // D_CFG_FILE_HAS_WIN32

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

    char              mode_buf[D_INTERNAL_FILE_OPEN_MODE_MAX] = {0};
    const char* const mode   = d_internal_file_open_mode(_mode,
                                                         mode_buf,
                                                         sizeof(mode_buf),
                                                         1);
    FILE* const       result = d_internal_file_open_named(_filename,
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
  The same mode decoration as d_file_open_stream, then freopen -- or, without
a file, the platform's change of mode in place (d_internal_file_open_in_place).
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

    char              mode_buf[D_INTERNAL_FILE_OPEN_MODE_MAX] = {0};
    const char* const mode   = d_internal_file_open_mode(_mode,
                                                         mode_buf,
                                                         sizeof(mode_buf),
                                                         1);
    FILE* const       result = _filename
                                   ? d_internal_file_open_renamed(_filename,
                                                                  mode,
                                                                  _stream)
                                   : d_internal_file_open_in_place(mode,
                                                                   _stream);

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
stream owns _fd. The mode gains the build's binary default, and on Windows its
translation is applied to the descriptor (d_internal_file_open_adopt); not
close-on-exec, which belongs to the descriptor. The new stream is given the
same configured buffering as any other open.
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

    char              mode_buf[D_INTERNAL_FILE_OPEN_MODE_MAX] = {0};
    const char* const mode   = d_internal_file_open_mode(_mode,
                                                         mode_buf,
                                                         sizeof(mode_buf),
                                                         0);
    FILE* const       result = d_internal_file_open_adopt(_fd,
                                                          mode);

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
