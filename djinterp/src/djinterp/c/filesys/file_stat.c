/*******************************************************************************
* djinterp [c]                                                       file_stat.c
*
* Implementation of the metadata queries declared in file_stat.h.
*   Every status query ends in one translation, d_internal_stat_fill, the only
* code that knows how the target spells a timestamp. On Linux, statx is tried
* first because it is the only route to a creation time -- for a descriptor
* as for a path, so the two answers agree -- and a kernel or sandbox that
* refuses it is remembered for the life of the process. The predicates are
* single stat calls, and they and d_file_mode take a path that is not there as
* an answer rather than a failure worth a notice.
*   The 64-bit twins are the implementations. The 32-bit ones call them and
* narrow the answer, failing with EOVERFLOW rather than truncate a value; the
* predicates never narrow, so no file is too large to exist.
*   MORE THAN POSIX. glibc declares statx only under _GNU_SOURCE. Build this
* file with -D_GNU_SOURCE on Linux to have it -- a feature-test macro comes
* from the build, never from a source (the C guide's rule); without it only
* the stat() path is compiled, and st_created reads 0 on Linux.
*
*
* path:      /src/djinterp/c/filesys/file_stat.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_stat.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EINVAL, ENOENT, ENOSYS, ENOTDIR, EPERM
#include <stdio.h>   // FILE, fileno
#include <string.h>  // memcpy, memset, strlen
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/filesys/file_path.h"    // d_path_root_length
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_stat.h"  // D_INTERNAL_FILE_STAT_*
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, int64_t,
                                                     // uint32_t, uint64_t,
                                                     // INT32_MAX, UINT32_MAX
// posix
#if (D_INTERNAL_FILE_STAT_STATX == 1)
    // makedev lives here on glibc. <sys/stat.h> only ever dragged it in
    // implicitly, and stopped doing so in 2.28 -- the same release that added
    // statx -- so a build new enough to have statx is exactly one that needs
    // this include.
    #include <sys/sysmacros.h>  // makedev
#endif


// the large-file interface this source needs (file_common.h, 6.4.1)
D_INTERNAL_FILE_ASSERT_LARGE_FILES;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_STAT_NATIVE
//   macro: the platform's own status structure, which d_internal_stat_fill
// translates from. A build with neither POSIX nor Windows has none, and no
// translation.
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_INTERNAL_STAT_NATIVE struct _stat64
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    #define D_INTERNAL_STAT_NATIVE struct stat
#endif

// D_INTERNAL_STAT_FOLLOW
//   constant: d_internal_stat_path's flag to follow a final symbolic link.
#define D_INTERNAL_STAT_FOLLOW 1

// D_INTERNAL_STAT_PROBE
//   constant: d_internal_stat_path's flag for a question whose "no" is an
// answer, so a path that is not there fails without a notice.
#define D_INTERNAL_STAT_PROBE 2

// D_INTERNAL_STAT_DEFAULT
//   constant: d_file_stat64's way of asking -- following a link unless the
// build has set D_CFG_FILE_STAT_FOLLOW_SYMLINKS to 0.
#define D_INTERNAL_STAT_DEFAULT                                                \
    ( (D_INTERNAL_FILE_STAT_FOLLOW == 1) ? D_INTERNAL_STAT_FOLLOW : 0 )

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_stat_fill_times
  Windows reports creation, last access and last write, in whole seconds
through the CRT, whose st_ctime IS the creation time. It has no
metadata-change time at all, so st_changed stays 0 -- substituting the
creation time there is exactly the conflation the field names exist to
prevent.
*/
static void
d_internal_stat_fill_times(
    const struct _stat64* _native,
    struct d_stat64_t*    _out
)
{
    _out->st_modified = (int64_t)_native->st_mtime;
    _out->st_accessed = (int64_t)_native->st_atime;

    // the creation time, where the build delivers one
    if (D_INTERNAL_FILE_STAT_BIRTHTIME == 1)
    {
        _out->st_created = (int64_t)_native->st_ctime;
    }

    return;
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)

#if (D_INTERNAL_FILE_STAT_NSEC == 1)

/*
d_internal_stat_fill_nsec
  The sub-second parts as POSIX.1-2008 spells them: st_mtim and its kin are
struct timespec.
*/
static void
d_internal_stat_fill_nsec(
    const struct stat* _native,
    struct d_stat64_t* _out
)
{
    _out->st_modified_nsec = (uint32_t)_native->st_mtim.tv_nsec;
    _out->st_accessed_nsec = (uint32_t)_native->st_atim.tv_nsec;
    _out->st_changed_nsec  = (uint32_t)_native->st_ctim.tv_nsec;

    return;
}

#elif (D_INTERNAL_FILE_STAT_NSEC == 2)

/*
d_internal_stat_fill_nsec
  The sub-second parts as macOS spells them, having got there before POSIX
under another name.
*/
static void
d_internal_stat_fill_nsec(
    const struct stat* _native,
    struct d_stat64_t* _out
)
{
    _out->st_modified_nsec = (uint32_t)_native->st_mtimespec.tv_nsec;
    _out->st_accessed_nsec = (uint32_t)_native->st_atimespec.tv_nsec;
    _out->st_changed_nsec  = (uint32_t)_native->st_ctimespec.tv_nsec;

    return;
}

#elif (D_INTERNAL_FILE_STAT_NSEC == 4)

/*
d_internal_stat_fill_nsec
  The sub-second parts as Darwin spells them for a build at a POSIX level
without _DARWIN_C_SOURCE: a long of nanoseconds beside each time_t.
*/
static void
d_internal_stat_fill_nsec(
    const struct stat* _native,
    struct d_stat64_t* _out
)
{
    _out->st_modified_nsec = (uint32_t)_native->st_mtimensec;
    _out->st_accessed_nsec = (uint32_t)_native->st_atimensec;
    _out->st_changed_nsec  = (uint32_t)_native->st_ctimensec;

    return;
}

#else

/*
d_internal_stat_fill_nsec
  Whole seconds only, by the platform or by D_CFG_FILE_STAT_NSEC: the
sub-second fields stay 0.
*/
static void
d_internal_stat_fill_nsec(
    const struct stat* _native,
    struct d_stat64_t* _out
)
{
    (void)_native;
    (void)_out;

    return;
}

#endif  // D_INTERNAL_FILE_STAT_NSEC

#if ( (D_INTERNAL_FILE_STAT_BIRTHTIME == 1) &&                                 \
      (D_INTERNAL_FILE_OS_DARWIN == 1) )

/*
d_internal_stat_fill_birth
  Apple's systems keep the creation time beside the other three. st_birthtime
reads it at every level Darwin builds at: a member of its own at a POSIX
level, and a name for st_birthtimespec.tv_sec otherwise.
*/
static void
d_internal_stat_fill_birth(
    const struct stat* _native,
    struct d_stat64_t* _out
)
{
    _out->st_created = (int64_t)_native->st_birthtime;

    return;
}

#else

/*
d_internal_stat_fill_birth
  stat(2) has no creation time to read here -- on Linux only statx reports
one -- so st_created stays 0.
*/
static void
d_internal_stat_fill_birth(
    const struct stat* _native,
    struct d_stat64_t* _out
)
{
    (void)_native;
    (void)_out;

    return;
}

#endif  // D_INTERNAL_FILE_STAT_BIRTHTIME == 1 && D_INTERNAL_FILE_OS_DARWIN

/*
d_internal_stat_fill_times
  POSIX: st_mtime and friends are MACROS here, not members -- the whole reason
d_stat64_t's fields are named st_modified. Reading them off the platform's
struct is fine; declaring fields by those names is not. The sub-second parts
and the creation time are read as the build spells them.
*/
static void
d_internal_stat_fill_times(
    const struct stat* _native,
    struct d_stat64_t* _out
)
{
    _out->st_modified = (int64_t)_native->st_mtime;
    _out->st_accessed = (int64_t)_native->st_atime;
    _out->st_changed  = (int64_t)_native->st_ctime;
    d_internal_stat_fill_nsec(_native,
                              _out);
    d_internal_stat_fill_birth(_native,
                               _out);

    return;
}

#endif  // D_CFG_FILE_HAS_WIN32, D_CFG_FILE_HAS_POSIX

#ifdef D_INTERNAL_STAT_NATIVE

/*
d_internal_stat_fill
  Translates the platform's struct stat into djinterp's. This and
d_internal_stat_fill_times are the only code that knows how the target spells
a timestamp, which is where D_CFG_FILE_HAS_STAT_NSEC earns its keep: env
cannot rename djinterp's own fields to dodge the st_mtime macro collision,
but it can say which member to read. The output is zeroed first, so every
field the platform cannot answer reads 0 rather than whatever was on the
caller's stack.
*/
static void
d_internal_stat_fill(
    const D_INTERNAL_STAT_NATIVE* _native,
    struct d_stat64_t*            _out
)
{
    memset(_out,
           0,
           sizeof(*_out));
    _out->st_size  = (uint64_t)_native->st_size;
    _out->st_mode  = (uint32_t)_native->st_mode;
    _out->st_nlink = (uint32_t)_native->st_nlink;
    _out->st_uid   = (uint32_t)_native->st_uid;
    _out->st_gid   = (uint32_t)_native->st_gid;
    _out->st_dev   = (uint64_t)_native->st_dev;
    _out->st_ino   = (uint64_t)_native->st_ino;
    d_internal_stat_fill_times(_native,
                               _out);

    return;
}

#endif  // D_INTERNAL_STAT_NATIVE

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

// D_INTERNAL_STAT_EPOCH_DIFF
//   constant: the seconds from 1601, where a FILETIME counts from, to 1970.
#define D_INTERNAL_STAT_EPOCH_DIFF ((int64_t)11644473600)

/*
d_internal_stat_unix_time
  A FILETIME -- 100-nanosecond ticks since 1601 -- in Unix seconds. A zeroed
one, which no file has, reads 0.
*/
static int64_t
d_internal_stat_unix_time(
    FILETIME _time
)
{
    const uint64_t ticks = ( ((uint64_t)_time.dwHighDateTime << 32) |
                             (uint64_t)_time.dwLowDateTime );

    // a time nobody set
    if (ticks == 0)
    {
        return 0;
    }

    return (int64_t)(ticks / 10000000u) - D_INTERNAL_STAT_EPOCH_DIFF;
}

/*
d_internal_stat_native_link
  A symbolic link or a junction, described as POSIX lstat describes a link --
S_IFLNK, with the link's own times and link count -- where _stat64 would
follow it. Its size reads 0. Returns 1 for such a link, and 0 for anything
else, which _stat64 then describes.
*/
static int
d_internal_stat_native_link(
    const char*        _path,
    struct d_stat64_t* _out
)
{
    BY_HANDLE_FILE_INFORMATION info = {0};

    // anything but a link is _stat64's to describe
    if (!d_internal_file_is_link_win32(_path,
                                       &info))
    {
        return 0;
    }

    memset(_out,
           0,
           sizeof(*_out));
    _out->st_mode     = (uint32_t)(S_IFLNK | 0777);
    _out->st_nlink    = (uint32_t)info.nNumberOfLinks;
    _out->st_modified = d_internal_stat_unix_time(info.ftLastWriteTime);
    _out->st_accessed = d_internal_stat_unix_time(info.ftLastAccessTime);

    // the creation time, where the build delivers one
    if (D_INTERNAL_FILE_STAT_BIRTHTIME == 1)
    {
        _out->st_created = d_internal_stat_unix_time(info.ftCreationTime);
    }

    return 1;
}

/*
d_internal_stat_trim
  The length of a path without the separators that end it -- never into its
root, and never the one separator a root such as "\\\\server\\share\\" ends
in.
*/
static size_t
d_internal_stat_trim(
    const char* _path,
    size_t      _length
)
{
    const size_t root   = d_path_root_length(_path);
    size_t       length = _length;

    // the separators after the last component
    while ( (length > root + 1) &&
            ( (_path[length - 1] == D_FILE_PATH_SEP) ||
              (_path[length - 1] == D_FILE_PATH_SEP_ALT) ) )
    {
        --length;
    }

    return length;
}

// D_INTERNAL_STAT_STAT64
//   macro: the CRT's _stat64, in the form the build's path text takes:
// _wstat64 where the build hands Windows UTF-16 (D_INTERNAL_FILE_WIDE).
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_STAT_STAT64 _wstat64
#else
    #define D_INTERNAL_STAT_STAT64 _stat64
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_STAT_ACCESS
//   macro: the CRT's _access, or _waccess.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_STAT_ACCESS _waccess
#else
    #define D_INTERNAL_STAT_ACCESS _access
#endif  // D_INTERNAL_FILE_WIDE == 1

// D_INTERNAL_STAT_CHMOD
//   macro: the CRT's _chmod, or _wchmod.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_STAT_CHMOD _wchmod
#else
    #define D_INTERNAL_STAT_CHMOD _chmod
#endif  // D_INTERNAL_FILE_WIDE == 1

/*
d_internal_stat_native
  The CRT's _stat64 on one path, translated; the path in the platform's text
first (d_internal_file_to_native).
*/
static int
d_internal_stat_native(
    const char*        _path,
    struct d_stat64_t* _out
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

    struct _stat64 native = {0};
    const int      failed = (D_INTERNAL_STAT_STAT64(path,
                                                    &native) != 0);

    d_internal_file_native_release(&held);

    // _stat64 has reported its own errno
    if (failed)
    {
        return -1;
    }

    d_internal_stat_fill(&native,
                         _out);

    return 0;
}

/*
d_internal_stat_native_dir
  A path that ends in separators, asked without them as msvcrt's _stat64
needs, and answered as POSIX answers it: the path must then name a directory,
so "file.txt\\" fails with ENOTDIR.
*/
static int
d_internal_stat_native_dir_in(
    const char*        _path,
    size_t             _length,
    struct d_stat64_t* _out,
    char*              _name_room
)
{
    char* const name = _name_room;

    // a name too long to copy is too long to stat
    if (_length > D_FILE_PATH_MAX)
    {
        D_INTERNAL_FILE_SET_ERR(ENAMETOOLONG);

        return -1;
    }

    memcpy(name,
           _path,
           _length);

    // the directory the separators ask for, or the stat's own failure
    if (d_internal_stat_native(name,
                               _out) != 0)
    {
        return -1;
    }

    // anything but a directory cannot be followed by a separator
    if (!S_ISDIR(_out->st_mode))
    {
        memset(_out,
               0,
               sizeof(*_out));
        D_INTERNAL_FILE_SET_ERR(ENOTDIR);

        return -1;
    }

    return 0;
}

/*
d_internal_stat_native_dir
  d_internal_stat_native_dir's work is d_internal_stat_native_dir_in; this
keeps the room it builds a path in, on the heap where that is 32 KiB
(d_internal_file_path).
*/
static int
d_internal_stat_native_dir(
    const char*        _path,
    size_t             _length,
    struct d_stat64_t* _out
)
{
    struct d_internal_file_path name_held = {0};
    char* const name = d_internal_file_path_room(&name_held);

    // d_internal_file_path_room has set errno, and reported the failure
    if (!name)
    {
        d_internal_file_path_release(&name_held);

        return -1;
    }

    const int result = d_internal_stat_native_dir_in(_path,
                                                     _length,
                                                     _out,
                                                     name);

    d_internal_file_path_release(&name_held);

    return result;
}

/*
d_internal_stat_native_path
  Win32's CRT has no lstat, and _stat64 always follows a reparse point, so not
following is done here: a symbolic link or a junction asked about without
following describes itself (d_internal_stat_native_link). A trailing
separator is handled as POSIX handles one, since msvcrt's _stat64 fails
"C:\\dir\\" with ENOENT (d_internal_stat_native_dir) -- and like POSIX it
reaches through a link, whatever _follow says.
*/
static int
d_internal_stat_native_path(
    const char*        _path,
    int                _follow,
    struct d_stat64_t* _out
)
{
    const size_t length  = strlen(_path);
    const size_t trimmed = d_internal_stat_trim(_path,
                                                length);

    // separators at the end ask for a directory, through any link
    if (trimmed != length)
    {
        return d_internal_stat_native_dir(_path,
                                          trimmed,
                                          _out);
    }

    // a link, unless followed, describes itself
    if ( (!_follow) &&
         (d_internal_stat_native_link(_path,
                                      _out)) )
    {
        return 0;
    }

    return d_internal_stat_native(_path,
                                  _out);
}

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_stat_native_fd
  The CRT's _fstat64 on a descriptor, translated.
*/
static int
d_internal_stat_native_fd(
    int                _fd,
    struct d_stat64_t* _out
)
{
    struct _stat64 native = {0};

    // _fstat64 reports its own errno
    if (_fstat64(_fd,
                 &native) != 0)
    {
        return -1;
    }

    d_internal_stat_fill(&native,
                         _out);

    return 0;
}

/*
d_internal_stat_fileno
  The CRT's spelling of fileno.
*/
static int
d_internal_stat_fileno(
    FILE* _stream
)
{
    return _fileno(_stream);
}

#endif  // !D_FILE_BACKEND_IS_STDC

/*
d_internal_stat_access
  The CRT has no notion of execute permission, so X_OK is masked off: asking
for it on a file that exists would report a failure, a worse answer than the
one Windows can give.
*/
static int
d_internal_stat_access(
    const char* _path,
    int         _mode
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

    const int result = D_INTERNAL_STAT_ACCESS(path,
                                              _mode & (~X_OK));

    d_internal_file_native_release(&held);

    return result;
}

/*
d_internal_stat_chmod
  The CRT's _chmod, which maps the mode onto the read-only attribute and
discards everything else without complaint.
*/
static int
d_internal_stat_chmod(
    const char* _path,
    uint32_t    _mode
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

    const int result = D_INTERNAL_STAT_CHMOD(path,
                                             (int)_mode);

    d_internal_file_native_release(&held);

    return result;
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)

/*
d_internal_stat_native_path
  stat resolves a symbolic link; lstat describes the link itself.
*/
static int
d_internal_stat_native_path(
    const char*        _path,
    int                _follow,
    struct d_stat64_t* _out
)
{
    struct stat native = {0};
    const int   result = _follow ? stat(_path,
                                        &native)
                                 : lstat(_path,
                                         &native);

    // stat and lstat report their own errno
    if (result != 0)
    {
        return -1;
    }

    d_internal_stat_fill(&native,
                         _out);

    return 0;
}

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_stat_native_fd
  fstat on a descriptor, translated.
*/
static int
d_internal_stat_native_fd(
    int                _fd,
    struct d_stat64_t* _out
)
{
    struct stat native = {0};

    // fstat reports its own errno
    if (fstat(_fd,
              &native) != 0)
    {
        return -1;
    }

    d_internal_stat_fill(&native,
                         _out);

    return 0;
}

/*
d_internal_stat_fileno
  POSIX fileno.
*/
static int
d_internal_stat_fileno(
    FILE* _stream
)
{
    return fileno(_stream);
}

#endif  // !D_FILE_BACKEND_IS_STDC

/*
d_internal_stat_access
  access(2), which checks the real user rather than the effective one.
*/
static int
d_internal_stat_access(
    const char* _path,
    int         _mode
)
{
    return access(_path,
                  _mode);
}

/*
d_internal_stat_chmod
  chmod(2).
*/
static int
d_internal_stat_chmod(
    const char* _path,
    uint32_t    _mode
)
{
    return chmod(_path,
                 (mode_t)_mode);
}

#else

/*
d_internal_stat_native_path
  Neither POSIX nor Windows is here to ask, and ISO C has no way to describe a
file, so this reports ENOSYS -- the ISO C backend's answer, which is the only
backend such a build can have. Every query is built on it, so each one says
the same; the predicates answer no.
*/
static int
d_internal_stat_native_path(
    const char*        _path,
    int                _follow,
    struct d_stat64_t* _out
)
{
    (void)_path;
    (void)_follow;
    (void)_out;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

/*
d_internal_stat_access
  No platform API to ask, so ENOSYS (d_internal_stat_native_path).
*/
static int
d_internal_stat_access(
    const char* _path,
    int         _mode
)
{
    (void)_path;
    (void)_mode;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

/*
d_internal_stat_chmod
  No platform API to ask, so ENOSYS (d_internal_stat_native_path).
*/
static int
d_internal_stat_chmod(
    const char* _path,
    uint32_t    _mode
)
{
    (void)_path;
    (void)_mode;

    D_INTERNAL_FILE_SET_ERR(ENOSYS);

    return -1;
}

#endif  // D_CFG_FILE_HAS_WIN32, D_CFG_FILE_HAS_POSIX

#if (D_INTERNAL_FILE_STAT_STATX == 1)

#if (D_INTERNAL_FILE_STAT_STATX_MEMO == 1)

// d_internal_stat_statx_ok
//   variable: 1 until statx is found refused -- by the kernel or a sandbox,
// for a path stat then answered. Read and written atomically, since any
// thread may be the one to find out.
static int d_internal_stat_statx_ok = 1;

/*
d_internal_stat_statx_on
  Whether statx is still worth asking.
*/
static int
d_internal_stat_statx_on(
    void
)
{
    return __atomic_load_n(&d_internal_stat_statx_ok,
                           __ATOMIC_RELAXED);
}

/*
d_internal_stat_statx_off
  Stops asking statx for the life of the process, and reports whether this
call is the one that did.
*/
static int
d_internal_stat_statx_off(
    void
)
{
    return __atomic_exchange_n(&d_internal_stat_statx_ok,
                               0,
                               __ATOMIC_RELAXED);
}

#else

/*
d_internal_stat_statx_on
  No memo in this build (D_INTERNAL_FILE_STAT_STATX_MEMO): statx is asked
every time, and a refusal costs one failed call per stat.
*/
static int
d_internal_stat_statx_on(
    void
)
{
    return 1;
}

/*
d_internal_stat_statx_off
  Nothing is remembered without atomics (d_internal_stat_statx_on).
*/
static int
d_internal_stat_statx_off(
    void
)
{
    return 0;
}

#endif  // D_INTERNAL_FILE_STAT_STATX_MEMO == 1

/*
d_internal_stat_statx
  Fills d_stat64_t from Linux statx(2). The reason this exists is btime: the
creation time is in the inode, and stat() has no field to hand it back in.
statx does, and returns in one syscall what stat reports in pieces. It is
asked as stat(2) asks, without automounting the last component
(AT_NO_AUTOMOUNT, which stat implies): a question about a path must not mount
a filesystem to answer.
  statx reports which fields it actually answered, via stx_mask, and that is
respected rather than assumed: a filesystem that does not store btime returns
success WITHOUT STATX_BTIME set, and reading stx_btime then would be reading a
zero and calling it a timestamp. The knobs are honoured here as on the stat
path -- statx hands back sub-second and birth times whether or not the build
asked for them, and a query macro that says no while the fields say yes is a
lie the caller cannot defend against.
*/
static int
d_internal_stat_statx(
    int                _dirfd,
    const char*        _path,
    int                _flags,
    struct d_stat64_t* _out
)
{
    struct statx stx = {0};

    // the caller decides what a refusal means
    if (statx(_dirfd,
              _path,
              _flags | AT_NO_AUTOMOUNT,
              STATX_BASIC_STATS | STATX_BTIME,
              &stx) != 0)
    {
        return -1;
    }

    memset(_out,
           0,
           sizeof(*_out));
    _out->st_size     = (uint64_t)stx.stx_size;
    _out->st_mode     = (uint32_t)stx.stx_mode;
    _out->st_nlink    = (uint32_t)stx.stx_nlink;
    _out->st_uid      = (uint32_t)stx.stx_uid;
    _out->st_gid      = (uint32_t)stx.stx_gid;
    _out->st_ino      = (uint64_t)stx.stx_ino;
    _out->st_modified = (int64_t)stx.stx_mtime.tv_sec;
    _out->st_accessed = (int64_t)stx.stx_atime.tv_sec;
    _out->st_changed  = (int64_t)stx.stx_ctime.tv_sec;

    // statx splits the device into major and minor; recombined, st_dev means
    // what stat's opaque dev_t means, and d_stat64_t stays one type
    _out->st_dev = (uint64_t)makedev(stx.stx_dev_major,
                                     stx.stx_dev_minor);

    // the sub-second parts, where the build asks for them
    if (D_INTERNAL_FILE_STAT_NSEC != 0)
    {
        _out->st_modified_nsec = (uint32_t)stx.stx_mtime.tv_nsec;
        _out->st_accessed_nsec = (uint32_t)stx.stx_atime.tv_nsec;
        _out->st_changed_nsec  = (uint32_t)stx.stx_ctime.tv_nsec;
    }

    // the whole point -- but only when the filesystem actually stored one
    if ( (D_INTERNAL_FILE_STAT_BIRTHTIME == 1) &&
         ((stx.stx_mask & STATX_BTIME) != 0) )
    {
        _out->st_created = (int64_t)stx.stx_btime.tv_sec;
    }

    return 0;
}

/*
d_internal_stat_refused
  Whether a statx failure refuses the call itself -- ENOSYS from a kernel
without it, EPERM from a sandbox that filters it -- rather than answering
about the path.
*/
static int
d_internal_stat_refused(
    int _error
)
{
    return ( (_error == ENOSYS) ||
             (_error == EPERM) );
}

/*
d_internal_stat_settle
  After a refused statx (_refusal, its errno), what stat did decides. A path
stat answered proves the refusal was statx's own, and statx is not asked
again in this process. A stat that failed as well leaves statx alone: the
file itself was refused, as a FUSE filesystem may refuse one file, and that
says nothing about the next one.
*/
static int
d_internal_stat_settle(
    int         _result,
    int         _refusal,
    const char* _fn
)
{
    // referenced only by the notification path, which may be compiled out
    (void)_fn;

    // statx answered or was not asked, or the file failed both
    if ( (_refusal == 0) ||
         (_result != 0) )
    {
        return _result;
    }

    // the one thread that switches statx off says so
    if (d_internal_stat_statx_off())
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                               _refusal,
                               _fn,
                               NULL,
                               "statx refused; using stat for the rest of "
                               "this process");
    }

    return _result;
}

/*
d_internal_stat_lookup_path
  statx where it is still worth asking, falling back to stat or lstat when
the call itself is refused. Any other statx failure is a real answer about a
real path and is returned as it is.
*/
static int
d_internal_stat_lookup_path(
    const char*        _path,
    int                _follow,
    struct d_stat64_t* _out,
    const char*        _fn
)
{
    int refusal = 0;

    // the fast path, while it has not been refused
    if (d_internal_stat_statx_on())
    {
        const int flags = _follow ? 0 : AT_SYMLINK_NOFOLLOW;

        // statx answered
        if (d_internal_stat_statx(AT_FDCWD,
                                  _path,
                                  flags,
                                  _out) == 0)
        {
            return 0;
        }

        // an answer about the path is the answer
        if (!d_internal_stat_refused(errno))
        {
            return -1;
        }

        refusal = errno;
    }

    return d_internal_stat_settle(d_internal_stat_native_path(_path,
                                                              _follow,
                                                              _out),
                                  refusal,
                                  _fn);
}

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_stat_lookup_fd
  d_internal_stat_lookup_path for a descriptor: statx on the descriptor
itself (AT_EMPTY_PATH), so the creation time a path reports is the one its
open file reports too, and fstat where statx is refused.
*/
static int
d_internal_stat_lookup_fd(
    int                _fd,
    struct d_stat64_t* _out,
    const char*        _fn
)
{
    int refusal = 0;

    // the fast path, while it has not been refused
    if (d_internal_stat_statx_on())
    {
        // statx answered
        if (d_internal_stat_statx(_fd,
                                  "",
                                  AT_EMPTY_PATH,
                                  _out) == 0)
        {
            return 0;
        }

        // an answer about the descriptor is the answer
        if (!d_internal_stat_refused(errno))
        {
            return -1;
        }

        refusal = errno;
    }

    return d_internal_stat_settle(d_internal_stat_native_fd(_fd,
                                                            _out),
                                  refusal,
                                  _fn);
}

#endif  // !D_FILE_BACKEND_IS_STDC

#else

/*
d_internal_stat_lookup_path
  No statx in this build: the platform's stat, and nothing to fall back from.
*/
static int
d_internal_stat_lookup_path(
    const char*        _path,
    int                _follow,
    struct d_stat64_t* _out,
    const char*        _fn
)
{
    (void)_fn;

    return d_internal_stat_native_path(_path,
                                       _follow,
                                       _out);
}

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_stat_lookup_fd
  No statx in this build: the platform's fstat.
*/
static int
d_internal_stat_lookup_fd(
    int                _fd,
    struct d_stat64_t* _out,
    const char*        _fn
)
{
    (void)_fn;

    return d_internal_stat_native_fd(_fd,
                                     _out);
}

#endif  // !D_FILE_BACKEND_IS_STDC

#endif  // D_INTERNAL_FILE_STAT_STATX == 1

/*
d_internal_stat_absent
  Whether a failure means only that nothing is there: ENOENT, or ENOTDIR for
a component on the way that is not a directory.
*/
static int
d_internal_stat_absent(
    int _error
)
{
    return ( (_error == ENOENT) ||
             (_error == ENOTDIR) );
}

/*
d_internal_stat_path
  The single path-based entry point, so the follow-or-not decision and the
report are made once rather than at every call site. _how is
D_INTERNAL_STAT_FOLLOW, D_INTERNAL_STAT_PROBE, both or neither. A failure is
reported as an error, except a probe's "nothing there", which is the answer
it asked for.
*/
static int
d_internal_stat_path(
    const char*        _path,
    struct d_stat64_t* _out,
    int                _how,
    const char*        _fn
)
{
    const int follow = ((_how & D_INTERNAL_STAT_FOLLOW) != 0);

    // the answer, from statx or stat
    if (d_internal_stat_lookup_path(_path,
                                    follow,
                                    _out,
                                    _fn) == 0)
    {
        return 0;
    }

    // a probe's "nothing there" is not a failure worth a notice
    if ( ((_how & D_INTERNAL_STAT_PROBE) == 0) ||
         (!d_internal_stat_absent(errno)) )
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               _fn,
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "stat failed");
    }

    return -1;
}

//==============================================================================
// 1.  METADATA
//==============================================================================

/*
d_file_stat64
  Follows, because that is what stat() means and what callers expect --
d_dir_exists on a link to a directory says yes. Whether it follows is
D_INTERNAL_FILE_STAT_FOLLOW, so a build can make every query here stop at the
link instead.
*/
int
d_file_stat64(
    const char*        _path,
    struct d_stat64_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_stat64",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_stat64",
                            _path,
                            "output buffer is NULL",
                            -1);

    return d_internal_stat_path(_path,
                                _buf,
                                D_INTERNAL_STAT_DEFAULT,
                                "d_file_stat64");
}

/*
d_file_stat_nofollow64
  Passes "do not follow" down explicitly, so it never depends on
D_CFG_FILE_STAT_FOLLOW_SYMLINKS. On Windows, where _stat64 follows every
reparse point, a link is described without it (d_internal_stat_native_link).
*/
int
d_file_stat_nofollow64(
    const char*        _path,
    struct d_stat64_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_stat_nofollow64",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_stat_nofollow64",
                            _path,
                            "output buffer is NULL",
                            -1);

    return d_internal_stat_path(_path,
                                _buf,
                                0,
                                "d_file_stat_nofollow64");
}

#if D_FILE_BACKEND_IS_STDC

/*
d_file_stat_fd64
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_stat_fd64(
    int                _fd,
    struct d_stat64_t* _buf
)
{
    (void)_fd;
    (void)_buf;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_stat_fd64",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_stat_fd64
  The descriptor asked as a path is asked -- statx where the build has it,
fstat otherwise -- and the same translation every path query uses, so the
two answers agree field for field.
*/
int
d_file_stat_fd64(
    int                _fd,
    struct d_stat64_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_stat_fd64",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_stat_fd64",
                            NULL,
                            "output buffer is NULL",
                            -1);

    // the answer, from statx or fstat
    if (d_internal_stat_lookup_fd(_fd,
                                  _buf,
                                  "d_file_stat_fd64") == 0)
    {
        return 0;
    }

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                           errno,
                           "d_file_stat_fd64",
                           NULL,
                           "fstat failed");

    return -1;
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_mode
  One stat, as d_file_stat64 asks, keeping only st_mode: the mode is 32-bit in
both layouts, so there is nothing to narrow and nothing that can overflow. It
asks what a path is, and "nothing" is one of the answers, so a missing path
fails without a notice.
*/
int
d_file_mode(
    const char* _path,
    uint32_t*   _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_mode",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_file_mode",
                            _path,
                            "output is NULL",
                            -1);

    struct d_stat64_t st = {0};

    // d_internal_stat_path reports any failure but absence
    if (d_internal_stat_path(_path,
                             &st,
                             D_INTERNAL_STAT_DEFAULT | D_INTERNAL_STAT_PROBE,
                             "d_file_mode") != 0)
    {
        return -1;
    }

    *_mode = st.st_mode;

    return 0;
}

/*
d_file_mode_nofollow
  d_file_mode without following a final link, so a link describes itself.
*/
int
d_file_mode_nofollow(
    const char* _path,
    uint32_t*   _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_mode_nofollow",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_mode != NULL,
                            EINVAL,
                            "d_file_mode_nofollow",
                            _path,
                            "output is NULL",
                            -1);

    struct d_stat64_t st = {0};

    // d_internal_stat_path reports any failure but absence
    if (d_internal_stat_path(_path,
                             &st,
                             D_INTERNAL_STAT_PROBE,
                             "d_file_mode_nofollow") != 0)
    {
        return -1;
    }

    *_mode = st.st_mode;

    return 0;
}

/*
d_file_access
  access(2), or the CRT's _access (d_internal_stat_access). A mode with any
bit beyond F_OK, R_OK, W_OK and X_OK is refused here, as POSIX refuses it --
the CRT would take it to its invalid-parameter handler, which by default ends
the process. A refusal is reported at info severity, because "no" is the
answer this function exists to give.
*/
int
d_file_access(
    const char* _path,
    int         _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_access",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE((_mode & ~(F_OK | R_OK | W_OK | X_OK)) == 0,
                            EINVAL,
                            "d_file_access",
                            _path,
                            "mode is not F_OK or R_OK, W_OK and X_OK",
                            -1);

    // a refusal is an answer, reported at info severity
    if (d_internal_stat_access(_path,
                               _mode) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                               errno,
                               "d_file_access",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "access denied or path absent");

        return -1;
    }

    return 0;
}

/*
d_file_chmod
  chmod, or the CRT's _chmod, which maps the mode onto the read-only attribute
and discards everything else without complaint (d_internal_stat_chmod).
*/
int
d_file_chmod(
    const char* _path,
    uint32_t    _mode
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_chmod",
                            NULL,
                            "path is NULL",
                            -1);

    // report the failure; errno is the platform's
    if (d_internal_stat_chmod(_path,
                              _mode) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_chmod",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "chmod failed");

        return -1;
    }

    return 0;
}

/*
d_file_size64
  What the filesystem says, which for a /proc or /sys entry is 0 though reading
it produces kilobytes -- the right answer to "what size does this file
report" and the wrong one to "how many bytes will I get". d_file_read_all
answers the second by distrusting a reported zero.
*/
int64_t
d_file_size64(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_size64",
                            NULL,
                            "path is NULL",
                            -1);

    struct d_stat64_t st = {0};

    // d_file_stat64 has already reported the failure
    if (d_file_stat64(_path,
                      &st) != 0)
    {
        return -1;
    }

    return (int64_t)st.st_size;
}

#if D_FILE_BACKEND_IS_STDC

/*
d_file_size_stream64
  The ISO C backend has no descriptors to ask, so this only reports ENOSYS.
*/
int64_t
d_file_size_stream64(
    FILE* _stream
)
{
    (void)_stream;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_size_stream64",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_size_stream64
  Asks the descriptor rather than seeking to the end: seeking would perturb
the position, fail on a pipe, and on a Windows text-mode stream report a
length that disagrees with what a read will actually produce. What the
stream still buffers is not the file's yet, so it is not counted.
*/
int64_t
d_file_size_stream64(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_size_stream64",
                            NULL,
                            "stream is NULL",
                            -1);

    const int fd = d_internal_stat_fileno(_stream);

    // a stream with no descriptor cannot be measured this way
    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_file_size_stream64",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    struct d_stat64_t st = {0};

    // d_file_stat_fd64 has already reported the failure
    if (d_file_stat_fd64(fd,
                         &st) != 0)
    {
        return -1;
    }

    return (int64_t)st.st_size;
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_exists
  One stat, as d_file_stat64 asks. A NULL path does not exist: the caller
asked a yes/no, and "no" is a meaningful answer rather than an error -- as it
is for a path that is not there, which raises no notice.
*/
int
d_file_exists(
    const char* _path
)
{
    struct d_stat64_t st = {0};

    // a NULL path does not exist
    if (!_path)
    {
        return 0;
    }

    // a path that cannot be examined is not reported as existing
    if (d_internal_stat_path(_path,
                             &st,
                             D_INTERNAL_STAT_DEFAULT | D_INTERNAL_STAT_PROBE,
                             "d_file_exists") != 0)
    {
        return 0;
    }

    return 1;
}

/*
d_file_is_regular
  Regular specifically: a directory, device, FIFO or socket all answer 0, and
so does a dangling symlink, which has nothing to follow to.
*/
int
d_file_is_regular(
    const char* _path
)
{
    struct d_stat64_t st = {0};

    // a NULL path is not a regular file
    if (!_path)
    {
        return 0;
    }

    // a path that cannot be examined is not reported as a regular file
    if (d_internal_stat_path(_path,
                             &st,
                             D_INTERNAL_STAT_DEFAULT | D_INTERNAL_STAT_PROBE,
                             "d_file_is_regular") != 0)
    {
        return 0;
    }

    return (S_ISREG(st.st_mode) != 0);
}

/*
d_dir_exists
  One stat, as d_file_stat64 asks, so a symbolic link to a directory counts as
a directory.
*/
int
d_dir_exists(
    const char* _path
)
{
    struct d_stat64_t st = {0};

    // a NULL path is not a directory
    if (!_path)
    {
        return 0;
    }

    // a path that cannot be examined is not reported as a directory
    if (d_internal_stat_path(_path,
                             &st,
                             D_INTERNAL_STAT_DEFAULT | D_INTERNAL_STAT_PROBE,
                             "d_dir_exists") != 0)
    {
        return 0;
    }

    return (S_ISDIR(st.st_mode) != 0);
}

//==============================================================================
// 2.  32-BIT TWINS
//==============================================================================

/*
d_internal_stat_fits_int32
  1 when a signed 64-bit value has a 32-bit twin: between INT32_MIN and
INT32_MAX, so a time before 1901 overflows as surely as one after 2038.
*/
static int
d_internal_stat_fits_int32(
    int64_t _value
)
{
    return ( (_value >= (int64_t)INT32_MIN) &&
             (_value <= (int64_t)INT32_MAX) );
}

/*
d_internal_stat_narrow
  Copies a d_stat64_t into the 32-bit layout, or fails with EOVERFLOW when any
value does not fit -- checked before anything is written, so the output is
the whole answer or the zeroes the caller already put there. Only the fields
that are 64-bit in the wide layout need a check; the rest are 32-bit in both.
*/
static int
d_internal_stat_narrow(
    const struct d_stat64_t* _wide,
    struct d_stat32_t*       _out,
    const char*              _fn,
    const char*              _path
)
{
    // referenced only by the notification path, which may be compiled out
    (void)_fn;
    (void)_path;

    // one value that does not fit fails the whole answer
    if ( (_wide->st_size > (uint64_t)UINT32_MAX)            ||
         (_wide->st_dev  > (uint64_t)UINT32_MAX)            ||
         (_wide->st_ino  > (uint64_t)UINT32_MAX)            ||
         (!d_internal_stat_fits_int32(_wide->st_modified)) ||
         (!d_internal_stat_fits_int32(_wide->st_accessed)) ||
         (!d_internal_stat_fits_int32(_wide->st_changed))  ||
         (!d_internal_stat_fits_int32(_wide->st_created)) )
    {
        D_INTERNAL_FILE_FAIL(EOVERFLOW,
                             _fn,
                             _path,
                             "a size, time, device or inode number exceeds "
                             "d_stat32_t",
                             -1);
    }

    _out->st_size          = (uint32_t)_wide->st_size;
    _out->st_modified      = (int32_t)_wide->st_modified;
    _out->st_accessed      = (int32_t)_wide->st_accessed;
    _out->st_changed       = (int32_t)_wide->st_changed;
    _out->st_created       = (int32_t)_wide->st_created;
    _out->st_modified_nsec = _wide->st_modified_nsec;
    _out->st_accessed_nsec = _wide->st_accessed_nsec;
    _out->st_changed_nsec  = _wide->st_changed_nsec;
    _out->st_mode          = _wide->st_mode;
    _out->st_nlink         = _wide->st_nlink;
    _out->st_uid           = _wide->st_uid;
    _out->st_gid           = _wide->st_gid;
    _out->st_dev           = (uint32_t)_wide->st_dev;
    _out->st_ino           = (uint32_t)_wide->st_ino;

    return 0;
}

/*
d_file_stat32
  d_file_stat64, narrowed. The output is zeroed first, so a failure of any
kind -- the stat's or the narrowing's -- leaves no stale value behind.
*/
int
d_file_stat32(
    const char*        _path,
    struct d_stat32_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_stat32",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_stat32",
                            _path,
                            "output buffer is NULL",
                            -1);

    struct d_stat64_t wide = {0};

    memset(_buf,
           0,
           sizeof(*_buf));

    // d_file_stat64 has already reported the failure
    if (d_file_stat64(_path,
                      &wide) != 0)
    {
        return -1;
    }

    return d_internal_stat_narrow(&wide,
                                  _buf,
                                  "d_file_stat32",
                                  _path);
}

/*
d_file_stat_nofollow32
  d_file_stat_nofollow64, narrowed, zeroing first as d_file_stat32 does.
*/
int
d_file_stat_nofollow32(
    const char*        _path,
    struct d_stat32_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_stat_nofollow32",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_stat_nofollow32",
                            _path,
                            "output buffer is NULL",
                            -1);

    struct d_stat64_t wide = {0};

    memset(_buf,
           0,
           sizeof(*_buf));

    // d_file_stat_nofollow64 has already reported the failure
    if (d_file_stat_nofollow64(_path,
                               &wide) != 0)
    {
        return -1;
    }

    return d_internal_stat_narrow(&wide,
                                  _buf,
                                  "d_file_stat_nofollow32",
                                  _path);
}

/*
d_file_stat_fd32
  d_file_stat_fd64, narrowed, zeroing first as d_file_stat32 does. On the ISO
C backend the 64-bit twin's ENOSYS passes straight through.
*/
int
d_file_stat_fd32(
    int                _fd,
    struct d_stat32_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_stat_fd32",
                            NULL,
                            "output buffer is NULL",
                            -1);

    struct d_stat64_t wide = {0};

    memset(_buf,
           0,
           sizeof(*_buf));

    // d_file_stat_fd64 checks the descriptor and reports its own failure
    if (d_file_stat_fd64(_fd,
                         &wide) != 0)
    {
        return -1;
    }

    return d_internal_stat_narrow(&wide,
                                  _buf,
                                  "d_file_stat_fd32",
                                  NULL);
}

/*
d_file_size32
  d_file_size64, narrowed: a size past INT32_MAX fails with EOVERFLOW, since
-1 must stay free to mean failure.
*/
int32_t
d_file_size32(
    const char* _path
)
{
    const int64_t size = d_file_size64(_path);

    // d_file_size64 has already reported the failure
    if (size < 0)
    {
        return -1;
    }

    // a size that does not fit is refused, not truncated
    if (size > (int64_t)INT32_MAX)
    {
        D_INTERNAL_FILE_FAIL(EOVERFLOW,
                             "d_file_size32",
                             _path,
                             "size exceeds int32_t",
                             -1);
    }

    return (int32_t)size;
}

/*
d_file_size_stream32
  d_file_size_stream64, narrowed as d_file_size32 is.
*/
int32_t
d_file_size_stream32(
    FILE* _stream
)
{
    const int64_t size = d_file_size_stream64(_stream);

    // d_file_size_stream64 has already reported the failure
    if (size < 0)
    {
        return -1;
    }

    // a size that does not fit is refused, not truncated
    if (size > (int64_t)INT32_MAX)
    {
        D_INTERNAL_FILE_FAIL(EOVERFLOW,
                             "d_file_size_stream32",
                             NULL,
                             "size exceeds int32_t",
                             -1);
    }

    return (int32_t)size;
}
