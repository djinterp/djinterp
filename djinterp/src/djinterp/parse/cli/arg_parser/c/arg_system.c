/*******************************************************************************
* djinterp [parse]                                                  arg_system.c
*
* What the parser asks of the operating system, declared in arg_internal.h
* (9.1 to 9.3 and 9.5), and the terminal's width, declared in arg_formatter.h
* (5.2.1).
*   The one source of the module that includes a system header. Each question is
* answered by the implementation env_arg_parser.h's facts select, chosen whole,
* a function at a time: a file's status through _wstat64 on Windows -- a
* directory named with a separator after it stated again without it, which the
* older runtime, msvcrt, needs -- through stat where <sys/stat.h> is, and by
* opening the file elsewhere; a file opened through the Microsoft runtime's
* secure functions where it deprecates the others; a variable through _dupenv_s
* or getenv; the command line through GetCommandLineW; the terminal through the
* console API or TIOCGWINSZ. On Windows a path is UTF-8, as CLI11 widens one,
* unless it is not valid UTF-8, when the runtime reads it in the ANSI code page,
* as `argv` holds it. The text encodings those conversions need come first.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_system.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fread, ferror, fclose
#include <stdlib.h>  // malloc, free, getenv
#include <string.h>  // strlen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h"  // d_arg_terminal_width
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint16_t,
                                                           // uint32_t,
                                                           // uintmax_t
// platform
#if (D_ENV_ARG_PARSER_STAT != D_ENV_ARG_PARSER_STAT_NONE)
    #include <sys/types.h>  // the types stat reports in
    #include <sys/stat.h>   // stat, _stat64, _wstat64
#endif
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #include <wchar.h>      // wchar_t, _wfopen, _wfopen_s
#endif
#if ( ( (D_ENV_ARG_PARSER_TERMINAL ==                                          \
         D_ENV_ARG_PARSER_TERMINAL_WINDOWS)          &&                        \
        (D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH == 1) ) ||                       \
      ( (D_ENV_ARG_PARSER_COMMAND_LINE ==                                      \
         D_ENV_ARG_PARSER_COMMAND_LINE_WINDOWS)      &&                        \
        (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1) ) )
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif  // WIN32_LEAN_AND_MEAN
    #include <windows.h>    // GetCommandLineW, GetConsoleScreenBufferInfo
#endif
#if ( (D_ENV_ARG_PARSER_TERMINAL == D_ENV_ARG_PARSER_TERMINAL_POSIX) &&       \
      (D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH == 1) )
    #include <sys/ioctl.h>  // ioctl, TIOCGWINSZ, struct winsize
    #include <unistd.h>     // isatty, STDOUT_FILENO
#endif


//==============================================================================
// 1.  TEXT ENCODINGS
//==============================================================================

// REPLACEMENT
//   constant: U+FFFD, written where a code point cannot be.
static const uint32_t REPLACEMENT = 0xFFFDu;

// UTF8_LEADS
//   constant: the bits a UTF-8 lead byte carries above its share of the code
// point, by the length of its sequence.
static const unsigned int UTF8_LEADS[5] = { 0x00u, 0x00u, 0xC0u, 0xE0u, 0xF0u };

/**
 * @brief Reads the shape of the UTF-8 sequence a lead byte starts.
 *
 * @param[in]  _lead  the byte.
 * @param[out] _low   receives the least the second byte may be.
 * @param[out] _high  receives the greatest the second byte may be.
 * @return the sequence's length, 1 to 4; 0 for a byte that cannot lead.
 *
 * The length of the UTF-8 sequence a lead byte starts, by RFC 3629, and the
 * range its second byte must lie in -- narrower after E0, ED, F0 and F4, which
 * is what refuses overlong forms, surrogates and values past U+10FFFF; 0 for a
 * byte that cannot lead.
 */
D_STATIC size_t
d_arg_internal_utf8_shape(
    unsigned int  _lead,
    unsigned int* _low,
    unsigned int* _high
)
{
    *_low  = 0x80u;
    *_high = 0xBFu;

    // one byte, or two
    if (_lead < 0x80u)
    {
        return 1u;
    }

    if ( (_lead >= 0xC2u) &&
         (_lead <= 0xDFu) )
    {
        return 2u;
    }

    // three bytes, but no overlong form and no surrogate
    if ( (_lead >= 0xE0u) &&
         (_lead <= 0xEFu) )
    {
        *_low  = (_lead == 0xE0u) ? 0xA0u : 0x80u;
        *_high = (_lead == 0xEDu) ? 0x9Fu : 0xBFu;

        return 3u;
    }

    // four bytes, but no overlong form and nothing past U+10FFFF
    if ( (_lead >= 0xF0u) &&
         (_lead <= 0xF4u) )
    {
        *_low  = (_lead == 0xF0u) ? 0x90u : 0x80u;
        *_high = (_lead == 0xF4u) ? 0x8Fu : 0xBFu;

        return 4u;
    }

    return 0u;
}

/*
d_arg_internal_utf8_next
  The lead byte's shape, then each continuation byte within its range, the
code point gathered six bits at a time; `*_at` moved only once the whole
sequence is read.
*/
bool
d_arg_internal_utf8_next(
    struct d_pack_text _text,
    size_t*            _at,
    uint32_t*          _code_point
)
{
    const unsigned char* const bytes = (const unsigned char*)_text.data;
    unsigned int               low   = 0u;
    unsigned int               high  = 0u;
    const size_t               count = d_arg_internal_utf8_shape(bytes[*_at],
                                                                 &low,
                                                                 &high);
    uint32_t                   value = (count > 1u)
                                       ? (bytes[*_at] & (0x7Fu >> count))
                                       : bytes[*_at];

    // a byte that cannot lead, or a sequence cut short
    if ( (count == 0u) ||
         (count > (_text.length - *_at)) )
    {
        return false;
    }

    // each continuation byte, the second within its narrower range
    for (size_t index = 1u; index < count; ++index)
    {
        const unsigned int next = bytes[*_at + index];

        if ( (next < ((index == 1u) ? low : 0x80u)) ||
             (next > ((index == 1u) ? high : 0xBFu)) )
        {
            return false;
        }

        value = (value << 6u) | (next & 0x3Fu);
    }

    *_at         += count;
    *_code_point  = value;

    return true;
}

/**
 * @brief Tells whether text is well-formed UTF-8.
 *
 * @param[in] _text  the text, NUL-terminated.
 * @return `true` for well-formed UTF-8; `false` otherwise.
 *
 * Whether NUL-terminated text is well-formed UTF-8.
 */
D_STATIC bool
d_arg_internal_utf8_valid(
    const char* _text
)
{
    const struct d_pack_text text       = d_arg_internal_text_of(_text);
    size_t                   at         = 0u;
    uint32_t                 code_point = 0u;

    // each sequence in turn
    while (at < text.length)
    {
        if (!d_arg_internal_utf8_next(text,
                                      &at,
                                      &code_point))
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Counts the UTF-16 units UTF-8 text needs.
 *
 * @param[in] _text  the text.
 * @pre    `_text` is well-formed UTF-8.
 * @return the count.
 *
 * How many UTF-16 units well-formed UTF-8 text needs: one for each code
 * point, two past U+FFFF.
 */
D_STATIC size_t
d_arg_internal_utf16_units(
    struct d_pack_text _text
)
{
    size_t   at         = 0u;
    size_t   units      = 0u;
    uint32_t code_point = 0u;

    // each sequence in turn
    while (d_arg_internal_utf8_next(_text,
                                    &at,
                                    &code_point))
    {
        units += (code_point > 0xFFFFu) ? 2u : 1u;
    }

    return units;
}

/*
d_arg_internal_wide_from_utf8
  UTF-8 checked, measured, then written as UTF-16, a code point past U+FFFF
as a surrogate pair, into an array that ends in a 0 unit.
*/
uint16_t*
d_arg_internal_wide_from_utf8(
    const char* _text
)
{
    // only well-formed text converts
    if ( (!_text) ||
         (!d_arg_internal_utf8_valid(_text)) )
    {
        return NULL;
    }

    const struct d_pack_text text  = d_arg_internal_text_of(_text);
    const size_t             units = d_arg_internal_utf16_units(text);
    uint16_t* const          wide  = (units < (((size_t)-1 /
                                                sizeof(uint16_t)) - 1u))
                                     ? (uint16_t*)malloc((units + 1u) *
                                                         sizeof(uint16_t))
                                     : NULL;
    size_t                   at    = 0u;
    size_t                   out   = 0u;
    uint32_t                 value = 0u;

    // check if memory allocation was successful
    if (!wide)
    {
        return NULL;
    }

    // each code point, a pair past U+FFFF
    while (d_arg_internal_utf8_next(text,
                                    &at,
                                    &value))
    {
        if (value > 0xFFFFu)
        {
            wide[out++] = (uint16_t)(0xD800u + ((value - 0x10000u) >> 10u));
            value       = 0xDC00u + ((value - 0x10000u) & 0x3FFu);
        }

        wide[out++] = (uint16_t)value;
    }

    wide[out] = 0u;

    return wide;
}

/*
d_arg_internal_utf8_append
  A code point as one to four UTF-8 bytes; one UTF-16 cannot carry, or
Unicode does not have, as U+FFFD.
*/
int
d_arg_internal_utf8_append(
    struct d_arg_buffer* _out,
    uint32_t             _code_point
)
{
    const uint32_t value = ( ( (_code_point >= 0xD800u) &&
                               (_code_point <= 0xDFFFu) ) ||
                             (_code_point > 0x10FFFFu) )
                           ? REPLACEMENT
                           : _code_point;
    const size_t   count    = (value < 0x80u)
                              ? 1u
                              : (value < 0x800u)
                                ? 2u
                                : (value < 0x10000u) ? 3u : 4u;
    char           bytes[4] = { 0, 0, 0, 0 };

    // six bits to each continuation byte, the last first
    for (size_t index = count - 1u; index > 0u; --index)
    {
        const uint32_t shift = 6u * (uint32_t)(count - 1u - index);

        bytes[index] = (char)(0x80u | ((value >> shift) & 0x3Fu));
    }

    bytes[0] = (char)(UTF8_LEADS[count] |
                      (value >> (6u * (uint32_t)(count - 1u))));

    return d_arg_buffer_append(_out,
                               bytes,
                               count);
}

/*
d_arg_internal_utf16_feed
  One UTF-16 unit: a low surrogate completes the high one waiting; a high
one waits; anything else is a code point, the unpaired surrogate before it,
if any, written U+FFFD.
*/
int
d_arg_internal_utf16_feed(
    struct d_arg_internal_utf16* _state,
    uint32_t                     _unit,
    struct d_arg_buffer*         _out
)
{
    const uint32_t unit   = _unit & 0xFFFFu;
    const bool     low    = ( (unit >= 0xDC00u) &&
                              (unit <= 0xDFFFu) );
    int            status = D_ARG_OK;

    // a pair completed
    if ( (_state->high != 0u) &&
         (low) )
    {
        const uint32_t high = _state->high;

        _state->high = 0u;

        return d_arg_internal_utf8_append(_out,
                                          0x10000u                    +
                                          ((high - 0xD800u) << 10u)   +
                                          (unit - 0xDC00u));
    }

    // a high surrogate left unpaired
    if (_state->high != 0u)
    {
        _state->high = 0u;
        status       = d_arg_internal_utf8_append(_out,
                                                  REPLACEMENT);
    }

    // a high surrogate waits for its pair
    if ( (unit >= 0xD800u) &&
         (unit <= 0xDBFFu) )
    {
        _state->high = unit;

        return status;
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_utf8_append(_out,
                                        unit)
           : status;
}

/*
d_arg_internal_utf16_finish
  The end of the units: a high surrogate still waiting is U+FFFD.
*/
int
d_arg_internal_utf16_finish(
    struct d_arg_internal_utf16* _state,
    struct d_arg_buffer*         _out
)
{
    // nothing waits
    if (_state->high == 0u)
    {
        return D_ARG_OK;
    }

    _state->high = 0u;

    return d_arg_internal_utf8_append(_out,
                                      REPLACEMENT);
}

//==============================================================================
// 2.  FILES
//==============================================================================

// PATH_NONE
//   constant: what is known of a path that does not exist.
static const struct d_arg_internal_path_info PATH_NONE;

// READ_CHUNK
//   constant: how many bytes a file is read in at a time.
enum
{
    READ_CHUNK = 4096
};

#if (D_ENV_ARG_PARSER_STAT == D_ENV_ARG_PARSER_STAT_WINDOWS)

/**
 * @brief Gets the status of a directory named with separators after it,
 *        which the older Microsoft runtime, msvcrt, does not find: the path
 *        stated again without them.
 *
 * @param[in,out] _wide    the path, UTF-16, which loses the separators that
 *                         end it.
 * @param[out]    _status  receives the status.
 * @return 0 for a directory found so; -1 otherwise -- no separator to drop,
 *         one that ends a root (`/`, `C:\\`), or a path that is not a
 *         directory.
 *
 * A separator after a name asks for a directory, as POSIX's stat reads one,
 * so a file found without it is not found. The Universal C Runtime finds
 * such a directory itself, and never gets here for one.
 */
D_STATIC int
d_arg_internal_system_stat_directory(
    uint16_t*       _wide,
    struct _stat64* _status
)
{
    size_t length = 0u;

    // the path's length
    while (_wide[length] != 0u)
    {
        ++length;
    }

    size_t kept = length;

    // the separators that end it
    while ( (kept > 0u) &&
            ( (_wide[kept - 1u] == (uint16_t)'/') ||
              (_wide[kept - 1u] == (uint16_t)'\\') ) )
    {
        --kept;
    }

    // nothing to drop, a root, or a drive's root
    if ( (kept == length) ||
         (kept == 0u)     ||
         (_wide[kept - 1u] == (uint16_t)':') )
    {
        return -1;
    }

    _wide[kept] = 0u;

    return ( (_wstat64((const wchar_t*)_wide,
                       _status) == 0) &&
             ((_status->st_mode & _S_IFDIR) != 0) )
           ? 0
           : -1;
}

/**
 * @brief Gets a path's status from the Microsoft runtime.
 *
 * @param[in]  _path    the path.
 * @param[out] _status  receives the status.
 * @return 0 on success; -1 on failure, as _stat64's.
 *
 * The Microsoft runtime's status of a path: a UTF-8 path widened for
 * _wstat64 -- and, where that finds nothing and the path ends in a
 * separator, stated again as a directory without it, for msvcrt -- any
 * other given to _stat64 in the ANSI code page, whose answer it keeps: a
 * separator's byte there can end a character of two.
 */
D_STATIC int
d_arg_internal_system_stat(
    const char*     _path,
    struct _stat64* _status
)
{
    // not UTF-8: the ANSI code page
    if (!d_arg_internal_utf8_valid(_path))
    {
        return _stat64(_path,
                       _status);
    }

    uint16_t* const wide   = d_arg_internal_wide_from_utf8(_path);
    int             result = (wide != NULL)
                             ? _wstat64((const wchar_t*)wide,
                                        _status)
                             : -1;

    // a directory named with a separator after it, which msvcrt misses
    if ( (result != 0) &&
         (wide != NULL) )
    {
        result = d_arg_internal_system_stat_directory(wide,
                                                      _status);
    }

    free(wide);

    return result;
}

/*
d_arg_internal_path_query
  _wstat64's answer: a directory or not, a regular file or not, the size,
and the permissions Microsoft's std::filesystem reports for any file --
read and execute always, write unless the file is read-only, which the
runtime's write bit says -- rather than the runtime's own execute bit,
which only certain extensions get.
*/
bool
d_arg_internal_path_query(
    const char*                      _path,
    struct d_arg_internal_path_info* _info
)
{
    // validate the parameters
    if (!_info)
    {
        return false;
    }

    *_info = PATH_NONE;

    struct _stat64 status = { 0 };

    // a path that cannot be examined
    if ( (!_path) ||
         (d_arg_internal_system_stat(_path,
                                     &status) != 0) )
    {
        return false;
    }

    _info->kind       = ((status.st_mode & _S_IFDIR) != 0) ? 2 : 1;
    _info->regular    = ((status.st_mode & _S_IFMT) == _S_IFREG);
    _info->size       = (status.st_size > 0) ? (uintmax_t)status.st_size : 0u;
    _info->readable   = true;
    _info->writable   = ((status.st_mode & _S_IWRITE) != 0);
    _info->executable = true;

    return true;
}

#elif (D_ENV_ARG_PARSER_STAT == D_ENV_ARG_PARSER_STAT_POSIX)

/*
d_arg_internal_path_query
  stat's answer, symbolic links followed: a directory or not, a regular file
or not, the size, and whether the file's bits let anyone -- owner, group or
others -- read, write or execute it, as CLI11's permission checks read
std::filesystem's.
*/
bool
d_arg_internal_path_query(
    const char*                      _path,
    struct d_arg_internal_path_info* _info
)
{
    // validate the parameters
    if (!_info)
    {
        return false;
    }

    *_info = PATH_NONE;

    struct stat status = { 0 };

    // a path that cannot be examined
    if ( (!_path) ||
         (stat(_path,
               &status) != 0) )
    {
        return false;
    }

    _info->kind       = (S_ISDIR(status.st_mode)) ? 2 : 1;
    _info->regular    = (S_ISREG(status.st_mode) != 0);
    _info->size       = (status.st_size > 0) ? (uintmax_t)status.st_size : 0u;
    _info->readable   = ((status.st_mode & (S_IRUSR | S_IRGRP | S_IROTH)) != 0);
    _info->writable   = ((status.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0);
    _info->executable = ((status.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0);

    return true;
}

#else

/*
d_arg_internal_path_query
  Without a status call: a path that opens for reading is a readable regular
file, its size what seeking to its end finds; anything else does not exist,
and directories cannot be told apart.
*/
bool
d_arg_internal_path_query(
    const char*                      _path,
    struct d_arg_internal_path_info* _info
)
{
    // validate the parameters
    if (!_info)
    {
        return false;
    }

    *_info = PATH_NONE;

    FILE* const stream = (_path != NULL)
                         ? fopen(_path,
                                 "rb")
                         : NULL;

    // a path that cannot be opened
    if (!stream)
    {
        return false;
    }

    const long end = (fseek(stream,
                            0L,
                            SEEK_END) == 0)
                     ? ftell(stream)
                     : -1L;

    _info->kind     = 1;
    _info->regular  = true;
    _info->size     = (end > 0L) ? (uintmax_t)end : 0u;
    _info->readable = true;
    (void)fclose(stream);

    return true;
}

#endif  // D_ENV_ARG_PARSER_STAT

/*
d_arg_internal_path_kind
  CLI11's check_path, from the one status query.
*/
int
d_arg_internal_path_kind(
    const char* _path
)
{
    struct d_arg_internal_path_info info = PATH_NONE;

    return (d_arg_internal_path_query(_path,
                                      &info))
           ? info.kind
           : 0;
}

#if ( (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) &&                                    \
      (D_ENV_ARG_PARSER_FOPEN == D_ENV_ARG_PARSER_FOPEN_SECURE) )

/**
 * @brief Opens a file for reading.
 *
 * @param[in] _path  the path.
 * @param[in] _text  whether in text mode, not binary.
 * @return the stream, which the caller closes; NULL when the file could not be
 *         opened.
 *
 * A file opened for reading, in text mode or binary, by the runtime's secure
 * functions: a UTF-8 path widened for _wfopen_s, any other given to fopen_s
 * in the ANSI code page.
 */
D_STATIC FILE*
d_arg_internal_system_open(
    const char* _path,
    bool        _text
)
{
    FILE* stream = NULL;

    // not UTF-8: the ANSI code page
    if (!d_arg_internal_utf8_valid(_path))
    {
        return (fopen_s(&stream,
                        _path,
                        (_text) ? "r" : "rb") == 0)
               ? stream
               : NULL;
    }

    uint16_t* const wide = d_arg_internal_wide_from_utf8(_path);

    // opened only once widened
    if ( (wide != NULL) &&
         (_wfopen_s(&stream,
                    (const wchar_t*)wide,
                    (_text) ? L"r" : L"rb") != 0) )
    {
        stream = NULL;
    }

    free(wide);

    return stream;
}

#elif D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)

/**
 * @brief Opens a file for reading.
 *
 * @param[in] _path  the path.
 * @param[in] _text  whether in text mode, not binary.
 * @return the stream, which the caller closes; NULL when the file could not be
 *         opened.
 *
 * A file opened for reading, in text mode or binary: a UTF-8 path widened
 * for _wfopen, any other given to fopen in the ANSI code page.
 */
D_STATIC FILE*
d_arg_internal_system_open(
    const char* _path,
    bool        _text
)
{
    // not UTF-8: the ANSI code page
    if (!d_arg_internal_utf8_valid(_path))
    {
        return fopen(_path,
                     (_text) ? "r" : "rb");
    }

    uint16_t* const wide   = d_arg_internal_wide_from_utf8(_path);
    FILE* const     stream = (wide != NULL)
                             ? _wfopen((const wchar_t*)wide,
                                       (_text) ? L"r" : L"rb")
                             : NULL;

    free(wide);

    return stream;
}

#else

/**
 * @brief Opens a file for reading.
 *
 * @param[in] _path  the path.
 * @param[in] _text  whether in text mode, not binary.
 * @return the stream, which the caller closes; NULL when the file could not be
 *         opened.
 *
 * A file opened for reading, its path the bytes the system takes, in text
 * mode or binary, which elsewhere read alike.
 */
D_STATIC FILE*
d_arg_internal_system_open(
    const char* _path,
    bool        _text
)
{
    return fopen(_path,
                 (_text) ? "r" : "rb");
}

#endif  // D_ENV_IS_OS_WINDOWS, D_ENV_ARG_PARSER_FOPEN

/**
 * @brief Reads an open stream to its end into a buffer.
 *
 * @param[in,out] _stream  the stream.
 * @param[in,out] _out     the buffer that gains its bytes.
 * @return D_ARG_OK; D_ARG_ERROR_FILE when a read failed; or the buffer's
 *         status.
 *
 * An open stream read to its end, a chunk at a time, into a buffer; a read
 * that fails is D_ARG_ERROR_FILE.
 */
D_STATIC int
d_arg_internal_read_stream(
    FILE*                _stream,
    struct d_arg_buffer* _out
)
{
    char chunk[READ_CHUNK] = { 0 };
    bool done              = false;
    int  status            = D_ARG_OK;

    // a chunk at a time, to the end
    while ( (status == D_ARG_OK) &&
            (!done) )
    {
        const size_t got = fread(chunk,
                                 1u,
                                 sizeof chunk,
                                 _stream);

        status = d_arg_buffer_append(_out,
                                     chunk,
                                     got);
        done   = (got < sizeof chunk);
    }

    return ( (status == D_ARG_OK) &&
             (ferror(_stream) != 0) )
           ? D_ARG_ERROR_FILE
           : status;
}

/*
d_arg_internal_read_file
  A whole file appended to a buffer, as its bytes or as the runtime reads
text, the buffer cut back to what it held when the file cannot be read to
its end.
*/
int
d_arg_internal_read_file(
    const char*          _path,
    bool                 _text,
    struct d_arg_buffer* _out
)
{
    // validate the parameters
    if ( (!_path) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    FILE* const stream = d_arg_internal_system_open(_path,
                                                    _text);

    // a file that does not open
    if (!stream)
    {
        return D_ARG_ERROR_FILE;
    }

    const size_t before = _out->length;
    int          status = d_arg_internal_read_stream(stream,
                                                     _out);

    (void)fclose(stream);

    // a failure leaves the buffer as it was
    if (status != D_ARG_OK)
    {
        d_arg_buffer_truncate(_out,
                              before);
    }

    return status;
}

//==============================================================================
// 3.  THE ENVIRONMENT
//==============================================================================

#if (D_ENV_ARG_PARSER_GETENV == D_ENV_ARG_PARSER_GETENV_DUPENV)

/*
d_arg_internal_getenv
  CLI11's get_environment_value under the Microsoft runtime: _dupenv_s's
copy of the value, appended and freed.
*/
int
d_arg_internal_getenv(
    const char*          _name,
    struct d_arg_buffer* _out
)
{
    // validate the parameters
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    char*  value  = NULL;
    size_t length = 0u;

    // unnamed, unset, or unreadable
    if ( (!_name)                            ||
         (_dupenv_s(&value,
                    &length,
                    _name) != 0)             ||
         (!value) )
    {
        return D_ARG_OK;
    }

    const int status = d_arg_buffer_append_cstr(_out,
                                                value);

    free(value);

    return status;
}

#else

/*
d_arg_internal_getenv
  CLI11's get_environment_value: getenv's value, appended.
*/
int
d_arg_internal_getenv(
    const char*          _name,
    struct d_arg_buffer* _out
)
{
    // validate the parameters
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const char* const value = (_name != NULL)
                              ? getenv(_name)
                              : NULL;

    return d_arg_buffer_append_cstr(_out,
                                    value);
}

#endif  // D_ENV_ARG_PARSER_GETENV

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

//==============================================================================
// 4.  THE PROCESS
//==============================================================================

#if (D_ENV_ARG_PARSER_COMMAND_LINE == D_ENV_ARG_PARSER_COMMAND_LINE_WINDOWS)

/*
d_arg_internal_command_line
  GetCommandLineW's line, which Windows keeps for every process as it was
given, converted from UTF-16.
*/
int
d_arg_internal_command_line(
    struct d_arg_buffer* _out
)
{
    // validate the parameters
    if (!_out)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const wchar_t* const        line   = GetCommandLineW();
    struct d_arg_internal_utf16 state  = { 0u };
    int                         status = D_ARG_OK;

    // a process without one
    if (!line)
    {
        return D_ARG_ERROR_UNSUPPORTED;
    }

    // each unit in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (line[index] != L'\0');
         ++index)
    {
        status = d_arg_internal_utf16_feed(&state,
                                           (uint32_t)line[index],
                                           _out);
    }

    return (status == D_ARG_OK)
           ? d_arg_internal_utf16_finish(&state,
                                         _out)
           : status;
}

#else

/*
d_arg_internal_command_line
  Where `argv` is the user's text already, there is no other line.
*/
int
d_arg_internal_command_line(
    struct d_arg_buffer* _out
)
{
    return (_out != NULL)
           ? D_ARG_ERROR_UNSUPPORTED
           : D_ARG_ERROR_BAD_ARGUMENT;
}

#endif  // D_ENV_ARG_PARSER_COMMAND_LINE

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

#if (D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH == 1)

//==============================================================================
// 5.  THE TERMINAL
//==============================================================================

// COLUMNS_MAX
//   constant: the widest terminal the COLUMNS variable is believed to report.
enum
{
    COLUMNS_MAX = 10000
};

#if (D_ENV_ARG_PARSER_TERMINAL == D_ENV_ARG_PARSER_TERMINAL_WINDOWS)

/**
 * @brief Measures the terminal standard output is shown in.
 *
 * @return its width in columns; 0 when standard output is not a terminal, or
 *         cannot be measured.
 *
 * The width of the console window standard output is shown in; 0 when
 * standard output is not a console.
 */
D_STATIC size_t
d_arg_internal_terminal_measure(void)
{
    CONSOLE_SCREEN_BUFFER_INFO info   = { 0 };
    const HANDLE               output = GetStdHandle(STD_OUTPUT_HANDLE);

    // not a console
    if ( (output == NULL)                              ||
         (output == INVALID_HANDLE_VALUE)              ||
         (!GetConsoleScreenBufferInfo(output,
                                      &info)) )
    {
        return 0u;
    }

    const int width = info.srWindow.Right - info.srWindow.Left + 1;

    return (width > 0) ? (size_t)width : 0u;
}

#elif ( (D_ENV_ARG_PARSER_TERMINAL == D_ENV_ARG_PARSER_TERMINAL_POSIX) &&     \
        (defined(TIOCGWINSZ)) )

/**
 * @brief Measures the terminal standard output is shown in.
 *
 * @return its width in columns; 0 when standard output is not a terminal, or
 *         cannot be measured.
 *
 * The width TIOCGWINSZ reports for the terminal standard output is; 0 when
 * standard output is not a terminal.
 */
D_STATIC size_t
d_arg_internal_terminal_measure(void)
{
    struct winsize size = { 0 };

    // not a terminal, or not measured
    if ( (!isatty(STDOUT_FILENO)) ||
         (ioctl(STDOUT_FILENO,
                TIOCGWINSZ,
                &size) != 0) )
    {
        return 0u;
    }

    return (size_t)size.ws_col;
}

#else

/**
 * @brief Measures the terminal standard output is shown in.
 *
 * @return its width in columns; 0 when standard output is not a terminal, or
 *         cannot be measured.
 *
 * Without a way to ask -- or with a system that hides TIOCGWINSZ from the
 * build's feature macros -- nothing is measured.
 */
D_STATIC size_t
d_arg_internal_terminal_measure(void)
{
    return 0u;
}

#endif  // D_ENV_ARG_PARSER_TERMINAL

/**
 * @brief Reads the COLUMNS variable as a width.
 *
 * @return the width, 1 to COLUMNS_MAX; 0 for no variable, or anything but such
 *         digits.
 *
 * The COLUMNS variable's width: digits alone, from 1 to COLUMNS_MAX; 0 for
 * anything else.
 */
D_STATIC size_t
d_arg_internal_terminal_columns(void)
{
    struct d_arg_buffer value   = D_ARG_BUFFER_INIT;
    size_t              columns = 0u;
    const int           status  = d_arg_internal_getenv("COLUMNS",
                                                        &value);

    // each digit, until one is not, or the number grows too wide
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < value.length);
         ++index)
    {
        if ( (!d_arg_internal_is_digit(value.data[index])) ||
             (columns > (size_t)COLUMNS_MAX) )
        {
            columns = 0u;

            break;
        }

        columns = (columns * 10u) + (size_t)(value.data[index] - '0');
    }

    d_arg_buffer_release(&value);

    return (columns <= (size_t)COLUMNS_MAX) ? columns : 0u;
}

/*
d_arg_terminal_width
  The terminal measured, or else the COLUMNS variable read.
*/
size_t
d_arg_terminal_width(void)
{
    const size_t measured = d_arg_internal_terminal_measure();

    return (measured > 0u)
           ? measured
           : d_arg_internal_terminal_columns();
}

#endif  // D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH
