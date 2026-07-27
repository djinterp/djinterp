#include "../../../../inc/djinterp/c/fs/file_ops.h"
#include "../../../../inc/djinterp/c/fs/file_desc.h"
#include "../../../../inc/djinterp/c/fs/file_io.h"
#include "../../../../inc/djinterp/c/fs/file_stat.h"

#if ( (D_INTERNAL_FILE_OPS_COPY_NATIVE == 1) &&                               \
      D_CFG_IS_ON(D_CFG_FILE_HAS_FCOPYFILE) )
    #include <copyfile.h>
#endif


///////////////////////////////////////////////////////////////////////////////
///             INTERNAL DEFINITIONS                                        ///
///////////////////////////////////////////////////////////////////////////////

#if !D_FILE_BACKEND_IS_STDC
//   Not built on the ISO C backend: the only caller is in the non-STDC
// branch below, so defining it there is an unused function and a warning.

/*
d_internal_ops_copy_portable
  Copies a file's contents by moving bytes through a user-space buffer.
  The fallback that always works: no kernel offload, no filesystem
cooperation, identical on every target. The native engines below are faster;
this one is the definition of correct.

Parameter(s):
  _in:  descriptor open for reading, positioned at 0.
  _out: descriptor open for writing, positioned at 0.
Return:
  0 on success, or -1 on failure with errno set.
*/
static int
d_internal_ops_copy_portable
(
    int _in,
    int _out
)
{
    char    buffer[D_INTERNAL_FILE_OPS_COPY_BUF];
    ssize_t got;

    for (;;)
    {
        got = d_read(_in, buffer, sizeof(buffer));

        if (got < 0)
        {
            return -1;
        }

        if (got == 0)
        {
            break;
        }

        // d_write_full, not d_write: a short write here is normal and losing
        // the remainder would corrupt the copy silently
        if (d_write_full(_out, buffer, (size_t)got) < 0)
        {
            return -1;
        }
    }

    return 0;
}


#if (D_INTERNAL_FILE_OPS_COPY_NATIVE == 1)

/*
d_internal_ops_copy_native
  Asks the platform to copy the file itself.
    Linux : copy_file_range(2) -- never leaves the kernel, and the filesystem
            may service it directly. On btrfs or XFS that means a reflink: the
            copy is instant and consumes no additional space until one side is
            written.
    macOS : fcopyfile(3) -- brings extended attributes and resource forks
            along, which the portable path silently drops.
  Both may decline. copy_file_range returns EXDEV across filesystems, and
EINVAL or ENOSYS on kernels or filesystems that never supported it; those are
"use the other path", not "the copy failed", and are reported as such.

Parameter(s):
  _in:   descriptor open for reading.
  _out:  descriptor open for writing.
  _size: the source's size in bytes.
Return:
  0 on success, 1 when the platform declined and the caller should fall back,
or -1 on a real failure with errno set.
*/
static int
d_internal_ops_copy_native
(
    int     _in,
    int     _out,
    int64_t _size
)
{
    #if D_CFG_IS_ON(D_CFG_FILE_HAS_COPY_FILE_RANGE)
    ssize_t moved;
    size_t  remaining;

    remaining = (size_t)_size;

    while (remaining > 0)
    {
        moved = copy_file_range(_in, NULL, _out, NULL, remaining, 0);

        if (moved < 0)
        {
            // the kernel or the filesystem cannot do this pairing -- not an
            // error, just a decline. Anything else is real.
            if ( (errno == EXDEV) ||
                 (errno == EINVAL) ||
                 (errno == ENOSYS) ||
                 (errno == EOPNOTSUPP) ||
                 (errno == EPERM) )
            {
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                                       errno,
                                       "d_copy_file",
                                       NULL,
                                       "copy_file_range declined; using the portable path");

                return 1;
            }

            if (errno == EINTR)
            {
                continue;
            }

            return -1;
        }

        // no progress and no error: stop rather than spin
        if (moved == 0)
        {
            break;
        }

        remaining -= (size_t)moved;
    }

    return 0;
    #elif D_CFG_IS_ON(D_CFG_FILE_HAS_FCOPYFILE)
    (void)_size;

    if (fcopyfile(_in, _out, NULL, COPYFILE_ALL) < 0)
    {
        return -1;
    }

    return 0;
    #else
    (void)_in;
    (void)_out;
    (void)_size;

    // a native engine was configured but none is reachable from here
    return 1;
    #endif
}

#endif  // D_INTERNAL_FILE_OPS_COPY_NATIVE

#endif // !D_FILE_BACKEND_IS_STDC


///////////////////////////////////////////////////////////////////////////////
///             I.  REMOVAL                                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
d_remove
  Removes a file or an empty directory (ISO C remove).
  Accepts both, which is the one thing that distinguishes it from d_unlink:
POSIX remove() calls rmdir() for a directory and unlink() otherwise.

Parameter(s):
  _path: path to remove.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_remove
(
    const char* _path
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_remove",
                            NULL,
                            "path is NULL",
                            -1);

    result = remove(_path);

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_remove",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "remove failed");

        return -1;
    }

    return 0;
}


/*
d_unlink
  Removes a name from the filesystem.
  Removes the NAME, not necessarily the file: the data survives while any
other hard link, or any process's open descriptor, still refers to it. That is
why unlinking an open file is a legitimate way to make a temporary that
disappears on exit even if the process is killed.
  Refuses directories, unlike d_remove.

Parameter(s):
  _path: path to unlink.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_unlink
(
    const char* _path
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_unlink",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    result = _unlink(_path);
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    result = unlink(_path);
#else
    // ISO C has only remove(), which also takes directories -- so on this
    // backend d_unlink cannot keep its promise to refuse them
    result = remove(_path);
#endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_unlink",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "unlink failed");

        return -1;
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
///             II.  MOVEMENT                                               ///
///////////////////////////////////////////////////////////////////////////////

/*
d_rename
  Renames or moves a file, atomically within one filesystem.
  Atomic is the point: an observer sees the old name or the new one, never
neither and never both, and never a partial file. That property is what makes
rename the last step of every safe-write pattern -- including d_fwrite_all's
own, when D_CFG_FILE_WRITE_ATOMIC is on.
  The atomicity does NOT extend across filesystems. rename() returns EXDEV
there, and by default this reports it rather than substituting a slow,
non-atomic copy-then-delete behind the caller's back (see
D_CFG_FILE_OPS_RENAME_CROSS_DEVICE).
  _overwrite is a parameter rather than a knob because it is a per-call
decision: POSIX rename always clobbers, Win32 rename never does, so neither
platform's default is portable and every caller must say which it wants.

Parameter(s):
  _old:       existing path.
  _new:       new path.
  _overwrite: non-zero to replace an existing _new, 0 to fail with EEXIST.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_rename
(
    const char* _old,
    const char* _new,
    int         _overwrite
)
{
    int result;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_old != NULL,
                            EINVAL,
                            "d_rename",
                            NULL,
                            "source path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_new != NULL,
                            EINVAL,
                            "d_rename",
                            _old,
                            "destination path is NULL",
                            -1);

    // POSIX rename replaces silently, so refusing has to be done here. This
    // check is NOT atomic with the rename below -- another process can create
    // _new in between -- but there is no portable rename-if-absent, and the
    // alternative is not offering the option at all.
    if (!_overwrite)
    {
        if (d_file_exists(_new))
        {
            D_INTERNAL_FILE_FAIL(EEXIST,
                                 "d_rename",
                                 _new,
                                 "destination exists and overwrite was not requested",
                                 -1);
        }
    }

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Win32's rename() fails when the destination exists; MoveFileEx is the
    // only spelling that replaces atomically
    if (_overwrite)
    {
        if (!MoveFileExA(_old, _new, MOVEFILE_REPLACE_EXISTING))
        {
            D_INTERNAL_FILE_SET_ERR(EACCES);
            result = -1;
        }
        else
        {
            result = 0;
        }
    }
    else
    {
        result = rename(_old, _new);
    }
#else
    result = rename(_old, _new);

    #if D_CFG_IS_ON(D_CFG_FILE_OPS_RENAME_CROSS_DEVICE)
    // the caller has explicitly accepted non-atomic movement across
    // filesystems; see the knob's note
    if ( (result != 0) &&
         (errno == EXDEV) )
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                               EXDEV,
                               "d_rename",
                               D_INTERNAL_FILE_NOTIFY_PATH(_old),
                               "cross-device; falling back to a NON-ATOMIC copy+delete");

        if (d_copy_file(_old, _new) != 0)
        {
            return -1;
        }

        if (d_unlink(_old) != 0)
        {
            // the copy landed but the original will not go away, so both
            // names now exist -- report it rather than claim success
            return -1;
        }

        return 0;
    }
    #endif
#endif

    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_rename",
                               D_INTERNAL_FILE_NOTIFY_PATH(_old),
                               "rename failed");

        return -1;
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  DUPLICATION                                           ///
///////////////////////////////////////////////////////////////////////////////

/*
d_copy_file
  Copies a file's contents to a new path.
  NOT atomic, and cannot be made so: a reader watching the destination sees it
grow, and a crash leaves it partial. If that matters, copy to a sibling
temporary and d_rename it into place -- which is exactly what
D_CFG_FILE_WRITE_ATOMIC does for d_fwrite_all.
  Takes the platform's engine where one exists and is configured
(D_CFG_FILE_OPS_COPY_NATIVE): on Linux copy_file_range may turn the copy into
a reflink, making it instant and free. It falls back to a portable buffered
copy whenever the platform declines, so behaviour is identical either way --
only the cost changes.
  Copies contents and, by default, permission bits. It does not copy owner,
timestamps, extended attributes or ACLs. (macOS's fcopyfile is the exception:
COPYFILE_ALL brings metadata along, so that path preserves more than the
portable one. Set D_CFG_FILE_OPS_COPY_NATIVE to 0 if that inconsistency
matters more than the speed.)

Parameter(s):
  _src: source path; must name a regular file.
  _dst: destination path.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_copy_file
(
    const char* _src,
    const char* _dst
)
{
#if D_FILE_BACKEND_IS_STDC
    (void)_src;
    (void)_dst;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_copy_file",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
#else
    struct d_stat_t st;
    int             in;
    int             out;
    int             flags;
    int             result;
    int             saved_errno;

    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_src != NULL,
                            EINVAL,
                            "d_copy_file",
                            NULL,
                            "source path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_dst != NULL,
                            EINVAL,
                            "d_copy_file",
                            _src,
                            "destination path is NULL",
                            -1);

    in = d_open(_src, O_RDONLY);

    if (in < 0)
    {
        return -1;
    }

    // stat the DESCRIPTOR, not the path: the file is already open, so this
    // cannot describe something other than what is about to be copied
    if (d_fstat(in, &st) != 0)
    {
        (void)d_close(in);

        return -1;
    }

    if (!S_ISREG(st.st_mode))
    {
        (void)d_close(in);
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_copy_file",
                             _src,
                             "source is not a regular file",
                             -1);
    }

    flags = O_WRONLY | O_CREAT | O_TRUNC;

    #if D_CFG_IS_OFF(D_CFG_FILE_OPS_COPY_OVERWRITE)
    // O_EXCL is the atomic form of "fail if it exists"; a d_file_exists check
    // here would race
    flags |= O_EXCL;
    #endif

    // create with the source's bits from the start where we can, so the file
    // is never briefly more permissive than its source
    out = d_open(_dst, flags, (int)(st.st_mode & 0777));

    if (out < 0)
    {
        saved_errno = errno;
        (void)d_close(in);
        errno = saved_errno;

        return -1;
    }

    result = 1;

    #if (D_INTERNAL_FILE_OPS_COPY_NATIVE == 1)
    // 1 means "the platform declined", which is not a failure
    result = d_internal_ops_copy_native(in, out, (int64_t)st.st_size);
    #endif

    if (result == 1)
    {
        result = d_internal_ops_copy_portable(in, out);
    }

    #if D_CFG_IS_ON(D_CFG_FILE_OPS_COPY_PRESERVE_MODE)
    // the umask may have taken bits off the create above; put them back.
    // Copying a 0600 private key into a 0644 file is a security bug, and it
    // is what happens by default if nobody does this.
    if (result == 0)
    {
        if (d_chmod(_dst, st.st_mode & 0777) != 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                                   errno,
                                   "d_copy_file",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_dst),
                                   "could not preserve the source's permissions");
        }
    }
    #endif

    saved_errno = errno;

    if (d_close(out) != 0)
    {
        // a close failure means buffered data never reached the file, so the
        // copy is incomplete however well the writes appeared to go
        result = -1;
        saved_errno = errno;
    }

    (void)d_close(in);
    errno = saved_errno;

    if (result != 0)
    {
        // never leave a half-written destination behind claiming to be a copy
        (void)d_remove(_dst);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               saved_errno,
                               "d_copy_file",
                               D_INTERNAL_FILE_NOTIFY_PATH(_dst),
                               "copy failed; the partial destination was removed");
        errno = saved_errno;

        return -1;
    }

    return 0;
#endif
}
