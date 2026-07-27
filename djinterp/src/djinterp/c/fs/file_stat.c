#include "../../../../inc/djinterp/c/fs/file_stat.h"

#if (D_INTERNAL_FILE_STAT_STATX == 1)
    // makedev lives here on glibc; <sys/stat.h> only used to drag it in and
    // stopped in 2.28, which is the same release that added statx -- so a
    // build new enough to have statx is exactly one that needs this include
    #include <sys/sysmacros.h>
#endif


///////////////////////////////////////////////////////////////////////////////
///             INTERNAL DEFINITIONS                                        ///
///////////////////////////////////////////////////////////////////////////////

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_INTERNAL_STAT_NATIVE struct _stat64
#else
    #define D_INTERNAL_STAT_NATIVE struct stat
#endif


#if (D_INTERNAL_FILE_STAT_STATX == 1)

/*
d_internal_stat_statx
  Fills d_stat_t from Linux statx(2).
  The reason this exists is btime: the creation time is in the inode, and
stat() has no field to hand it back in. statx does, and it returns in one
syscall what stat reports in pieces.
  It reports which fields it actually answered, via stx_mask, and this
respects that rather than assuming -- a filesystem that does not store btime
returns success WITHOUT STATX_BTIME set, and reading stx_btime then would be
reading a zero and calling it a timestamp.

Parameter(s):
  _path:   path to query.
  _flags:  AT_SYMLINK_NOFOLLOW to describe a link rather than its target.
  _out:    receives the status.
Return:
  0 on success, or -1 on failure with errno set. ENOSYS means the kernel or
the sandbox refused and the caller should fall back to stat().
*/
static int
d_internal_stat_statx
(
    const char*      _path,
    int              _flags,
    struct d_stat_t* _out
)
{
    struct statx stx;

    if (statx(AT_FDCWD, _path, _flags, STATX_ALL, &stx) != 0)
    {
        return -1;
    }

    memset(_out, 0, sizeof(*_out));

    _out->st_size  = (uint64_t)stx.stx_size;
    _out->st_mode  = (uint32_t)stx.stx_mode;
    _out->st_nlink = (uint32_t)stx.stx_nlink;
    _out->st_uid   = (uint32_t)stx.stx_uid;
    _out->st_gid   = (uint32_t)stx.stx_gid;
    _out->st_ino   = (uint64_t)stx.stx_ino;

    // statx reports the device split into major/minor rather than as the
    // opaque dev_t stat uses; recombine so st_dev means the same thing on
    // both paths and d_stat_t stays one type
    _out->st_dev = (uint64_t)makedev(stx.stx_dev_major, stx.stx_dev_minor);

    _out->st_modified      = (int64_t)stx.stx_mtime.tv_sec;
    _out->st_accessed      = (int64_t)stx.stx_atime.tv_sec;
    _out->st_changed       = (int64_t)stx.stx_ctime.tv_sec;

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

    // the whole point -- but only when the filesystem actually stored one.
    // statx succeeds without STATX_BTIME on a filesystem that does not, and
    // reading stx_btime then would be reading a zero and calling it a date.
    #if (D_INTERNAL_FILE_STAT_BIRTHTIME == 1)
    if ((stx.stx_mask & STATX_BTIME) != 0)
    {
        _out->st_created = (int64_t)stx.stx_btime.tv_sec;
    }
    #endif

    return 0;
}

#endif  // D_INTERNAL_FILE_STAT_STATX


/*
d_internal_stat_fill
  Translates the platform's struct stat into djinterp's.
  This is the one function that knows how the target spells a timestamp, and
it is where D_CFG_FILE_HAS_STAT_NSEC earns its keep: POSIX 2008 says
st_mtim.tv_nsec, macOS and the BSDs say st_mtimespec.tv_nsec, and older hosts
say nothing at all. env cannot rename djinterp's own fields to dodge the
st_mtime macro collision, but it can say which member to read -- and this is
the only place that has to care.

Parameter(s):
  _native: the platform's status structure.
  _out:    the djinterp structure to populate; fully overwritten.
Return:
  none.
*/
static void
d_internal_stat_fill
(
    const D_INTERNAL_STAT_NATIVE* _native,
    struct d_stat_t*              _out
)
{
    // zero first: every field this platform cannot answer must read 0 rather
    // than whatever was on the caller's stack
    memset(_out, 0, sizeof(*_out));

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

    #if ( (D_INTERNAL_FILE_STAT_BIRTHTIME == 1) && defined(__APPLE__) )
    _out->st_created = (int64_t)_native->st_birthtimespec.tv_sec;
    #endif
#endif

    return;
}


/*
d_internal_stat_path
  The single path-based stat entry point, so the follow-vs-do-not-follow
decision is made once rather than at three call sites.

Parameter(s):
  _path:   path to query.
  _out:    receives the status.
  _follow: 1 to resolve a symbolic link to its target, 0 to describe the link.
  _fn:     caller's name, for diagnostics.
Return:
  0 on success, or -1 on failure with errno set.
*/
static int
d_internal_stat_path
(
    const char*      _path,
    struct d_stat_t* _out,
    int              _follow,
    const char*      _fn
)
{
    D_INTERNAL_STAT_NATIVE native;
    int                    result;

    // referenced only by the notification path, which may be compiled out
    (void)_fn;

#if (D_INTERNAL_FILE_STAT_STATX == 1)
    // one syscall, more fields, and the only route to a creation time here.
    // Falls through to stat() when the kernel is too old or a sandbox has
    // filtered the call -- st_created then reads 0, which is the same honest
    // answer every other btime-less platform gives.
    {
        static int statx_usable = 1;

        if (statx_usable)
        {
            int flags;

            flags = _follow ? 0 : AT_SYMLINK_NOFOLLOW;

            if (d_internal_stat_statx(_path, flags, _out) == 0)
            {
                return 0;
            }

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
                                       "statx unavailable; falling back to stat for this process");
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

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Win32's CRT has no lstat. Reparse points exist, but _stat64 always
    // follows them, so a link cannot be described here -- d_lstat says so
    // rather than silently returning the target's status.
    (void)_follow;
    result = _stat64(_path, &native);
#else
    if (_follow)
    {
        result = stat(_path, &native);
    }
    else
    {
        result = lstat(_path, &native);
    }
#endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               _fn,
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "stat failed");

        return -1;
    }

    d_internal_stat_fill(&native, _out);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
///             I.  STATUS                                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_stat
  Retrieves the status of a path, following symbolic links.
  Follows because that is what stat() means and what callers expect -- d_is_dir
on a link to a directory says yes. Use d_lstat to describe the link itself,
or set D_CFG_FILE_STAT_FOLLOW_SYMLINKS to 0 to make every query here stop at
the link (an archiver or backup tool that must not traverse).

Parameter(s):
  _path: path to query.
  _buf:  receives the status; fully overwritten, including fields this
         platform cannot answer, which are zeroed rather than left alone.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_stat
(
    const char*      _path,
    struct d_stat_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_stat",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_stat",
                            _path,
                            "output buffer is NULL",
                            -1);

    return d_internal_stat_path(_path,
                                _buf,
                                D_INTERNAL_FILE_STAT_FOLLOW,
                                "d_stat");
}


/*
d_lstat
  Retrieves the status of a path WITHOUT following a symbolic link, so the
result describes the link itself.
  On Windows the CRT has no lstat and _stat64 always follows a reparse point.
Rather than silently returning the target's status -- which would make
d_lstat and d_stat indistinguishable, and any link-detection built on them
wrong -- this behaves identically to d_stat there. Check
D_INTERNAL_FILE_HAS_SYMLINKS and use d_is_symlink (file_link) when the
distinction matters.

Parameter(s):
  _path: path to query.
  _buf:  receives the status.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_lstat
(
    const char*      _path,
    struct d_stat_t* _buf
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_lstat",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_lstat",
                            _path,
                            "output buffer is NULL",
                            -1);

    return d_internal_stat_path(_path, _buf, 0, "d_lstat");
}


/*
d_fstat
  Retrieves the status of an open descriptor.
  The only member of this module with no TOCTOU hazard: a descriptor names one
open file description for as long as you hold it, and nothing can swap it
underneath you. When a decision must be about the file you are actually going
to use, open first and ask this -- not d_stat on the path you are about to
open.

Parameter(s):
  _fd:  an open descriptor.
  _buf: receives the status.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_fstat
(
    int              _fd,
    struct d_stat_t* _buf
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_fd;
    (void)_buf;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_fstat",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    D_INTERNAL_STAT_NATIVE native;
    int                    result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_fd >= 0,
                            EBADF,
                            "d_fstat",
                            NULL,
                            "descriptor is negative",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_buf != NULL,
                            EINVAL,
                            "d_fstat",
                            NULL,
                            "output buffer is NULL",
                            -1);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _fstat64(_fd, &native);
    #else
    result = fstat(_fd, &native);
    #endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_fstat",
                               NULL,
                               "fstat failed");

        return -1;
    }

    d_internal_stat_fill(&native, _buf);

    return 0;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             II.  PERMISSIONS                                            ///
///////////////////////////////////////////////////////////////////////////////

/*
d_access
  Asks whether the current user may do something to a path.
  Read the answer narrowly. It is a statement about permission bits at one
instant, not a promise: the file can change between this call and the open,
and on a set-uid program access() checks the REAL user while open() checks the
effective one -- which is the classic privilege-escalation pattern this
function is famous for. Use it to produce a better error message, not to make
a security decision. To decide, just open the file and handle the failure.

Parameter(s):
  _path: path to test.
  _mode: F_OK, or R_OK / W_OK / X_OK OR'd together.
Return:
  0 when the access is permitted, or -1 otherwise with errno set.
*/
int
d_access
(
    const char* _path,
    int         _mode
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_access",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // the CRT has no notion of execute permission; asking for it on a file
    // that exists would report failure, which is a worse answer than the one
    // Windows can actually give
    result = _access(_path, _mode & (~X_OK));
#else
    result = access(_path, _mode);
#endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                               errno,
                               "d_access",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "access denied or path absent");

        return -1;
    }

    return 0;
}


/*
d_chmod
  Sets the permission bits of a path.
  On Windows only the write bit exists: the CRT maps the mode to the read-only
attribute and discards everything else. Passing 0600 there produces a
writable file readable by anyone, and no error -- the platform simply has no
way to express what you asked for.

Parameter(s):
  _path: path to modify.
  _mode: permission bits (S_IRUSR, S_IWUSR, ...).
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_chmod
(
    const char* _path,
    uint32_t    _mode
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_chmod",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _chmod(_path, (int)_mode);
#else
    result = chmod(_path, (mode_t)_mode);
#endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_chmod",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "chmod failed");

        return -1;
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  SIZE                                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_file_size
  Reports the size of a file in bytes.
  It reports what the filesystem says, which for a /proc or /sys entry is 0
even though reading it produces kilobytes. That is the correct answer to
"what size does this file report" and the wrong answer to "how many bytes will
I get". For the second question use d_fread_all (file_io), which distrusts a
reported zero for exactly this reason.

Parameter(s):
  _path: path to measure.
Return:
  The size in bytes, or -1 on failure with errno set.
*/
int64_t
d_file_size
(
    const char* _path
)
{
    struct d_stat_t st;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_size",
                            NULL,
                            "path is NULL",
                            -1);

    if (d_stat(_path, &st) != 0)
    {
        return -1;
    }

    return (int64_t)st.st_size;
}


/*
d_file_size_stream
  Reports the size of the file behind a stream.
  Uses the descriptor rather than seeking to the end: seeking would perturb
the position, fail on a pipe, and on a Windows text-mode stream report a
length that disagrees with what a read will actually produce.

Parameter(s):
  _stream: an open stream.
Return:
  The size in bytes, or -1 on failure with errno set.
*/
int64_t
d_file_size_stream
(
    FILE* _stream
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_stream;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_size_stream",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    struct d_stat_t st;
    int             fd;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_stream != NULL,
                            EINVAL,
                            "d_file_size_stream",
                            NULL,
                            "stream is NULL",
                            -1);

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    fd = _fileno(_stream);
    #else
    fd = fileno(_stream);
    #endif

    if (fd < 0)
    {
        D_INTERNAL_FILE_FAIL(EBADF,
                             "d_file_size_stream",
                             NULL,
                             "stream has no descriptor",
                             -1);
    }

    if (d_fstat(fd, &st) != 0)
    {
        return -1;
    }

    return (int64_t)st.st_size;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///             IV.  PREDICATES                                             ///
///////////////////////////////////////////////////////////////////////////////

/*
d_file_exists
  Reports whether a path names anything at all -- file, directory, device or
socket.
  One syscall, and an answer that is already stale when it returns. Fine for a
diagnostic; not a basis for a decision. See the TOCTOU note in file_stat.h.

Parameter(s):
  _path: path to test; may be NULL.
Return:
  Non-zero when the path exists, 0 when it does not or is NULL.
*/
int
d_file_exists
(
    const char* _path
)
{
    struct d_stat_t st;

    // a NULL path does not exist; the caller asked a yes/no and "no" is a
    // meaningful answer rather than an error
    if (!_path)
    {
        return 0;
    }

    return (d_stat(_path, &st) == 0);
}


/*
d_is_file
  Reports whether a path names a regular file.
  Regular specifically: a directory, device, FIFO or socket all answer 0, and
so does a dangling symlink (nothing to follow to).

Parameter(s):
  _path: path to test; may be NULL.
Return:
  Non-zero when the path is a regular file, 0 otherwise or when NULL.
*/
int
d_is_file
(
    const char* _path
)
{
    struct d_stat_t st;

    if (!_path)
    {
        return 0;
    }

    if (d_stat(_path, &st) != 0)
    {
        return 0;
    }

    return (S_ISREG(st.st_mode) != 0);
}


/*
d_is_dir
  Reports whether a path names a directory.

Parameter(s):
  _path: path to test; may be NULL.
Return:
  Non-zero when the path is a directory, 0 otherwise or when NULL.
*/
int
d_is_dir
(
    const char* _path
)
{
    struct d_stat_t st;

    if (!_path)
    {
        return 0;
    }

    if (d_stat(_path, &st) != 0)
    {
        return 0;
    }

    return (S_ISDIR(st.st_mode) != 0);
}
