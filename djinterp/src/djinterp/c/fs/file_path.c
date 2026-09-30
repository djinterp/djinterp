/*******************************************************************************
* djinterp [c]                                                       file_path.c
*
* Implementation of the lexical path operations declared in file_path.h.
*   Four file-local helpers carry every rule: which characters separate, how
* long a root is, where a path's meaningful text ends, and how a result is
* copied out -- where a result that does not fit is an ERANGE failure, never a
* truncated path. The public functions compose them and make no system call.
*
*
* path:      /src/djinterp/c/fs/file_path.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_path.h"  // corresponding header
// std
#include <errno.h>   // EINVAL, ERANGE
#include <stddef.h>  // NULL, size_t
#include <string.h>  // memmove, strlen
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/fs/cfg_file_path.h"  // D_INTERNAL_FILE_PATH_*


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/*
d_internal_path_is_sep
  On POSIX only '/' separates -- '\\' is an ordinary byte in a filename there,
and treating it as a separator would silently corrupt legitimate names. The
backslash separates only when this build parses Windows paths.
*/
static int
d_internal_path_is_sep(
    char _c
)
{
    // the forward slash separates on every target
    if (_c == '/')
    {
        return 1;
    }

#if (D_INTERNAL_FILE_PATH_ALT_SEP == 1)
    // the backslash only where Windows syntax is understood
    if (_c == '\\')
    {
        return 1;
    }
#endif

    return 0;
}

/*
d_internal_path_copy
  One place for the truncation decision: a path that does not fit is a
failure, not a shortened path. Silently handing back a prefix of a path is how
a program deletes the wrong directory. The source need not be NUL-terminated;
the copy always is.
*/
static char*
d_internal_path_copy(
    char*       _buf,
    size_t      _bufsize,
    const char* _src,
    size_t      _length
)
{
    // a result that does not fit is refused, never truncated
    if ((_length + 1) > _bufsize)
    {
        D_INTERNAL_FILE_SET_ERR(ERANGE);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ERANGE,
                               "file_path",
                               NULL,
                               "result does not fit the caller's buffer");

        return NULL;
    }

    // an empty result needs only its terminator
    if (_length > 0)
    {
        memmove(_buf,
                _src,
                _length);
    }

    _buf[_length] = '\0';

    return _buf;
}

/*
d_internal_path_root_len
  Measures the root prefix of a path -- the leading run that names a starting
point rather than a component, and that ".." may never climb above. The forms
recognised depend on configuration, not on the host, so a POSIX build can be
told to parse Windows paths (a cross-compiler, an archiver) and a Windows
build always parses its own:
  POSIX     "/"                    -> 1
            "//"                   -> 2   (POSIX reserves exactly two)
            "///"                  -> 1   (three or more is just root)
  Windows   "C:"                   -> 2   (drive-relative; NOT absolute)
            "C:\\"                 -> 3   (drive-absolute)
            "\\\\server\\share"    -> whole prefix
            "\\\\?\\C:\\"          -> whole prefix
            "\\"                   -> 1   (rooted on the current drive)
*/
static size_t
d_internal_path_root_len(
    const char* _path
)
{
#if (D_INTERNAL_FILE_PATH_HAS_DRIVE == 1)
    // "C:" -- a drive letter followed by a colon
    if ( (_path[0] != '\0') &&
         (_path[1] == ':')  &&
         ( ( (_path[0] >= 'A') &&
             (_path[0] <= 'Z') ) ||
           ( (_path[0] >= 'a') &&
             (_path[0] <= 'z') ) ) )
    {
        // "C:\\" is anchored; bare "C:" means "wherever that drive is",
        // which is a root for climbing purposes but is NOT absolute
        if (d_internal_path_is_sep(_path[2]))
        {
            return 3;
        }

        return 2;
    }
#endif

#if (D_INTERNAL_FILE_PATH_HAS_UNC == 1)
    // "\\\\server\\share" or "\\\\?\\..." -- two separators, then a name,
    // then optionally one more name
    if ( (d_internal_path_is_sep(_path[0])) &&
         (d_internal_path_is_sep(_path[1])) &&
         (_path[2] != '\0')                 &&
         (!d_internal_path_is_sep(_path[2])) )
    {
        size_t idx = 2;

        // server (or the "?" of an extended path)
        while ( (_path[idx] != '\0') &&
                (!d_internal_path_is_sep(_path[idx])) )
        {
            ++idx;
        }

        // share
        if (d_internal_path_is_sep(_path[idx]))
        {
            ++idx;

            while ( (_path[idx] != '\0') &&
                    (!d_internal_path_is_sep(_path[idx])) )
            {
                ++idx;
            }
        }

        return idx;
    }
#endif

    // a leading separator roots the path on every target
    if (d_internal_path_is_sep(_path[0]))
    {
        // POSIX gives exactly two leading slashes an implementation-defined
        // meaning and three or more none at all, so "//" is preserved as a
        // root while "///" collapses to "/"
        if ( (d_internal_path_is_sep(_path[1])) &&
             (!d_internal_path_is_sep(_path[2])) )
        {
            return 2;
        }

        return 1;
    }

    return 0;
}

/*
d_internal_path_end
  "a/b/" and "a/b" have the same final component; trimming trailing
separators is what makes them agree. A root is never trimmed away -- "/" would
otherwise become "". The result is the index one past the last meaningful
byte.
*/
static size_t
d_internal_path_end(
    const char* _path,
    size_t      _length,
    size_t      _root_len
)
{
    size_t end = _length;

    // trim trailing separators, but never into the root
    while ( (end > _root_len) &&
            (d_internal_path_is_sep(_path[end - 1])) )
    {
        --end;
    }

    return end;
}

//==============================================================================
// 1.  PATHS
//==============================================================================

/*
d_path_dirname
  Lexical: the result is what the path says its parent is, whether or not
either exists. From the meaningful end, the walk goes back to the separator
that ends the parent and then past the separator run itself, never into the
root. A path that is only a root is its own parent, and a relative path with
no separator has "." as its parent.
*/
char*
d_path_dirname(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_path_dirname",
                            NULL,
                            "path is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_path_dirname",
                            _path,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 0,
                            EINVAL,
                            "d_path_dirname",
                            _path,
                            "buffer size is 0",
                            NULL);

    const size_t length   = strlen(_path);
    const size_t root_len = d_internal_path_root_len(_path);
    size_t       end      = d_internal_path_end(_path,
                                                length,
                                                root_len);

    // a path that is nothing but its root is its own parent
    if (end <= root_len)
    {
        // no root either: the parent is the current directory
        if (root_len == 0)
        {
            return d_internal_path_copy(_buf,
                                        _bufsize,
                                        ".",
                                        1);
        }

        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    _path,
                                    root_len);
    }

    // walk back to the separator that ends the parent
    while ( (end > root_len) &&
            (!d_internal_path_is_sep(_path[end - 1])) )
    {
        --end;
    }

    // no separator at all: the parent is the current directory
    if (end <= root_len)
    {
        // a relative path's parent is the current directory
        if (root_len == 0)
        {
            return d_internal_path_copy(_buf,
                                        _bufsize,
                                        ".",
                                        1);
        }

        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    _path,
                                    root_len);
    }

    // drop the separator itself, unless doing so would eat the root
    while ( (end > root_len) &&
            (d_internal_path_is_sep(_path[end - 1])) )
    {
        --end;
    }

    // never shorter than the root
    if (end < root_len)
    {
        end = root_len;
    }

    return d_internal_path_copy(_buf,
                                _bufsize,
                                _path,
                                end);
}

/*
d_path_basename
  The final component runs from the separator before the meaningful end up to
that end; a path that is nothing but a root names itself.
*/
char*
d_path_basename(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_path_basename",
                            NULL,
                            "path is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_path_basename",
                            _path,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 0,
                            EINVAL,
                            "d_path_basename",
                            _path,
                            "buffer size is 0",
                            NULL);

    const size_t length   = strlen(_path);
    const size_t root_len = d_internal_path_root_len(_path);
    const size_t end      = d_internal_path_end(_path,
                                                length,
                                                root_len);

    // nothing but a root: the root names itself
    if (end <= root_len)
    {
        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    _path,
                                    root_len);
    }

    size_t start = end;

    // walk back to the separator that opens the final component
    while ( (start > root_len) &&
            (!d_internal_path_is_sep(_path[start - 1])) )
    {
        --start;
    }

    return d_internal_path_copy(_buf,
                                _bufsize,
                                _path + start,
                                end - start);
}

/*
d_path_extension
  Two passes over the input and no copy: the first finds where the final
component starts, the second finds its last dot. A dot that opens the name is
hiding the file, not typing it, so it starts no extension.
*/
const char*
d_path_extension(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_path_extension",
                            NULL,
                            "path is NULL",
                            NULL);

    const char* name   = _path;
    const char* cursor = _path;

    // find the start of the final component without copying it
    while (*cursor != '\0')
    {
        // the name starts after every separator
        if (d_internal_path_is_sep(*cursor))
        {
            name = cursor + 1;
        }

        ++cursor;
    }

    const char* dot = NULL;

    // last dot within the final component only
    cursor = name;

    while (*cursor != '\0')
    {
        // remember the latest dot
        if (*cursor == '.')
        {
            dot = cursor;
        }

        ++cursor;
    }

    // no dot, no extension
    if (!dot)
    {
        return NULL;
    }

    // a dot that opens the name is hiding it, not typing it
    if (dot == name)
    {
        return NULL;
    }

    return dot;
}

/*
d_path_stem
  Built from the other two rather than re-deriving their rules: the basename
is written into _buf, and d_path_extension on that copy says where to cut, so
stem and extension can never disagree.
*/
char*
d_path_stem(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_path_stem",
                            NULL,
                            "path is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_path_stem",
                            _path,
                            "buffer is NULL",
                            NULL);

    // reuse the basename rules rather than re-deriving them
    if (!d_path_basename(_path,
                         _buf,
                         _bufsize))
    {
        return NULL;
    }

    // and the extension rules, so the two can never disagree
    const char* const ext = d_path_extension(_buf);

    // cut the extension off where it starts
    if (ext)
    {
        const size_t length = (size_t)(ext - _buf);

        _buf[length] = '\0';
    }

    return _buf;
}

/*
d_path_join
  An absent component leaves the other standing alone. With
D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS an absolute second component is a
replacement, not a suffix -- the alternative silently builds
"/base/etc/passwd" for code that meant to be handed an absolute override.
Otherwise the first component's trailing separators and the second's leading
ones are trimmed and exactly one is emitted between them, unless the first
component was nothing but separators: then it was a root, and "/" joined with
"b" is "/b", not "b".
*/
char*
d_path_join(
    char*       _buf,
    size_t      _bufsize,
    const char* _path1,
    const char* _path2
)
{
    // parameter validation: the components are optional, the buffer is not
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_path_join",
                            NULL,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 0,
                            EINVAL,
                            "d_path_join",
                            NULL,
                            "buffer size is 0",
                            NULL);

    size_t len1 = _path1 ? strlen(_path1) : 0;
    size_t len2 = _path2 ? strlen(_path2) : 0;

    // an absent first component leaves the second standing alone
    if (len1 == 0)
    {
        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    _path2 ? _path2 : "",
                                    len2);
    }

    // ...and vice versa
    if (len2 == 0)
    {
        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    _path1,
                                    len1);
    }

#if D_CFG_IS_ON(D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS)
    // an absolute second component is not a suffix, it is a replacement
    if (d_path_is_absolute(_path2))
    {
        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    _path2,
                                    len2);
    }
#endif

    // exactly one separator, however many the caller supplied
    while ( (len1 > 0) &&
            (d_internal_path_is_sep(_path1[len1 - 1])) )
    {
        --len1;
    }

    int need_sep = 1;

    // ...unless trimming ate the whole first component, which means it WAS a
    // root ("/" joined with "b" is "/b", not "b")
    if (len1 == 0)
    {
        len1     = 1;
        need_sep = 0;
    }

    // the second component brings no separators of its own
    while ( (len2 > 0) &&
            (d_internal_path_is_sep(_path2[0])) )
    {
        ++_path2;
        --len2;
    }

    // the joined path and its terminator must fit
    if ((len1 + (size_t)need_sep + len2 + 1) > _bufsize)
    {
        D_INTERNAL_FILE_SET_ERR(ERANGE);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ERANGE,
                               "d_path_join",
                               NULL,
                               "joined path does not fit the caller's buffer");

        return NULL;
    }

    memmove(_buf,
            _path1,
            len1);

    size_t out = len1;

    // one separator between the two, unless the first was a root
    if (need_sep)
    {
        _buf[out++] = D_INTERNAL_FILE_PATH_OUT_SEP;
    }

    // a second component of nothing but separators adds nothing
    if (len2 > 0)
    {
        memmove(_buf + out,
                _path2,
                len2);
        out += len2;
    }

    _buf[out] = '\0';

    return _buf;
}

/*
d_path_is_absolute
  Absolute means having a root -- except a bare drive root, "C:", which is
drive-relative: x relative to whatever the current directory on drive C
happens to be, a per-drive cursor Win32 still maintains.
*/
int
d_path_is_absolute(
    const char* _path
)
{
    // a NULL path is not absolute; it is also not an error worth reporting,
    // since the answer "no" is meaningful and the caller asked a yes/no
    if (!_path)
    {
        return 0;
    }

    const size_t root_len = d_internal_path_root_len(_path);

    // no root, not absolute
    if (root_len == 0)
    {
        return 0;
    }

#if (D_INTERNAL_FILE_PATH_HAS_DRIVE == 1)
    // "C:" without a separator is drive-RELATIVE, not absolute
    if ( (root_len == 2) &&
         (_path[1] == ':') )
    {
        return 0;
    }
#endif

    return 1;
}

/*
d_path_root_length
  The public face of d_internal_path_root_len, with NULL answered as a path
that has no root.
*/
size_t
d_path_root_length(
    const char* _path
)
{
    // a NULL path has no root
    if (!_path)
    {
        return 0;
    }

    return d_internal_path_root_len(_path);
}

/*
d_path_normalize
  One pass over the input, writing components into _buf as they survive. The
root is copied through with only its separators respelled: they are structure,
not punctuation, and normalizing "//" or a "\\?\" prefix would change their
meaning. A separator run says nothing a single separator does not, and "."
means "stay here". ".." removes the last emitted component unless there is
nothing to climb over or that component is itself a ".." that had to be kept;
directly above an absolute root it is dropped, as POSIX says "/.." is "/". A
result with nothing left is ".", because "" is not a path.
*/
char*
d_path_normalize(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_path_normalize",
                            NULL,
                            "path is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_path_normalize",
                            _path,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 1,
                            EINVAL,
                            "d_path_normalize",
                            _path,
                            "buffer is too small to hold anything",
                            NULL);

    const size_t length   = strlen(_path);
    const size_t root_len = d_internal_path_root_len(_path);

#if D_CFG_IS_ON(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)
    const int is_absolute = d_path_is_absolute(_path);
#endif

    // the result is never longer than the input, so the input must fit
    if ((length + 1) > _bufsize)
    {
        D_INTERNAL_FILE_SET_ERR(ERANGE);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               ERANGE,
                               "d_path_normalize",
                               NULL,
                               "path does not fit the caller's buffer");

        return NULL;
    }

    size_t out = 0;

    // the root is copied through untouched -- its separators are structure,
    // not punctuation, and normalizing "//" or "\\\\?\\" would change meaning
    for (; out < root_len; ++out)
    {
        // respell a separator; copy anything else as it is
        if (d_internal_path_is_sep(_path[out]))
        {
            _buf[out] = D_INTERNAL_FILE_PATH_OUT_SEP;
        }
        else
        {
            _buf[out] = _path[out];
        }
    }

    size_t idx = root_len;

    // walk the components after the root
    while (idx < length)
    {
        // skip the separators between components; a run of them says nothing
        // a single one does not
        if (d_internal_path_is_sep(_path[idx]))
        {
            ++idx;
            continue;
        }

        const size_t seg_start = idx;

        // measure the component
        while ( (idx < length) &&
                (!d_internal_path_is_sep(_path[idx])) )
        {
            ++idx;
        }

        const size_t seg_len = idx - seg_start;

        // "." is a component that means "stay here"
        if ( (seg_len == 1) &&
             (_path[seg_start] == '.') )
        {
            continue;
        }

#if D_CFG_IS_ON(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)
        // ".." climbs, where there is something to climb over
        if ( (seg_len == 2)                &&
             (_path[seg_start] == '.')     &&
             (_path[seg_start + 1] == '.') )
        {
            // climb, if there is anything above us to climb to
            if (out > root_len)
            {
                size_t back = out;

                // do not climb over a ".." we already had to keep
                if ( (back >= 2)             &&
                     (_buf[back - 1] == '.') &&
                     (_buf[back - 2] == '.') &&
                     ( (back == 2) ||
                       (_buf[back - 3] == D_INTERNAL_FILE_PATH_OUT_SEP) ) )
                {
                    // fall through and keep this one too
                }
                else
                {
                    // back over the last component...
                    while ( (back > root_len) &&
                            (_buf[back - 1] != D_INTERNAL_FILE_PATH_OUT_SEP) )
                    {
                        --back;
                    }

                    // ...and the separators before it
                    while ( (back > root_len) &&
                            (_buf[back - 1] == D_INTERNAL_FILE_PATH_OUT_SEP) )
                    {
                        --back;
                    }

                    out = back;
                    continue;
                }
            }
            else if (is_absolute)
            {
                // the root has no parent; POSIX says "/.." is "/"
                continue;
            }
        }
#endif

        // emit a separator before every component but the first, and never
        // immediately after a root that already ends in one
        if ( (out > 0) &&
             (_buf[out - 1] != D_INTERNAL_FILE_PATH_OUT_SEP) )
        {
            _buf[out++] = D_INTERNAL_FILE_PATH_OUT_SEP;
        }

        memmove(_buf + out,
                _path + seg_start,
                seg_len);
        out += seg_len;
    }

#if D_CFG_IS_ON(D_CFG_FILE_PATH_STRIP_TRAILING_SEP)
    // trim a trailing separator, but never the one that IS the root
    while ( (out > root_len) &&
            (out > 1)        &&
            (_buf[out - 1] == D_INTERNAL_FILE_PATH_OUT_SEP) )
    {
        --out;
    }
#endif

    // everything cancelled out; "" is not a path, "." is
    if (out == 0)
    {
        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    ".",
                                    1);
    }

    _buf[out] = '\0';

    return _buf;
}
