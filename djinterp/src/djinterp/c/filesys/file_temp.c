/*******************************************************************************
* djinterp [c]                                                       file_temp.c
*
* Implementation of the temporary-file calls declared in file_temp.h.
*   Anonymous files come from tmpfile, where the C library has it -- WASI's does
* not. Named ones are made here on every platform: a name of this module's
* choosing, created exclusively through d_file_open, and retried when another
* process holds that name already. mkstemp would do the same, but its descriptor
* is not close-on-exec, its mode could only be set by moving the umask every
* thread shares, and the Windows CRT has none -- _mktemp_s runs out after 26
* names per template. The search takes its mode from its caller, so
* d_file_write_all makes its replacement with it too
* (d_internal_file_temp_create).
* d_file_temp_name, the racy form, is compiled only when the build allows it.
*
*
* path:      /src/djinterp/c/filesys/file_temp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_temp.h"  // corresponding header
// std
#include <errno.h>   // errno, EINVAL, EEXIST, ERANGE, EIO, ENOSYS
#include <stdio.h>   // FILE, tmpfile
#include <stdlib.h>  // getenv
#include <string.h>  // memcpy, memset, strlen
#include <time.h>    // clock, time
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/filesys/file_desc.h"    // d_file_open
#include "../../../../inc/djinterp/c/filesys/file_dir.h"     // d_dir_entry_path
#include "../../../../inc/djinterp/c/filesys/file_path.h"    // d_path_root_length
#include "../../../../inc/djinterp/c/filesys/file_stat.h"    // d_file_exists
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_temp.h"  // D_INTERNAL_FILE_TEMP_*
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint64_t, uintptr_t,
                                                     // UINT64_C


// the large-file interface this source needs (file_common.h, 6.4.1)
D_INTERNAL_FILE_ASSERT_LARGE_FILES;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// the names are needed where a temporary is created, or a racy name made
#if ( (!D_FILE_BACKEND_IS_STDC) ||                                             \
      (D_INTERNAL_FILE_TEMP_TMPNAM == 1) )

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_temp_entropy
  What the platform adds to a name's seed: the process, the thread, and the
high-resolution counter, where the machine has one.
*/
static uint64_t
d_internal_temp_entropy(
    void
)
{
    LARGE_INTEGER ticks;
    uint64_t      bits = ((uint64_t)GetCurrentProcessId() << 32) ^
                         (uint64_t)GetCurrentThreadId();

    // the counter is optional hardware
    if (QueryPerformanceCounter(&ticks))
    {
        bits ^= (uint64_t)ticks.QuadPart;
    }

    return bits;
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)

/*
d_internal_temp_entropy
  What the platform adds to a name's seed: the process.
*/
static uint64_t
d_internal_temp_entropy(
    void
)
{
    return ((uint64_t)getpid() << 32);
}

#else

/*
d_internal_temp_entropy
  What the platform adds to a name's seed: nothing, on ISO C alone.
*/
static uint64_t
d_internal_temp_entropy(
    void
)
{
    return 0;
}

#endif  // D_CFG_FILE_HAS_WIN32

#if D_CFG_IS_ON(D_CFG_FILE_HAS_CLOCK)

/*
d_internal_temp_clock
  What the processor clock adds to a name's seed.
*/
static uint64_t
d_internal_temp_clock(
    void
)
{
    return (uint64_t)clock();
}

#else

/*
d_internal_temp_clock
  What the processor clock adds to a name's seed: nothing, where there is no
processor clock to read (cfg_file_common.h, 1.9.19).
*/
static uint64_t
d_internal_temp_clock(
    void
)
{
    return 0;
}

#endif  // D_CFG_FILE_HAS_CLOCK

/*
d_internal_temp_seed
  Where a run of names starts: the time, the processor clock
(d_internal_temp_clock), the platform's own values (d_internal_temp_entropy),
and the address of a local, which differs on every thread's stack and is
moved by address randomization. Nothing here is secret, and nothing needs to
be: the exclusive create is what makes a temporary safe, and the seed only
makes two callers unlikely to choose the same names.
*/
static uint64_t
d_internal_temp_seed(
    void
)
{
    uint64_t seed = (uint64_t)time(NULL);

    seed ^= (d_internal_temp_clock() << 16) ^
            d_internal_temp_entropy() ^
            (uint64_t)(uintptr_t)&seed;

    return seed;
}

/*
d_internal_temp_fill
  Writes the next name's D_INTERNAL_FILE_TEMP_SUFFIX_LEN characters at
_suffix, from a SplitMix64 step on _state. They are digits and lower-case
letters only, so a filesystem that folds case cannot fold two names into one:
36^6 names, from one 64-bit step.
*/
static void
d_internal_temp_fill(
    char*     _suffix,
    uint64_t* _state
)
{
    static const char alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    *_state += UINT64_C(0x9E3779B97F4A7C15);

    uint64_t bits = *_state;

    bits = (bits ^ (bits >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    bits = (bits ^ (bits >> 27)) * UINT64_C(0x94D049BB133111EB);
    bits =  bits ^ (bits >> 31);

    // one character per base-36 digit
    for (size_t idx = 0; idx < D_INTERNAL_FILE_TEMP_SUFFIX_LEN; ++idx)
    {
        _suffix[idx] = alphabet[bits % 36];
        bits        /= 36;
    }

    return;
}

#endif  // !D_FILE_BACKEND_IS_STDC || D_INTERNAL_FILE_TEMP_TMPNAM

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_temp_suffix
  Checks a template here rather than leaving it to the platform -- an
implementation handed a bad one may fail with EINVAL or may quietly do
something else, and the caller cannot tell which -- and finds the
placeholder the names are written into. The result is NULL, reported, for a
template that is not one.
*/
static char*
d_internal_temp_suffix(
    char* _template
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_template != NULL,
                            EINVAL,
                            "d_file_temp_create",
                            NULL,
                            "template is NULL",
                            NULL);

    const size_t length = strlen(_template);

    // the template must have room for the placeholder
    if (length < (size_t)D_INTERNAL_FILE_TEMP_SUFFIX_LEN)
    {
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_file_temp_create",
                             _template,
                             "template is shorter than the required XXXXXX",
                             NULL);
    }

    char* const suffix = _template + length - D_INTERNAL_FILE_TEMP_SUFFIX_LEN;

    // the placeholder must consist entirely of 'X' characters
    for (size_t idx = 0; idx < D_INTERNAL_FILE_TEMP_SUFFIX_LEN; ++idx)
    {
        // anything else is not a template
        if (suffix[idx] != 'X')
        {
            D_INTERNAL_FILE_FAIL(EINVAL,
                                 "d_file_temp_create",
                                 _template,
                                 "template must end in exactly six 'X' "
                                 "characters",
                                 NULL);
        }
    }

    return suffix;
}

#endif  // !D_FILE_BACKEND_IS_STDC

#if D_CFG_IS_ON(D_CFG_FILE_HAS_TMPFILE)

/*
d_internal_temp_anonymous
  tmpfile; errno is the platform's.
*/
static FILE*
d_internal_temp_anonymous(
    void
)
{
    return tmpfile();
}

#else

/*
d_internal_temp_anonymous
  No tmpfile here (cfg_file_common.h, 1.9.18): ENOSYS.
*/
static FILE*
d_internal_temp_anonymous(
    void
)
{
    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return NULL;
}

#endif  // D_CFG_FILE_HAS_TMPFILE

/*
d_internal_temp_root_len
  The part of a temporary directory's path that keeps its separators: on
Windows file_path's root, since cfg_file_path insists on Windows syntax
there, so "C:\\" keeps its separator and does not become "C:", drive C's
current directory; on POSIX the leading slash.
*/
static size_t
d_internal_temp_root_len(
    const char* _dir
)
{
    // Windows' roots are file_path's; POSIX has only the one
    return D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
               ? d_path_root_length(_dir)
               : (size_t)(_dir[0] == '/');
}

/*
d_internal_temp_trim
  Drops a temporary directory's trailing separators -- the kernel's, both of
them on Windows -- but never into its root, and never to nothing, and reports
the length left.
*/
static size_t
d_internal_temp_trim(
    char*  _dir,
    size_t _length
)
{
    const size_t root = d_internal_temp_root_len(_dir);

    // the separators after the last component
    while ( (_length > root) &&
            (_length > 1)    &&
            ( (_dir[_length - 1] == D_FILE_PATH_SEP) ||
              (_dir[_length - 1] == D_FILE_PATH_SEP_ALT) ) )
    {
        _dir[--_length] = '\0';
    }

    return _length;
}

//==============================================================================
// 1.  TEMPORARY FILES
//==============================================================================

/*
d_file_temp_stream
  tmpfile's file is anonymous: there is no name for an attacker to race and no
cleanup for anyone to forget, which is why this is the preferred form. The
wrapper adds only the failure notice, and ENOSYS where there is no tmpfile
(d_internal_temp_anonymous).
*/
FILE*
d_file_temp_stream(
    void
)
{
    FILE* const result = d_internal_temp_anonymous();

    // report the failure; errno is the platform's
    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_temp_stream",
                               NULL,
                               "tmpfile failed");
    }

    return result;
}

/*
d_file_temp_stream_s
  The out-parameter receives d_file_temp_stream's result directly, so on
failure it holds NULL rather than an indeterminate pointer. A failure that
left errno clear is reported as EIO rather than as success.
*/
int
d_file_temp_stream_s(
    FILE** _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_temp_stream_s",
                            NULL,
                            "stream out-parameter is NULL",
                            EINVAL);

    *_stream = d_file_temp_stream();

    // translate a failure into a code the caller can return
    if (!*_stream)
    {
        return errno ? errno : EIO;
    }

    return 0;
}

/*
d_file_temp_create
  The search is d_internal_file_temp_create's, which d_file_write_all shares;
this call gives it the mode this build makes temporaries with.
*/
int
d_file_temp_create(
    char* _template
)
{
    return d_internal_file_temp_create(_template,
                                       D_INTERNAL_FILE_TEMP_MODE);
}

#if (D_INTERNAL_FILE_TEMP_TMPNAM == 1)

/*
d_file_temp_name
  "Does not currently exist" is a statement about the past by the time this
returns, and nothing inside it can change that: the flaw is the interface,
which separates naming from opening. Names are d_file_temp_create's, in the
temporary directory (d_dir_entry_path), and each is tested for existence --
the answer already stale -- for a bounded number of tries. No state is shared
between calls, so concurrent callers race only for names, never in memory.
*/
static int
d_internal_file_temp_name_in(
    char*  _s,
    size_t _maxsize,
    char*  _dir_room
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_s != NULL,
                            EINVAL,
                            "d_file_temp_name",
                            NULL,
                            "buffer is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_maxsize > 1,
                            EINVAL,
                            "d_file_temp_name",
                            NULL,
                            "buffer is too small to hold a name",
                            EINVAL);

    char* const dir = _dir_room;

    // the name lives in the temporary directory
    if (!d_dir_temp(dir,
                    D_FILE_PATH_MAX))
    {
        return EIO;
    }

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                           0,
                           "d_file_temp_name",
                           NULL,
                           "generated name is racy by construction; prefer "
                           "d_file_temp_create");

    char        name[] = "djtmp_XXXXXX";
    char* const suffix = name + (sizeof(name) - 1) -
                         D_INTERNAL_FILE_TEMP_SUFFIX_LEN;
    uint64_t    state  = d_internal_temp_seed();

    // a bounded number of names, each tested once
    for (int tries = 0; tries < D_INTERNAL_FILE_TEMP_TRIES; ++tries)
    {
        d_internal_temp_fill(suffix,
                             &state);

        // a name that does not fit is not the name that was generated
        if (!d_dir_entry_path(_s,
                              _maxsize,
                              dir,
                              name))
        {
            return ERANGE;
        }

        // the check that is already stale when it returns
        if (!d_file_exists(_s))
        {
            return 0;
        }
    }

    return EEXIST;
}

/*
d_file_temp_name
  d_file_temp_name's work is d_internal_file_temp_name_in; this keeps the room
it builds a path in, on the heap where that is 32 KiB (d_internal_file_path).
*/
int
d_file_temp_name(
    char*  _s,
    size_t _maxsize
)
{
    struct d_internal_file_path dir_held = {0};
    char* const dir = d_internal_file_path_room(&dir_held);

    // d_internal_file_path_room has set errno, and reported the failure
    if (!dir)
    {
        d_internal_file_path_release(&dir_held);

        return ENOMEM;
    }

    const int result = d_internal_file_temp_name_in(_s,
                                                    _maxsize,
                                                    dir);

    d_internal_file_path_release(&dir_held);

    return result;
}

#endif  // D_INTERNAL_FILE_TEMP_TMPNAM

#if (D_INTERNAL_FILE_WIDE == 1)

/**
 * @brief d_dir_temp's search where the build hands Windows UTF-16: the
 *        environment and GetTempPathW answer in UTF-16, and the answer is
 *        given back as UTF-8.
 *
 * @param[out] _buf      receives the directory.
 * @param[in]  _bufsize  capacity of `_buf`, in bytes; more than one.
 * @return `_buf`, or `NULL` with errno set: ERANGE when the directory does
 *         not fit, EILSEQ for one UTF-8 cannot carry, EIO when GetTempPathW
 *         fails.
 *
 * Read through getenv, the variables would arrive in the code page, which a
 * user's own name -- the usual temp directory sits in the profile -- need not
 * fit, and which this build no longer reads as text.
 */
static char*
d_internal_temp_dir_find(
    char*  _buf,
    size_t _bufsize
)
{
    const wchar_t* found = NULL;

#if D_CFG_IS_ON(D_CFG_FILE_TEMP_HONOUR_ENV)
    static const wchar_t* const names[] = { L"TMPDIR", L"TMP", L"TEMP" };
    const size_t                count   = sizeof(names) / sizeof(names[0]);

    // TMPDIR is the POSIX spelling; TMP and TEMP are what Windows sets
    for (size_t i = 0; ( (!found) && (i < count) ); ++i)
    {
        const wchar_t* const value = _wgetenv(names[i]);

        // an empty variable says nothing
        if ( (value) &&
             (value[0] != L'\0') )
        {
            found = value;
        }
    }
#endif  // D_CFG_FILE_TEMP_HONOUR_ENV

    // GetTempPathW's answer runs to MAX_PATH + 1 characters, which the room
    // has (D_INTERNAL_FILE_NATIVE_ROOM)
    struct d_internal_file_native held = {0};

    // with nothing from the environment, ask the platform
    if (!found)
    {
        d_internal_file_nchar_t* const asked =
            d_internal_file_native_room(&held);

        // d_internal_file_alloc has reported the failure
        if (!asked)
        {
            return NULL;
        }

        const DWORD size = (DWORD)D_INTERNAL_FILE_NATIVE_ROOM;
        const DWORD n    = GetTempPathW(size,
                                        asked);

        // zero is a failure of the call itself
        if (n == 0)
        {
            d_internal_file_native_release(&held);
            D_INTERNAL_FILE_FAIL(EIO,
                                 "d_dir_temp",
                                 NULL,
                                 "GetTempPath failed",
                                 NULL);
        }

        // a value past the buffer is the size it needed
        if (n >= size)
        {
            d_internal_file_native_release(&held);
            D_INTERNAL_FILE_FAIL(ERANGE,
                                 "d_dir_temp",
                                 NULL,
                                 "temp directory does not fit the buffer",
                                 NULL);
        }

        found = asked;
    }

    // UTF-8 for the caller; an answer that does not fit, or that UTF-8 cannot
    // carry, has set errno
    const int given = d_internal_file_from_native(found,
                                                  _buf,
                                                  _bufsize);

    d_internal_file_native_release(&held);

    if (given != 0)
    {
        const int code = errno;

        D_INTERNAL_FILE_FAIL(code,
                             "d_dir_temp",
                             NULL,
                             "temp directory cannot be given as UTF-8",
                             NULL);
    }

    // no trailing separator, but the root's
    (void)d_internal_temp_trim(_buf,
                               strlen(_buf));

    return _buf;
}

#else

/**
 * @brief d_dir_temp's search where a path is handed on as it is: the
 *        environment, then GetTempPathA on Windows and the configured
 *        fallback elsewhere.
 *
 * @param[out] _buf      receives the directory.
 * @param[in]  _bufsize  capacity of `_buf`, in bytes; more than one.
 * @return `_buf`, or `NULL` with errno set: ERANGE when the directory does
 *         not fit, EIO when GetTempPathA fails.
 */
static char*
d_internal_temp_dir_find(
    char*  _buf,
    size_t _bufsize
)
{
    const char* found = NULL;

#if D_CFG_IS_ON(D_CFG_FILE_TEMP_HONOUR_ENV)
    // TMPDIR is the POSIX spelling; TMP and TEMP are what Windows sets
    found = getenv("TMPDIR");

    // fall back through the Windows spellings
    if ( (!found) ||
         (found[0] == '\0') )
    {
        found = getenv("TMP");
    }

    if ( (!found) ||
         (found[0] == '\0') )
    {
        found = getenv("TEMP");
    }
#endif

    // with nothing from the environment, ask the platform
    if ( (!found) ||
         (found[0] == '\0') )
    {
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
        // ask the API rather than guess: there is no /tmp here, and the real
        // answer is per-user; a buffer past a DWORD is offered as MAXDWORD
        const DWORD size = (_bufsize > (size_t)MAXDWORD) ? MAXDWORD
                                                         : (DWORD)_bufsize;
        const DWORD n    = GetTempPathA(size,
                                        _buf);

        // zero is a failure of the call itself
        if (n == 0)
        {
            D_INTERNAL_FILE_FAIL(EIO,
                                 "d_dir_temp",
                                 NULL,
                                 "GetTempPath failed",
                                 NULL);
        }

        // a value past the buffer is the size it needed
        if (n >= size)
        {
            D_INTERNAL_FILE_FAIL(ERANGE,
                                 "d_dir_temp",
                                 NULL,
                                 "temp directory does not fit the buffer",
                                 NULL);
        }

        // GetTempPath appends a separator; drop it, as from every answer
        (void)d_internal_temp_trim(_buf,
                                   (size_t)n);

        return _buf;
#else
        found = D_CFG_FILE_TEMP_DIR_FALLBACK;
#endif
    }

    const size_t length = strlen(found);

    // the directory and its terminator must fit
    if ((length + 1) > _bufsize)
    {
        D_INTERNAL_FILE_FAIL(ERANGE,
                             "d_dir_temp",
                             NULL,
                             "temp directory does not fit the buffer",
                             NULL);
    }

    memcpy(_buf,
           found,
           length + 1);

    // no trailing separator, but the root's
    (void)d_internal_temp_trim(_buf,
                               length);

    return _buf;
}

#endif  // D_INTERNAL_FILE_WIDE == 1

/*
d_dir_temp
  TMPDIR is the POSIX spelling; TMP and TEMP are what Windows sets. With the
environment ignored or silent, Windows is asked through GetTempPath -- there
is no /tmp there, and the real answer is per-user -- and elsewhere the
configured fallback is used. Every answer loses its trailing separators but
never its root's (d_internal_temp_trim), so a name joins it with
d_dir_entry_path and exactly one separator. Where the build hands Windows
UTF-16, every answer is asked for in UTF-16 and given back as UTF-8
(d_internal_temp_dir_find).
*/
char*
d_dir_temp(
    char*  _buf,
    size_t _bufsize
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_dir_temp",
                            NULL,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 1,
                            EINVAL,
                            "d_dir_temp",
                            NULL,
                            "buffer is too small",
                            NULL);

    return d_internal_temp_dir_find(_buf,
                                    _bufsize);
}

//==============================================================================
// 2.  INTERNAL SUPPORT
//==============================================================================

#if D_FILE_BACKEND_IS_STDC

/*
d_internal_file_temp_create
  The ISO C backend has no descriptors to return, so this only reports ENOSYS.
*/
int
d_internal_file_temp_create(
    char* _template,
    int   _mode
)
{
    (void)_template;
    (void)_mode;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_temp_create",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_internal_file_temp_create
  Names are tried in the template's placeholder (d_internal_temp_fill), each
created exclusively through d_file_open -- so the descriptor gets this build's
close-on-exec and binary-mode policy like every other, atomically, and the
mode is _mode under the umask, which is never moved. A name another process
holds already is passed over; any other failure ends the search. A template
that found no file is put back as it was.
*/
int
d_internal_file_temp_create(
    char* _template,
    int   _mode
)
{
    char* const suffix = d_internal_temp_suffix(_template);

    // d_internal_temp_suffix has already reported a template that is not one
    if (!suffix)
    {
        return -1;
    }

    uint64_t state = d_internal_temp_seed();

    // a bounded number of names, each created exclusively
    for (int tries = 0; tries < D_INTERNAL_FILE_TEMP_TRIES; ++tries)
    {
        d_internal_temp_fill(suffix,
                             &state);

        const int fd = d_file_open(_template,
                                   O_RDWR | O_CREAT | O_EXCL,
                                   _mode);

        // made: the template now holds the file's name
        if (fd >= 0)
        {
            return fd;
        }

        // a name already taken is passed over; anything else is a failure,
        // which d_file_open has reported
        if (errno != EEXIST)
        {
            const int saved_errno = errno;

            memset(suffix,
                   'X',
                   D_INTERNAL_FILE_TEMP_SUFFIX_LEN);
            errno = saved_errno;

            return -1;
        }
    }

    memset(suffix,
           'X',
           D_INTERNAL_FILE_TEMP_SUFFIX_LEN);
    D_INTERNAL_FILE_FAIL(EEXIST,
                         "d_file_temp_create",
                         _template,
                         "exhausted attempts to find a free temporary name",
                         -1);
}

#endif  // D_FILE_BACKEND_IS_STDC
