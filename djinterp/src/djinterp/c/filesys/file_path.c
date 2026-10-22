/*******************************************************************************
* djinterp [c]                                                       file_path.c
*
* Implementation of the lexical path operations declared in file_path.h.
*   File-local helpers carry every rule: which characters separate, how long a
* root is and what kind it is, where a path's meaningful text ends, and how a
* result is copied out -- where a result that does not fit is an ERANGE
* failure, never a truncated path. The public functions compose them and make
* no system call.
*   Roots come in three kinds, and the rules hang on the difference. One that
* ends in a separator ("/", "C:\\") is a root directory: nothing follows it
* but a component. A share ("\\\\server\\share") names a volume and needs a
* separator before its first component. A bare drive ("C:") names a volume
* and no directory on it -- "C:x" is x in drive C's current directory, so a
* component follows it directly, and ".." may climb out of it.
*
*
* path:      /src/djinterp/c/filesys/file_path.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_path.h"  // corresponding header
// std
#include <errno.h>   // EINVAL, ERANGE
#include <stddef.h>  // NULL, size_t
#include <string.h>  // memmove, strlen
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_path.h"  // D_INTERNAL_FILE_PATH_*


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_internal_path_span
//   struct: a run of a path's bytes -- a whole path, its root, a run of
// separators -- that need not end in a NUL, so the helpers below can be
// handed part of a path without a copy. A span holding a root has the whole
// path as its text and the root's length as its length.
struct d_internal_path_span
{
    const char* text;    // the first byte
    size_t      length;  // the number of bytes
};

/*
d_internal_path_is_sep
  On POSIX only '/' separates -- '\\' is an ordinary byte in a filename there,
and treating it as a separator would silently corrupt legitimate names. Under
Windows syntax '\\' always separates, being Windows' own, and '/' does as well
unless D_CFG_FILE_PATH_ACCEPT_ALT_SEP turns it away.
*/
static int
d_internal_path_is_sep(
    char _c
)
{
    // the forward slash, unless Windows syntax turns it away
    if (_c == '/')
    {
        return ( (D_INTERNAL_FILE_PATH_WINDOWS == 0) ||
                 (D_INTERNAL_FILE_PATH_ALT_SEP == 1) );
    }

    // the backslash, under Windows syntax alone
    return ( (_c == '\\') &&
             (D_INTERNAL_FILE_PATH_WINDOWS == 1) );
}

/*
d_internal_path_is_letter
  An ASCII letter, compared by range so the C locale has no say in what a
drive letter is.
*/
static int
d_internal_path_is_letter(
    char _c
)
{
    return ( ( (_c >= 'A') &&
               (_c <= 'Z') ) ||
             ( (_c >= 'a') &&
               (_c <= 'z') ) );
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
d_internal_path_share_len
  The length of the share that opens _path, which begins with two separators
and a name: the server (or the "?" of an extended path), then a separator and
the share's own name, where there is one.
*/
static size_t
d_internal_path_share_len(
    const char* _path
)
{
    size_t idx = 2;

    // the server, or the "?" of an extended path
    while ( (_path[idx] != '\0') &&
            (!d_internal_path_is_sep(_path[idx])) )
    {
        ++idx;
    }

    // the share's name, after its separator
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

/**
 * @brief The root that follows the long-path prefix, "\\\\?\\": a drive, a
 *        share after "UNC\\", or a volume name.
 *
 * @param[in] _rest  the text after the prefix.
 * @return its length: 3 for "C:\\" and 2 for "C:", as without the prefix; a
 *         share as d_internal_path_share_len measures one, the separator after
 *         it aside; a volume name with the separator after it.
 */
static size_t
d_internal_path_long_root_len(
    const char* _rest
)
{
    // "C:\\" or "C:" -- a drive, as one is without the prefix
    if ( (d_internal_path_is_letter(_rest[0])) &&
         (_rest[1] == ':') )
    {
        return (d_internal_path_is_sep(_rest[2])) ? 3 : 2;
    }

    size_t idx = 0;

    // "UNC\\server\\share" -- the server, then the share
    if ( ( (_rest[0] == 'U') || (_rest[0] == 'u') ) &&
         ( (_rest[1] == 'N') || (_rest[1] == 'n') ) &&
         ( (_rest[2] == 'C') || (_rest[2] == 'c') ) &&
         (d_internal_path_is_sep(_rest[3])) )
    {
        idx = 4;

        while ( (_rest[idx] != '\0') &&
                (!d_internal_path_is_sep(_rest[idx])) )
        {
            ++idx;
        }

        // the share's name, after its separator
        if (d_internal_path_is_sep(_rest[idx]))
        {
            ++idx;

            while ( (_rest[idx] != '\0') &&
                    (!d_internal_path_is_sep(_rest[idx])) )
            {
                ++idx;
            }
        }

        return idx;
    }

    // a volume name -- "Volume{...}" -- and the separator after it
    while ( (_rest[idx] != '\0') &&
            (!d_internal_path_is_sep(_rest[idx])) )
    {
        ++idx;
    }

    return (d_internal_path_is_sep(_rest[idx])) ? (idx + 1) : idx;
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
            "\\\\?\\C:"            -> whole prefix
            "\\"                   -> 1   (rooted on the current drive)
Each byte is read only once the one before it has been seen not to end the
string, so a short path is never read past its terminator.
*/
static size_t
d_internal_path_root_len(
    const char* _path
)
{
    // "C:" -- a drive letter and a colon, where drives are parsed
    if ( (D_INTERNAL_FILE_PATH_HAS_DRIVE == 1) &&
         (d_internal_path_is_letter(_path[0])) &&
         (_path[1] == ':') )
    {
        // "C:\\" is anchored; bare "C:" means "wherever that drive is",
        // which is a root for climbing purposes but is NOT absolute
        if (d_internal_path_is_sep(_path[2]))
        {
            return 3;
        }

        return 2;
    }

    // "\\\\?\\" -- the long-path prefix, where shares are parsed: the root
    // it opens is a drive, a share or a volume, and the prefix belongs to it
    // (d_internal_path_long_root_len). Windows reads the prefix only as it is
    // written, in backslashes.
    if ( (D_INTERNAL_FILE_PATH_HAS_UNC == 1) &&
         (_path[0] == '\\')                   &&
         (_path[1] == '\\')                   &&
         (_path[2] == '?')                    &&
         (_path[3] == '\\') )
    {
        return 4 + d_internal_path_long_root_len(_path + 4);
    }

    // "\\\\server\\share" -- two separators, then a name, where shares are
    // parsed
    if ( (D_INTERNAL_FILE_PATH_HAS_UNC == 1) &&
         (d_internal_path_is_sep(_path[0]))  &&
         (d_internal_path_is_sep(_path[1]))  &&
         (_path[2] != '\0')                  &&
         (!d_internal_path_is_sep(_path[2])) )
    {
        return d_internal_path_share_len(_path);
    }

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
d_internal_path_is_bare_drive
  "C:" with nothing after the colon that separates: the drive alone, naming
no directory on it. It anchors nothing -- ".." may climb out of "C:x" -- and
a component follows it directly, since "C:\\x" means something else.
*/
static int
d_internal_path_is_bare_drive(
    const char* _path,
    size_t      _root_len
)
{
    return ( (D_INTERNAL_FILE_PATH_HAS_DRIVE == 1) &&
             (_root_len == 2)                      &&
             (_path[1] == ':') );
}

/*
d_internal_path_root_needs_sep
  Whether the first component after a root needs a separator of its own. Only
a share does: a root directory ends in one already, a bare drive takes its
component directly, and an empty root is no root at all.
*/
static int
d_internal_path_root_needs_sep(
    const char* _path,
    size_t      _root_len
)
{
    return ( (_root_len > 0) &&
             (!d_internal_path_is_sep(_path[_root_len - 1])) &&
             (!d_internal_path_is_bare_drive(_path,
                                             _root_len)) );
}

/*
d_internal_path_root_name_len
  The part of a root that names a volume -- "C:" of "C:\\", the whole of
"\\\\server\\share" -- as against a root directory, which names none. POSIX
roots have no name. A join keeps the first path's name when the second brings
only a root directory ("\\x"), and lets the second's replace it unless the two
name the same volume.
*/
static size_t
d_internal_path_root_name_len(
    const char* _path,
    size_t      _root_len
)
{
    // a drive letter and its colon
    if ( (D_INTERNAL_FILE_PATH_HAS_DRIVE == 1) &&
         (_root_len >= 2)                      &&
         (_path[1] == ':') )
    {
        return 2;
    }

    // a share: two separators, then a name
    if ( (D_INTERNAL_FILE_PATH_HAS_UNC == 1)    &&
         (_root_len > 2)                       &&
         (d_internal_path_is_sep(_path[0]))    &&
         (d_internal_path_is_sep(_path[1]))    &&
         (!d_internal_path_is_sep(_path[2])) )
    {
        return _root_len;
    }

    return 0;
}

/*
d_internal_path_fold
  A byte of a root name as Windows compares it: either separator the same,
and ASCII letters without case.
*/
static char
d_internal_path_fold(
    char _c
)
{
    // either separator spells the same thing
    if (d_internal_path_is_sep(_c))
    {
        return '/';
    }

    // and a letter's case is no difference
    if ( (_c >= 'A') &&
         (_c <= 'Z') )
    {
        return (char)(_c - 'A' + 'a');
    }

    return _c;
}

/*
d_internal_path_same_name
  Whether two root names name the same volume, compared as Windows compares
them: "c:" is "C:", and "//server/share" is "\\\\SERVER\\share".
*/
static int
d_internal_path_same_name(
    const char* _a,
    size_t      _a_len,
    const char* _b,
    size_t      _b_len
)
{
    // names of different lengths cannot match
    if (_a_len != _b_len)
    {
        return 0;
    }

    // byte by byte, folded
    for (size_t idx = 0; idx < _a_len; ++idx)
    {
        // the first difference decides it
        if (d_internal_path_fold(_a[idx]) != d_internal_path_fold(_b[idx]))
        {
            return 0;
        }
    }

    return 1;
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

/*
d_internal_path_concat
  Writes _head, a separator if _sep says so, and _tail into _buf -- or refuses
with ERANGE when they do not fit. _head may begin at _buf itself, as when a
caller joins onto its own buffer; _tail may not overlap it.
*/
static char*
d_internal_path_concat(
    char*                       _buf,
    size_t                      _bufsize,
    struct d_internal_path_span _head,
    int                         _sep,
    struct d_internal_path_span _tail
)
{
    // the joined path and its terminator must fit
    if ((_head.length + (size_t)(_sep != 0) + _tail.length + 1) > _bufsize)
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
            _head.text,
            _head.length);

    size_t out = _head.length;

    // one separator between the two, where the head needs it
    if (_sep)
    {
        _buf[out++] = D_INTERNAL_FILE_PATH_OUT_SEP;
    }

    // a tail of nothing adds nothing
    if (_tail.length > 0)
    {
        memmove(_buf + out,
                _tail.text,
                _tail.length);
        out += _tail.length;
    }

    _buf[out] = '\0';

    return _buf;
}

/*
d_internal_path_join_relative
  The textual join: the first path without its trailing separators -- never
cut into its root, so "//" and "C:" survive whole -- one separator unless the
first path is a root that needs none, then the second path without its leading
separators. "/" and "b" give "/b", "C:" and "b" give "C:b", and a share and
"b" give "\\\\server\\share\\b". The first path is a whole path, ending in its
terminator, since its root is measured; the second is only copied.
*/
static char*
d_internal_path_join_relative(
    char*                       _buf,
    size_t                      _bufsize,
    struct d_internal_path_span _path1,
    struct d_internal_path_span _path2
)
{
    // nothing to add: the first path stands alone
    if (_path2.length == 0)
    {
        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    _path1.text,
                                    _path1.length);
    }

    const size_t root1 = d_internal_path_root_len(_path1.text);
    const size_t end1  = d_internal_path_end(_path1.text,
                                             _path1.length,
                                             root1);
    const int    sep   = (end1 > root1) ||
                         (d_internal_path_root_needs_sep(_path1.text,
                                                         root1));

    // the first path without its trailing separators, its root kept whole
    const struct d_internal_path_span head = { _path1.text, end1 };

    // the second component brings no separators of its own
    while ( (_path2.length > 0) &&
            (d_internal_path_is_sep(_path2.text[0])) )
    {
        ++_path2.text;
        --_path2.length;
    }

    return d_internal_path_concat(_buf,
                                  _bufsize,
                                  head,
                                  sep,
                                  _path2);
}

/*
d_internal_path_join_rooted
  A second path with a root decides how much of the first survives, as
std::filesystem's operator/ and Python's ntpath.join decide it: an absolute
one replaces it outright; one that names a volume ("D:x") replaces it unless
the first is on the same volume, where the rest joins on ("C:\\a" and "C:x"
give "C:\\a\\x"); and a root directory alone ("\\x") keeps the first path's
volume and nothing else ("C:\\a" and "\\x" give "C:\\x"). Under POSIX syntax
every root is absolute, so the first rule is the only one.
*/
static char*
d_internal_path_join_rooted(
    char*                       _buf,
    size_t                      _bufsize,
    struct d_internal_path_span _path1,
    struct d_internal_path_span _path2,
    size_t                      _root2
)
{
    // an anchored second path is not a suffix, it is a replacement
    if (d_path_is_absolute(_path2.text))
    {
        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    _path2.text,
                                    _path2.length);
    }

    // the part of each root that names a volume, where one does
    const size_t name1 = d_internal_path_root_name_len(
                             _path1.text,
                             d_internal_path_root_len(_path1.text));
    const size_t name2 = d_internal_path_root_name_len(_path2.text,
                                                       _root2);

    // a root directory alone: the first path's volume, then the second
    if (name2 == 0)
    {
        const struct d_internal_path_span volume1 = { _path1.text, name1 };

        return d_internal_path_concat(_buf,
                                      _bufsize,
                                      volume1,
                                      0,
                                      _path2);
    }

    // another volume replaces the first path outright
    if (!d_internal_path_same_name(_path1.text,
                                   name1,
                                   _path2.text,
                                   name2))
    {
        return d_internal_path_copy(_buf,
                                    _bufsize,
                                    _path2.text,
                                    _path2.length);
    }

    // the same volume: what follows the second's name joins the first
    const struct d_internal_path_span rest2 = { _path2.text + name2,
                                                _path2.length - name2 };

    return d_internal_path_join_relative(_buf,
                                         _bufsize,
                                         _path1,
                                         rest2);
}

/*
d_internal_path_ends_in_dotdot
  Whether the last component written is a ".." that had to be kept -- one
with nothing left to climb over -- which a later ".." must keep company with
rather than climb over.
*/
static int
d_internal_path_ends_in_dotdot(
    const char* _buf,
    size_t      _out,
    size_t      _root_len
)
{
    return ( ((_out - _root_len) >= 2) &&
             (_buf[_out - 1] == '.')   &&
             (_buf[_out - 2] == '.')   &&
             ( ((_out - 2) == _root_len) ||
               (_buf[_out - 3] == D_INTERNAL_FILE_PATH_OUT_SEP) ) );
}

/*
d_internal_path_climb
  Backs the written output over its last component and the separators before
it, never into the root, and reports where the output now ends. A "./" left at
the front can only be the one written ahead of a drive-like component
(d_internal_path_looks_like_drive) -- a "." of the input is never written --
so it goes with that component.
*/
static size_t
d_internal_path_climb(
    const char* _buf,
    size_t      _out,
    size_t      _root_len
)
{
    size_t back = _out;

    // back over the last component...
    while ( (back > _root_len) &&
            (_buf[back - 1] != D_INTERNAL_FILE_PATH_OUT_SEP) )
    {
        --back;
    }

    // ...and the separators before it
    while ( (back > _root_len) &&
            (_buf[back - 1] == D_INTERNAL_FILE_PATH_OUT_SEP) )
    {
        --back;
    }

    // ...and the "./" that guarded it, if that is all that is left
    if ( (_root_len == 0) &&
         (back == 1)      &&
         (_buf[0] == '.') )
    {
        back = 0;
    }

    return back;
}

/*
d_internal_path_put_seps
  Writes _count of this build's separators at _out and reports where the
output now ends.
*/
static size_t
d_internal_path_put_seps(
    char*  _buf,
    size_t _out,
    size_t _count
)
{
    // one separator per count
    for (size_t idx = 0; idx < _count; ++idx)
    {
        _buf[_out++] = D_INTERNAL_FILE_PATH_OUT_SEP;
    }

    return _out;
}

/*
d_internal_path_separate
  Writes the separators that go before a component -- _run, the input's run
of them -- and reports where the output now ends. After another component
they are the run, collapsed to one under D_CFG_FILE_PATH_COLLAPSE_SEPARATORS
and kept whole without it. Directly after the root, a run that follows the
root in the input is kept whole without collapsing ("///a" stays "///a");
otherwise only a share takes a separator there, since a root directory ends
in one and a bare drive takes its component directly ("C:a\\..\\b" is "C:b").
*/
static size_t
d_internal_path_separate(
    char*                       _buf,
    size_t                      _out,
    struct d_internal_path_span _root,
    struct d_internal_path_span _run
)
{
    const int collapse = D_CFG_IS_ON(D_CFG_FILE_PATH_COLLAPSE_SEPARATORS);

    // after a component: the run, or one separator for it
    if (_out > _root.length)
    {
        return d_internal_path_put_seps(_buf,
                                        _out,
                                        collapse ? 1 : _run.length);
    }

    // a run straight after the root, kept whole where runs are kept
    if ( (!collapse) &&
         (_run.text == (_root.text + _root.length)) )
    {
        return d_internal_path_put_seps(_buf,
                                        _out,
                                        _run.length);
    }

    // otherwise the root decides: only a share needs one
    if (d_internal_path_root_needs_sep(_root.text,
                                       _root.length))
    {
        _buf[_out++] = D_INTERNAL_FILE_PATH_OUT_SEP;
    }

    return _out;
}

/*
d_internal_path_trailing
  The separators that end a path -- _run, the input's last run of them --
dropped under D_CFG_FILE_PATH_STRIP_TRAILING_SEP, and otherwise kept: one, or
the whole run where runs are kept, after the last component, or after a
share, whose root directory they are. A root that ends in a separator already
keeps itself as it is.
*/
static size_t
d_internal_path_trailing(
    char*                       _buf,
    size_t                      _out,
    struct d_internal_path_span _root,
    struct d_internal_path_span _run
)
{
    const int collapse = D_CFG_IS_ON(D_CFG_FILE_PATH_COLLAPSE_SEPARATORS);

    // stripped, or nothing to follow but a root that needs none
    if ( (D_CFG_IS_ON(D_CFG_FILE_PATH_STRIP_TRAILING_SEP)) ||
         ( (_out <= _root.length) &&
           (!d_internal_path_root_needs_sep(_root.text,
                                            _root.length)) ) )
    {
        return _out;
    }

    // kept: the separators after the last component
    return d_internal_path_put_seps(_buf,
                                    _out,
                                    collapse ? 1 : _run.length);
}

/*
d_internal_path_looks_like_drive
  Whether a component written first in a relative path would read back as a
drive: a letter and a colon. In "./D:" the component is a name; written as
"D:" it would be drive D's current directory instead, so normalize keeps a
"./" ahead of it, as Python's ntpath.normpath does.
*/
static int
d_internal_path_looks_like_drive(
    const char* _seg,
    size_t      _len
)
{
    return ( (D_INTERNAL_FILE_PATH_HAS_DRIVE == 1) &&
             (_len >= 2)                           &&
             (d_internal_path_is_letter(_seg[0]))  &&
             (_seg[1] == ':') );
}

/*
d_internal_path_is_dot
  Whether the _len bytes at _seg are "." (_dots 1) or ".." (_dots 2).
*/
static int
d_internal_path_is_dot(
    const char* _seg,
    size_t      _len,
    size_t      _dots
)
{
    return ( (_len == _dots) &&
             (_seg[0] == '.') &&
             ( (_dots == 1) ||
               (_seg[1] == '.') ) );
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
component starts, past the root -- a root is no component, so "C:.txt" names
".txt", whose dot hides it -- and the second finds its last dot. A dot that
opens the name is hiding the file, not typing it, so it starts no extension.
A path that ends in a separator has an empty final name here, and so no
extension: the result is a suffix of _path, which would carry the separator.
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

    const char* name   = _path + d_internal_path_root_len(_path);
    const char* cursor = name;

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
is written into _buf, and the extension of the same path says how much of its
end to cut. Where there is an extension the path does not end in a separator,
so the extension is the end of the basename, and stem and extension can never
disagree.
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
    const char* const ext = d_path_extension(_path);

    // cut the extension off the end of the basename
    if (ext)
    {
        _buf[strlen(_buf) - strlen(ext)] = '\0';
    }

    return _buf;
}

/*
d_path_join
  An absent component leaves the other standing alone. With
D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS a second component with a root decides how
much of the first survives (d_internal_path_join_rooted) -- the alternative
silently builds "/base/etc/passwd" for code that meant to be handed an
absolute override. Otherwise the two are joined as text, with exactly one
separator between them where the first is not a root that needs none.
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

    const size_t len1 = _path1 ? strlen(_path1) : 0;
    const size_t len2 = _path2 ? strlen(_path2) : 0;

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

    const struct d_internal_path_span path1 = { _path1, len1 };
    const struct d_internal_path_span path2 = { _path2, len2 };
    const size_t                      root2 = d_internal_path_root_len(_path2);

    // a second component with a root decides what of the first survives
    if ( (D_CFG_IS_ON(D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS)) &&
         (root2 > 0) )
    {
        return d_internal_path_join_rooted(_buf,
                                           _bufsize,
                                           path1,
                                           path2,
                                           root2);
    }

    return d_internal_path_join_relative(_buf,
                                         _bufsize,
                                         path1,
                                         path2);
}

/*
d_path_is_absolute
  Absolute means anchored: a root directory on a named volume, or a share.
Under Windows syntax neither a bare drive ("C:x", x in drive C's current
directory) nor a root directory alone ("\\x", x at the root of the current
drive) is anchored -- each leaves the volume or the directory to the process,
as std::filesystem and Python's ntpath both hold. Under POSIX syntax every
root is absolute.
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

    // no root, or a bare drive: not absolute
    if ( (root_len == 0) ||
         (d_internal_path_is_bare_drive(_path,
                                        root_len)) )
    {
        return 0;
    }

    // a lone separator roots the path on the current drive, under Windows
    return !( (D_INTERNAL_FILE_PATH_HAS_DRIVE == 1) &&
              (root_len == 1) );
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
  One pass over the input, writing components into _buf as they survive, each
with the separators before it (d_internal_path_separate). The root is copied
through with only its separators respelled: they are structure, not
punctuation, and normalizing "//" or a "\\?\" prefix would change their
meaning. "." means "stay here", and goes with the separators before it. ".."
removes the last written component unless there is nothing to climb over or
that component is itself a ".." that had to be kept; directly above a root
that anchors -- any but a bare drive -- it is dropped, as POSIX says "/.." is
"/". A result with nothing left is ".", because "" is not a path, and a
relative result that would begin with a drive begins with "./" instead
(d_internal_path_looks_like_drive). Output is never longer than input, so the
input's size bounds the buffer, and _buf may be _path itself.
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
    const int    anchored = (root_len > 0) &&
                            (!d_internal_path_is_bare_drive(_path,
                                                            root_len));

    const struct d_internal_path_span root = { _path, root_len };

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

    // the root, with only its separators respelled
    for (size_t idx = 0; idx < root_len; ++idx)
    {
        _buf[idx] = d_internal_path_is_sep(_path[idx])
                        ? (char)D_INTERNAL_FILE_PATH_OUT_SEP
                        : _path[idx];
    }

    size_t out = root_len;
    size_t idx = root_len;

    // each component after the root, with the separators before it
    while (idx < length)
    {
        const size_t run_start = idx;

        // the separators before the next component
        while ( (idx < length) &&
                (d_internal_path_is_sep(_path[idx])) )
        {
            ++idx;
        }

        const struct d_internal_path_span run = { _path + run_start,
                                                  idx - run_start };

        // separators with no component after them end the path
        if (idx == length)
        {
            out = d_internal_path_trailing(_buf,
                                           out,
                                           root,
                                           run);

            break;
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
        if (d_internal_path_is_dot(_path + seg_start,
                                   seg_len,
                                   1))
        {
            continue;
        }

        // ".." climbs, where it may: over a component that is not a kept
        // "..", and not at all above a root that anchors
        if ( (D_CFG_IS_ON(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)) &&
             (d_internal_path_is_dot(_path + seg_start,
                                     seg_len,
                                     2)) )
        {
            // a component to climb over
            if ( (out > root_len) &&
                 (!d_internal_path_ends_in_dotdot(_buf,
                                                  out,
                                                  root_len)) )
            {
                out = d_internal_path_climb(_buf,
                                            out,
                                            root_len);

                continue;
            }

            // the root has no parent; POSIX says "/.." is "/"
            if ( (out == root_len) &&
                 (anchored) )
            {
                continue;
            }
        }

        out = d_internal_path_separate(_buf,
                                       out,
                                       root,
                                       run);

        // a first component that would read back as a drive keeps a "./"
        // ahead of it; the input had at least that much before it, since a
        // drive there would have been the root
        if ( (out == 0) &&
             (d_internal_path_looks_like_drive(_path + seg_start,
                                               seg_len)) )
        {
            _buf[out++] = '.';
            _buf[out++] = D_INTERNAL_FILE_PATH_OUT_SEP;
        }

        memmove(_buf + out,
                _path + seg_start,
                seg_len);
        out += seg_len;
    }

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
