/*******************************************************************************
* djinterp [c]                                                       file_temp.c
*
* Implementation of the temporary-file calls declared in file_temp.h.
*   Anonymous files come from tmpfile. Named ones come from mkstemp on POSIX,
* under a pinned umask, and on Windows from _mktemp_s plus an exclusive create,
* retried when another process wins the race for a name. d_file_temp_name, the
* racy form, is compiled only when the build allows it.
*
*
* path:      /src/djinterp/c/fs/file_temp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_temp.h"  // corresponding header
// std
#include <errno.h>   // errno, EINVAL, EEXIST, ERANGE, ENAMETOOLONG, EIO
#include <stdio.h>   // FILE, tmpfile, snprintf
#include <stdlib.h>  // getenv, mkstemp
#include <string.h>  // memcpy, strlen
#include <time.h>    // clock
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/fs/file_desc.h"    // d_file_open
#include "../../../../inc/djinterp/c/fs/file_stat.h"    // d_file_exists
#include "../../../../inc/djinterp/config/c/fs/cfg_file_temp.h"  // D_INTERNAL_FILE_TEMP_*


//==============================================================================
// 1.  TEMPORARY FILES
//==============================================================================

/*
d_file_temp_stream
  tmpfile's file is anonymous: there is no name for an attacker to race and no
cleanup for anyone to forget, which is why this is the preferred form. The
wrapper adds only the failure notice.
*/
FILE*
d_file_temp_stream(
    void
)
{
    FILE* const result = tmpfile();

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

#if D_FILE_BACKEND_IS_STDC

/*
d_file_temp_create
  The ISO C backend has no descriptors to return, so this only reports ENOSYS.
*/
int
d_file_temp_create(
    char* _template
)
{
    (void)_template;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_temp_create",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_temp_create
  The template is checked here rather than left to the platform: an
implementation handed a bad template may fail with EINVAL or may quietly do
something else, and the caller cannot tell which.
  POSIX has mkstemp, run under a pinned umask so the file's mode is this
build's decision on every libc. The CRT has no mkstemp, and _mktemp_s names
without opening, so on Windows the race is closed by hand: O_EXCL makes the
create atomic, and a name lost to another process is retried rather than
reported.
*/
int
d_file_temp_create(
    char* _template
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_template != NULL,
                            EINVAL,
                            "d_file_temp_create",
                            NULL,
                            "template is NULL",
                            -1);

    const size_t length = strlen(_template);

    // check the template here rather than letting the platform do it: an
    // implementation handed a bad template may fail with EINVAL, or may
    // quietly do something else, and the caller cannot tell which
    if (length < (size_t)D_INTERNAL_FILE_TEMP_SUFFIX_LEN)
    {
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_file_temp_create",
                             _template,
                             "template is shorter than the required XXXXXX",
                             -1);
    }

    // the suffix must consist entirely of placeholder characters
    for (size_t idx = length - D_INTERNAL_FILE_TEMP_SUFFIX_LEN;
         idx < length;
         ++idx)
    {
        if (_template[idx] != 'X')
        {
            D_INTERNAL_FILE_FAIL(EINVAL,
                                 "d_file_temp_create",
                                 _template,
                                 "template must end in exactly six 'X' "
                                 "characters",
                                 -1);
        }
    }

    int result = -1;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // the CRT has no mkstemp. _mktemp_s names but does not open, so the race
    // has to be closed by hand: O_EXCL makes the create atomic, and a name
    // that lost the race is retried rather than reported.
    {
        char attempt[D_FILE_PATH_MAX];

        // try a bounded number of names before giving up
        for (int tries = 0; tries < 128; ++tries)
        {
            // the working copy must hold the template and its terminator
            if (length >= sizeof(attempt))
            {
                D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                                     "d_file_temp_create",
                                     _template,
                                     "template is longer than D_FILE_PATH_MAX",
                                     -1);
            }

            memcpy(attempt,
                   _template,
                   length + 1);

            // name a candidate from the template
            if (_mktemp_s(attempt,
                          length + 1) != 0)
            {
                D_INTERNAL_FILE_FAIL(EEXIST,
                                     "d_file_temp_create",
                                     _template,
                                     "no unique name available",
                                     -1);
            }

            result = d_file_open(attempt,
                                 O_RDWR | O_CREAT | O_EXCL,
                                 D_INTERNAL_FILE_TEMP_MODE);

            // report the chosen name back through the template
            if (result >= 0)
            {
                memcpy(_template,
                       attempt,
                       length + 1);

                return result;
            }

            // somebody took the name between naming and opening -- which is
            // exactly the race d_file_temp_name cannot escape. Try another.
            if (errno != EEXIST)
            {
                return -1;
            }
        }

        D_INTERNAL_FILE_FAIL(EEXIST,
                             "d_file_temp_create",
                             _template,
                             "exhausted attempts to find a free temporary name",
                             -1);
    }
#else
    {
        // mkstemp is specified to create with 0600 -- but only since POSIX
        // 2008, and older implementations used 0666 & ~umask. Pinning the
        // umask around the call makes the mode this build's decision on every
        // libc rather than a question of vintage.
        const mode_t saved_mask =
            umask((mode_t)(0777 & ~D_INTERNAL_FILE_TEMP_MODE));

        result = mkstemp(_template);
        (void)umask(saved_mask);
    }
#endif

    // report the failure; errno is the platform's
    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_temp_create",
                               NULL,
                               "mkstemp failed");
    }

    return result;
}

#endif  // D_FILE_BACKEND_IS_STDC

#if (D_INTERNAL_FILE_TEMP_TMPNAM == 1)

/*
d_file_temp_name
  "Does not currently exist" is a statement about the past by the time this
returns, and nothing inside it can change that: the flaw is the interface,
which separates naming from opening. Names join the temporary directory, the
clock and a process-wide counter -- not a security measure, only a way to
reduce accidental collisions between concurrent callers -- and each is tested
for existence, with the answer already stale, for a bounded number of tries.
*/
int
d_file_temp_name(
    char*  _s,
    size_t _maxsize
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

    char dir[D_FILE_PATH_MAX];

    // the name lives in the temporary directory
    if (!d_dir_temp(dir,
                    sizeof(dir)))
    {
        return EIO;
    }

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                           0,
                           "d_file_temp_name",
                           NULL,
                           "generated name is racy by construction; prefer "
                           "d_file_temp_create");

    // shared by every call, so successive names differ
    static int counter = 0;

    // try a bounded number of names before giving up
    for (int tries = 0; tries < 128; ++tries)
    {
        // not a security measure -- nothing here can be one. It only reduces
        // accidental collisions between concurrent callers.
        const int written = snprintf(_s,
                                     _maxsize,
                                     "%s%cdjtmp_%lu_%d",
                                     dir,
                                     D_FILE_PATH_SEP,
                                     (unsigned long)clock(),
                                     counter++);

        // an encoding failure leaves nothing usable
        if (written < 0)
        {
            return EIO;
        }

        // a truncated name is not the name that was generated
        if ((size_t)written >= _maxsize)
        {
            D_INTERNAL_FILE_SET_ERR(ERANGE);

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

#endif  // D_INTERNAL_FILE_TEMP_TMPNAM

/*
d_dir_temp
  TMPDIR is the POSIX spelling; TMP and TEMP are what Windows sets. With the
environment ignored or silent, Windows is asked through GetTempPath -- there
is no /tmp there, and the real answer is per-user -- and elsewhere the
configured fallback is used. Every answer loses its trailing separators, so
d_path_join needs no special case.
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
        // answer is per-user
        DWORD n = GetTempPathA((DWORD)_bufsize,
                               _buf);

        // zero is failure; a value past the buffer is the size it needed
        if ( (n == 0) ||
             (n >= (DWORD)_bufsize) )
        {
            D_INTERNAL_FILE_FAIL(ERANGE,
                                 "d_dir_temp",
                                 NULL,
                                 "temp directory does not fit the buffer",
                                 NULL);
        }

        // GetTempPath appends a separator; strip it so the result is a
        // directory name like every other path this subframework returns
        while ( (n > 1) &&
                ( (_buf[n - 1] == '\\') ||
                  (_buf[n - 1] == '/') ) )
        {
            _buf[--n] = '\0';
        }

        return _buf;
#else
        found = D_CFG_FILE_TEMP_DIR_FALLBACK;
#endif
    }

    size_t length = strlen(found);

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

    // no trailing separator, so d_path_join needs no special case
    while ( (length > 1) &&
            ( (_buf[length - 1] == '/') ||
              (_buf[length - 1] == D_FILE_PATH_SEP) ) )
    {
        _buf[--length] = '\0';
    }

    return _buf;
}
