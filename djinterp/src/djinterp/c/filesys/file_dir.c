/*******************************************************************************
* djinterp [c]                                                        file_dir.c
*
* Implementation of the directory operations declared in file_dir.h.
*   struct d_dir_t wraps the platform's handle -- a DIR* on POSIX, a
* FindFirstFile search on Windows -- and owns the entry d_dir_read64 hands
* out, so the entry's lifetime is the handle's on every target. A build with
* neither has no directory API to wrap, and every call that needs one reports
* ENOSYS. Each platform's calls sit in helpers of their own. Entry types
* come from the kernel where it reports them, and from one stat per entry
* where it does not.
*   d_dir_read32 reads through d_dir_read64 and narrows into a second entry
* the handle owns, so one handle serves both widths; d_dir_read_name reads
* through it too and narrows nothing.
*   Every path built here is for the running kernel to open, so it is built
* with the kernel's separators and roots (d_dir_entry_path) rather than with
* file_path's grammar, which D_CFG_FILE_PATH_SYNTAX may set to the other
* platform's.
*
*
* path:      /src/djinterp/c/filesys/file_dir.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_dir.h"  // corresponding header
// std
#include <errno.h>   // errno, EACCES, EEXIST, EINVAL, EIO, ENAMETOOLONG,
                     // ENOENT, ENOMEM, ENOTDIR, EOVERFLOW, ERANGE
#include <limits.h>  // INT_MAX
#include <stddef.h>  // NULL, size_t
#include <stdlib.h>  // free, realpath
#include <string.h>  // memcpy, memmove, memset, strlen
#if D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    #include <fcntl.h>     // openat, O_DIRECTORY, O_NOFOLLOW, O_CLOEXEC
    #include <sys/stat.h>  // fstatat, S_ISLNK
    #include <unistd.h>    // unlinkat, close
#endif
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*, d_internal_file_alloc
#include "../../../../inc/djinterp/c/filesys/file_path.h"    // d_path_root_length
#include "../../../../inc/djinterp/c/filesys/file_ops.h"     // d_file_unlink
#include "../../../../inc/djinterp/c/filesys/file_stat.h"    // d_file_stat_nofollow64, d_dir_exists
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_dir.h"  // D_INTERNAL_FILE_DIR_*
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint32_t,
                                                     // uint64_t, UINT32_MAX


// the large-file interface this source needs (file_common.h, 6.4.1)
D_INTERNAL_FILE_ASSERT_LARGE_FILES;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// The Win32 calls, in the form the build's path text takes: the W functions
// and the CRT's _w calls where the build hands Windows UTF-16
// (D_INTERNAL_FILE_WIDE), the A functions and the plain calls otherwise.

// D_INTERNAL_DIR_FIND
//   macro: the find data a search hands back.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_DIR_FIND WIN32_FIND_DATAW
#else
    #define D_INTERNAL_DIR_FIND WIN32_FIND_DATAA
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_DIR_FIND_FIRST
//   macro: FindFirstFile.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_DIR_FIND_FIRST FindFirstFileW
#else
    #define D_INTERNAL_DIR_FIND_FIRST FindFirstFileA
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_DIR_FIND_NEXT
//   macro: FindNextFile.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_DIR_FIND_NEXT FindNextFileW
#else
    #define D_INTERNAL_DIR_FIND_NEXT FindNextFileA
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_DIR_ATTRS
//   macro: GetFileAttributes.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_DIR_ATTRS GetFileAttributesW
#else
    #define D_INTERNAL_DIR_ATTRS GetFileAttributesA
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_DIR_FULL_PATH
//   macro: GetFullPathName.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_DIR_FULL_PATH GetFullPathNameW
#else
    #define D_INTERNAL_DIR_FULL_PATH GetFullPathNameA
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_DIR_MKDIR
//   macro: the CRT's _mkdir.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_DIR_MKDIR _wmkdir
#else
    #define D_INTERNAL_DIR_MKDIR _mkdir
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_DIR_RMDIR
//   macro: the CRT's _rmdir.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_DIR_RMDIR _wrmdir
#else
    #define D_INTERNAL_DIR_RMDIR _rmdir
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_DIR_CHDIR
//   macro: the CRT's _chdir.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_DIR_CHDIR _wchdir
#else
    #define D_INTERNAL_DIR_CHDIR _chdir
#endif  // D_INTERNAL_FILE_WIDE == 1

// d_dir_t
//   struct: the opaque directory handle promised by file_common.h. It owns the
// entries the readers hand back, which makes that pointer's lifetime a
// property of the HANDLE rather than of the platform: POSIX readdir returns a
// pointer into the DIR's own storage and says almost nothing about how long
// it lasts, while a copy here gives one answer on every target -- valid until
// the next read on this handle, dead at d_dir_close. There is one entry per
// width, since d_dir_read32 narrows what d_dir_read64 has just filled.
struct d_dir_t
{
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    HANDLE              handle;        // FindFirstFile handle
    HANDLE              pin;           // the directory, held (d_dir_open_at)
    D_INTERNAL_DIR_FIND find_data;     // the entry Win32 hands back
    int                 pending;       // 1 when find_data is unread
    int                 empty;         // 1 when nothing matched
    char*               pattern;       // for rewind: the path and "\\*"
    size_t              pattern_room;  // pattern's bytes
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    DIR*                handle;
    int                 fd;            // handle's descriptor, for the *at calls                    // the platform's stream
#else
    void*               handle;                    // none: no directory API
#endif
    struct d_dirent64_t entry;         // what d_dir_read64 returns
    struct d_dirent32_t entry32;       // what d_dir_read32 returns
    char*               path;          // for stat and the search
    char                text[];        // path, then the pattern: as long as
                                       // they are, not D_FILE_PATH_MAX
};


/**
 * @brief A handle for a directory at `_path`, its text sized to the path.
 *
 * @param[in] _path    the path, `_length` bytes and a terminator.
 * @param[in] _length  its length.
 * @return the handle, zeroed but for its path (and, on Windows, its pattern's
 *         room and an empty hold), or `NULL` with errno set.
 */
static struct d_dir_t*
d_internal_dir_new(
    const char* _path,
    size_t      _length
)
{
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // the path, then the pattern: the path, a separator, "*", a terminator
    const size_t room = (_length + 1) + (_length + 3);
#else
    const size_t room = _length + 1;
#endif
    struct d_dir_t* const dir =
        (struct d_dir_t*)d_internal_file_alloc(sizeof(struct d_dir_t) + room);

    // d_internal_file_alloc has reported the failure
    if (!dir)
    {
        return NULL;
    }

    memset(dir,
           0,
           sizeof(*dir));
    dir->path = dir->text;
    memcpy(dir->path,
           _path,
           _length + 1);
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    dir->pin          = INVALID_HANDLE_VALUE;
    dir->pattern      = dir->text + _length + 1;
    dir->pattern_room = _length + 3;
#endif

    return dir;
}

/*
d_internal_dir_is_sep
  A separator as the running kernel reads one: D_FILE_PATH_SEP, or the
alternate that Windows accepts as well. file_path answers the same question
for the configured grammar, which on a POSIX build reading Windows paths is
a different question.
*/
static int
d_internal_dir_is_sep(
    char _c
)
{
    return ( (_c == D_FILE_PATH_SEP) ||
             (_c == D_FILE_PATH_SEP_ALT) );
}

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_dir_root_len
  The root of a path as the kernel reads it. On Windows that is file_path's
answer, since cfg_file_path insists on Windows syntax there.
*/
static size_t
d_internal_dir_root_len(
    const char* _path
)
{
    return d_path_root_length(_path);
}

#else

/*
d_internal_dir_root_len
  The root of a path as the kernel reads it: its leading slashes, however many
there are. file_path answers for the configured grammar instead, and a POSIX
build told to read Windows syntax would take "//srv/share" for a share where
the kernel sees the root and two directories.
*/
static size_t
d_internal_dir_root_len(
    const char* _path
)
{
    size_t idx = 0;

    // every leading slash belongs to the root
    while (_path[idx] == '/')
    {
        ++idx;
    }

    return idx;
}

#endif  // D_CFG_FILE_HAS_WIN32

/*
d_internal_dir_needs_sep
  Whether a separator goes between a directory and an entry's name: not after
an empty directory or one that ends in a separator already, and -- on Windows
-- not after a bare drive, since "C:" names drive C's current directory and
"C:x" is x inside it.
*/
static int
d_internal_dir_needs_sep(
    const char* _dir,
    size_t      _length
)
{
    // nothing to separate from, or a separator there already
    if ( (_length == 0) ||
         (d_internal_dir_is_sep(_dir[_length - 1])) )
    {
        return 0;
    }

    // a bare drive takes its entry directly, on Windows alone
    return !( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) &&
              (_length == 2)                      &&
              (_dir[1] == ':') );
}

/*
d_internal_dir_join
  d_dir_entry_path's work, without its validation: the directory, a separator
where one is needed, then the name -- or ERANGE when they do not fit.
*/
static char*
d_internal_dir_join(
    char*       _buf,
    size_t      _bufsize,
    const char* _dir,
    const char* _name
)
{
    const size_t dir_len  = strlen(_dir);
    const size_t name_len = strlen(_name);
    const size_t sep      = (size_t)d_internal_dir_needs_sep(_dir,
                                                             dir_len);

    // the whole path and its terminator must fit; a path is never truncated
    if ((dir_len + sep + name_len + 1) > _bufsize)
    {
        D_INTERNAL_FILE_FAIL(ERANGE,
                             "d_dir_entry_path",
                             _dir,
                             "entry path does not fit the caller's buffer",
                             NULL);
    }

    memmove(_buf,
            _dir,
            dir_len);

    // one separator between the two, where the directory needs it
    if (sep)
    {
        _buf[dir_len] = D_FILE_PATH_SEP;
    }

    memcpy(_buf + dir_len + sep,
           _name,
           name_len + 1);

    return _buf;
}

#if (D_INTERNAL_FILE_DIR_TYPE_BY_STAT == 1)

/*
d_internal_dir_type_by_stat
  The expensive path, taken only when the kernel would not say: one stat per
entry, which on a network filesystem is one round trip per entry (see
D_CFG_FILE_DIR_FILL_TYPE). fstatat on the handle's descriptor, so the entry is
the one in the directory read; AT_SYMLINK_NOFOLLOW, so a dangling symlink
reports DT_LNK instead of vanishing. An entry that cannot be described is
DT_UNKNOWN.
*/
static uint8_t
d_internal_dir_type_by_stat(
    struct d_dir_t* _dir,
    const char*     _name
)
{
    struct stat st;

    // the entry looked up in the directory read, not along a path built
    // afresh: nothing renamed meanwhile can answer for it, and no path is
    // too long to ask about. Without following it, so a dangling link is
    // still DT_LNK rather than an entry that appears not to exist
    if (fstatat(_dir->fd,
                _name,
                &st,
                AT_SYMLINK_NOFOLLOW) != 0)
    {
        return DT_UNKNOWN;
    }

    // map the mode's file type onto the matching DT_* constant
    if (S_ISREG(st.st_mode))
    {
        return DT_REG;
    }

    if (S_ISDIR(st.st_mode))
    {
        return DT_DIR;
    }

    if (S_ISLNK(st.st_mode))
    {
        return DT_LNK;
    }

    if (S_ISCHR(st.st_mode))
    {
        return DT_CHR;
    }

    if (S_ISBLK(st.st_mode))
    {
        return DT_BLK;
    }

    if (S_ISFIFO(st.st_mode))
    {
        return DT_FIFO;
    }

    if (S_ISSOCK(st.st_mode))
    {
        return DT_SOCK;
    }

    return DT_UNKNOWN;
}

#endif  // D_INTERNAL_FILE_DIR_TYPE_BY_STAT == 1

#if ( ( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) &&                                 \
        (D_INTERNAL_FILE_WIDE != 1) )                                      ||  \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) )

/*
d_internal_dir_copy_name
  Local rather than d_strcpy_s from the string module: this is five lines, and
the alternative is that every consumer of file_dir links dstring for one call.
The filesys subframework's only dependencies should be the ones it genuinely
needs. A name that does not fit is refused rather than truncated.
*/
static int
d_internal_dir_copy_name(
    char*       _dst,
    size_t      _dstsize,
    const char* _src
)
{
    const size_t length = strlen(_src);

    // a name that does not fit is refused, not truncated
    if ((length + 1) > _dstsize)
    {
        return -1;
    }

    memcpy(_dst,
           _src,
           length + 1);

    return 0;
}

#endif  // narrow D_CFG_FILE_HAS_WIN32 || D_CFG_FILE_HAS_POSIX

/*
d_internal_dir_is_dots
  Recognises exactly "." and "..", by their bytes rather than by string
comparison.
*/
static int
d_internal_dir_is_dots(
    const char* _name
)
{
    // both names begin with a dot
    if (_name[0] != '.')
    {
        return 0;
    }

    // "."
    if (_name[1] == '\0')
    {
        return 1;
    }

    // ".."
    if ( (_name[1] == '.') &&
         (_name[2] == '\0') )
    {
        return 1;
    }

    return 0;
}

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_dir_mkdir
  The CRT's _mkdir, which takes no mode: Windows has no permission bits for
directories. It raises no notice -- d_dir_create reports its own failures,
and d_dir_create_parents expects EEXIST and reports only the failures that
are failures.
*/
static int
d_internal_dir_mkdir(
    const char* _path,
    uint32_t    _mode
)
{
    // Windows has no permission bits for directories
    (void)_mode;

    struct d_internal_file_native        held = {0};
    const d_internal_file_nchar_t* const path =
        d_internal_file_native_of(&held,
                                  _path);

    // the conversion has set errno
    if (!path)
    {
        return -1;
    }

    const int result = D_INTERNAL_DIR_MKDIR(path);

    d_internal_file_native_release(&held);

    return result;
}

/*
d_internal_dir_rmdir
  The CRT's _rmdir.
*/
static int
d_internal_dir_rmdir(
    const char* _path
)
{
    struct d_internal_file_native        held = {0};
    const d_internal_file_nchar_t* const path =
        d_internal_file_native_of(&held,
                                  _path);

    // the conversion has set errno
    if (!path)
    {
        return -1;
    }

    const int result = D_INTERNAL_DIR_RMDIR(path);

    d_internal_file_native_release(&held);

    return result;
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)

/*
d_internal_dir_mkdir
  mkdir, with the mode the process umask then reduces. It raises no notice,
for the reason the Windows definition gives.
*/
static int
d_internal_dir_mkdir(
    const char* _path,
    uint32_t    _mode
)
{
    return mkdir(_path,
                 (mode_t)_mode);
}

/*
d_internal_dir_rmdir
  rmdir.
*/
static int
d_internal_dir_rmdir(
    const char* _path
)
{
    return rmdir(_path);
}

#else

/*
d_internal_dir_mkdir
  Neither POSIX nor Windows is here, and ISO C makes no directories, so this
reports ENOSYS, as every directory operation of such a build does.
*/
static int
d_internal_dir_mkdir(
    const char* _path,
    uint32_t    _mode
)
{
    (void)_path;
    (void)_mode;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

/*
d_internal_dir_rmdir
  ENOSYS, as d_internal_dir_mkdir.
*/
static int
d_internal_dir_rmdir(
    const char* _path
)
{
    (void)_path;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

#endif  // D_CFG_FILE_HAS_WIN32, D_CFG_FILE_HAS_POSIX

/*
d_internal_dir_ensure
  One step of d_dir_create_parents: makes the directory _path names, or
accepts one already there -- the normal case, since another process building
the same tree is expected. What exists must be a directory, and for the leaf
it must not have existed at all when D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK is 0.
Failures are reported against _whole, the path the caller asked for.
*/
static int
d_internal_dir_ensure(
    const char* _path,
    uint32_t    _mode,
    int         _leaf,
    const char* _whole
)
{
    // named only in the notices, which a build may compile out
    (void)_whole;

    // made now
    if (d_internal_dir_mkdir(_path,
                             _mode) == 0)
    {
        return 0;
    }

    // a failure other than finding something there already
    if (errno != EEXIST)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_create_parents",
                               D_INTERNAL_FILE_NOTIFY_PATH(_whole),
                               "could not create a directory on the path");

        return -1;
    }

    // EEXIST on a NON-directory is a real failure: the path cannot be brought
    // into existence as a directory
    if (!d_dir_exists(_path))
    {
        D_INTERNAL_FILE_FAIL(ENOTDIR,
                             "d_dir_create_parents",
                             _whole,
                             "a component exists and is not a directory",
                             -1);
    }

    // the caller asked for mkdir semantics on the leaf
    if ( (_leaf) &&
         (D_CFG_IS_OFF(D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK)) )
    {
        D_INTERNAL_FILE_FAIL(EEXIST,
                             "d_dir_create_parents",
                             _whole,
                             "target exists and EXISTING_OK is 0",
                             -1);
    }

    return 0;
}

/*
d_internal_dir_ensure_root
  d_dir_create_parents for a path that is nothing but a root, which is never
made: it is there or it is not -- a drive that is not mounted, a share that
is not reachable. Being there satisfies the request unless
D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK is 0.
*/
static int
d_internal_dir_ensure_root(
    const char* _root,
    const char* _whole
)
{
    // named only in the notices, which a build may compile out
    (void)_whole;

    // a root cannot be made, so one that is not there is a failure
    if (!d_dir_exists(_root))
    {
        D_INTERNAL_FILE_FAIL(ENOENT,
                             "d_dir_create_parents",
                             _whole,
                             "the root does not exist",
                             -1);
    }

    // the caller asked for mkdir semantics on the leaf, and the root is it
    if (D_CFG_IS_OFF(D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK))
    {
        D_INTERNAL_FILE_FAIL(EEXIST,
                             "d_dir_create_parents",
                             _whole,
                             "target exists and EXISTING_OK is 0",
                             -1);
    }

    return 0;
}

// D_INTERNAL_DIR_DONE
//   constant: d_internal_dir_next found the end of the directory, or failed and
// has said so.
#define D_INTERNAL_DIR_DONE    0

// D_INTERNAL_DIR_TAKEN
//   constant: d_internal_dir_next put the next entry in the handle's own.
#define D_INTERNAL_DIR_TAKEN   1

// D_INTERNAL_DIR_SKIPPED
//   constant: d_internal_dir_next passed over an entry it cannot report.
#define D_INTERNAL_DIR_SKIPPED 2

#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) ||                                   \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) )

/*
d_internal_dir_skip_long
  Passes over an entry whose name is longer than D_FILE_NAME_MAX: it cannot be
reported, and a truncated name would name another file -- so the caller is
told, and the next entry is read.
*/
static int
d_internal_dir_skip_long(
    void
)
{
    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                           ENAMETOOLONG,
                           "d_dir_read64",
                           NULL,
                           "entry name exceeds D_FILE_NAME_MAX; skipped");

    return D_INTERNAL_DIR_SKIPPED;
}

#endif  // D_CFG_FILE_HAS_WIN32 || D_CFG_FILE_HAS_POSIX

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_dir_search
  Starts the handle's search, or starts it again. FindFirstFile both opens AND
reads the first entry, so that entry is marked pending for the next read to
hand out -- skipping it is the classic bug in every hand-rolled Win32 dirent
shim. A search that matches nothing is no failure when the directory is
there: a drive's root or a share lists no "." or "..", so an empty one matches
nothing at all, and the handle then reads as empty. A failed search asks what
the path names, so that a file fails with ENOTDIR, as POSIX opendir fails it.
*/
static int
d_internal_dir_search(
    struct d_dir_t* _dir
)
{
    struct d_internal_file_native        held = {0};
    const d_internal_file_nchar_t* const pattern =
        d_internal_file_native_of(&held,
                                  _dir->pattern);

    // the conversion has set errno, and no search has started
    if (!pattern)
    {
        _dir->handle  = INVALID_HANDLE_VALUE;
        _dir->pending = 0;
        _dir->empty   = 0;

        return -1;
    }

    _dir->handle  = D_INTERNAL_DIR_FIND_FIRST(pattern,
                                              &_dir->find_data);

    d_internal_file_native_release(&held);

    _dir->pending = (_dir->handle != INVALID_HANDLE_VALUE);
    _dir->empty   = 0;

    // the search started, its first entry pending
    if (_dir->pending)
    {
        return 0;
    }

    // read before the conversion, which can set it
    const DWORD error  = GetLastError();
    const d_internal_file_nchar_t* const path =
        d_internal_file_native_of(&held,
                                  _dir->path);
    const DWORD attrs  = (path) ? D_INTERNAL_DIR_ATTRS(path)
                                : INVALID_FILE_ATTRIBUTES;

    d_internal_file_native_release(&held);

    const int   exists = (attrs != INVALID_FILE_ATTRIBUTES);
    const int   is_dir = (exists) &&
                         ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);

    // nothing matched in a directory that is there: an empty root
    if ( (error == ERROR_FILE_NOT_FOUND) &&
         (is_dir) )
    {
        _dir->empty = 1;

        return 0;
    }

    // something is there and is no directory, whichever reason the search
    // gave -- that varies with the Windows version
    const int code = ( (exists) &&
                       (!is_dir) ) ? ENOTDIR
                                   : d_internal_file_errno_win32(error);

    // named only by errno, which a build may not report through
    D_INTERNAL_FILE_SET_ERR(code);
    (void)code;

    return -1;
}

/*
d_internal_dir_start
  Win32 has no opendir: the handle's view of the directory is a search over
every entry of it (d_internal_dir_search), its pattern built for the kernel.
*/
static int
d_internal_dir_start(
    struct d_dir_t* _dir
)
{
    // every entry of the directory, as a pattern the kernel reads
    if (!d_internal_dir_join(_dir->pattern,
                             _dir->pattern_room,
                             _dir->path,
                             "*"))
    {
        return -1;
    }

    return d_internal_dir_search(_dir);
}

/*
d_internal_dir_restart
  Win32 has no rewind either: the search is closed and started again, and
that can fail.
*/
static int
d_internal_dir_restart(
    struct d_dir_t* _dir
)
{
    // the old search goes first, where there was one
    if (_dir->handle != INVALID_HANDLE_VALUE)
    {
        FindClose(_dir->handle);
    }

    return d_internal_dir_search(_dir);
}

/*
d_internal_dir_win32_type
  An entry's type from its find data, when the build fills types: only a
symbolic link or a junction is a link -- every other reparse point is the file
or directory it appears to be, except one without a tag to tell, which is taken
for a link, as d_internal_file_is_link_win32 takes it.
*/
static uint8_t
d_internal_dir_win32_type(
    const D_INTERNAL_DIR_FIND* _data
)
{
    // the build does not fill types
    if (D_CFG_IS_OFF(D_CFG_FILE_DIR_FILL_TYPE))
    {
        return DT_UNKNOWN;
    }

    // a link, or a reparse point with no tag to say otherwise
    if ( ((_data->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) &&
         ( (_data->dwReserved0 == 0) ||
           (d_internal_file_tag_is_link_win32(_data->dwReserved0)) ) )
    {
        return DT_LNK;
    }

    return ((_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
               ? (uint8_t)DT_DIR
               : (uint8_t)DT_REG;
}

#if (D_INTERNAL_FILE_WIDE == 1)

/**
 * @brief An entry's UTF-16 name, as the UTF-8 the caller reads.
 *
 * @param[out] _dst      receives the name and its terminator.
 * @param[in]  _dstsize  capacity of `_dst`, in bytes.
 * @param[in]  _src      the name the search handed back.
 * @return D_INTERNAL_DIR_TAKEN, or D_INTERNAL_DIR_SKIPPED for a name that
 *         cannot be reported -- longer than D_FILE_NAME_MAX in UTF-8 -- after a
 *         notice saying so. A lone surrogate is no longer such a name: it
 *         comes back in WTF-8 (d_internal_file_from_native).
 *
 * errno is the caller's again afterwards: a skipped name is no failure of the
 * read, which goes on to the next entry.
 */
static int
d_internal_dir_take_name(
    char*        _dst,
    size_t       _dstsize,
    const WCHAR* _src
)
{
    const int saved  = errno;
    const int result = d_internal_file_from_native(_src,
                                                   _dst,
                                                   _dstsize);
    const int code   = errno;

    errno = saved;

    // reported as it is
    if (result == 0)
    {
        return D_INTERNAL_DIR_TAKEN;
    }

    // too long, as in any build
    if (code != EILSEQ)
    {
        return d_internal_dir_skip_long();
    }

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                           EILSEQ,
                           "d_dir_read64",
                           NULL,
                           "entry name UTF-8 cannot carry; skipped");

    return D_INTERNAL_DIR_SKIPPED;
}

#else

/**
 * @brief An entry's name, as the search handed it back.
 *
 * @param[out] _dst      receives the name and its terminator.
 * @param[in]  _dstsize  capacity of `_dst`, in bytes.
 * @param[in]  _src      the name the search handed back.
 * @return D_INTERNAL_DIR_TAKEN, or D_INTERNAL_DIR_SKIPPED after a notice for
 *         a name longer than D_FILE_NAME_MAX.
 */
static int
d_internal_dir_take_name(
    char*       _dst,
    size_t      _dstsize,
    const char* _src
)
{
    // a name longer than D_FILE_NAME_MAX cannot be reported
    if (d_internal_dir_copy_name(_dst,
                                 _dstsize,
                                 _src) != 0)
    {
        return d_internal_dir_skip_long();
    }

    return D_INTERNAL_DIR_TAKEN;
}

#endif  // D_INTERNAL_FILE_WIDE == 1

/*
d_internal_dir_next
  The next entry of the search. The one FindFirstFile produced is handed out
before FindNextFile is asked for another -- skipping it is the classic bug in
every hand-rolled Win32 dirent shim. FindNextFile failing other than at the
end sets EIO, since a caller who cleared errno would read an untouched one as
the end.
*/
static int
d_internal_dir_next(
    struct d_dir_t* _dir
)
{
    // nothing pending: a search that matched nothing, or ask for another
    if ( (!_dir->pending) &&
         ( (_dir->empty) ||
           (!D_INTERNAL_DIR_FIND_NEXT(_dir->handle,
                                      &_dir->find_data)) ) )
    {
        // ERROR_NO_MORE_FILES is the end; anything else failed
        if ( (!_dir->empty) &&
             (GetLastError() != ERROR_NO_MORE_FILES) )
        {
            D_INTERNAL_FILE_SET_ERR(EIO);
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   EIO,
                                   "d_dir_read64",
                                   NULL,
                                   "FindNextFile failed");
        }

        return D_INTERNAL_DIR_DONE;
    }

    _dir->pending = 0;

    const int taken = d_internal_dir_take_name(_dir->entry.d_name,
                                               sizeof(_dir->entry.d_name),
                                               _dir->find_data.cFileName);

    // a name that cannot be reported is skipped (d_internal_dir_take_name)
    if (taken != D_INTERNAL_DIR_TAKEN)
    {
        return taken;
    }

    _dir->entry.d_ino  = 0;   // Win32 has no inode to report
    _dir->entry.d_type = d_internal_dir_win32_type(&_dir->find_data);

    return D_INTERNAL_DIR_TAKEN;
}

/*
d_internal_dir_release
  Ends the handle's search, where one started. FindClose sets no errno, so its
failure is recorded as EBADF.
*/
static int
d_internal_dir_release(
    struct d_dir_t* _dir
)
{
    // a search that never started has nothing to close
    if (_dir->handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    // FindClose sets no errno; name the failure
    if (!FindClose(_dir->handle))
    {
        D_INTERNAL_FILE_SET_ERR(EBADF);

        return -1;
    }

    return 0;
}

#if (D_INTERNAL_FILE_WIDE == 1)

/**
 * @brief The working directory, asked for in UTF-16 and given back as UTF-8.
 *
 * @param[out] _buf   receives the directory.
 * @param[in]  _size  capacity of `_buf`, in bytes.
 * @return `_buf`, or `NULL` with errno set: ERANGE for a directory that does
 *         not fit, whether D_FILE_PATH_MAX characters or `_size` bytes, and
 *         EILSEQ for one UTF-8 cannot carry.
 */
static char*
d_internal_dir_getcwd(
    char*  _buf,
    size_t _size
)
{
    struct d_internal_file_native  held = {0};
    d_internal_file_nchar_t* const room = d_internal_file_native_room(&held);

    // d_internal_file_alloc has reported the failure
    if (!room)
    {
        return NULL;
    }

    // the CRT's answer, then UTF-8 for the caller; either sets errno
    const int given = ( (_wgetcwd(room,
                                  (int)D_INTERNAL_FILE_NATIVE_MAX) != NULL) &&
                        (d_internal_file_from_native(room,
                                                     _buf,
                                                     _size) == 0) );

    d_internal_file_native_release(&held);

    return (given) ? _buf : NULL;
}

#else

/*
d_internal_dir_getcwd
  The CRT's _getcwd, which takes an int: a larger buffer is offered as INT_MAX
bytes.
*/
static char*
d_internal_dir_getcwd(
    char*  _buf,
    size_t _size
)
{
    return _getcwd(_buf,
                   (int)( (_size > (size_t)INT_MAX) ? (size_t)INT_MAX
                                                    : _size ));
}

#endif  // D_INTERNAL_FILE_WIDE == 1

/*
d_internal_dir_chdir
  The CRT's _chdir.
*/
static int
d_internal_dir_chdir(
    const char* _path
)
{
    struct d_internal_file_native        held = {0};
    const d_internal_file_nchar_t* const path =
        d_internal_file_native_of(&held,
                                  _path);

    // the conversion has set errno
    if (!path)
    {
        return -1;
    }

    const int result = D_INTERNAL_DIR_CHDIR(path);

    d_internal_file_native_release(&held);

    return result;
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)

/*
d_internal_dir_start
  The handle's view of the directory: the platform's DIR stream.
*/
static int
d_internal_dir_start(
    struct d_dir_t* _dir
)
{
    // open, then fdopendir, rather than opendir: the descriptor is kept for
    // the directory-relative calls, as dirfd is not everywhere to be had
    const int fd = open(_dir->path,
                        O_RDONLY | O_DIRECTORY | O_CLOEXEC);

    if (fd < 0)
    {
        return -1;
    }

    _dir->handle = fdopendir(fd);

    // fdopendir leaves the descriptor to the caller when it fails
    if (!_dir->handle)
    {
        const int code = errno;

        (void)close(fd);
        errno = code;

        return -1;
    }

    _dir->fd = fd;

    return 0;
}

/*
d_internal_dir_restart
  rewinddir, which cannot fail.
*/
static int
d_internal_dir_restart(
    struct d_dir_t* _dir
)
{
    rewinddir(_dir->handle);

    return 0;
}

#if (D_INTERNAL_FILE_DIR_TYPE_FROM_KERNEL == 1)

/*
d_internal_dir_kernel_type
  The type the kernel put in the entry, which may be DT_UNKNOWN.
*/
static uint8_t
d_internal_dir_kernel_type(
    const struct dirent* _native
)
{
    return (uint8_t)_native->d_type;
}

#else

/*
d_internal_dir_kernel_type
  This build's dirent carries no type, or it fills none: DT_UNKNOWN.
*/
static uint8_t
d_internal_dir_kernel_type(
    const struct dirent* _native
)
{
    (void)_native;

    return DT_UNKNOWN;
}

#endif  // D_INTERNAL_FILE_DIR_TYPE_FROM_KERNEL == 1

#if (D_INTERNAL_FILE_DIR_TYPE_BY_STAT == 1)

/*
d_internal_dir_posix_type
  The kernel's answer, and where it is DT_UNKNOWN -- XFS without ftype, most
network filesystems -- one lstat (d_internal_dir_type_by_stat), though never
for "." and "..", which are directories by definition. See
D_CFG_FILE_DIR_FILL_TYPE for what that costs.
*/
static uint8_t
d_internal_dir_posix_type(
    struct d_dir_t*      _dir,
    const struct dirent* _native
)
{
    const uint8_t type = d_internal_dir_kernel_type(_native);

    // the kernel said
    if (type != DT_UNKNOWN)
    {
        return type;
    }

    return d_internal_dir_is_dots(_dir->entry.d_name)
               ? (uint8_t)DT_DIR
               : d_internal_dir_type_by_stat(_dir,
                                             _dir->entry.d_name);
}

#else

/*
d_internal_dir_posix_type
  The kernel's answer alone: this build asks no more.
*/
static uint8_t
d_internal_dir_posix_type(
    struct d_dir_t*      _dir,
    const struct dirent* _native
)
{
    (void)_dir;

    return d_internal_dir_kernel_type(_native);
}

#endif  // D_INTERNAL_FILE_DIR_TYPE_BY_STAT == 1

/*
d_internal_dir_next
  The next entry of the stream. readdir returns NULL for BOTH the end and an
error, and errno is the only way to tell, so it is cleared first: a stale
value from some earlier call cannot then pass for this one's failure. The name
is copied out of the platform's storage into the handle's, so the entry's
lifetime is the handle's.
*/
static int
d_internal_dir_next(
    struct d_dir_t* _dir
)
{
    errno = 0;

    struct dirent* const native = readdir(_dir->handle);

    // the end, or a failure worth reporting
    if (!native)
    {
        // a set errno means readdir failed rather than ran out
        if (errno != 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   errno,
                                   "d_dir_read64",
                                   NULL,
                                   "readdir failed");
        }

        return D_INTERNAL_DIR_DONE;
    }

    // a name longer than D_FILE_NAME_MAX cannot be reported
    if (d_internal_dir_copy_name(_dir->entry.d_name,
                                 sizeof(_dir->entry.d_name),
                                 native->d_name) != 0)
    {
        return d_internal_dir_skip_long();
    }

    _dir->entry.d_ino  = (uint64_t)native->d_ino;
    _dir->entry.d_type = d_internal_dir_posix_type(_dir,
                                                   native);

    return D_INTERNAL_DIR_TAKEN;
}

/*
d_internal_dir_release
  closedir, where the stream opened.
*/
static int
d_internal_dir_release(
    struct d_dir_t* _dir
)
{
    return (_dir->handle != NULL) ? closedir(_dir->handle)
                                  : 0;
}

/*
d_internal_dir_getcwd
  getcwd.
*/
static char*
d_internal_dir_getcwd(
    char*  _buf,
    size_t _size
)
{
    return getcwd(_buf,
                  _size);
}

/*
d_internal_dir_chdir
  chdir.
*/
static int
d_internal_dir_chdir(
    const char* _path
)
{
    return chdir(_path);
}

#else

/*
d_internal_dir_start
  No directory API to open one with: ENOSYS, as d_internal_dir_mkdir.
*/
static int
d_internal_dir_start(
    struct d_dir_t* _dir
)
{
    (void)_dir;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

/*
d_internal_dir_restart
  ENOSYS: no handle could have opened.
*/
static int
d_internal_dir_restart(
    struct d_dir_t* _dir
)
{
    (void)_dir;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

/*
d_internal_dir_next
  ENOSYS: no handle could have opened, so there is nothing to read.
*/
static int
d_internal_dir_next(
    struct d_dir_t* _dir
)
{
    (void)_dir;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return D_INTERNAL_DIR_DONE;
}

/*
d_internal_dir_release
  Nothing opened, so nothing to close.
*/
static int
d_internal_dir_release(
    struct d_dir_t* _dir
)
{
    (void)_dir;

    return 0;
}

/*
d_internal_dir_getcwd
  ISO C has no current directory to ask after: ENOSYS.
*/
static char*
d_internal_dir_getcwd(
    char*  _buf,
    size_t _size
)
{
    (void)_buf;
    (void)_size;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return NULL;
}

/*
d_internal_dir_chdir
  ISO C has no current directory to change: ENOSYS.
*/
static int
d_internal_dir_chdir(
    const char* _path
)
{
    (void)_path;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

#endif  // D_CFG_FILE_HAS_WIN32, D_CFG_FILE_HAS_POSIX

#if (D_INTERNAL_FILE_HAS_REALPATH == 1)

/*
d_internal_dir_resolve
  realpath, into _found's D_FILE_PATH_MAX bytes. Handed a buffer, realpath
writes up to PATH_MAX bytes into it, and overruns one when D_CFG_FILE_PATH_MAX
is set lower -- so it is asked to allocate instead, as POSIX.1-2008 lets it,
and its answer is copied over only when it fits: a longer one fails with
ENAMETOOLONG. The C library's allocation goes back to the C library's free.
*/
static int
d_internal_dir_resolve(
    const char* _path,
    char*       _found
)
{
    char* const answer = realpath(_path,
                                  NULL);

    // realpath has set errno
    if (!answer)
    {
        return -1;
    }

    const size_t length = strlen(answer);

    // an answer that does not fit is refused, never truncated
    if ((length + 1) > D_FILE_PATH_MAX)
    {
        free(answer);
        D_INTERNAL_FILE_SET_ERR(ENAMETOOLONG);

        return -1;
    }

    memcpy(_found,
           answer,
           length + 1);
    free(answer);

    return 0;
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

#if (D_INTERNAL_FILE_WIDE == 1)

/**
 * @brief d_path_resolve's answer on Windows where the build hands it UTF-16:
 *        GetFullPathNameW, given back as UTF-8 in `_found`.
 *
 * @param[in]  _path   the path to resolve, in UTF-8.
 * @param[out] _found  receives the answer; D_FILE_PATH_MAX bytes.
 * @return 0, or -1 with errno set: ENAMETOOLONG for an answer past
 *         D_FILE_PATH_MAX characters or bytes, as realpath's is; EILSEQ for
 *         text that is not UTF-8, or that UTF-8 cannot carry; ENOENT for an
 *         answer that names nothing; Win32's reason, otherwise.
 *
 * As the narrow form: lexical, so its answer is checked for existence.
 */
static int
d_internal_dir_resolve(
    const char* _path,
    char*       _found
)
{
    struct d_internal_file_native        in_held  = {0};
    struct d_internal_file_native        out_held = {0};
    const d_internal_file_nchar_t* const path     =
        d_internal_file_native_of(&in_held,
                                  _path);
    d_internal_file_nchar_t* const       out      =
        (path) ? d_internal_file_native_room(&out_held)
               : NULL;

    // the conversion has set errno, or the allocation reported its failure
    if (!out)
    {
        d_internal_file_native_release(&in_held);

        return -1;
    }

    const DWORD room   = (DWORD)D_INTERNAL_FILE_NATIVE_MAX;
    const DWORD length = D_INTERNAL_DIR_FULL_PATH(path,
                                                  room,
                                                  out,
                                                  NULL);

    // Windows has said why; its reason, in errno's words
    if (length == 0)
    {
        const int code = d_internal_file_errno_win32(GetLastError());

        D_INTERNAL_FILE_SET_ERR(code);
        (void)code;
        d_internal_file_native_release(&in_held);
        d_internal_file_native_release(&out_held);

        return -1;
    }

    // a longer answer comes back as the size it needs, and is refused; so is
    // one whose UTF-8 does not fit _found's bytes
    if ( ((size_t)length >= D_INTERNAL_FILE_NATIVE_MAX) ||
         (d_internal_file_from_native(out,
                                      _found,
                                      D_FILE_PATH_MAX) != 0) )
    {
        if ( ((size_t)length >= D_INTERNAL_FILE_NATIVE_MAX) ||
             (errno == ERANGE) )
        {
            D_INTERNAL_FILE_SET_ERR(ENAMETOOLONG);
        }

        d_internal_file_native_release(&in_held);
        d_internal_file_native_release(&out_held);

        return -1;
    }

    d_internal_file_native_release(&in_held);
    d_internal_file_native_release(&out_held);

    // the long-path prefix this build gave a path it was not given is its
    // own, and the caller gets the path back as it would have written it
    if ( (strncmp(_path, "\\\\?\\", 4) != 0) &&
         (strncmp(_found, "\\\\?\\", 4) == 0) )
    {
        const int    unc  = (strncmp(_found + 4, "UNC\\", 4) == 0);
        const size_t drop = (unc) ? 7 : 4;  // the UNC form keeps one

        // a share's first backslash stays where it is, and the prefix's last
        // one becomes its second
        memmove(_found + ((unc) ? 1 : 0),
                _found + drop,
                strlen(_found + drop) + 1);
    }

    // a resolved path must name something that exists
    if (!d_file_exists(_found))
    {
        D_INTERNAL_FILE_SET_ERR(ENOENT);

        return -1;
    }

    return 0;
}

#else

/*
d_internal_dir_resolve
  GetFullPathNameA, into _found's D_FILE_PATH_MAX bytes; an answer past them is
ENAMETOOLONG, as realpath's is. It is called directly rather than through the
CRT's _fullpath, which reports a buffer too small as ERANGE in one runtime and
leaves errno alone in another, where GetFullPathNameA answers with the size it
needs everywhere. It is lexical and does NOT verify existence, unlike realpath,
so its answer is checked: reporting a canonical path for something that is not
there would make the two platforms disagree about what d_path_resolve means.
*/
static int
d_internal_dir_resolve(
    const char* _path,
    char*       _found
)
{
    const DWORD length = GetFullPathNameA(_path,
                                          (DWORD)D_FILE_PATH_MAX,
                                          _found,
                                          NULL);

    // Windows has said why; its reason, in errno's words
    if (length == 0)
    {
        const int code = d_internal_file_errno_win32(GetLastError());

        // named only by errno, which a build may not report through
        D_INTERNAL_FILE_SET_ERR(code);
        (void)code;

        return -1;
    }

    // a longer answer comes back as the size it needs, and is refused
    if ((size_t)length >= D_FILE_PATH_MAX)
    {
        D_INTERNAL_FILE_SET_ERR(ENAMETOOLONG);

        return -1;
    }

    // a resolved path must name something that exists
    if (!d_file_exists(_found))
    {
        D_INTERNAL_FILE_SET_ERR(ENOENT);

        return -1;
    }

    return 0;
}

#endif  // D_INTERNAL_FILE_WIDE == 1

#else

/*
d_internal_dir_resolve
  No resolver here: refuse with ENOSYS, rather than return a lexical answer
that silently differs from what the kernel would do with symlinks.
*/
static int
d_internal_dir_resolve(
    const char* _path,
    char*       _found
)
{
    (void)_path;
    (void)_found;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

#endif  // D_INTERNAL_FILE_HAS_REALPATH == 1, D_CFG_FILE_HAS_WIN32

/*
d_internal_dir_hand_back
  The resolved path in the caller's buffer -- ERANGE when it does not fit,
never a truncated path -- or, with no buffer where the build allows that, in
one allocated to fit it exactly. The allocation goes through the filesys
allocator, so D_CFG_FILE_MALLOC still owns every byte this subframework
hands out.
*/
static char*
d_internal_dir_hand_back(
    const char* _found,
    char*       _resolved,
    size_t      _size
)
{
    const size_t length = strlen(_found);

    // no buffer: one allocated to fit, where the build allows it
    if ( (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 1) &&
         (_resolved == NULL) )
    {
        char* const owned = (char*)d_internal_file_alloc(length + 1);

        // d_internal_file_alloc has already reported a failure
        if (owned)
        {
            memcpy(owned,
                   _found,
                   length + 1);
        }

        return owned;
    }

    // the caller's buffer must hold the path and its terminator
    if ((length + 1) > _size)
    {
        D_INTERNAL_FILE_FAIL(ERANGE,
                             "d_path_resolve",
                             NULL,
                             "the resolved path does not fit the buffer",
                             NULL);
    }

    memcpy(_resolved,
           _found,
           length + 1);

    return _resolved;
}

//==============================================================================
// 1.  DIRECTORIES
//==============================================================================

/*
d_dir_create
  mkdir, through d_internal_dir_mkdir. A missing parent is a failure here --
creating parents is d_dir_create_parents' job.
*/
int
d_dir_create(
    const char* _path,
    uint32_t    _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_create",
                            NULL,
                            "path is NULL",
                            -1);

    // report the failure; errno is the platform's
    if (d_internal_dir_mkdir(_path,
                             _mode) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_create",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "mkdir failed");

        return -1;
    }

    return 0;
}

/*
d_dir_create_parents
  The request is "ensure this path exists", so an existing path satisfies it.
The path is copied into a working buffer and each component boundary is
briefly overwritten with a terminator, so every prefix can be made in turn
(d_internal_dir_ensure). The root, as the kernel reads it, is never made: "/"
and "C:\\" exist by definition, and mkdir on them fails in platform-specific
ways -- a path that is nothing but a root is only checked. Trailing
separators are dropped first, so the last real component is the leaf and gets
the caller's mode, and an empty component ("a//b") is passed over.
*/
static int
d_internal_dir_create_parents_in(
    const char* _path,
    uint32_t    _mode,
    char*       _work_room
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_create_parents",
                            NULL,
                            "path is NULL",
                            -1);

    size_t length = strlen(_path);
    char* const work = _work_room;

    // the working copy must hold the path and its terminator
    if ((length + 1) > D_FILE_PATH_MAX)
    {
        D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                             "d_dir_create_parents",
                             _path,
                             "path is longer than D_FILE_PATH_MAX",
                             -1);
    }

    // an empty path names nothing to create
    if (length == 0)
    {
        D_INTERNAL_FILE_FAIL(ENOENT,
                             "d_dir_create_parents",
                             _path,
                             "path is empty",
                             -1);
    }

    memcpy(work,
           _path,
           length + 1);

    const size_t root_len = d_internal_dir_root_len(work);

    // trailing separators name no further component
    while ( (length > root_len) &&
            (d_internal_dir_is_sep(work[length - 1])) )
    {
        --length;
    }

    work[length] = '\0';

    // nothing but a root: nothing to make, only an answer to give
    if (length == root_len)
    {
        return d_internal_dir_ensure_root(work,
                                          _path);
    }

    // each prefix that ends a component, in turn; the last is the leaf
    for (size_t idx = root_len + 1; idx <= length; ++idx)
    {
        // a component ends at a separator or at the end, and an empty one
        // ("a//b") is no component
        if ( ( (idx < length) &&
               (!d_internal_dir_is_sep(work[idx])) ) ||
             (d_internal_dir_is_sep(work[idx - 1])) )
        {
            continue;
        }

        const char     saved = work[idx];
        const int      leaf  = (idx == length);
        const uint32_t mode  = leaf ? _mode
                                    : (uint32_t)D_INTERNAL_FILE_DIR_CREATE_MODE;

        // the prefix alone; the caller's mode is for the leaf, and parents
        // get the configured one
        work[idx] = '\0';

        const int result = d_internal_dir_ensure(work,
                                                 mode,
                                                 leaf,
                                                 _path);

        work[idx] = saved;

        // d_internal_dir_ensure has already reported the failure
        if (result != 0)
        {
            return -1;
        }
    }

    return 0;
}

/*
d_dir_create_parents
  d_dir_create_parents's work is d_internal_dir_create_parents_in; this keeps
the room it builds a path in, on the heap where that is 32 KiB
(d_internal_file_path).
*/
int
d_dir_create_parents(
    const char* _path,
    uint32_t    _mode
)
{
    struct d_internal_file_path work_held = {0};
    char* const work = d_internal_file_path_room(&work_held);

    // d_internal_file_path_room has set errno, and reported the failure
    if (!work)
    {
        d_internal_file_path_release(&work_held);

        return -1;
    }

    const int result = d_internal_dir_create_parents_in(_path,
                                                        _mode,
                                                        work);

    d_internal_file_path_release(&work_held);

    return result;
}

/*
d_dir_remove
  rmdir, or the CRT's _rmdir (d_internal_dir_rmdir). Empty specifically: there
is deliberately no recursive form, because deleting a tree is a policy
decision with a symlink-traversal hazard attached.
*/
int
d_dir_remove(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_remove",
                            NULL,
                            "path is NULL",
                            -1);

    // report the failure; errno is the platform's
    if (d_internal_dir_rmdir(_path) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_remove",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "rmdir failed");

        return -1;
    }

    return 0;
}

/*
d_dir_open
  The handle is allocated through the filesys allocator and keeps a copy of the
path, for the stat fallback and the Win32 search, and d_internal_dir_start
opens the platform's view of the directory. An empty path is refused before
either platform sees it -- POSIX would refuse it too, but Win32 would search
the current directory. Any failure releases the handle before reporting,
keeping the platform's errno.
*/
struct d_dir_t*
d_dir_open(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_open",
                            NULL,
                            "path is NULL",
                            NULL);

    const size_t length = strlen(_path);

    // an empty path names no directory
    if (length == 0)
    {
        D_INTERNAL_FILE_FAIL(ENOENT,
                             "d_dir_open",
                             _path,
                             "path is empty",
                             NULL);
    }

    // the handle keeps a copy of the path, which must fit
    if ((length + 1) > D_FILE_PATH_MAX)
    {
        D_INTERNAL_FILE_FAIL(ENAMETOOLONG,
                             "d_dir_open",
                             _path,
                             "path is longer than D_FILE_PATH_MAX",
                             NULL);
    }

    struct d_dir_t* const dir = d_internal_dir_new(_path,
                                                   length);


    // d_internal_dir_new has reported the failure
    if (!dir)
    {
        return NULL;
    }

    // a directory that cannot open releases the handle, keeping the
    // platform's errno across the release
    if (d_internal_dir_start(dir) != 0)
    {
        const int saved_errno = errno;

        d_internal_file_free(dir);
        errno = saved_errno;
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               saved_errno,
                               "d_dir_open",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "opendir failed");

        return NULL;
    }

    return dir;
}

/*
d_internal_dir_skipped
  Whether the caller asked not to see an entry: "." and "..", when the build
skips them.
*/
static int
d_internal_dir_skipped(
    const char* _name
)
{
    return ( (D_CFG_IS_ON(D_CFG_FILE_DIR_SKIP_DOTS)) &&
             (d_internal_dir_is_dots(_name)) );
}

/*
d_dir_read64
  Each pass takes one platform entry (d_internal_dir_next), which copies its
name into the handle's own entry -- so the result's lifetime is the handle's
-- and fills the type: from the find data on Windows, from the kernel where it
reports d_type, and by one lstat per entry where it answers DT_UNKNOWN. A name
longer than D_FILE_NAME_MAX is skipped with a warning rather than truncated,
and "." and ".." are skipped when the build asks.
*/
struct d_dirent64_t*
d_dir_read64(
    struct d_dir_t* _dir
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_dir_read64",
                            NULL,
                            "handle is NULL",
                            NULL);

    // take platform entries until one is worth returning
    for (;;)
    {
        const int taken = d_internal_dir_next(_dir);

        // the end, or a failure d_internal_dir_next has already reported
        if (taken == D_INTERNAL_DIR_DONE)
        {
            return NULL;
        }

        // an entry that cannot be reported, or one the caller asked not to see
        if ( (taken == D_INTERNAL_DIR_SKIPPED) ||
             (d_internal_dir_skipped(_dir->entry.d_name)) )
        {
            continue;
        }

        return &_dir->entry;
    }
}

/*
d_dir_read32
  Reads through d_dir_read64 and narrows into the handle's second entry. The
only field that can fail to fit is the inode number; such an entry fails on
its own -- NULL with EOVERFLOW, as a 32-bit readdir fails it -- and, since
d_dir_read64 has already moved past it, the next call reads on.
*/
struct d_dirent32_t*
d_dir_read32(
    struct d_dir_t* _dir
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_dir_read32",
                            NULL,
                            "handle is NULL",
                            NULL);

    const struct d_dirent64_t* const wide = d_dir_read64(_dir);

    // the end, or a failure d_dir_read64 has already reported
    if (!wide)
    {
        return NULL;
    }

    // this entry cannot be described in 32 bits; the next one may be
    if (wide->d_ino > (uint64_t)UINT32_MAX)
    {
        D_INTERNAL_FILE_FAIL(EOVERFLOW,
                             "d_dir_read32",
                             NULL,
                             "inode number exceeds d_dirent32_t",
                             NULL);
    }

    memcpy(_dir->entry32.d_name,
           wide->d_name,
           sizeof(_dir->entry32.d_name));
    _dir->entry32.d_ino  = (uint32_t)wide->d_ino;
    _dir->entry32.d_type = wide->d_type;

    return &_dir->entry32;
}

/*
d_dir_read_name
  Reads through d_dir_read64 and hands back the name and type alone. With no
inode number in the answer there is nothing to narrow, so this is the reader
a 32-bit build -- or one below the 64-bit floor, which cannot name
d_dirent64_t -- walks with when the inode number does not matter.
*/
const char*
d_dir_read_name(
    struct d_dir_t* _dir,
    uint8_t*        _type
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_dir_read_name",
                            NULL,
                            "handle is NULL",
                            NULL);

    const struct d_dirent64_t* const entry = d_dir_read64(_dir);

    // the end, or a failure d_dir_read64 has already reported
    if (!entry)
    {
        return NULL;
    }

    // the type is optional; the name is the answer
    if (_type)
    {
        *_type = entry->d_type;
    }

    return entry->d_name;
}

/*
d_dir_entry_path
  The kernel's join, not file_path's. d_path_join reads both operands in the
configured grammar, so a POSIX build reading Windows syntax would find a root
in an entry's name -- POSIX names may hold ':' and '\\' -- and drop the
directory: "C:x" would replace it outright. Here the name is never parsed; it
is one entry of the directory, appended after D_FILE_PATH_SEP where one is
needed (d_internal_dir_needs_sep). The directory may share _buf, as when a
caller extends its own buffer; the name may not.
*/
char*
d_dir_entry_path(
    char*       _buf,
    size_t      _bufsize,
    const char* _dir,
    const char* _name
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_dir_entry_path",
                            NULL,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_bufsize > 0,
                            EINVAL,
                            "d_dir_entry_path",
                            NULL,
                            "buffer size is 0",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_dir_entry_path",
                            NULL,
                            "directory is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_name != NULL,
                            EINVAL,
                            "d_dir_entry_path",
                            _dir,
                            "name is NULL",
                            NULL);

    return d_internal_dir_join(_buf,
                               _bufsize,
                               _dir,
                               _name);
}

/*
d_dir_rewind
  Reports failure, unlike POSIX rewinddir, which returns void. That is fine on
POSIX, where the call cannot fail -- but Win32 has no rewind at all, so the
search is closed and restarted, and THAT can fail. A void return would hide
it and leave the caller iterating a dead handle.
*/
int
d_dir_rewind(
    struct d_dir_t* _dir
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_dir_rewind",
                            NULL,
                            "handle is NULL",
                            -1);

    // a search that cannot restart leaves nothing pending, and says why
    if (d_internal_dir_restart(_dir) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_rewind",
                               NULL,
                               "could not restart the directory search");

        return -1;
    }

    return 0;
}

/*
d_dir_close
  The handle is released whatever the platform says about the close
(d_internal_dir_release): it is not usable either way, and leaking it would
be the worse failure.
*/
int
d_dir_close(
    struct d_dir_t* _dir
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_dir != NULL,
                            EINVAL,
                            "d_dir_close",
                            NULL,
                            "handle is NULL",
                            -1);

    const int result = d_internal_dir_release(_dir);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // the hold d_dir_open_at took on the directory itself
    if ( (_dir->pin != NULL) &&
         (_dir->pin != INVALID_HANDLE_VALUE) )
    {
        (void)CloseHandle(_dir->pin);
    }
#endif

    // release the handle whatever the platform said: it is not usable either
    // way, and leaking it would be the worse failure
    d_internal_file_free(_dir);

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_close",
                               NULL,
                               "closedir failed");

        return -1;
    }

    return 0;
}

/*
d_dir_get_current
  Requires a buffer. POSIX getcwd(NULL, 0) allocates instead, which is
convenient and is also a second ownership contract for one function -- so this
one does not offer it.
*/
char*
d_dir_get_current(
    char*  _buf,
    size_t _size
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_dir_get_current",
                            NULL,
                            "buffer is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(_size > 0,
                            EINVAL,
                            "d_dir_get_current",
                            NULL,
                            "buffer size is 0",
                            NULL);

    // report the failure; errno is the platform's
    if (!d_internal_dir_getcwd(_buf,
                               _size))
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_get_current",
                               NULL,
                               "getcwd failed");

        return NULL;
    }

    return _buf;
}

/*
d_dir_set_current
  chdir, or the CRT's _chdir (d_internal_dir_chdir). Process-wide by nature,
which is why the declaration steers callers toward absolute paths instead.
*/
int
d_dir_set_current(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_set_current",
                            NULL,
                            "path is NULL",
                            -1);

    // report the failure; errno is the platform's
    if (d_internal_dir_chdir(_path) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_dir_set_current",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "chdir failed");

        return -1;
    }

    return 0;
}

/*
d_path_resolve
  The platform's resolver into a buffer of D_FILE_PATH_MAX bytes
(d_internal_dir_resolve), then the answer handed back in the caller's buffer
or one allocated to fit (d_internal_dir_hand_back). Resolving into a buffer
of our own first is what lets the caller's be any size: the resolvers want
room for the longest path before they start, and the answer's length is only
known once they are done.
*/
static char*
d_internal_path_resolve_in(
    const char* _path,
    char*       _resolved,
    size_t      _size,
    char*       _found_room
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_path_resolve",
                            NULL,
                            "path is NULL",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(( (_resolved != NULL) ||
                              (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 1) ),
                            EINVAL,
                            "d_path_resolve",
                            _path,
                            "buffer is NULL and REALPATH_ALLOC is 0",
                            NULL);
    D_INTERNAL_FILE_REQUIRE(( (_resolved == NULL) ||
                              (_size > 0) ),
                            EINVAL,
                            "d_path_resolve",
                            _path,
                            "buffer size is 0",
                            NULL);

    char* const found = _found_room;

    // the platform's resolver has set errno; report it as it stands
    if (d_internal_dir_resolve(_path,
                               found) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_path_resolve",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "the path could not be resolved");

        return NULL;
    }

    return d_internal_dir_hand_back(found,
                                    _resolved,
                                    _size);
}

/*
d_path_resolve
  d_path_resolve's work is d_internal_path_resolve_in; this keeps the room it
builds a path in, on the heap where that is 32 KiB (d_internal_file_path).
*/
char*
d_path_resolve(
    const char* _path,
    char*       _resolved,
    size_t      _size
)
{
    struct d_internal_file_path found_held = {0};
    char* const found = d_internal_file_path_room(&found_held);

    // d_internal_file_path_room has set errno, and reported the failure
    if (!found)
    {
        d_internal_file_path_release(&found_held);

        return NULL;
    }

    char* const result = d_internal_path_resolve_in(_path,
                                                    _resolved,
                                                    _size,
                                                    found);

    d_internal_file_path_release(&found_held);

    return result;
}


//==============================================================================
// directory-relative operations
//==============================================================================

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // D_INTERNAL_DIR_HOLD
    //   macro: CreateFile in the build's text, to hold a directory open.
    #if (D_INTERNAL_FILE_WIDE == 1)
        #define D_INTERNAL_DIR_HOLD CreateFileW
    #else
        #define D_INTERNAL_DIR_HOLD CreateFileA
    #endif  // D_INTERNAL_FILE_WIDE == 1
#endif

/**
 * @brief Whether `_name` is one entry's name: not empty, not "." or "..", and
 *        without a separator.
 *
 * @param[in] _name  the name.
 * @return 1 for a name, 0 otherwise.
 */
static int
d_internal_dir_is_name(
    const char* _name
)
{
    // nothing, or a step up or nowhere, names no entry
    if ( (_name[0] == '\0') ||
         (d_internal_dir_is_dots(_name)) )
    {
        return 0;
    }

    // a separator would make it a path
    for (const char* at = _name; *at != '\0'; ++at)
    {
        if (d_internal_dir_is_sep(*at))
        {
            return 0;
        }
    }

    return 1;
}

#if ( D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX) ||                                     \
      D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32) )

/**
 * @brief The path of an entry of an open directory, in a block the caller
 *        frees: the directory's path, a separator where it lacks one, the name.
 *
 * @param[in]  _dir     the open directory.
 * @param[in]  _name    the entry's name.
 * @param[out] _length  receives the path's length.
 * @return the path, or `NULL` with errno set.
 */
static char*
d_internal_dir_compose(
    const struct d_dir_t* _dir,
    const char*           _name,
    size_t*               _length
)
{
    const size_t room = strlen(_dir->path) + strlen(_name) + 2;
    char* const  text = (char*)d_internal_file_alloc(room);

    // d_internal_file_alloc has reported the failure
    if (!text)
    {
        return NULL;
    }

    // the room is the path, a separator, the name and a terminator
    if (!d_internal_dir_join(text,
                             room,
                             _dir->path,
                             _name))
    {
        d_internal_file_free(text);
        D_INTERNAL_FILE_SET_ERR(ENAMETOOLONG);

        return NULL;
    }

    *_length = strlen(text);

    return text;
}

#endif  // D_CFG_FILE_HAS_POSIX || D_CFG_FILE_HAS_WIN32

#if D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)

/**
 * @brief A handle on a directory descriptor, which it takes over.
 *
 * @param[in] _fd      a descriptor on a directory, closed on failure.
 * @param[in] _path    the path the handle reports.
 * @param[in] _length  its length.
 * @return the handle, or `NULL` with errno set.
 */
static struct d_dir_t*
d_internal_dir_adopt(
    int         _fd,
    const char* _path,
    size_t      _length
)
{
    DIR* const stream = fdopendir(_fd);

    // fdopendir leaves the descriptor to the caller when it fails
    if (!stream)
    {
        const int code = errno;

        (void)close(_fd);
        errno = code;

        return NULL;
    }

    struct d_dir_t* const dir = d_internal_dir_new(_path,
                                                   _length);

    // d_internal_dir_new has reported the failure
    if (!dir)
    {
        const int code = errno;

        (void)closedir(stream);
        errno = code;

        return NULL;
    }

    dir->handle = stream;
    dir->fd     = _fd;

    return dir;
}

/**
 * @brief Opens a directory without following a link at its last component:
 *        relative to `_parent`'s descriptor, or to the working directory.
 *
 * @param[in] _at      the directory descriptor, or AT_FDCWD.
 * @param[in] _name    what to open there.
 * @param[in] _path    the path the handle reports.
 * @param[in] _length  its length.
 * @return the handle, or `NULL` with errno set: ELOOP for a link (EMLINK, as
 *         FreeBSD names it, is ELOOP here), ENOTDIR for anything else that
 *         is no directory.
 */
static struct d_dir_t*
d_internal_dir_open_held(
    int         _at,
    const char* _name,
    const char* _path,
    size_t      _length
)
{
#if defined(O_NOFOLLOW)
    const int fd = openat(_at,
                          _name,
                          O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
#else
    // no O_NOFOLLOW at this level (Darwin hides it at a strict POSIX one):
    // the entry is looked at without following it, and what opens must be
    // the very directory looked at, or it was swapped in between
    struct stat seen;

    if (fstatat(_at,
                _name,
                &seen,
                AT_SYMLINK_NOFOLLOW) != 0)
    {
        return NULL;
    }

    if (S_ISLNK(seen.st_mode))
    {
        errno = ELOOP;

        return NULL;
    }

    const int fd = openat(_at,
                          _name,
                          O_RDONLY | O_DIRECTORY | O_CLOEXEC);

    if (fd >= 0)
    {
        struct stat opened;

        if ( (fstat(fd,
                    &opened) != 0)            ||
             (opened.st_dev != seen.st_dev) ||
             (opened.st_ino != seen.st_ino) )
        {
            (void)close(fd);
            errno = ELOOP;

            return NULL;
        }
    }
#endif

    // a link is refused at the open itself: nothing is followed. Linux says
    // ENOTDIR for one when O_DIRECTORY is asked too, and FreeBSD EMLINK, so a
    // refusal is looked at once more -- only to name it, ELOOP for a link
    if (fd < 0)
    {
        struct stat st;
        const int   code = errno;

        if ( ( (code == ENOTDIR) ||
               (code == EMLINK) ) &&
             (fstatat(_at,
                      _name,
                      &st,
                      AT_SYMLINK_NOFOLLOW) == 0) &&
             (S_ISLNK(st.st_mode)) )
        {
            errno = ELOOP;
        }
        else
        {
            errno = (code == EMLINK) ? ELOOP : code;
        }

        return NULL;
    }

    return d_internal_dir_adopt(fd,
                                _path,
                                _length);
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/**
 * @brief Opens a directory without following a link at its last component,
 *        and holds it -- open without FILE_SHARE_DELETE -- so that nobody can
 *        rename it, or replace it with a link, while the handle lives: the
 *        path of every entry beneath then names what was read.
 *
 * @param[in] _path    the directory's path.
 * @param[in] _length  its length.
 * @return the handle, or `NULL` with errno set: ELOOP for a link or a
 *         junction, ENOTDIR for anything else that is no directory.
 */
static struct d_dir_t*
d_internal_dir_open_held(
    const char* _path,
    size_t      _length
)
{
    struct d_internal_file_native        held   = {0};
    const d_internal_file_nchar_t* const native =
        d_internal_file_native_of(&held,
                                  _path);

    // the conversion has set errno
    if (!native)
    {
        return NULL;
    }

    HANDLE const pin   = D_INTERNAL_DIR_HOLD(native,
                                             FILE_READ_ATTRIBUTES,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_FLAG_BACKUP_SEMANTICS |
                                             FILE_FLAG_OPEN_REPARSE_POINT,
                                             NULL);
    const DWORD  error = (pin == INVALID_HANDLE_VALUE) ? GetLastError() : 0;

    d_internal_file_native_release(&held);

    // nothing to hold
    if (pin == INVALID_HANDLE_VALUE)
    {
        D_INTERNAL_FILE_SET_ERR(d_internal_file_errno_win32(error));

        return NULL;
    }

    FILE_ATTRIBUTE_TAG_INFO tag  = {0};
    int                     code = 0;

    // what was opened, asked of the open handle itself
    if (!GetFileInformationByHandleEx(pin,
                                      FileAttributeTagInfo,
                                      &tag,
                                      (DWORD)sizeof(tag)))
    {
        code = d_internal_file_errno_win32(GetLastError());
    }
    else if ( ((tag.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) &&
              ( (tag.ReparseTag == 0) ||
                (d_internal_file_tag_is_link_win32(tag.ReparseTag)) ) )
    {
        code = ELOOP;
    }
    else if ((tag.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        code = ENOTDIR;
    }

    struct d_dir_t* const dir = (code == 0) ? d_internal_dir_new(_path,
                                                                 _length)
                                            : NULL;

    // a link, no directory, or no handle to make
    if (!dir)
    {
        (void)CloseHandle(pin);

        if (code != 0)
        {
            D_INTERNAL_FILE_SET_ERR(code);
        }

        return NULL;
    }

    // the search, under the hold
    if (d_internal_dir_start(dir) != 0)
    {
        const int saved = errno;

        (void)CloseHandle(pin);
        d_internal_file_free(dir);
        errno = saved;

        return NULL;
    }

    dir->pin = pin;

    return dir;
}

#endif  // D_CFG_FILE_HAS_POSIX

/*
d_dir_open_at
  POSIX opens the entry relative to the parent's descriptor with O_NOFOLLOW,
so the name is looked up in the directory already open, whatever has been
renamed since. Windows has no such call in Win32; the parent's hold keeps its
path naming it, and the child is held in turn and examined through the hold.
*/
struct d_dir_t*
d_dir_open_at(
    struct d_dir_t* _parent,
    const char*     _name
)
{
    D_INTERNAL_FILE_REQUIRE( ( (_parent != NULL) &&
                               (_name != NULL) ),
                            EINVAL,
                            "d_dir_open_at",
                            NULL,
                            "handle or name is NULL",
                            NULL);

    // a name, not a path
    if (!d_internal_dir_is_name(_name))
    {
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_dir_open_at",
                             _name,
                             "not one entry's name",
                             NULL);
    }

#if ( D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX) ||                                     \
      D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32) )
    size_t     length = 0;
    char* const path  = d_internal_dir_compose(_parent,
                                               _name,
                                               &length);

    // d_internal_dir_compose has set errno
    if (!path)
    {
        return NULL;
    }

#if D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    struct d_dir_t* const dir = d_internal_dir_open_held(_parent->fd,
                                                         _name,
                                                         path,
                                                         length);
#else
    struct d_dir_t* const dir = d_internal_dir_open_held(path,
                                                         length);
#endif
    const int saved = errno;

    d_internal_file_free(path);
    errno = saved;

    return dir;
#else
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_dir_open_at",
                         _name,
                         "this backend has no directories",
                         NULL);
#endif
}

/*
d_dir_remove_at
  unlinkat relative to the open directory on POSIX; on Windows, the entry's
path under the directory's hold.
*/
int
d_dir_remove_at(
    struct d_dir_t* _dir,
    const char*     _name,
    int             _flags
)
{
    D_INTERNAL_FILE_REQUIRE( ( (_dir != NULL) &&
                               (_name != NULL) ),
                            EINVAL,
                            "d_dir_remove_at",
                            NULL,
                            "handle or name is NULL",
                            -1);

    // a name, not a path
    if (!d_internal_dir_is_name(_name))
    {
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_dir_remove_at",
                             _name,
                             "not one entry's name",
                             -1);
    }

#if D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    return unlinkat(_dir->fd,
                    _name,
                    ((_flags & D_DIR_REMOVE_DIRECTORY) != 0) ? AT_REMOVEDIR
                                                             : 0);
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    size_t     length = 0;
    char* const path  = d_internal_dir_compose(_dir,
                                               _name,
                                               &length);

    // d_internal_dir_compose has set errno
    if (!path)
    {
        return -1;
    }

    (void)length;

    const int result = ((_flags & D_DIR_REMOVE_DIRECTORY) != 0)
                           ? d_internal_dir_rmdir(path)
                           : d_file_unlink(path);
    const int saved  = errno;

    d_internal_file_free(path);
    errno = saved;

    return result;
#else
    (void)_flags;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_dir_remove_at",
                         _name,
                         "this backend has no directories",
                         -1);
#endif
}

#if ( D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX) ||                                     \
      D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32) )

// d_internal_dir_frame
//   struct: one directory on the path d_dir_remove_all has opened down to,
// and the name its parent knows it by (none for the root).
struct d_internal_dir_frame
{
    struct d_dir_t* dir;
    char*           name;
};

/**
 * @brief Closes every directory a removal still holds, and frees the stack.
 *
 * @param[in] _stack  the frames.
 * @param[in] _depth  how many are open.
 */
static void
d_internal_dir_unwind(
    struct d_internal_dir_frame* _stack,
    size_t                       _depth
)
{
    while (_depth > 0)
    {
        --_depth;
        (void)d_dir_close(_stack[_depth].dir);
        d_internal_file_free(_stack[_depth].name);
    }

    d_internal_file_free(_stack);

    return;
}

/**
 * @brief Takes the entry it was handed: a directory is opened (with
 *        d_dir_open_at, which refuses a link) and pushed, anything else
 *        removed as itself.
 *
 * @param[in,out] _stack  the frames; grown as needed.
 * @param[in,out] _depth  how many are open.
 * @param[in,out] _room   how many the stack has room for.
 * @param[in]     _name   the entry's name.
 * @param[in]     _type   its DT_* type, DT_UNKNOWN where the reader could not
 *                        tell.
 * @return 0, or -1 with errno set.
 */
static int
d_internal_dir_take_entry(
    struct d_internal_dir_frame** _stack,
    size_t*                       _depth,
    size_t*                       _room,
    const char*                   _name,
    uint8_t                       _type
)
{
    struct d_dir_t* const top = (*_stack)[*_depth - 1].dir;

    // a directory, or what may be one: opened without following a link
    if ( (_type == DT_DIR) ||
         (_type == DT_UNKNOWN) )
    {
        struct d_dir_t* const child = d_dir_open_at(top,
                                                    _name);

        if (child)
        {
            const size_t length = strlen(_name);
            char* const  name   = (char*)d_internal_file_alloc(length + 1);

            // room for one more frame
            if ( (name) &&
                 (*_depth == *_room) )
            {
                struct d_internal_dir_frame* const grown =
                    (struct d_internal_dir_frame*)d_internal_file_realloc(
                        *_stack,
                        (*_room * 2) * sizeof(**_stack));

                if (!grown)
                {
                    d_internal_file_free(name);
                    (void)d_dir_close(child);

                    return -1;
                }

                *_stack = grown;
                *_room *= 2;
            }

            // d_internal_file_alloc has reported the failure
            if (!name)
            {
                (void)d_dir_close(child);

                return -1;
            }

            memcpy(name,
                   _name,
                   length + 1);
            (*_stack)[*_depth].dir  = child;
            (*_stack)[*_depth].name = name;
            ++*_depth;

            return 0;
        }

        // gone already, which is what was asked
        if (errno == ENOENT)
        {
            return 0;
        }

        // a failure other than "not a directory after all"
        if ( (errno != ELOOP) &&
             (errno != ENOTDIR) )
        {
            return -1;
        }
    }

    // a file, a link, or a directory swapped for one: removed as itself
    if ( (d_dir_remove_at(top,
                          _name,
                          0) != 0) &&
         (errno != ENOENT) )
    {
        return -1;
    }

    return 0;
}

#endif  // D_CFG_FILE_HAS_POSIX || D_CFG_FILE_HAS_WIN32

/*
d_dir_remove_all
  Iterative, so that depth costs heap rather than stack: a stack of the
directories open from the root down, each opened with d_dir_open_at, which
refuses to follow a link. An entry is removed relative to the directory it was
read from (d_dir_remove_at), never by a path built afresh, so no link swapped
in between can send the removal elsewhere -- the race std::filesystem's own
remove_all had (CVE-2022-21658). A directory is removed once read to its end,
after its handle is closed. One handle is open per level of depth.
*/
int
d_dir_remove_all(
    const char* _path
)
{
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_dir_remove_all",
                            NULL,
                            "path is NULL",
                            -1);

    const size_t length = strlen(_path);

    // an empty path names nothing to remove
    if (length == 0)
    {
        D_INTERNAL_FILE_FAIL(ENOENT,
                             "d_dir_remove_all",
                             _path,
                             "path is empty",
                             -1);
    }

#if ( D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX) ||                                     \
      D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32) )
#if D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    struct d_dir_t* const root = d_internal_dir_open_held(AT_FDCWD,
                                                          _path,
                                                          _path,
                                                          length);
#else
    struct d_dir_t* const root = d_internal_dir_open_held(_path,
                                                          length);
#endif

    // the root is no directory: removed as itself, never what a link names
    if (!root)
    {
        if (errno == ENOENT)
        {
            return 0;
        }

        if ( (errno != ELOOP) &&
             (errno != ENOTDIR) )
        {
            return -1;
        }

        return ( (d_file_unlink(_path) == 0) ||
                 (errno == ENOENT) ) ? 0
                                     : -1;
    }

    size_t                       room  = 16;
    size_t                       depth = 1;
    struct d_internal_dir_frame* stack =
        (struct d_internal_dir_frame*)d_internal_file_alloc(room *
                                                            sizeof(*stack));

    // d_internal_file_alloc has reported the failure
    if (!stack)
    {
        const int saved = errno;

        (void)d_dir_close(root);
        errno = saved;

        return -1;
    }

    stack[0].dir  = root;
    stack[0].name = NULL;

    while (depth > 0)
    {
        struct d_dir_t* const top  = stack[depth - 1].dir;
        uint8_t               type = DT_UNKNOWN;

        errno = 0;

        const char* const name = d_dir_read_name(top,
                                                 &type);

        // an entry: taken, unless it is "." or ".."
        if (name)
        {
            if ( (!d_internal_dir_is_dots(name)) &&
                 (d_internal_dir_take_entry(&stack,
                                            &depth,
                                            &room,
                                            name,
                                            type) != 0) )
            {
                break;
            }

            continue;
        }

        // a failed read
        if (errno != 0)
        {
            break;
        }

        // the end: the directory is closed, then removed from its parent
        char* const frame_name = stack[depth - 1].name;
        int         result     = d_dir_close(top);

        --depth;

        if (result == 0)
        {
            result = (depth > 0) ? d_dir_remove_at(stack[depth - 1].dir,
                                                   frame_name,
                                                   D_DIR_REMOVE_DIRECTORY)
                                 : d_internal_dir_rmdir(_path);

            if ( (result != 0) &&
                 (errno == ENOENT) )
            {
                result = 0;
            }
        }

        d_internal_file_free(frame_name);

        // the removal failed: what is still open is closed below
        if (result != 0)
        {
            break;
        }
    }

    // a failure leaves frames open
    if (depth > 0)
    {
        const int saved = (errno != 0) ? errno : EIO;

        d_internal_dir_unwind(stack,
                              depth);
        errno = saved;

        return -1;
    }

    d_internal_file_free(stack);

    return 0;
#else
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_dir_remove_all",
                         _path,
                         "this backend has no directories",
                         -1);
#endif
}
