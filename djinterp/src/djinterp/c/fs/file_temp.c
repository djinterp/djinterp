#include <time.h>
#include "../../../../inc/djinterp/c/fs/file_temp.h"
#include "../../../../inc/djinterp/c/fs/file_desc.h"
#include "../../../../inc/djinterp/c/fs/file_stat.h"


///////////////////////////////////////////////////////////////////////////////
///             I.  ATOMIC CREATION                                         ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tmpfile
  Creates an anonymous temporary file, removed when it is closed or the
process exits.
  Anonymous is the safety property: there is no name for an attacker to race,
and no cleanup for you to forget. Prefer it whenever the file does not need to
be handed to another process by path.

Parameter(s):
  none.
Return:
  An open read/write stream, or NULL on failure with errno set.
*/
FILE*
d_tmpfile
(
    void
)
{
    FILE* result;

    result = tmpfile();

    if (!result)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_tmpfile",
                               NULL,
                               "tmpfile failed");
    }

    return result;
}


/*
d_tmpfile_s
  Creates an anonymous temporary file, reporting through the return value
(C11 Annex K shape).

Parameter(s):
  _stream: receives the stream; must not be NULL. Cleared before anything else
           happens, so a caller who ignores the return code holds NULL rather
           than an indeterminate pointer.
Return:
  0 on success, or a non-zero error code -- EINVAL for a NULL out-parameter.
*/
int
d_tmpfile_s
(
    FILE** _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_tmpfile_s",
                            NULL,
                            "stream out-parameter is NULL",
                            EINVAL);

    *_stream = d_tmpfile();

    if (!*_stream)
    {
        return errno ? errno : EIO;
    }

    return 0;
}


/*
d_mkstemp
  Creates and opens a uniquely-named temporary file from a template.
  The safe named form: it chooses the name and opens it in ONE operation, so
there is no instant at which the name exists and the file does not. That is
the whole difference from d_tmpnam_s, and it is why this should be the only
one you use.
  The template is modified in place: its trailing "XXXXXX" is replaced with
the chosen suffix, so the caller learns the name. It must therefore be
writable -- a string literal will crash.
  Created with D_CFG_FILE_TEMP_MODE (0600). The file is NOT removed for you.

Parameter(s):
  _template: a writable path ending in exactly six 'X' characters.
Return:
  An open descriptor on success, or -1 on failure with errno set.
*/
int
d_mkstemp
(
    char* _template
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_template;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_mkstemp",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    size_t length;
    size_t idx;
    int    result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_template != NULL,
                            EINVAL,
                            "d_mkstemp",
                            NULL,
                            "template is NULL",
                            -1);

    length = strlen(_template);

    // check the template here rather than letting the platform do it: an
    // implementation handed a bad template may fail with EINVAL, or may
    // quietly do something else, and the caller cannot tell which
    if (length < (size_t)D_INTERNAL_FILE_TEMP_SUFFIX_LEN)
    {
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_mkstemp",
                             _template,
                             "template is shorter than the required XXXXXX",
                             -1);
    }

    for (idx = length - D_INTERNAL_FILE_TEMP_SUFFIX_LEN; idx < length; ++idx)
    {
        if (_template[idx] != 'X')
        {
            D_INTERNAL_FILE_FAIL(EINVAL,
                                 "d_mkstemp",
                                 _template,
                                 "template must end in exactly six 'X' characters",
                                 -1);
        }
    }

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // the CRT has no mkstemp. _mktemp_s names but does not open, so the race
    // has to be closed by hand: O_EXCL makes the create atomic, and a name
    // that lost the race is retried rather than reported.
    {
        char   attempt[D_FILE_PATH_MAX];
        int    tries;

        for (tries = 0; tries < 128; ++tries)
        {
            if (length >= sizeof(attempt))
            {
                D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                                     "d_mkstemp",
                                     _template,
                                     "template is longer than D_FILE_PATH_MAX",
                                     -1);
            }

            memcpy(attempt, _template, length + 1);

            if (_mktemp_s(attempt, length + 1) != 0)
            {
                D_INTERNAL_FILE_FAIL(EEXIST,
                                     "d_mkstemp",
                                     _template,
                                     "no unique name available",
                                     -1);
            }

            result = d_open(attempt,
                            O_RDWR | O_CREAT | O_EXCL,
                            D_INTERNAL_FILE_TEMP_MODE);

            if (result >= 0)
            {
                memcpy(_template, attempt, length + 1);

                return result;
            }

            // somebody took the name between naming and opening -- which is
            // exactly the race d_tmpnam_s cannot escape. Try another.
            if (errno != EEXIST)
            {
                return -1;
            }
        }

        D_INTERNAL_FILE_FAIL(EEXIST,
                             "d_mkstemp",
                             _template,
                             "exhausted attempts to find a free temporary name",
                             -1);
    }
    #else
    {
        mode_t saved_mask;

        // mkstemp is specified to create with 0600 -- but only since POSIX
        // 2008, and older implementations used 0666 & ~umask. Pinning the
        // umask around the call makes the mode this build's decision on every
        // libc rather than a question of vintage.
        saved_mask = umask((mode_t)(0777 & ~D_INTERNAL_FILE_TEMP_MODE));
        result = mkstemp(_template);
        (void)umask(saved_mask);
    }
    #endif

    if (result < 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_mkstemp",
                               NULL,
                               "mkstemp failed");
    }

    return result;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             II.  NAME GENERATION                                        ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_FILE_TEMP_TMPNAM == 1)

/*
d_tmpnam_s
  Generates a filename that does not currently exist.
  READ THIS. "Does not currently exist" is a statement about the past by the
time it returns. Between this call and your open, anybody with write access to
that directory can create the name -- classically as a symlink to a file you
have permission to destroy -- and your program then writes there, with your
privileges. That is not a hypothetical; it is the oldest bug in Unix
temporary-file handling and it is why tmpnam is deprecated everywhere.
  There is no way to fix it from inside this function: the flaw is the
interface, which separates naming from opening. d_mkstemp does not, which is
why it is the answer.
  Compiled only when D_CFG_FILE_TEMP_ALLOW_TMPNAM is 1, so a codebase can set
it to 0 and have the compiler prove there are no uses left.

Parameter(s):
  _s:       buffer to receive the name.
  _maxsize: size of _s, in bytes.
Return:
  0 on success, or a non-zero error code on failure.
*/
int
d_tmpnam_s
(
    char*  _s,
    size_t _maxsize
)
{
    char        dir[D_FILE_PATH_MAX];
    static int  counter = 0;
    int         tries;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_s != NULL,
                            EINVAL,
                            "d_tmpnam_s",
                            NULL,
                            "buffer is NULL",
                            EINVAL);
    D_INTERNAL_FILE_REQUIRE(_maxsize > 1,
                            EINVAL,
                            "d_tmpnam_s",
                            NULL,
                            "buffer is too small to hold a name",
                            EINVAL);

    if (!d_tempdir(dir, sizeof(dir)))
    {
        return EIO;
    }

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                           0,
                           "d_tmpnam_s",
                           NULL,
                           "generated name is racy by construction; prefer d_mkstemp");

    for (tries = 0; tries < 128; ++tries)
    {
        int written;

        // not a security measure -- nothing here can be one. It only reduces
        // accidental collisions between concurrent callers.
        written = snprintf(_s,
                           _maxsize,
                           "%s%cdjtmp_%lu_%d",
                           dir,
                           D_FILE_PATH_SEP,
                           (unsigned long)clock(),
                           counter++);

        if (written < 0)
        {
            return EIO;
        }

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


///////////////////////////////////////////////////////////////////////////////
///             III.  LOCATION                                              ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tempdir
  Reports the directory temporary files should go in.
  Consults TMPDIR, then TMP, then TEMP, then falls back to
D_CFG_FILE_TEMP_DIR_FALLBACK -- unless D_CFG_FILE_TEMP_HONOUR_ENV is 0, which
is what a set-uid program wants, since an attacker who controls the
environment otherwise controls where your files land.
  No trailing separator, so d_path_join works without special-casing.

Parameter(s):
  _buf:     buffer to receive the path.
  _bufsize: size of _buf, in bytes.
Return:
  _buf on success, or NULL on failure with errno set.
*/
char*
d_tempdir
(
    char*  _buf,
    size_t _bufsize
)
{
    const char* found;
    size_t      length;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_tempdir",
                            NULL,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 1,
                            EINVAL,
                            "d_tempdir",
                            NULL,
                            "buffer is too small",
                            NULL);

    found = NULL;

#if D_CFG_IS_ON(D_CFG_FILE_TEMP_HONOUR_ENV)
    // TMPDIR is the POSIX spelling; TMP and TEMP are what Windows sets
    found = getenv("TMPDIR");

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

    if ( (!found) ||
         (found[0] == '\0') )
    {
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
        // ask the API rather than guess: there is no /tmp here, and the real
        // answer is per-user
        {
            DWORD n;

            n = GetTempPathA((DWORD)_bufsize, _buf);

            if ( (n == 0) ||
                 (n >= (DWORD)_bufsize) )
            {
                D_INTERNAL_FILE_FAIL(ERANGE,
                                     "d_tempdir",
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
        }
#else
        found = D_CFG_FILE_TEMP_DIR_FALLBACK;
#endif
    }

    length = strlen(found);

    if ((length + 1) > _bufsize)
    {
        D_INTERNAL_FILE_FAIL(ERANGE,
                             "d_tempdir",
                             NULL,
                             "temp directory does not fit the buffer",
                             NULL);
    }

    memcpy(_buf, found, length + 1);

    // no trailing separator, so d_path_join needs no special case
    while ( (length > 1) &&
            ( (_buf[length - 1] == '/') ||
              (_buf[length - 1] == D_FILE_PATH_SEP) ) )
    {
        _buf[--length] = '\0';
    }

    return _buf;
}
