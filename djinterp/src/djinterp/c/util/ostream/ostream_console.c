/*******************************************************************************
* djinterp [c]                                                 ostream_console.c
*
*   The console destination's backends. Each defines the same file-local
* functions -- open, write, flush, close, query and vt -- and everything after
* them is written once against those names, so no #if splits a function body.
*
*   NATIVE IS COMPILED ONLY BY A WINDOWS BUILD. Its UTF-8 carrying and
* chunking were checked on Linux against a copy of these functions, and the
* unit was cross-compiled with MinGW-w64; it has not yet run on Windows.
*
*
* path:      /src/djinterp/c/util/ostream/ostream_console.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/ostream/ostream_console.h"
// std
#include <errno.h>   // errno, EBADF, ENOSYS
#include <stddef.h>  // size_t
#if (D_INTERNAL_OSTREAM_CONSOLE != D_CFG_OSTREAM_CONSOLE_NONE)
    #include <stdio.h>   // FILE, stdout, stderr, fwrite, fflush, fileno
#endif
#if (D_INTERNAL_OSTREAM_CONSOLE == D_CFG_OSTREAM_CONSOLE_POSIX)
    #include <stdlib.h>  // getenv
    #include <string.h>  // strcmp
    #include <unistd.h>  // isatty
#elif (D_INTERNAL_OSTREAM_CONSOLE == D_CFG_OSTREAM_CONSOLE_NATIVE)
    #include <string.h>  // memcpy
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif  // WIN32_LEAN_AND_MEAN
    // windows
    #include <windows.h>  // HANDLE, DWORD, WriteConsoleW, WriteFile, ...
#endif
// djinterp
#include "../../../../../inc/djinterp/c/util/ostream/ostream.h"
                                                // struct dj_ostream
#include "../../../../../inc/djinterp/c/util/sink_common.h"
                                                // struct dj_pack_sink
#include "../../../../../inc/djinterp/config/c/util/ostream/cfg_ostream.h"
                                               // D_INTERNAL_OSTREAM_CONSOLE
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_CONSOLE_DEVICE
//   constant: state bit: the channel is a terminal, or on Windows a console.
#define D_INTERNAL_CONSOLE_DEVICE  0x1

// D_INTERNAL_CONSOLE_BINARY
//   constant: state bit: the stream was made with D_OSTREAM_BINARY.
#define D_INTERNAL_CONSOLE_BINARY  0x2

// D_INTERNAL_CONSOLE_TRANSLATES
//   constant: 1 where the C library rewrites newlines on a text stream.
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_INTERNAL_CONSOLE_TRANSLATES 1
#else
    #define D_INTERNAL_CONSOLE_TRANSLATES 0
#endif

/**
 * @brief Puts a console state into its starting condition, before a backend
 *        opens it.
 *
 * @param[out] _console  the state.
 * @param[in]  _channel  D_OSTREAM_CONSOLE_OUT or _ERR.
 * @param[in]  _flags    the stream's flags.
 */
static void
d_internal_console_clear(
    struct dj_ostream_console* _console,
    int32_t                    _channel,
    int32_t                    _flags
)
{
    _console->handle        = NULL;
    _console->stdio         = NULL;
    _console->channel       = _channel;
    _console->state         = ((_flags & D_OSTREAM_BINARY) != 0)
                                  ? D_INTERNAL_CONSOLE_BINARY
                                  : 0;
    _console->pending_count = 0;
    _console->pending[0]    = 0u;
    _console->pending[1]    = 0u;
    _console->pending[2]    = 0u;
    _console->pending[3]    = 0u;

    return;
}


//==============================================================================
// BACKEND: NONE
//==============================================================================

#if (D_INTERNAL_OSTREAM_CONSOLE == D_CFG_OSTREAM_CONSOLE_NONE)

/**
 * @brief No console exists to open.
 *
 * @param[in] _console  the state.
 * @return D_OSTREAM_UNSUPPORTED.
 */
static int32_t
d_internal_console_open(
    struct dj_ostream_console* _console
)
{
    (void)_console;
    errno = ENOSYS;

    return D_OSTREAM_UNSUPPORTED;
}

/**
 * @brief Never reached: d_ostream_init_console refuses every stream first.
 *
 * @param[in] _context  the state.
 * @param[in] _data     the bytes.
 * @param[in] _size     how many.
 * @return 0.
 */
static size_t
d_internal_console_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_context;
    (void)_data;
    (void)_size;

    return 0u;
}

/**
 * @brief Nothing is held back.
 *
 * @param[in] _context  the state.
 * @return 1.
 */
static int32_t
d_internal_console_flush(
    void* _context
)
{
    (void)_context;

    return 1;
}

/**
 * @brief Nothing to describe: every capability stays 0.
 *
 * @param[in]  _context  the state.
 * @param[out] _out      the description.
 * @return 1.
 */
static int32_t
d_internal_console_query(
    void*                   _context,
    struct dj_ostream_info* _out
)
{
    (void)_context;
    (void)_out;

    return 1;
}

/**
 * @brief No console, so no escape sequences.
 *
 * @param[in] _console  the state.
 * @return 0.
 */
static int32_t
d_internal_console_vt(
    struct dj_ostream_console* _console
)
{
    (void)_console;

    return 0;
}

#endif  // NONE


//==============================================================================
// BACKENDS: STDC AND POSIX -- the bytes go through stdio
//==============================================================================

#if ( (D_INTERNAL_OSTREAM_CONSOLE == D_CFG_OSTREAM_CONSOLE_STDC) ||            \
      (D_INTERNAL_OSTREAM_CONSOLE == D_CFG_OSTREAM_CONSOLE_POSIX) )

/**
 * @brief stdio's fwrite, on the channel's FILE*.
 *
 * @param[in] _context  the state.
 * @param[in] _data     the bytes.
 * @param[in] _size     how many.
 * @return How many fwrite took.
 */
static size_t
d_internal_console_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    const struct dj_ostream_console* const console =
        (const struct dj_ostream_console*)_context;

    return fwrite(_data, (size_t)1u, _size, (FILE*)console->stdio);
}

/**
 * @brief stdio's fflush, on the channel's FILE*.
 *
 * @param[in] _context  the state.
 * @return 1 or 0.
 */
static int32_t
d_internal_console_flush(
    void* _context
)
{
    const struct dj_ostream_console* const console =
        (const struct dj_ostream_console*)_context;

    return (fflush((FILE*)console->stdio) == 0) ? 1 : 0;
}

#endif  // STDC or POSIX


#if (D_INTERNAL_OSTREAM_CONSOLE == D_CFG_OSTREAM_CONSOLE_STDC)

/**
 * @brief Takes the channel's FILE*.
 *
 * @param[in,out] _console  the state, cleared.
 * @return D_OSTREAM_OK or D_OSTREAM_UNSUPPORTED.
 *
 * Binary output is refused where the C library rewrites newlines on it, because
 * ISO C has no way to change that.
 */
static int32_t
d_internal_console_open(
    struct dj_ostream_console* _console
)
{
    _console->stdio  = (_console->channel == D_OSTREAM_CONSOLE_ERR)
                           ? (void*)stderr
                           : (void*)stdout;
    _console->handle = _console->stdio;

    if ( ((_console->state & D_INTERNAL_CONSOLE_BINARY) != 0) &&
         (D_INTERNAL_CONSOLE_TRANSLATES != 0) )
    {
        errno = ENOSYS;

        return D_OSTREAM_UNSUPPORTED;
    }

    return D_OSTREAM_OK;
}

/**
 * @brief ISO C cannot tell a terminal from a file; it can say whether bytes
 *        arrive unaltered, from what c/filesys knows of the platform.
 *
 * @param[in]  _context  the state.
 * @param[out] _out      the description; arrives zeroed.
 * @return 1.
 */
static int32_t
d_internal_console_query(
    void*                   _context,
    struct dj_ostream_info* _out
)
{
    (void)_context;

    _out->is_binary_safe = (D_INTERNAL_CONSOLE_TRANSLATES == 0) ? 1 : 0;

    return 1;
}

/**
 * @brief Not knowable in ISO C, so not claimed.
 *
 * @param[in] _console  the state.
 * @return 0.
 */
static int32_t
d_internal_console_vt(
    struct dj_ostream_console* _console
)
{
    (void)_console;

    return 0;
}

#endif  // STDC


#if (D_INTERNAL_OSTREAM_CONSOLE == D_CFG_OSTREAM_CONSOLE_POSIX)

/**
 * @brief Whether TERM names a terminal that interprets escape sequences: set,
 *        not empty, and not "dumb".
 * @return 1 or 0.
 */
static int32_t
d_internal_console_term_has_vt(void)
{
    const char* const term = getenv("TERM");

    return ( (term != NULL) &&
             (term[0] != '\0') &&
             (strcmp(term, "dumb") != 0) ) ? 1 : 0;
}

/**
 * @brief Takes the channel's FILE*, and asks isatty about its descriptor.
 *
 * @param[in,out] _console  the state, cleared.
 * @return D_OSTREAM_OK.
 *
 * isatty sets errno when the answer is no, so errno is put back.
 */
static int32_t
d_internal_console_open(
    struct dj_ostream_console* _console
)
{
    FILE* const stream = (_console->channel == D_OSTREAM_CONSOLE_ERR)
                             ? stderr
                             : stdout;
    const int   saved  = errno;
    const int   fd     = fileno(stream);

    _console->stdio  = stream;
    _console->handle = stream;

    if ( (fd >= 0) &&
         (isatty(fd) == 1) )
    {
        _console->state |= D_INTERNAL_CONSOLE_DEVICE;
    }

    errno = saved;

    return D_OSTREAM_OK;
}

/**
 * @brief A terminal, and whether it interprets escape sequences; POSIX stdio
 *        never rewrites a byte.
 *
 * @param[in]  _context  the state.
 * @param[out] _out      the description; arrives zeroed.
 * @return 1.
 */
static int32_t
d_internal_console_query(
    void*                   _context,
    struct dj_ostream_info* _out
)
{
    const struct dj_ostream_console* const console =
        (const struct dj_ostream_console*)_context;
    const int32_t device = ((console->state & D_INTERNAL_CONSOLE_DEVICE) != 0)
                               ? 1
                               : 0;

    _out->is_terminal    = device;
    _out->has_vt         = (device != 0) ? d_internal_console_term_has_vt() : 0;
    _out->is_binary_safe = 1;

    return 1;
}

/**
 * @brief A POSIX terminal needs no switching on; this only answers.
 *
 * @param[in] _console  the state.
 * @return 1 or 0.
 */
static int32_t
d_internal_console_vt(
    struct dj_ostream_console* _console
)
{
    return ((_console->state & D_INTERNAL_CONSOLE_DEVICE) != 0)
               ? d_internal_console_term_has_vt()
               : 0;
}

#endif  // POSIX


//==============================================================================
// BACKEND: NATIVE (Win32)
//==============================================================================

#if (D_INTERNAL_OSTREAM_CONSOLE == D_CFG_OSTREAM_CONSOLE_NATIVE)

// D_INTERNAL_CONSOLE_WIDE
//   constant: the most UTF-8 bytes converted at once, and so the size of the
// UTF-16 buffer: no UTF-8 byte ever becomes more than one UTF-16 unit.
#define D_INTERNAL_CONSOLE_WIDE   1024

// D_INTERNAL_CONSOLE_CHUNK
//   constant: the most bytes one WriteFile is asked for, so the length fits
// its DWORD on every target.
#define D_INTERNAL_CONSOLE_CHUNK  ((size_t)0x40000000u)

// D_INTERNAL_CONSOLE_VT_MODE
//   constant: ENABLE_VIRTUAL_TERMINAL_PROCESSING, spelled here because SDKs
// before Windows 10's lack the name.
#define D_INTERNAL_CONSOLE_VT_MODE  ((DWORD)0x0004u)

/**
 * @brief The nearest errno value to a Win32 error.
 *
 * @param[in] _code  what GetLastError returned.
 * @return The errno value.
 */
static int
d_internal_console_errno(
    DWORD _code
)
{
    switch (_code)
    {
        case ERROR_INVALID_HANDLE:         { return EBADF;  }
        case ERROR_BROKEN_PIPE:
        case ERROR_NO_DATA:                { return EPIPE;  }
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:            { return ENOMEM; }
        case ERROR_DISK_FULL:
        case ERROR_HANDLE_DISK_FULL:       { return ENOSPC; }
        case ERROR_NO_UNICODE_TRANSLATION: { return EILSEQ; }
        default:                           { return EIO;    }
    }
}

/**
 * @brief How many bytes a UTF-8 sequence takes, from its first byte.
 *
 * @param[in] _lead  the first byte.
 * @return 1 to 4.
 *
 * A continuation byte or an invalid lead counts as one byte -- one replacement
 * character.
 */
static size_t
d_internal_console_utf8_length(
    unsigned char _lead
)
{
    if (_lead < 0x80u)
    {
        return 1u;
    }

    if ((_lead & 0xE0u) == 0xC0u)
    {
        return 2u;
    }

    if ((_lead & 0xF0u) == 0xE0u)
    {
        return 3u;
    }

    return ((_lead & 0xF8u) == 0xF0u) ? 4u : 1u;
}

/**
 * @brief The longest prefix of _data that does not end inside a UTF-8 sequence.
 *
 * @param[in] _data  the bytes.
 * @param[in] _size  how many.
 * @return The prefix's length; less than _size by at most three.
 *
 * Only a sequence that is valid so far and short of its length is cut off;
 * anything already invalid is left in, for the converter to replace.
 */
static size_t
d_internal_console_utf8_prefix(
    const unsigned char* _data,
    size_t               _size
)
{
    size_t back = 0u;

    while ( (back < 3u) &&
            (back < _size) &&
            ((_data[_size - 1u - back] & 0xC0u) == 0x80u) )
    {
        ++back;
    }

    if (back == _size)
    {
        return _size;
    }

    const size_t need =
        d_internal_console_utf8_length(_data[_size - 1u - back]);

    return (need > (back + 1u)) ? (_size - 1u - back) : _size;
}

/**
 * @brief Converts whole UTF-8 sequences to UTF-16 and writes them to the
 *        console.
 *
 * @param[in] _console  the state; a console.
 * @param[in] _data     the bytes; no more than D_INTERNAL_CONSOLE_WIDE.
 * @param[in] _size     how many; not 0.
 * @return 1 or 0, with errno set.
 *
 * WriteConsoleW may take fewer units than offered, so it is called until it has
 * taken them all.
 */
static int32_t
d_internal_console_emit(
    const struct dj_ostream_console* _console,
    const unsigned char*             _data,
    size_t                           _size
)
{
    WCHAR     wide[D_INTERNAL_CONSOLE_WIDE];
    const int units = MultiByteToWideChar(CP_UTF8,
                                          0,
                                          (LPCSTR)_data,
                                          (int)_size,
                                          wide,
                                          D_INTERNAL_CONSOLE_WIDE);
    int       at    = 0;

    if (units <= 0)
    {
        errno = d_internal_console_errno(GetLastError());

        return 0;
    }

    while (at < units)
    {
        DWORD done = 0;

        if ( (WriteConsoleW((HANDLE)_console->handle,
                            wide + at,
                            (DWORD)(units - at),
                            &done,
                            NULL) == 0) ||
             (done == 0u) )
        {
            errno = d_internal_console_errno(GetLastError());

            return 0;
        }

        at += (int)done;
    }

    return 1;
}

/**
 * @brief Completes the sequence carried over from the last write with the bytes
 *        that begin this one, and writes it once it is complete -- or once a
 *        byte that cannot continue it shows that it never will be, which writes
 *        it as it is, for the converter to replace.
 *
 * @param[in,out] _console  the state; holding a carried sequence.
 * @param[in]     _data     this write's bytes.
 * @param[in]     _size     how many.
 * @param[out]    _used     receives how many of them went into the sequence.
 * @return 1 or 0.
 */
static int32_t
d_internal_console_carry(
    struct dj_ostream_console* _console,
    const unsigned char*       _data,
    size_t                     _size,
    size_t*                    _used
)
{
    const size_t need = d_internal_console_utf8_length(_console->pending[0]);
    size_t       used = 0u;

    while ( ((size_t)_console->pending_count < need) &&
            (used < _size) &&
            ((_data[used] & 0xC0u) == 0x80u) )
    {
        _console->pending[_console->pending_count] = _data[used];
        ++_console->pending_count;
        ++used;
    }

    *_used = used;

    if ( ((size_t)_console->pending_count < need) &&
         (used == _size) )
    {
        return 1;
    }

    const size_t count = (size_t)_console->pending_count;

    _console->pending_count = 0;

    return d_internal_console_emit(_console, _console->pending, count);
}

/**
 * @brief Writes UTF-8 to a console as UTF-16: first any sequence carried over,
 *        then whole sequences in chunks the conversion buffer holds, then
 *        carries over the start of a sequence this write did not finish.
 *
 * @param[in,out] _console  the state; a console, in text mode.
 * @param[in]     _data     the bytes.
 * @param[in]     _size     how many.
 * @return 1 or 0.
 */
static int32_t
d_internal_console_write_text(
    struct dj_ostream_console* _console,
    const unsigned char*       _data,
    size_t                     _size
)
{
    size_t at = 0u;

    if ( (_console->pending_count > 0) &&
         (d_internal_console_carry(_console, _data, _size, &at) == 0) )
    {
        return 0;
    }

    while ( (_console->pending_count == 0) &&
            (at < _size) )
    {
        const size_t left = _size - at;
        const size_t span = (left > (size_t)D_INTERNAL_CONSOLE_WIDE)
                                ? (size_t)D_INTERNAL_CONSOLE_WIDE
                                : left;
        const size_t take = d_internal_console_utf8_prefix(_data + at, span);

        if (take == 0u)
        {
            // at most three bytes, the start of a sequence: carry them over
            memcpy(_console->pending, _data + at, left);
            _console->pending_count = (int32_t)left;

            return 1;
        }

        if (d_internal_console_emit(_console, _data + at, take) == 0)
        {
            return 0;
        }

        at += take;
    }

    return 1;
}

/**
 * @brief Writes a sequence still carried when the stream ends, as it is: it
 *        will never be completed, and the converter replaces it.
 *
 * @param[in] _console  the state.
 * @return 1 or 0.
 */
static int32_t
d_internal_console_finish(
    struct dj_ostream_console* _console
)
{
    if (_console->pending_count <= 0)
    {
        return 1;
    }

    const size_t count = (size_t)_console->pending_count;

    _console->pending_count = 0;

    return d_internal_console_emit(_console, _console->pending, count);
}

/**
 * @brief Writes bytes untranslated with WriteFile, in chunks its DWORD can
 *        count.
 *
 * @param[in] _console  the state.
 * @param[in] _data     the bytes.
 * @param[in] _size     how many.
 * @return 1 or 0, with errno set.
 */
static int32_t
d_internal_console_write_raw(
    const struct dj_ostream_console* _console,
    const unsigned char*             _data,
    size_t                           _size
)
{
    size_t at = 0u;

    while (at < _size)
    {
        const size_t left  = _size - at;
        const DWORD  chunk = (left > D_INTERNAL_CONSOLE_CHUNK)
                                 ? (DWORD)D_INTERNAL_CONSOLE_CHUNK
                                 : (DWORD)left;
        DWORD        done  = 0;

        if ( (WriteFile((HANDLE)_console->handle,
                        _data + at,
                        chunk,
                        &done,
                        NULL) == 0) ||
             (done == 0u) )
        {
            errno = d_internal_console_errno(GetLastError());

            return 0;
        }

        at += (size_t)done;
    }

    return 1;
}

/**
 * @brief Takes the channel's standard handle, and asks whether it is a console.
 *
 * @param[in,out] _console  the state, cleared.
 * @return D_OSTREAM_OK or D_OSTREAM_FAILED.
 *
 * A GUI process has no standard handles, and its stream fails here.
 */
static int32_t
d_internal_console_open(
    struct dj_ostream_console* _console
)
{
    const int    err    = (_console->channel == D_OSTREAM_CONSOLE_ERR) ? 1 : 0;
    const HANDLE handle = GetStdHandle((err != 0) ? STD_ERROR_HANDLE
                                                  : STD_OUTPUT_HANDLE);
    DWORD        mode   = 0;

    _console->stdio = (err != 0) ? (void*)stderr : (void*)stdout;

    if ( (handle == NULL) ||
         (handle == INVALID_HANDLE_VALUE) )
    {
        errno = EBADF;

        return D_OSTREAM_FAILED;
    }

    _console->handle = (void*)handle;

    if (GetConsoleMode(handle, &mode) != 0)
    {
        _console->state |= D_INTERNAL_CONSOLE_DEVICE;
    }

    return D_OSTREAM_OK;
}

/**
 * @brief Chooses the path: redirected text through the C library, as printf's
 *        would go; anything else directly, after flushing the C library's
 *        buffer so that an earlier printf is not overtaken -- binary through
 *        WriteFile, console text through WriteConsoleW.
 *
 * @param[in] _context  the state.
 * @param[in] _data     the bytes.
 * @param[in] _size     how many.
 * @return _size, or less on failure.
 */
static size_t
d_internal_console_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct dj_ostream_console* const console =
        (struct dj_ostream_console*)_context;
    const int32_t binary = ((console->state & D_INTERNAL_CONSOLE_BINARY) != 0)
                               ? 1
                               : 0;

    if ( (binary == 0) &&
         ((console->state & D_INTERNAL_CONSOLE_DEVICE) == 0) )
    {
        return fwrite(_data, (size_t)1u, _size, (FILE*)console->stdio);
    }

    if (fflush((FILE*)console->stdio) != 0)
    {
        return 0u;
    }

    const int32_t ok = (binary != 0)
        ? d_internal_console_write_raw(console,
                                       (const unsigned char*)_data,
                                       _size)
        : d_internal_console_write_text(console,
                                        (const unsigned char*)_data,
                                        _size);

    return (ok != 0) ? _size : 0u;
}

/**
 * @brief Flushes the C library's buffer.
 *
 * @param[in] _context  the state.
 * @return 1 or 0.
 *
 * A carried sequence stays: the next write may complete it.
 */
static int32_t
d_internal_console_flush(
    void* _context
)
{
    const struct dj_ostream_console* const console =
        (const struct dj_ostream_console*)_context;

    return (fflush((FILE*)console->stdio) == 0) ? 1 : 0;
}

/**
 * @brief A console, whether it interprets escape sequences now, and whether
 *        bytes arrive unaltered -- only in binary mode.
 *
 * @param[in]  _context  the state.
 * @param[out] _out      the description; arrives zeroed.
 * @return 1.
 */
static int32_t
d_internal_console_query(
    void*                   _context,
    struct dj_ostream_info* _out
)
{
    const struct dj_ostream_console* const console =
        (const struct dj_ostream_console*)_context;
    DWORD                                 mode    = 0;

    if ((console->state & D_INTERNAL_CONSOLE_DEVICE) != 0)
    {
        _out->is_terminal = 1;
        _out->has_vt      = ( (GetConsoleMode((HANDLE)console->handle,
                                              &mode) != 0) &&
                              ((mode & D_INTERNAL_CONSOLE_VT_MODE) != 0u) )
                                ? 1
                                : 0;
    }

    _out->is_binary_safe = ((console->state & D_INTERNAL_CONSOLE_BINARY) != 0)
                               ? 1
                               : 0;

    return 1;
}

/**
 * @brief Sets ENABLE_VIRTUAL_TERMINAL_PROCESSING on a console that lacks it.
 *
 * @param[in] _console  the state.
 * @return 1 or 0.
 *
 * Before Windows 10 the console refuses, and the answer is 0.
 */
static int32_t
d_internal_console_vt(
    struct dj_ostream_console* _console
)
{
    DWORD mode = 0;

    if ( ((_console->state & D_INTERNAL_CONSOLE_DEVICE) == 0) ||
         (GetConsoleMode((HANDLE)_console->handle, &mode) == 0) )
    {
        return 0;
    }

    if ((mode & D_INTERNAL_CONSOLE_VT_MODE) != 0u)
    {
        return 1;
    }

    return (SetConsoleMode((HANDLE)_console->handle,
                           mode | D_INTERNAL_CONSOLE_VT_MODE) != 0) ? 1 : 0;
}

#endif  // NATIVE


#if (D_INTERNAL_OSTREAM_CONSOLE != D_CFG_OSTREAM_CONSOLE_NATIVE)

/**
 * @brief Nothing is carried between writes off Windows, so nothing is left to
 *        write.
 *
 * @param[in] _console  the state.
 * @return 1.
 */
static int32_t
d_internal_console_finish(
    struct dj_ostream_console* _console
)
{
    (void)_console;

    return 1;
}

#endif  // not NATIVE


//==============================================================================
// THE STREAM
//==============================================================================

/**
 * @brief Finishes a console stream: whatever the backend still carries is
 *        written, then the buffers are flushed.
 *
 * @param[in] _context  the state.
 * @param[in] _release  ignored.
 * @return 1 or 0.
 *
 * Nothing is released -- a stream never closes stdout.
 */
static int32_t
d_internal_console_close(
    void*   _context,
    int32_t _release
)
{
    (void)_release;

    if (d_internal_console_finish((struct dj_ostream_console*)_context) == 0)
    {
        return 0;
    }

    return d_internal_console_flush(_context);
}

// d_internal_console_ops
//   table: the console's operations; a transport, so no grow slot.
static const struct dj_ostream_ops d_internal_console_ops =
{
    sizeof(struct dj_ostream_ops),
    NULL,
    &d_internal_console_flush,
    &d_internal_console_close,
    &d_internal_console_query
};

/*
d_ostream_init_console
  Makes a stream over stdout or stderr. A stream the backend refuses keeps
the backend's reason: D_OSTREAM_UNSUPPORTED or D_OSTREAM_FAILED, with errno.
*/
int32_t
d_ostream_init_console(
    struct dj_ostream*         _os,
    struct dj_ostream_console* _console,
    int32_t                    _channel,
    int32_t                    _flags
)
{
    if (_os == NULL)
    {
        return 0;
    }

    int32_t opened = D_OSTREAM_INVALID;

    if ( (_console != NULL) &&
         ( (_channel == D_OSTREAM_CONSOLE_OUT) ||
           (_channel == D_OSTREAM_CONSOLE_ERR) ) )
    {
        d_internal_console_clear(_console, _channel, _flags);
        opened = d_internal_console_open(_console);
    }

    const int          reason = errno;
    int32_t            flags  = _flags & (D_OSTREAM_BINARY |
                                          D_OSTREAM_LINE_BUFFERED);
    struct dj_pack_sink sink;

    sink.write   = NULL;
    sink.context = NULL;

    if (opened == D_OSTREAM_OK)
    {
        sink.write   = &d_internal_console_write;
        sink.context = _console;

        if ((_console->state & D_INTERNAL_CONSOLE_DEVICE) != 0)
        {
            flags |= D_OSTREAM_LINE_BUFFERED;
        }
    }

    if (d_ostream_init(_os, sink, &d_internal_console_ops, flags) != 0)
    {
        return 1;
    }

    if (opened != D_OSTREAM_INVALID)
    {
        _os->status     = opened;
        _os->last_errno = (int32_t)reason;
    }

    return 0;
}

/*
d_ostream_console_enable_vt
  Asks for escape sequences to be interpreted.
*/
int32_t
d_ostream_console_enable_vt(
    struct dj_ostream_console* _console
)
{
    if (_console == NULL)
    {
        return 0;
    }

    return d_internal_console_vt(_console);
}

/*
d_ostream_console_backend
  The console backend this build uses.
*/
int32_t
d_ostream_console_backend(void)
{
    return D_INTERNAL_OSTREAM_CONSOLE;
}
