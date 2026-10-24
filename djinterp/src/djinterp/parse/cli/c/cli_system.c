/*******************************************************************************
* djinterp [parse]                                                  cli_system.c
*
* What the command-line kit asks of the operating system, declared in
* cli_internal.h (section 1).
*   The one source of the kit that includes a system header. Each question
* is answered by the implementation env_cli.h's facts select, chosen whole, a
* function at a time: a terminal through isatty or the console API, its width
* through TIOCGWINSZ or the console's screen buffer, the standard streams in
* binary mode on Windows and a console written through WriteConsoleW, the
* environment through _wgetenv on Windows and getenv elsewhere, the
* program's file through the system's own way of saying it, and modules
* through dlopen or LoadLibraryExW. On Windows every text that crosses is
* UTF-8 on the kit's side and UTF-16 on the system's.
*   Needs POSIX.1-2008 with XSI where the C library hides it in a strict ISO
* mode, as the C guide has every build give it (`-D_XOPEN_SOURCE=700`); and,
* on macOS and FreeBSD, the headers' own extensions, which that level hides
* on neither.
*
*
* path:      /src/djinterp/parse/cli/c/cli_system.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "./cli_internal.h"  // corresponding header
// std
#include <errno.h>   // errno, EILSEQ, EINVAL, ENOENT, ENOMEM
#include <stdio.h>   // stdin, stdout, stderr, fwrite, fflush, fileno
#include <stdlib.h>  // getenv, __argc and __argv on Windows, and the hooks'
                     // malloc, realloc and free
#include <string.h>  // memcpy, strcmp, strlen, strerror, strchr
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/filesys/file_dir.h"  // d_path_resolve
#include "../../../../../inc/djinterp/parse/cli/c/cli_io.h"  // d_cli_stream
#include "../../../../../inc/djinterp/env/parse/cli/env_cli.h"  // D_ENV_CLI_*
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t
// platform
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif  // WIN32_LEAN_AND_MEAN
    #include <windows.h>    // GetStdHandle, GetConsoleMode, WriteConsoleW,
                            // LoadLibraryExW, GetModuleFileNameW
    #include <fcntl.h>      // _O_BINARY
    #include <io.h>         // _setmode, _fileno
    #include <sys/stat.h>   // _S_IFMT, _S_IFREG, _S_IFDIR
    #include <wchar.h>      // wchar_t, _wgetenv
#else
    #include <sys/stat.h>   // S_ISREG, S_ISDIR
#endif
#if (D_ENV_CLI_CONSOLE == D_ENV_CLI_CONSOLE_POSIX)
    #include <sys/ioctl.h>  // ioctl, TIOCGWINSZ, struct winsize
    #include <unistd.h>     // isatty, readlink, access, X_OK
#elif !D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #include <unistd.h>     // readlink, access, X_OK
#endif
#if (D_ENV_CLI_MODULES == D_ENV_CLI_MODULES_DLOPEN)
    #include <dlfcn.h>      // dlopen, dlsym, dlclose, dlerror
#endif
#if (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_MACOS)
    #include <mach-o/dyld.h>  // _NSGetExecutablePath
#elif (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_SYSCTL)
    #include <sys/types.h>    // u_int
    #include <sys/sysctl.h>   // sysctl, CTL_KERN, KERN_PROC,
                              // KERN_PROC_PATHNAME
#endif


//==============================================================================
// 0.  WIDE TEXT (Windows)
//==============================================================================

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)

/**
 * @brief Converts UTF-8 to UTF-16, in a block to be freed with free.
 *
 * @param[in] _text    the text.
 * @param[in] _length  its length in bytes; (size_t)-1 for a C string, whose
 *                     NUL is converted too.
 * @param[out] _units  receives how many UTF-16 units were written, not
 *                     counting a NUL; may be NULL.
 * @return the text, NUL-terminated; or NULL, errno set: EILSEQ when it is
 *         not UTF-8, ENOMEM when there is no room.
 */
static wchar_t*
d_cli_internal_widen(
    const char* _text,
    size_t      _length,
    size_t*     _units
)
{
    const int length = (_length == (size_t)-1)
                       ? -1
                       : (int)_length;
    const int units  = (_length == 0u)
                       ? 0
                       : MultiByteToWideChar(CP_UTF8,
                                             MB_ERR_INVALID_CHARS,
                                             _text,
                                             length,
                                             NULL,
                                             0);

    // not UTF-8, or empty
    if ( (units <= 0) &&
         (_length != 0u) )
    {
        errno = EILSEQ;

        return NULL;
    }

    wchar_t* const wide = (wchar_t*)D_INTERNAL_CLI_MALLOC(
                              ((size_t)units + 1u) * sizeof(wchar_t));

    // no room
    if (!wide)
    {
        errno = ENOMEM;

        return NULL;
    }

    // convert, and terminate past what a length-given text holds
    if (units > 0)
    {
        (void)MultiByteToWideChar(CP_UTF8,
                                  MB_ERR_INVALID_CHARS,
                                  _text,
                                  length,
                                  wide,
                                  units);
    }

    wide[units] = L'\0';

    if (_units != NULL)
    {
        *_units = (length < 0)
                  ? (size_t)units - 1u
                  : (size_t)units;
    }

    return wide;
}

/**
 * @brief Converts UTF-16 to UTF-8, in a block to be freed with free.
 *
 * @param[in] _text  the NUL-terminated text.
 * @return the text, NUL-terminated, or NULL when there is no room -- or
 *         when it does not convert, which UTF-16 always does, a unit that
 *         pairs with nothing becoming U+FFFD.
 */
static char*
d_cli_internal_narrow(
    const wchar_t* _text
)
{
    const int bytes = WideCharToMultiByte(CP_UTF8,
                                          0,
                                          _text,
                                          -1,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL);

    // nothing converts
    if (bytes <= 0)
    {
        return NULL;
    }

    char* const narrow = (char*)D_INTERNAL_CLI_MALLOC((size_t)bytes);

    // no room
    if (!narrow)
    {
        return NULL;
    }

    (void)WideCharToMultiByte(CP_UTF8,
                              0,
                              _text,
                              -1,
                              narrow,
                              bytes,
                              NULL,
                              NULL);

    return narrow;
}

/**
 * @brief The console handle of a standard stream, or NULL when the stream
 *        is no console.
 *
 * @param[in] _stream  a d_cli_stream value.
 * @return the handle, or NULL.
 */
static HANDLE
d_cli_internal_console(
    int _stream
)
{
    const DWORD which  = (_stream == (int)D_CLI_STREAM_INPUT)
                         ? STD_INPUT_HANDLE
                         : (_stream == (int)D_CLI_STREAM_OUTPUT)
                           ? STD_OUTPUT_HANDLE
                           : STD_ERROR_HANDLE;
    const HANDLE handle = GetStdHandle(which);
    DWORD        mode   = 0;

    // a console answers GetConsoleMode; a file or a pipe does not
    if ( (handle == NULL)                 ||
         (handle == INVALID_HANDLE_VALUE) ||
         (!GetConsoleMode(handle,
                          &mode)) )
    {
        return NULL;
    }

    return handle;
}

#endif  // D_ENV_IS_OS_WINDOWS


//==============================================================================
// 1.  THE SYSTEM
//==============================================================================

// 1.1    Terminals
//------------------------------------------------------------------------------

/*
d_cli_internal_is_terminal
  A console on Windows, isatty's answer on POSIX, and nothing elsewhere.
*/
bool
d_cli_internal_is_terminal(
    int _stream
)
{
    // only the three standard streams
    if ( (_stream < (int)D_CLI_STREAM_INPUT) ||
         (_stream > (int)D_CLI_STREAM_ERROR) )
    {
        return false;
    }

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    return (d_cli_internal_console(_stream) != NULL);
#elif (D_ENV_CLI_CONSOLE == D_ENV_CLI_CONSOLE_POSIX)
    return (isatty(_stream) != 0);
#else
    return false;
#endif
}

/*
d_cli_internal_take_sequences
  A POSIX terminal takes ANSI sequences as it is. A Windows console takes
them once ENABLE_VIRTUAL_TERMINAL_PROCESSING is on, which Windows 10 from
1511 accepts and an older console refuses -- the refusal is the answer.
*/
bool
d_cli_internal_take_sequences(
    int _stream
)
{
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif  // ENABLE_VIRTUAL_TERMINAL_PROCESSING

    const HANDLE console = d_cli_internal_console(_stream);
    DWORD        mode    = 0;

    // no console, or one that will not say its mode
    if ( (console == NULL) ||
         (!GetConsoleMode(console,
                          &mode)) )
    {
        return false;
    }

    // already on, or turned on now
    return ( ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0u) ||
             (SetConsoleMode(console,
                             mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
                  != 0) );
#else
    return d_cli_internal_is_terminal(_stream);
#endif
}

/*
d_cli_internal_terminal_columns
  The console's window on Windows, TIOCGWINSZ on POSIX; 0 when standard
output is no terminal, or the system will not say.
*/
size_t
d_cli_internal_terminal_columns(void)
{
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    const HANDLE               console = d_cli_internal_console(
                                             (int)D_CLI_STREAM_OUTPUT);
    CONSOLE_SCREEN_BUFFER_INFO info;

    // no console, or one that will not say
    if ( (console == NULL) ||
         (!GetConsoleScreenBufferInfo(console,
                                      &info)) )
    {
        return 0u;
    }

    return (size_t)(info.srWindow.Right - info.srWindow.Left + 1);
#elif ( (D_ENV_CLI_CONSOLE == D_ENV_CLI_CONSOLE_POSIX) &&                     \
        (defined(TIOCGWINSZ)) )
    struct winsize size;

    // no terminal, or one that will not say
    if ( (!isatty(STDOUT_FILENO)) ||
         (ioctl(STDOUT_FILENO,
                TIOCGWINSZ,
                &size) != 0) )
    {
        return 0u;
    }

    return (size_t)size.ws_col;
#else
    return 0u;
#endif
}

// 1.2    The standard streams
//------------------------------------------------------------------------------

/*
d_cli_internal_binary_streams
  Windows' C runtime opens the standard streams in text mode, which writes
CR LF for every LF (R18 wants LF) and reads a Ctrl-Z as the end; binary mode
writes and reads the bytes as they are. Once is enough, and a flag says
whether it has been done.
*/
void
d_cli_internal_binary_streams(void)
{
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    static int done = 0;

    // the first call only
    if (!done)
    {
        done = 1;
        (void)fflush(stdout);
        (void)fflush(stderr);
        (void)_setmode(_fileno(stdin),
                       _O_BINARY);
        (void)_setmode(_fileno(stdout),
                       _O_BINARY);
        (void)_setmode(_fileno(stderr),
                       _O_BINARY);
    }
#endif

    return;
}

#if (D_INTERNAL_CLI_CONSOLE_WIDE == 1)

/**
 * @brief How long a UTF-8 sequence its lead byte starts is, by RFC 3629's
 *        ranges: 1 to 4, or 1 for a byte that leads nothing, which is then
 *        written as it is and replaced by the conversion.
 *
 * @param[in] _lead  the byte.
 * @return the sequence's length.
 */
static size_t
d_cli_internal_utf8_length(
    unsigned char _lead
)
{
    if (_lead < 0xC2u)
    {
        return 1u;
    }

    if (_lead < 0xE0u)
    {
        return 2u;
    }

    if (_lead < 0xF0u)
    {
        return 3u;
    }

    return (_lead < 0xF5u) ? 4u : 1u;
}

/**
 * @brief Writes UTF-8 to a console as UTF-16, all of it.
 *
 * @param[in] _console  the console.
 * @param[in] _data     the text, whole sequences.
 * @param[in] _size     its length.
 * @return `true` if the console took every unit.
 */
static bool
d_cli_internal_console_write(
    HANDLE      _console,
    const char* _data,
    size_t      _size
)
{
    size_t         units = 0u;
    wchar_t* const wide  = d_cli_internal_widen(_data,
                                                _size,
                                                &units);

    // text that is not UTF-8 goes as the code page would read it
    if (!wide)
    {
        DWORD written = 0;

        return ( (WriteFile(_console,
                            _data,
                            (DWORD)_size,
                            &written,
                            NULL) != 0) &&
                 (written == (DWORD)_size) );
    }

    size_t at = 0u;
    bool   ok = true;

    // the console may take fewer units than it was given
    while ( (ok) &&
            (at < units) )
    {
        DWORD wrote = 0;

        ok = ( (WriteConsoleW(_console,
                              wide + at,
                              (DWORD)(units - at),
                              &wrote,
                              NULL) != 0) &&
               (wrote > 0u) );
        at += (size_t)wrote;
    }

    D_INTERNAL_CLI_FREE(wide);

    return ok;
}

#endif  // D_INTERNAL_CLI_CONSOLE_WIDE

#if (D_INTERNAL_CLI_CONSOLE_WIDE == 1)

// d_cli_internal_held / d_cli_internal_held_size
//   variables: the start of a UTF-8 sequence a write to each standard stream
// ended in, held back until the write that finishes it.
static unsigned char d_cli_internal_held[3][4];
static size_t        d_cli_internal_held_size[3];

/**
 * @brief Finishes the sequence held back from a stream's last write with
 *        the first bytes of this one, and writes it.
 *
 * @param[in]  _console  the console.
 * @param[in]  _which    the stream, a d_cli_stream value.
 * @param[in]  _bytes    this write's bytes.
 * @param[in]  _size     how many.
 * @param[out] _used     receives how many of them it took.
 * @return 1 when the sequence was written, or none was held; 0 when this
 *         write did not finish it either, every byte now held; -1 when the
 *         console refused it.
 */
static int
d_cli_internal_console_finish(
    HANDLE               _console,
    size_t               _which,
    const unsigned char* _bytes,
    size_t               _size,
    size_t*              _used
)
{
    unsigned char* const held = d_cli_internal_held[_which];
    size_t* const        size = &d_cli_internal_held_size[_which];

    *_used = 0u;

    // nothing held
    if (*size == 0u)
    {
        return 1;
    }

    const size_t need = d_cli_internal_utf8_length(held[0]);

    while ( (*size < need) &&
            (*_used < _size) )
    {
        held[(*size)++] = _bytes[(*_used)++];
    }

    // still short: kept for the next write
    if (*size < need)
    {
        return 0;
    }

    *size = 0u;

    return d_cli_internal_console_write(_console,
                                        (const char*)held,
                                        need)
           ? 1
           : -1;
}

/**
 * @brief Holds back the start of a UTF-8 sequence the end of a write
 *        splits, for the next write to finish.
 *
 * @param[in] _which  the stream, a d_cli_stream value.
 * @param[in] _bytes  the write's bytes.
 * @param[in] _used   where the bytes still to write begin.
 * @param[in] _size   how many bytes there are.
 * @return where the bytes to write now end.
 */
static size_t
d_cli_internal_console_hold(
    size_t               _which,
    const unsigned char* _bytes,
    size_t               _used,
    size_t               _size
)
{
    size_t end  = _size;
    size_t back = 0u;

    // the continuation bytes at the end, at most three
    while ( (back < 3u) &&
            (end > _used) &&
            ((_bytes[end - 1u] & 0xC0u) == 0x80u) )
    {
        --end;
        ++back;
    }

    // a lead byte whose sequence is longer than what follows it
    if ( (end > _used) &&
         (d_cli_internal_utf8_length(_bytes[end - 1u]) > (back + 1u)) )
    {
        --end;
        d_cli_internal_held_size[_which] = _size - end;
        memcpy(d_cli_internal_held[_which],
               _bytes + end,
               _size - end);

        return end;
    }

    return _size;
}

/**
 * @brief Writes UTF-8 to a standard stream that is a console: whole
 *        sequences, as UTF-16, a sequence a write splits held back until
 *        the next.
 *
 * @param[in] _console  the console.
 * @param[in] _stream   the stream, a d_cli_stream value.
 * @param[in] _data     the bytes.
 * @param[in] _size     how many.
 * @return `_size` when the console took them, or held them; 0 when it
 *         refused them.
 */
static size_t
d_cli_internal_write_console(
    HANDLE      _console,
    int         _stream,
    const void* _data,
    size_t      _size
)
{
    const unsigned char* const bytes = (const unsigned char*)_data;
    size_t                     used  = 0u;
    const int                  done  = d_cli_internal_console_finish(
                                           _console,
                                           (size_t)_stream,
                                           bytes,
                                           _size,
                                           &used);

    // refused, or all of it held
    if (done <= 0)
    {
        return (done == 0) ? _size : 0u;
    }

    const size_t end = d_cli_internal_console_hold((size_t)_stream,
                                                   bytes,
                                                   used,
                                                   _size);

    return ( (end == used) ||
             (d_cli_internal_console_write(_console,
                                           (const char*)(bytes + used),
                                           end - used)) )
           ? _size
           : 0u;
}

#endif  // D_INTERNAL_CLI_CONSOLE_WIDE

/*
d_cli_internal_write_stream
  A Windows console takes UTF-16, so UTF-8 bound for one is converted, a
sequence split across two writes held back until the rest arrives. Anything
else -- a file, a pipe, a POSIX terminal -- takes the bytes, flushed at
once, so that a diagnostic and a result written in turn reach a terminal in
that order.
*/
size_t
d_cli_internal_write_stream(
    int         _stream,
    const void* _data,
    size_t      _size
)
{
    FILE* const stream = (_stream == (int)D_CLI_STREAM_ERROR)
                         ? stderr
                         : stdout;

    // nothing to write
    if ( (!_data) ||
         (_size == 0u) )
    {
        return 0u;
    }

    d_cli_internal_binary_streams();

#if (D_INTERNAL_CLI_CONSOLE_WIDE == 1)
    const HANDLE console = d_cli_internal_console(_stream);

    // a console: what the C runtime buffered first, then the text
    if (console != NULL)
    {
        (void)fflush(stream);

        return d_cli_internal_write_console(console,
                                            _stream,
                                            _data,
                                            _size);
    }
#endif

    const size_t written = fwrite(_data,
                                  1u,
                                  _size,
                                  stream);

    // a short write, or a flush that failed, is a failure
    if ( (fflush(stream) != 0) ||
         (written != _size) )
    {
        return (written < _size) ? written : 0u;
    }

    return written;
}

// 1.3    The environment
//------------------------------------------------------------------------------

/*
d_cli_internal_getenv
  _wgetenv on Windows, whose environment is UTF-16, converted; getenv
elsewhere, copied, so the caller frees either the same way. The name is
ASCII, as every variable the kit reads is named. A value there and not
copied is the want of memory, not a variable unset.
*/
int
d_cli_internal_getenv(
    const char* _name,
    char**      _value
)
{
    // nowhere to put it, or no name
    if (!_value)
    {
        return EINVAL;
    }

    *_value = NULL;

    if (!_name)
    {
        return 0;
    }

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    wchar_t* const name = d_cli_internal_widen(_name,
                                               (size_t)-1,
                                               NULL);

    // no room for the name
    if (!name)
    {
        return ENOMEM;
    }

    const wchar_t* const value = _wgetenv(name);

    *_value = (value != NULL) ? d_cli_internal_narrow(value) : NULL;
    D_INTERNAL_CLI_FREE(name);
#else
    const char* const value = getenv(_name);

    *_value = d_cli_internal_copy(value);
#endif

    return ( (value != NULL) &&
             (*_value == NULL) ) ? ENOMEM : 0;
}

/*
d_cli_internal_main_arguments
  On Windows, the runtime's own record of what it gave main, `__argc` and
`__argv`, compared by text: MinGW's startup hands main a copy of `__argv`,
not `__argv` itself, so an address says nothing. A program built on wmain
has no `__argv`, and its arguments are its own to convert.
*/
bool
d_cli_internal_main_arguments(
    int                _argc,
    const char* const* _argv
)
{
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    char** const given = __argv;

    // none to compare, or not as many
    if ( (!_argv)          ||
         (!given)          ||
         (_argc != __argc) )
    {
        return false;
    }

    // each the same text
    for (int index = 0; index < _argc; ++index)
    {
        if ( (!_argv[index])           ||
             (!given[index])           ||
             (strcmp(_argv[index],
                     given[index]) != 0) )
        {
            return false;
        }
    }

    return true;
#else
    (void)_argc;
    (void)_argv;

    return false;
#endif
}

// 1.4    The program's file
//------------------------------------------------------------------------------

#if (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_ARGV) ||                     \
    (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_SOLARIS)

/**
 * @brief Tries one directory of PATH for the program: the directory, `/`
 *        and the name, which must be a file that runs.
 *
 * @param[in]  _directory  the directory; an empty one is the current.
 * @param[in]  _length     its length.
 * @param[in]  _name       the program's name.
 * @param[out] _path       receives the program's path, resolved, when it
 *                         is there.
 * @return 0 when it is there; ENOENT when it is not; ENOMEM.
 */
static int
d_cli_internal_try_path(
    const char* _directory,
    size_t      _length,
    const char* _name,
    char**      _path
)
{
    const size_t      name      = strlen(_name);
    const char* const directory = (_length > 0u) ? _directory : ".";
    const size_t      length    = (_length > 0u) ? _length : 1u;
    char* const       whole     = (char*)D_INTERNAL_CLI_MALLOC(length +
                                                               name + 2u);
    int               result    = ENOENT;

    if (!whole)
    {
        return ENOMEM;
    }

    memcpy(whole,
           directory,
           length);
    whole[length] = '/';
    memcpy(whole + length + 1u,
           _name,
           name + 1u);

    // there, and it runs
    if (access(whole,
               X_OK) == 0)
    {
        *_path = d_path_resolve(whole,
                                NULL,
                                0u);
        result = (*_path != NULL) ? 0 : ENOENT;
    }

    D_INTERNAL_CLI_FREE(whole);

    return result;
}

/**
 * @brief Works out the program's file from `argv[0]`: resolved where it
 *        names a directory, and looked for along PATH where it does not.
 *
 * @param[in]  _argv0  the program's `argv[0]`.
 * @param[out] _path   receives the path, to be freed with free.
 * @return 0, or an errno value.
 */
static int
d_cli_internal_executable_argv(
    const char* _argv0,
    char**      _path
)
{
    // nothing to work from
    if ( (!_argv0) ||
         (_argv0[0] == '\0') )
    {
        return ENOENT;
    }

    // a path: resolve it
    if (strchr(_argv0,
               '/') != NULL)
    {
        *_path = d_path_resolve(_argv0,
                                NULL,
                                0u);

        return (*_path != NULL) ? 0 : ENOENT;
    }

    char*       search = NULL;
    const int   read   = d_cli_internal_getenv("PATH",
                                               &search);
    const char* at     = search;
    int         result = (read == ENOMEM) ? ENOMEM : ENOENT;

    // each directory of PATH in turn, until the program is found
    while ( (at != NULL) &&
            (result == ENOENT) )
    {
        const char* const end = strchr(at,
                                       ':');

        result = d_cli_internal_try_path(at,
                                         (end != NULL) ? (size_t)(end - at)
                                                       : strlen(at),
                                         _argv0,
                                         _path);
        at     = (end != NULL) ? (end + 1) : NULL;
    }

    D_INTERNAL_CLI_FREE(search);

    return result;
}

#endif  // D_ENV_CLI_EXECUTABLE_ARGV || D_ENV_CLI_EXECUTABLE_SOLARIS

#if (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_WINDOWS)

/**
 * @brief The program's file as Windows says it: GetModuleFileNameW, its
 *        buffer grown until the name fits.
 *
 * @param[out] _path  receives the path, in UTF-8.
 * @return 0, ENOENT or ENOMEM.
 */
static int
d_cli_internal_executable_system(
    char** _path
)
{
    DWORD    room   = MAX_PATH;
    DWORD    length = 0u;
    wchar_t* wide   = NULL;

    // grow until the name fits
    for (;;)
    {
        wchar_t* const grown = (wchar_t*)D_INTERNAL_CLI_REALLOC(
                                   wide,
                                   (size_t)room * sizeof(wchar_t));

        if (!grown)
        {
            D_INTERNAL_CLI_FREE(wide);

            return ENOMEM;
        }

        wide   = grown;
        length = GetModuleFileNameW(NULL,
                                    wide,
                                    room);

        // failed, or fitted
        if ( (length == 0u) ||
             (length < room) )
        {
            break;
        }

        room *= 2u;
    }

    *_path = (length > 0u) ? d_cli_internal_narrow(wide) : NULL;
    D_INTERNAL_CLI_FREE(wide);

    // a name the system gave, and no room to convert it
    return (*_path != NULL) ? 0 : (length > 0u) ? ENOMEM : ENOENT;
}

#elif (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_PROC)

/**
 * @brief The program's file as the process file system says it: the link
 *        D_ENV_CLI_EXECUTABLE_LINK, read into a buffer grown until it fits.
 *
 * @param[out] _path  receives the path.
 * @return 0, ENOENT when the process file system is not mounted, or ENOMEM.
 */
static int
d_cli_internal_executable_system(
    char** _path
)
{
    size_t room = 256u;
    char*  text = NULL;

    // grow until the link fits
    for (;;)
    {
        char* const grown = (char*)D_INTERNAL_CLI_REALLOC(text,
                                                          room);

        if (!grown)
        {
            D_INTERNAL_CLI_FREE(text);

            return ENOMEM;
        }

        text = grown;

        const ssize_t length = readlink(D_ENV_CLI_EXECUTABLE_LINK,
                                        text,
                                        room);

        // no link: the process file system is not mounted
        if (length < 0)
        {
            D_INTERNAL_CLI_FREE(text);

            return ENOENT;
        }

        if ((size_t)length < room)
        {
            text[length] = '\0';
            *_path       = text;

            return 0;
        }

        room *= 2u;
    }
}

#elif (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_MACOS)

/**
 * @brief The program's file as dyld says it, resolved.
 *
 * @param[out] _path  receives the path.
 * @return 0, ENOENT or ENOMEM.
 */
static int
d_cli_internal_executable_system(
    char** _path
)
{
    uint32_t room = 0u;

    (void)_NSGetExecutablePath(NULL,
                               &room);

    char* const text = (char*)D_INTERNAL_CLI_MALLOC((size_t)room + 1u);

    // no room, or no answer
    if (!text)
    {
        return ENOMEM;
    }

    if (_NSGetExecutablePath(text,
                             &room) != 0)
    {
        D_INTERNAL_CLI_FREE(text);

        return ENOENT;
    }

    *_path = d_path_resolve(text,
                            NULL,
                            0u);
    D_INTERNAL_CLI_FREE(text);

    return (*_path != NULL) ? 0 : ENOENT;
}

#elif (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_SYSCTL)

/**
 * @brief The program's file as the kernel says it: KERN_PROC_PATHNAME, its
 *        length asked first.
 *
 * @param[out] _path  receives the path.
 * @return 0, ENOENT or ENOMEM.
 */
static int
d_cli_internal_executable_system(
    char** _path
)
{
    int    request[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t room       = 0u;

    // how long, then the path
    if (sysctl(request,
               4u,
               NULL,
               &room,
               NULL,
               0u) != 0)
    {
        return ENOENT;
    }

    char* const text = (char*)D_INTERNAL_CLI_MALLOC(room + 1u);

    if (!text)
    {
        return ENOMEM;
    }

    if (sysctl(request,
               4u,
               text,
               &room,
               NULL,
               0u) != 0)
    {
        D_INTERNAL_CLI_FREE(text);

        return ENOENT;
    }

    text[room] = '\0';
    *_path     = text;

    return 0;
}

#endif  // D_ENV_CLI_EXECUTABLE

/*
d_cli_internal_executable
  The system's own answer, where it has one, and `argv[0]` where it has
none: the implementation is chosen whole, a function each.
*/
int
d_cli_internal_executable(
    const char* _argv0,
    char**      _path
)
{
    *_path = NULL;

#if (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_SOLARIS)
    const char* const name = getexecname();

    // relative to where the process started, as argv[0] would be
    return d_cli_internal_executable_argv((name != NULL) ? name : _argv0,
                                          _path);
#elif (D_ENV_CLI_EXECUTABLE == D_ENV_CLI_EXECUTABLE_ARGV)
    return d_cli_internal_executable_argv(_argv0,
                                          _path);
#else
    (void)_argv0;

    return d_cli_internal_executable_system(_path);
#endif
}

// 1.5    Modules
//------------------------------------------------------------------------------

#if (D_ENV_CLI_MODULES == D_ENV_CLI_MODULES_DLOPEN)

/**
 * @brief Opens a module with dlopen: RTLD_NOW, so a module missing a symbol
 *        is refused here rather than when a call reaches it, and
 *        RTLD_LOCAL, so its symbols resolve nothing loaded after it.
 *
 * @param[in]     _path    the module's file.
 * @param[out]    _module  receives the module.
 * @param[in,out] _why     dlerror's words, on failure; may be NULL.
 * @return D_ARG_OK, or D_ARG_ERROR_FILE.
 */
static int
d_cli_internal_open_system(
    const char*          _path,
    void**               _module,
    struct d_arg_buffer* _why
)
{
    *_module = dlopen(_path,
                      RTLD_NOW | RTLD_LOCAL);

    // dlerror's words say why
    if ( (*_module == NULL) &&
         (_why != NULL) )
    {
        const char* const words = dlerror();

        (void)d_arg_buffer_append_cstr(_why,
                                       (words != NULL)
                                       ? words
                                       : "it could not be loaded");
    }

    return (*_module != NULL) ? D_ARG_OK : D_ARG_ERROR_FILE;
}

#elif (D_ENV_CLI_MODULES == D_ENV_CLI_MODULES_WINDOWS)

/**
 * @brief Opens a module with LoadLibraryExW, LOAD_WITH_ALTERED_SEARCH_PATH,
 *        so the libraries it needs are looked for beside it first; its name
 *        widened, which is the one place opening it asks for memory.
 *
 * @param[in]     _path    the module's file, in UTF-8.
 * @param[out]    _module  receives the module.
 * @param[in,out] _why     why not, on failure; may be NULL.
 * @return D_ARG_OK; D_ARG_ERROR_FILE; or D_ARG_ERROR_OUT_OF_MEMORY when
 *         there was no room for the name.
 */
static int
d_cli_internal_open_system(
    const char*          _path,
    void**               _module,
    struct d_arg_buffer* _why
)
{
    wchar_t* const path  = d_cli_internal_widen(_path,
                                                (size_t)-1,
                                                NULL);
    const int      wrong = errno;

    // no room for the name the system takes
    if ( (!path) &&
         (wrong == ENOMEM) )
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const bool    named  = (path != NULL);
    const HMODULE module = (named)
                           ? LoadLibraryExW(path,
                                            NULL,
                                            LOAD_WITH_ALTERED_SEARCH_PATH)
                           : NULL;
    const DWORD   code   = GetLastError();

    D_INTERNAL_CLI_FREE(path);

    // a name that is not UTF-8 names no file
    if ( (!named) &&
         (_why != NULL) )
    {
        (void)d_arg_buffer_append_cstr(_why,
                                       "its name is not UTF-8");
    }
    // the system's code says why another was not loaded
    else if ( (!module) &&
              (_why != NULL) )
    {
        (void)d_arg_buffer_printf(_why,
                                  "it could not be loaded (Windows error "
                                  "%lu)",
                                  (unsigned long)code);
    }

    *_module = (void*)module;

    return (module != NULL) ? D_ARG_OK : D_ARG_ERROR_FILE;
}

#else

/**
 * @brief Where there are no modules: says so.
 *
 * @param[in]     _path    unused.
 * @param[out]    _module  unused.
 * @param[in,out] _why     why not; may be NULL.
 * @return D_ARG_ERROR_FILE.
 */
static int
d_cli_internal_open_system(
    const char*          _path,
    void**               _module,
    struct d_arg_buffer* _why
)
{
    (void)_path;
    (void)_module;

    if (_why != NULL)
    {
        (void)d_arg_buffer_append_cstr(_why,
                                       "this system has no loadable "
                                       "modules");
    }

    return D_ARG_ERROR_FILE;
}

#endif  // D_ENV_CLI_MODULES

/*
d_cli_internal_module_open
  The system's own way of opening a module, as env_cli.h selects it.
*/
int
d_cli_internal_module_open(
    const char*          _path,
    void**               _module,
    struct d_arg_buffer* _why
)
{
    *_module = NULL;

    return d_cli_internal_open_system(_path,
                                      _module,
                                      _why);
}

/*
d_cli_internal_module_entry
  dlsym gives an object pointer and GetProcAddress a FARPROC; either is
copied into the function pointer it is, which POSIX guarantees can hold it
and ISO C forbids casting to.
*/
d_cli_plugin_entry_fn
d_cli_internal_module_entry(
    void* _module
)
{
    d_cli_plugin_entry_fn entry = NULL;

    // no module
    if (!_module)
    {
        return NULL;
    }

#if (D_ENV_CLI_MODULES == D_ENV_CLI_MODULES_DLOPEN)
    void* const symbol = dlsym(_module,
                               D_CLI_PLUGIN_ENTRY_NAME);

    D_STATIC_ASSERT(sizeof(symbol) == sizeof(entry),
                    "a data pointer must hold a function pointer for dlsym");

    // copied, since ISO C has no cast from one to the other
    if (symbol != NULL)
    {
        memcpy(&entry,
               &symbol,
               sizeof(entry));
    }
#elif (D_ENV_CLI_MODULES == D_ENV_CLI_MODULES_WINDOWS)
    const FARPROC symbol = GetProcAddress((HMODULE)_module,
                                          D_CLI_PLUGIN_ENTRY_NAME);

    D_STATIC_ASSERT(sizeof(symbol) == sizeof(entry),
                    "FARPROC and the entry point must be one size");

    if (symbol != NULL)
    {
        memcpy(&entry,
               &symbol,
               sizeof(entry));
    }
#endif

    return entry;
}

/*
d_cli_internal_module_close
  dlclose or FreeLibrary.
*/
void
d_cli_internal_module_close(
    void* _module
)
{
    // nothing open
    if (!_module)
    {
        return;
    }

#if (D_ENV_CLI_MODULES == D_ENV_CLI_MODULES_DLOPEN)
    (void)dlclose(_module);
#elif (D_ENV_CLI_MODULES == D_ENV_CLI_MODULES_WINDOWS)
    (void)FreeLibrary((HMODULE)_module);
#endif

    return;
}

// 1.6    Errors and files
//------------------------------------------------------------------------------

/*
d_cli_internal_error_text
  strerror: a program reporting an error is not printing from two threads.
*/
const char*
d_cli_internal_error_text(
    int _errnum
)
{
    const char* const text = strerror(_errnum);

    return (text != NULL) ? text : "unknown error";
}

/*
d_cli_internal_mode_kind
  S_ISREG and S_ISDIR, or the runtime's _S_IFMT bits on Windows.
*/
int
d_cli_internal_mode_kind(
    uint32_t _mode
)
{
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    const uint32_t kind = _mode & (uint32_t)_S_IFMT;

    return (kind == (uint32_t)_S_IFREG) ? 1
         : (kind == (uint32_t)_S_IFDIR) ? 2
                                        : 0;
#else
    const mode_t mode = (mode_t)_mode;

    return (S_ISREG(mode)) ? 1
         : (S_ISDIR(mode)) ? 2
                           : 0;
#endif
}
