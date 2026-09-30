/*******************************************************************************
* djinterp [c]                                                       file_stat.c
*
* Implementation of the metadata queries declared in file_stat.h.
*   Every status query ends in one translation, d_internal_stat_fill, the only
* code that knows how the target spells a timestamp. On Linux, statx is tried
* first because it is the only route to a creation time; a kernel or sandbox
* that refuses it is remembered for the life of the process. The predicates
* are single d_file_stat calls.
*
*
* path:      /src/djinterp/c/fs/file_stat.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_stat.h"  // corresponding header
// std
#include <errno.h>   // errno, EBADF, EINVAL, ENOSYS, EPERM
#include <stdint.h>  // int64_t, uint32_t, uint64_t
#include <stdio.h>   // FILE, fileno
#include <string.h>  // memset
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/fs/cfg_file_stat.h"  // D_INTERNAL_FILE_STAT_*
// posix
#if (D_INTERNAL_FILE_STAT_STATX == 1)
    // makedev lives here on glibc. <sys/stat.h> only ever dragged it in
    // implicitly, and stopped doing so in 2.28 -- the same release that added
    // statx -- so a build new enough to have statx is exactly one that needs
    // this include.
    #include <sys/sysmacros.h>  // makedev
#endif


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_STAT_NATIVE
//   macro: the platform's own status structure, which d_internal_stat_fill
// translates from.
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_INTERNAL_STAT_NATIVE struct _stat64
#else
    #define D_INTERNAL_STAT_NATIVE struct stat
#endif

#if (D_INTERNAL_FILE_STAT_STATX == 1)

/*
d_internal_stat_statx
  Fills d_stat_t from Linux statx(2). The reason this exists is btime: the
creation time is in the inode, and stat() has no field to hand it back in.
statx does, and returns in one syscall what stat reports in pieces.
  statx reports which fields it actually answered, via stx_mask, and that is
respected rather than assumed: a filesystem that does not store btime returns
success WITHOUT STATX_BTIME set, and reading stx_btime then would be reading a
zero and calling it a timestamp. A failure with ENOSYS or EPERM means the
kernel or a sandbox refused, and the caller falls back to stat().
*/
static int
d_internal_stat_statx(
    const char*      _path,
    int              _flags,
    struct d_stat_t* _out
)
{
    struct statx stx;

    // the caller decides what a refusal means
    if (statx(AT_FDCWD,
              _path,
              _flags,
              STATX_ALL,
              &stx) != 0)
    {
        return -1;
    }

    memset(_out,
           0,
           sizeof(*_out));
    _out->st_size  = (uint64_t)stx.stx_size;
    _out->st_mode  = (uint32_t)stx.stx_mode;
    _out->st_nlink = (uint32_t)stx.stx_nlink;
    _out->st_uid   = (uint32_t)stx.stx_uid;
    _out->st_gid   = (uint32_t)stx.stx_gid;
    _out->st_ino   = (uint64_t)stx.stx_ino;

    // statx reports the device split into major/minor rather than as the
    // opaque dev_t stat uses; recombine so st_dev means the same thing on
    // both paths and d_stat_t stays one type
    _out->st_dev = (uint64_t)makedev(stx.stx_dev_major,
                                     stx.stx_dev_minor);

    _out->st_modified = (int64_t)stx.stx_mtime.tv_sec;
    _out->st_accessed = (int64_t)stx.stx_atime.tv_sec;
    _out->st_changed  = (int64_t)stx.stx_ctime.tv_sec;

    //   The knobs are honoured HERE too, not just on the plain-stat path.
    // statx hands back sub-second and birth times whether or not this build
    // asked for them, so filling them unconditionally made the query macros
    // lie in the opposite direction from the bug they were added for:
    // D_FILE_STAT_HAS_NSEC would report 0 while the fields carried real data.
    // A caller cannot defend against a macro that says no and means yes any
    // more than one that says yes and means no.
#if (D_INTERNAL_FILE_STAT_NSEC != 0)
    _out->st_modified_nsec = (uint32_t)stx.stx_mtime.tv_nsec;
    _out->st_accessed_nsec = (uint32_t)stx.stx_atime.tv_nsec;
    _out->st_changed_nsec  = (uint32_t)stx.stx_ctime.tv_nsec;
#endif

#if (D_INTERNAL_FILE_STAT_BIRTHTIME == 1)
    // the whole point -- but only when the filesystem actually stored one.
    // statx succeeds without STATX_BTIME on a filesystem that does not, and
    // reading stx_btime then would be reading a zero and calling it a date.
    if ((stx.stx_mask & STATX_BTIME) != 0)
    {
        _out->st_created = (int64_t)stx.stx_btime.tv_sec;
    }
#endif

    return 0;
}

#endif  // D_INTERNAL_FILE_STAT_STATX == 1

/*
d_internal_stat_fill
  Translates the platform's struct stat into djinterp's. This is the one
function that knows how the target spells a timestamp, and it is where
D_CFG_FILE_HAS_STAT_NSEC earns its keep: POSIX 2008 says st_mtim.tv_nsec,
macOS and the BSDs say st_mtimespec.tv_nsec, and older hosts say nothing at
all. env cannot rename djinterp's own fields to dodge the st_mtime macro
collision, but it can say which member to read -- and this is the only place
that has to care. The output is zeroed first, so every field the platform
cannot answer reads 0 rather than whatever was on the caller's stack.
*/
static void
d_internal_stat_fill(
    const D_INTERNAL_STAT_NATIVE* _native,
    struct d_stat_t*              _out
)
{
    // zero first: every field this platform cannot answer must read 0 rather
    // than whatever was on the caller's stack
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

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Windows reports creation, last-access and last-write. It has no
    // metadata-change time at all, so st_changed stays 0 -- substituting
    // st_created there is exactly the conflation the field names exist to
    // prevent.
    _out->st_modified = (int64_t)_native->st_mtime;
    _out->st_accessed = (int64_t)_native->st_atime;
    #if (D_INTERNAL_FILE_STAT_BIRTHTIME == 1)
    _out->st_created  = (int64_t)_native->st_ctime;   // Win32 ctime IS creation
    #endif
#else
    // POSIX: st_mtime and friends are MACROS here, not members -- the whole
    // reason d_stat_t's fields are named st_modified. Reading them off the
    // platform's struct is fine; declaring fields by those names is not.
    _out->st_modified = (int64_t)_native->st_mtime;
    _out->st_accessed = (int64_t)_native->st_atime;
    _out->st_changed  = (int64_t)_native->st_ctime;
    #if (D_INTERNAL_FILE_STAT_NSEC == 1)
    // POSIX.1-2008: st_mtim is a struct timespec
    _out->st_modified_nsec = (uint32_t)_native->st_mtim.tv_nsec;
    _out->st_accessed_nsec = (uint32_t)_native->st_atim.tv_nsec;
    _out->st_changed_nsec  = (uint32_t)_native->st_ctim.tv_nsec;
    #elif (D_INTERNAL_FILE_STAT_NSEC == 2)
    // macOS / BSD got there first with a different member name
    _out->st_modified_nsec = (uint32_t)_native->st_mtimespec.tv_nsec;
    _out->st_accessed_nsec = (uint32_t)_native->st_atimespec.tv_nsec;
    _out->st_changed_nsec  = (uint32_t)_native->st_ctimespec.tv_nsec;
    #endif
    #if ( (D_INTERNAL_FILE_STAT_BIRTHTIME == 1) &&                             \
          (defined(__APPLE__)) )
    _out->st_created = (int64_t)_native->st_birthtimespec.tv_sec;
    #endif
#endif

    return;
}

/*
d_internal_stat_path
  The single path-based stat entry point, so the follow-or-not decision is made
once rather than at three call sites. Where statx is compiled in it goes
first; a refusal (ENOSYS, EPERM) is remembered in a function-local flag for
the life of the process and the call falls through to stat(), while any other
statx failure is a real error about a real path and is returned as such.
*/
static int
d_internal_stat_path(
    const char*      _path,
    struct d_stat_t* _out,
    int              _follow,
    const char*      _fn
)
{
    // referenced only by the notification path, which may be compiled out
    (void)_fn;

#if (D_INTERNAL_FILE_STAT_STATX == 1)
    // one syscall, more fields, and the only route to a creation time here.
    // Falls through to stat() when the kernel is too old or a sandbox has
    // filtered the call -- st_created then reads 0, which is the same honest
    // answer every other btime-less platform gives.
    {
        static int statx_usable = 1;

        // once refused, statx is not asked again
        if (statx_usable)
        {
            const int flags = _follow ? 0 : AT_SYMLINK_NOFOLLOW;

            // the fast path answered
            if (d_internal_stat_statx(_path,
                                      flags,
                                      _out) == 0)
            {
                return 0;
            }

            // a refusal is remembered; anything else is a real error
            if ( (errno == ENOSYS) ||
                 (errno == EPERM) )
            {
                // remember, so the next 10,000 calls do not each pay for a
                // syscall that is never going to work
                statx_usable = 0;
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                                       errno,
                                       _fn,
                                       NULL,
                                       "statx unavailable; falling back to "
                                       "stat for this process");
            }
            else
            {
                // a real error about a real path -- report it as statx saw it
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                       errno,
                                       _fn,
                                       D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                       "statx failed");

                return -1;
            }
        }
    }
#endif

    D_INTERNAL_STAT_NATIVE native;
    int                    result = -1;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Win32's CRT has no lstat. Reparse points exist, but _stat64 always
    // follows them, so a link cannot be described here -- d_file_stat_nofollow
    // says so rather than silently returning the target's status.
    (void)_follow;
    result = _stat64(_path,
                     &native);
#else
    // stat resolves a symbolic link; lstat describes the link itself
    if (_follow)
    {
        result = stat(_path,
                      &native);
    }
    else
    {
        result = lstat(_path,
                       &native);
    }
#endif

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               _fn,
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "stat failed");

        return -1;
    }

    d_internal_stat_fill(&native,
                         _out);

    return 0;
}

//==============================================================================
// 1.  METADATA
//==============================================================================

/*
d_file_stat
  Follows, because that is what stat() means and what callers expect --
d_dir_exists on a link to a directory says yes. Whether it follows is
D_INTERNAL_FILE_STAT_FOLLOW, so a build can make every query here stop at the
link instead.
*/
int
d_file_stat(
    const char*      _path,
    struct d_stat_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_stat",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_stat",
                            _path,
                            "output buffer is NULL",
                            -1);

    return d_internal_stat_path(_path,
                                _buf,
                                D_INTERNAL_FILE_STAT_FOLLOW,
                                "d_file_stat");
}

/*
d_file_stat_nofollow
  Passes "do not follow" down explicitly, so it never depends on
D_CFG_FILE_STAT_FOLLOW_SYMLINKS. On Windows the underlying _stat64 follows
reparse points regardless, which the declaration warns about.
*/
int
d_file_stat_nofollow(
    const char*      _path,
    struct d_stat_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_stat_nofollow",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_stat_nofollow",
                            _path,
                            "output buffer is NULL",
                            -1);

    return d_internal_stat_path(_path,
                                _buf,
                                0,
                                "d_file_stat_nofollow");
}

#if D_FILE_BACKEND_IS_STDC

/*
d_file_stat_fd
  The ISO C backend has no descriptors, so this only reports ENOSYS.
*/
int
d_file_stat_fd(
    int              _fd,
    struct d_stat_t* _buf
)
{
    (void)_fd;
    (void)_buf;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_stat_fd",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_stat_fd
  fstat on the descriptor, then the same translation every path query uses,
so the two answers agree field for field.
*/
int
d_file_stat_fd(
    int              _fd,
    struct d_stat_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_file_stat_fd",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_file_stat_fd",
                            NULL,
                            "output buffer is NULL",
                            -1);

    D_INTERNAL_STAT_NATIVE native;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int result = _fstat64(_fd,
                                &native);
#else
    const int result = fstat(_fd,
                             &native);
#endif

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_stat_fd",
                               NULL,
                               "fstat failed");

        return -1;
    }

    d_internal_stat_fill(&native,
                         _buf);

    return 0;
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_access
  The CRT has no notion of execute permission, so X_OK is masked off on
Windows: asking for it on a file that exists would report a failure, a worse
answer than the one Windows can give. A refusal is reported at info severity,
because "no" is the answer this function exists to give.
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

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // the CRT has no notion of execute permission; asking for it on a file
    // that exists would report failure, which is a worse answer than the one
    // Windows can actually give
    const int result = _access(_path,
                               _mode & (~X_OK));
#else
    const int result = access(_path,
                              _mode);
#endif

    // a refusal is an answer, reported at info severity
    if (result != 0)
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
and discards everything else without complaint.
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

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int result = _chmod(_path,
                              (int)_mode);
#else
    const int result = chmod(_path,
                             (mode_t)_mode);
#endif

    // report the failure; errno is the platform's
    if (result != 0)
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
d_file_size
  What the filesystem says, which for a /proc or /sys entry is 0 though reading
it produces kilobytes -- the right answer to "what size does this file
report" and the wrong one to "how many bytes will I get". d_file_read_all
answers the second by distrusting a reported zero.
*/
int64_t
d_file_size(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_size",
                            NULL,
                            "path is NULL",
                            -1);

    struct d_stat_t st;

    // d_file_stat has already reported the failure
    if (d_file_stat(_path,
                    &st) != 0)
    {
        return -1;
    }

    return (int64_t)st.st_size;
}

#if D_FILE_BACKEND_IS_STDC

/*
d_file_size_stream
  The ISO C backend has no descriptors to ask, so this only reports ENOSYS.
*/
int64_t
d_file_size_stream(
    FILE* _stream
)
{
    (void)_stream;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_size_stream",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_size_stream
  Asks the descriptor rather than seeking to the end: seeking would perturb
the position, fail on a pipe, and on a Windows text-mode stream report a
length that disagrees with what a read will actually produce.
*/
int64_t
d_file_size_stream(
    FILE* _stream
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_size_stream",
                            NULL,
                            "stream is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int fd = _fileno(_stream);
#else
    const int fd = fileno(_stream);
#endif

    // a stream with no descriptor cannot be measured this way
    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_file_size_stream",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    struct d_stat_t st;

    // d_file_stat_fd has already reported the failure
    if (d_file_stat_fd(fd,
                       &st) != 0)
    {
        return -1;
    }

    return (int64_t)st.st_size;
}

#endif  // D_FILE_BACKEND_IS_STDC

/*
d_file_exists
  One stat call. A NULL path does not exist: the caller asked a yes/no, and
"no" is a meaningful answer rather than an error.
*/
int
d_file_exists(
    const char* _path
)
{
    // a NULL path does not exist; the caller asked a yes/no and "no" is a
    // meaningful answer rather than an error
    if (!_path)
    {
        return 0;
    }

    struct d_stat_t st;

    return (d_file_stat(_path,
                        &st) == 0);
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
    // a NULL path is not a regular file
    if (!_path)
    {
        return 0;
    }

    struct d_stat_t st;

    // a path that cannot be examined is not reported as a regular file
    if (d_file_stat(_path,
                    &st) != 0)
    {
        return 0;
    }

    return (S_ISREG(st.st_mode) != 0);
}

/*
d_dir_exists
  One stat call through d_file_stat, so a symbolic link to a directory counts
as a directory.
*/
int
d_dir_exists(
    const char* _path
)
{
    // a NULL path is not a directory
    if (!_path)
    {
        return 0;
    }

    struct d_stat_t st;

    // a path that cannot be examined is not reported as a directory
    if (d_file_stat(_path,
                    &st) != 0)
    {
        return 0;
    }

    return (S_ISDIR(st.st_mode) != 0);
}
