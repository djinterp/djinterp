/******************************************************************************
* djinterp [c]                                                     file_path.c
*
* path:      /src/djinterp/c/fs/file_path.c
******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/c/fs/file_path.h"


// Internal definitions

/*
d_internal_path_is_sep
  Reports whether a character separates path components in this build.
  On POSIX only '/' qualifies -- '\\' is an ordinary byte in a filename there,
and treating it as a separator would silently corrupt legitimate names.

Parameter(s):
  _c: the character to classify.
Return:
  1 when _c is a separator, 0 otherwise.
*/
static int
d_internal_path_is_sep
(
    char _c
)
{
    if (_c == '/')
    {
        return 1;
    }

#if (D_INTERNAL_FILE_PATH_ALT_SEP == 1)
    if (_c == '\\')
    {
        return 1;
    }
#endif

    return 0;
}


/*
d_internal_path_copy
  Copies a byte range into the caller's buffer, NUL-terminating it.
  One place for the truncation decision: a path that does not fit is a
failure, not a shortened path. Silently handing back a prefix of a path is how
a program deletes the wrong directory.

Parameter(s):
  _buf:     destination buffer.
  _bufsize: size of _buf, in bytes.
  _src:     source bytes; need not be NUL-terminated.
  _length:  number of bytes to copy.
Return:
  _buf on success, or NULL when the result would not fit, with errno set to
ERANGE.
*/
static char*
d_internal_path_copy
(
    char*       _buf,
    size_t      _bufsize,
    const char* _src,
    size_t      _length
)
{
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

    if (_length > 0)
    {
        memmove(_buf, _src, _length);
    }

    _buf[_length] = '\0';

    return _buf;
}


/*
d_internal_path_root_len
  Measures the root prefix of a path -- the leading run that names a starting
point rather than a component, and that "..". may never climb above.
  The forms recognised depend on configuration, not on the host, so a POSIX
build can be told to parse Windows paths (a cross-compiler, an archiver) and a
Windows build always parses its own.

  POSIX     "/"                    -> 1
            "//"                   -> 2   (POSIX reserves exactly two)
            "///"                  -> 1   (three or more is just root)
  Windows   "C:"                   -> 2   (drive-relative; NOT absolute)
            "C:\\"                 -> 3   (drive-absolute)
            "\\\\server\\share"    -> whole prefix
            "\\\\?\\C:\\"          -> whole prefix
            "\\"                   -> 1   (rooted on the current drive)

Parameter(s):
  _path: the path to measure; must not be NULL.
Return:
  The number of leading bytes forming the root; 0 when the path is relative.
*/
static size_t
d_internal_path_root_len
(
    const char* _path
)
{
#if (D_INTERNAL_FILE_PATH_HAS_UNC == 1)
    size_t idx;
#endif

#if (D_INTERNAL_FILE_PATH_HAS_DRIVE == 1)
    // "C:" -- a drive letter followed by a colon
    if ( (_path[0] != '\0') &&
         (_path[1] == ':') &&
         ( ((_path[0] >= 'A') && (_path[0] <= 'Z')) ||
           ((_path[0] >= 'a') && (_path[0] <= 'z')) ) )
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
    if ( d_internal_path_is_sep(_path[0]) &&
         d_internal_path_is_sep(_path[1]) &&
         (_path[2] != '\0') &&
         (!d_internal_path_is_sep(_path[2])) )
    {
        idx = 2;

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

    if (d_internal_path_is_sep(_path[0]))
    {
        // POSIX gives exactly two leading slashes an implementation-defined
        // meaning and three or more none at all, so "//" is preserved as a
        // root while "///" collapses to "/"
        if ( d_internal_path_is_sep(_path[1]) &&
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
  Finds the end of a path's meaningful text, ignoring trailing separators.
  "a/b/" and "a/b" have the same final component; this is what makes them
agree. A root is never trimmed away -- "/" would otherwise become "".

Parameter(s):
  _path:     the path.
  _length:   its length in bytes.
  _root_len: the length of its root prefix.
Return:
  The index one past the last meaningful byte.
*/
static size_t
d_internal_path_end
(
    const char* _path,
    size_t      _length,
    size_t      _root_len
)
{
    size_t end;

    end = _length;

    while ( (end > _root_len) &&
            d_internal_path_is_sep(_path[end - 1]) )
    {
        --end;
    }

    return end;
}


// I.    Decomposition

/*
d_dirname
  Extracts the directory component of a path.
  Lexical: the result is what the path says its parent is, whether or not
either exists. Trailing separators are ignored, so "a/b/" and "a/b" both give
"a".

  "/path/to/file.txt" -> "/path/to"      "file.txt" -> "."
  "/file.txt"         -> "/"             ""         -> "."
  "a/b/"              -> "a"             "/"        -> "/"

Parameter(s):
  _path:    the path to decompose.
  _buf:     buffer to receive the directory component.
  _bufsize: size of _buf, in bytes.
Return:
  _buf on success, or NULL on failure with errno set.
*/
char*
d_dirname
(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    size_t length;
    size_t root_len;
    size_t end;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dirname",
                            NULL,
                            "path is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_dirname",
                            _path,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 0,
                            EINVAL,
                            "d_dirname",
                            _path,
                            "buffer size is 0",
                            NULL);

    length   = strlen(_path);
    root_len = d_internal_path_root_len(_path);
    end      = d_internal_path_end(_path, length, root_len);

    // a path that is nothing but its root is its own parent
    if (end <= root_len)
    {
        if (root_len == 0)
        {
            return d_internal_path_copy(_buf, _bufsize, ".", 1);
        }

        return d_internal_path_copy(_buf, _bufsize, _path, root_len);
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
        if (root_len == 0)
        {
            return d_internal_path_copy(_buf, _bufsize, ".", 1);
        }

        return d_internal_path_copy(_buf, _bufsize, _path, root_len);
    }

    // drop the separator itself, unless doing so would eat the root
    while ( (end > root_len) &&
            d_internal_path_is_sep(_path[end - 1]) )
    {
        --end;
    }

    if (end < root_len)
    {
        end = root_len;
    }

    return d_internal_path_copy(_buf, _bufsize, _path, end);
}


/*
d_basename
  Extracts the final component of a path.
  Trailing separators are ignored, so "a/b/" gives "b" -- which is what a
caller means by "the name of this thing", and what the shell's basename does.

  "/path/to/file.txt" -> "file.txt"      "file.txt" -> "file.txt"
  "a/b/"              -> "b"             "/"        -> "/"
  ""                  -> ""

Parameter(s):
  _path:    the path to decompose.
  _buf:     buffer to receive the final component.
  _bufsize: size of _buf, in bytes.
Return:
  _buf on success, or NULL on failure with errno set.
*/
char*
d_basename
(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    size_t length;
    size_t root_len;
    size_t end;
    size_t start;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_basename",
                            NULL,
                            "path is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_basename",
                            _path,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 0,
                            EINVAL,
                            "d_basename",
                            _path,
                            "buffer size is 0",
                            NULL);

    length   = strlen(_path);
    root_len = d_internal_path_root_len(_path);
    end      = d_internal_path_end(_path, length, root_len);

    // nothing but a root: the root names itself
    if (end <= root_len)
    {
        return d_internal_path_copy(_buf, _bufsize, _path, root_len);
    }

    start = end;

    while ( (start > root_len) &&
            (!d_internal_path_is_sep(_path[start - 1])) )
    {
        --start;
    }

    return d_internal_path_copy(_buf, _bufsize, _path + start, end - start);
}


/*
d_get_extension
  Returns the extension of a path's final component, including the dot.
  A leading dot does NOT start an extension: ".bashrc" is a hidden file whose
whole name is ".bashrc", not a file with a ".bashrc" extension. Nor does a dot
in a parent directory count -- "/etc/x.d/file" has no extension.

  "file.txt"     -> ".txt"       "archive.tar.gz" -> ".gz"
  "filename"     -> NULL         ".bashrc"        -> NULL
  "/a.d/file"    -> NULL         "file."          -> "."

Parameter(s):
  _path: the path to inspect.
Return:
  A pointer INTO _path at the dot, or NULL when there is no extension. The
result is not a copy and lives exactly as long as _path does.
*/
const char*
d_get_extension
(
    const char* _path
)
{
    const char* dot;
    const char* cursor;
    const char* name;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_get_extension",
                            NULL,
                            "path is NULL",
                            NULL);

    // find the start of the final component without copying it
    name   = _path;
    cursor = _path;

    while (*cursor != '\0')
    {
        if (d_internal_path_is_sep(*cursor))
        {
            name = cursor + 1;
        }

        ++cursor;
    }

    // last dot within the final component only
    dot = NULL;
    cursor = name;

    while (*cursor != '\0')
    {
        if (*cursor == '.')
        {
            dot = cursor;
        }

        ++cursor;
    }

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
  Extracts the final component of a path with its extension removed.
  The complement of d_get_extension, and it agrees with it by construction:
stem + extension reconstructs the basename for every input, including hidden
files (".bashrc" -> stem ".bashrc", extension none).

  "/a/file.txt" -> "file"        ".bashrc"        -> ".bashrc"
  "archive.tar.gz" -> "archive.tar"

Parameter(s):
  _path:    the path to decompose.
  _buf:     buffer to receive the stem.
  _bufsize: size of _buf, in bytes.
Return:
  _buf on success, or NULL on failure with errno set.
*/
char*
d_path_stem
(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    const char* ext;
    size_t      length;

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
    if (!d_basename(_path, _buf, _bufsize))
    {
        return NULL;
    }

    // and the extension rules, so the two can never disagree
    ext = d_get_extension(_buf);

    if (ext)
    {
        length = (size_t)(ext - _buf);
        _buf[length] = '\0';
    }

    return _buf;
}


// II.   Composition

/*
d_path_join
  Joins two path components with exactly one separator between them.
  A NULL or empty component is skipped rather than being an error, so a caller
may pass an optional base directory straight through without branching.
  When D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS is set (the default) an absolute
second component discards the first, matching every other path library. The
alternative silently builds "/base/etc/passwd" for code that meant to be
handed an absolute override.

  ("a", "b")     -> "a/b"        ("a/", "b")  -> "a/b"
  (NULL, "b")    -> "b"          ("a", NULL)  -> "a"
  ("a", "/b")    -> "/b"         ("", "")     -> ""

Parameter(s):
  _buf:     buffer to receive the joined path.
  _bufsize: size of _buf, in bytes.
  _path1:   first component; may be NULL or empty.
  _path2:   second component; may be NULL or empty.
Return:
  _buf on success, or NULL on failure with errno set.
*/
char*
d_path_join
(
    char*       _buf,
    size_t      _bufsize,
    const char* _path1,
    const char* _path2
)
{
    size_t len1;
    size_t len2;
    size_t out;
    int    need_sep;

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

    len1 = _path1 ? strlen(_path1) : 0;
    len2 = _path2 ? strlen(_path2) : 0;

    // an absent first component leaves the second standing alone
    if (len1 == 0)
    {
        return d_internal_path_copy(_buf, _bufsize, _path2 ? _path2 : "", len2);
    }

    // ...and vice versa
    if (len2 == 0)
    {
        return d_internal_path_copy(_buf, _bufsize, _path1, len1);
    }

#if D_CFG_IS_ON(D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS)
    // an absolute second component is not a suffix, it is a replacement
    if (d_path_is_absolute(_path2))
    {
        return d_internal_path_copy(_buf, _bufsize, _path2, len2);
    }
#endif

    // exactly one separator, however many the caller supplied
    while ( (len1 > 0) &&
            d_internal_path_is_sep(_path1[len1 - 1]) )
    {
        --len1;
    }

    // ...unless trimming would eat the whole first component, which means it
    // WAS a root ("/" joined with "b" is "/b", not "b")
    need_sep = 1;

    if (len1 == 0)
    {
        len1     = 1;
        need_sep = 0;
    }

    while ( (len2 > 0) &&
            d_internal_path_is_sep(_path2[0]) )
    {
        ++_path2;
        --len2;
    }

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

    memmove(_buf, _path1, len1);
    out = len1;

    if (need_sep)
    {
        _buf[out++] = D_INTERNAL_FILE_PATH_OUT_SEP;
    }

    if (len2 > 0)
    {
        memmove(_buf + out, _path2, len2);
        out += len2;
    }

    _buf[out] = '\0';

    return _buf;
}


// III.  Inspection

/*
d_path_is_absolute
  Reports whether a path names a fixed starting point.
  Note the Windows subtlety this gets right and string comparison does not:
"C:x" is NOT absolute. It means "x, relative to whatever the current directory
on drive C happens to be" -- a per-drive cursor Win32 still maintains. Only
"C:\\x" is anchored.

Parameter(s):
  _path: the path to inspect; may be NULL.
Return:
  Non-zero when the path is absolute, 0 when it is relative or NULL.
*/
int
d_path_is_absolute
(
    const char* _path
)
{
    size_t root_len;

    // a NULL path is not absolute; it is also not an error worth reporting,
    // since the answer "no" is meaningful and the caller asked a yes/no
    if (!_path)
    {
        return 0;
    }

    root_len = d_internal_path_root_len(_path);

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
  Reports the length of a path's root prefix -- the leading bytes that name a
starting point rather than a component.
  Exposed because it is what a caller needs to split a path safely: ".." may
never climb above it, and a join must never insert a separator inside it.

Parameter(s):
  _path: the path to inspect; may be NULL.
Return:
  The root's length in bytes, or 0 when the path is relative or NULL.
*/
size_t
d_path_root_length
(
    const char* _path
)
{
    if (!_path)
    {
        return 0;
    }

    return d_internal_path_root_len(_path);
}


// IV.   Canonicalization

/*
d_path_normalize
  Cleans a path lexically: collapses separator runs, drops "." components,
resolves ".." against the preceding component, and emits this build's
separator.

  READ THIS BEFORE USING IT ON A REAL PATH. Resolving ".." lexically is only
equivalent to what the kernel does when no component is a symbolic link.
Given /x/link -> /y/z, this function says "/x/link/.." is "/x"; the kernel
says it is "/y". Both are defensible and they are not the same answer. If the
path names something that exists and the difference matters, call d_realpath
(file_dir), which asks the filesystem rather than guessing. Use this one for
paths that do not exist yet, for untrusted input you want to bound, and for
display.

  "/path/to/../file.txt" -> "/path/file.txt"    "a//b" -> "a/b"
  "./a/./b"              -> "a/b"               "a/b/" -> "a/b"
  "../../a"              -> "../../a"   (kept: nothing to resolve against)
  "/../a"                -> "/a"        (dropped: root has no parent)
  ""                     -> "."

Parameter(s):
  _path:    the path to normalize.
  _buf:     buffer to receive the normalized path.
  _bufsize: size of _buf, in bytes.
Return:
  _buf on success, or NULL on failure with errno set.
*/
char*
d_path_normalize
(
    const char* _path,
    char*       _buf,
    size_t      _bufsize
)
{
    size_t root_len;
    size_t idx;
    size_t out;
    size_t seg_start;
    size_t seg_len;
    size_t length;
#if D_CFG_IS_ON(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)
    int    is_absolute;
#endif

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

    length   = strlen(_path);
    root_len = d_internal_path_root_len(_path);
#if D_CFG_IS_ON(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)
    is_absolute = d_path_is_absolute(_path);
#endif

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

    // the root is copied through untouched -- its separators are structure,
    // not punctuation, and normalizing "//" or "\\\\?\\" would change meaning
    for (out = 0; out < root_len; ++out)
    {
        if (d_internal_path_is_sep(_path[out]))
        {
            _buf[out] = D_INTERNAL_FILE_PATH_OUT_SEP;
        }
        else
        {
            _buf[out] = _path[out];
        }
    }

    idx = root_len;

    while (idx < length)
    {
        // skip the separators between components; a run of them says nothing
        // a single one does not
        if (d_internal_path_is_sep(_path[idx]))
        {
            ++idx;
            continue;
        }

        seg_start = idx;

        while ( (idx < length) &&
                (!d_internal_path_is_sep(_path[idx])) )
        {
            ++idx;
        }

        seg_len = idx - seg_start;

        // "." is a component that means "stay here"
        if ( (seg_len == 1) &&
             (_path[seg_start] == '.') )
        {
            continue;
        }

#if D_CFG_IS_ON(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)
        if ( (seg_len == 2) &&
             (_path[seg_start] == '.') &&
             (_path[seg_start + 1] == '.') )
        {
            // climb, if there is anything above us to climb to
            if (out > root_len)
            {
                size_t back;

                back = out;

                // do not climb over a ".." we already had to keep
                if ( (back >= 2) &&
                     (_buf[back - 1] == '.') &&
                     (_buf[back - 2] == '.') &&
                     ( (back == 2) ||
                       (_buf[back - 3] == D_INTERNAL_FILE_PATH_OUT_SEP) ) )
                {
                    // fall through and keep this one too
                }
                else
                {
                    while ( (back > root_len) &&
                            (_buf[back - 1] != D_INTERNAL_FILE_PATH_OUT_SEP) )
                    {
                        --back;
                    }

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

        memmove(_buf + out, _path + seg_start, seg_len);
        out += seg_len;
    }

#if D_CFG_IS_ON(D_CFG_FILE_PATH_STRIP_TRAILING_SEP)
    // trim a trailing separator, but never the one that IS the root
    while ( (out > root_len) &&
            (out > 1) &&
            (_buf[out - 1] == D_INTERNAL_FILE_PATH_OUT_SEP) )
    {
        --out;
    }
#endif

    // everything cancelled out; "" is not a path, "." is
    if (out == 0)
    {
        return d_internal_path_copy(_buf, _bufsize, ".", 1);
    }

    _buf[out] = '\0';

    return _buf;
}
