/*******************************************************************************
* djinterp [c]                                                        file_ops.c
*
* Implementation of the whole-file operations declared in file_ops.h.
*   Removal and renaming are thin wrappers that settle the platforms'
* disagreements: POSIX rename always replaces, Win32 rename never does. The
* copy opens both ends as descriptors, tries the platform's own engine, falls
* back to a buffered loop, and removes its destination on any failure.
*
*
* path:      /src/djinterp/c/fs/file_ops.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.29
*******************************************************************************/
// Linux declares copy_file_range(2) only under _GNU_SOURCE; a feature-test
// macro must precede every include, so it sits here, ahead of the header.
#if defined(__linux__)
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE 1
    #endif  // _GNU_SOURCE
#endif  // __linux__

#include "../../../../inc/djinterp/c/fs/file_ops.h"  // corresponding header
// std
#include <errno.h>   // errno, EEXIST, EINTR, EXDEV and the other E* codes
#include <stddef.h>  // NULL, size_t
#include <stdint.h>  // int64_t
#include <stdio.h>   // remove, rename
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/fs/file_desc.h"    // d_file_open, d_file_close_fd
#include "../../../../inc/djinterp/c/fs/file_io.h"      // d_file_read_fd, d_file_write_full_fd
#include "../../../../inc/djinterp/c/fs/file_stat.h"    // d_file_stat_fd, d_file_chmod, d_file_exists
#include "../../../../inc/djinterp/config/c/fs/cfg_file_ops.h"  // D_INTERNAL_FILE_OPS_*
// apple
#if ( (D_INTERNAL_FILE_OPS_COPY_NATIVE == 1) &&                                \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_FCOPYFILE)) )
    #include <copyfile.h>  // fcopyfile, COPYFILE_ALL
#endif


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if !D_FILE_BACKEND_IS_STDC

/*
d_internal_ops_copy_portable
  Copies by moving bytes through a user-space buffer: the fallback that always
works, with no kernel offload and no filesystem cooperation, identical on
every target. The native engines are faster; this one is the definition of
correct. Both descriptors are expected at offset 0. Only the non-ISO backends
define it, because only they have a d_file_copy that calls it.
*/
static int
d_internal_ops_copy_portable(
    int _in,
    int _out
)
{
    char buffer[D_INTERNAL_FILE_OPS_COPY_BUF];

    // move the file a buffer at a time until the source runs out
    for (;;)
    {
        const ssize_t got = d_file_read_fd(_in,
                                           buffer,
                                           sizeof(buffer));

        // a failed read ends the copy
        if (got < 0)
        {
            return -1;
        }

        // end of file
        if (got == 0)
        {
            break;
        }

        // d_file_write_full_fd, not d_file_write_fd: a short write here is
        // normal, and losing the remainder would corrupt the copy silently
        if (d_file_write_full_fd(_out,
                                 buffer,
                                 (size_t)got) < 0)
        {
            return -1;
        }
    }

    return 0;
}

#if (D_INTERNAL_FILE_OPS_COPY_NATIVE == 1)

#if D_CFG_IS_ON(D_CFG_FILE_HAS_COPY_FILE_RANGE)

/*
d_internal_ops_copy_native
  Linux copy_file_range(2): the bytes never leave the kernel, and the
filesystem may service the request directly -- on btrfs or XFS as a reflink,
instant and consuming no space until one side is written. EXDEV (across
filesystems), EINVAL, ENOSYS, EOPNOTSUPP and EPERM mean "use the other path",
not "the copy failed", and return 1 so the caller falls back. EINTR is
retried, and a pass that moves nothing ends the loop rather than spinning.
*/
static int
d_internal_ops_copy_native(
    int     _in,
    int     _out,
    int64_t _size
)
{
    size_t remaining = (size_t)_size;

    // let the kernel move the bytes until none remain
    while (remaining > 0)
    {
        const ssize_t moved = copy_file_range(_in,
                                              NULL,
                                              _out,
                                              NULL,
                                              remaining,
                                              0);

        // sort a failure into a decline, an interruption or a real error
        if (moved < 0)
        {
            // the kernel or the filesystem cannot do this pairing -- not an
            // error, just a decline. Anything else is real.
            if ( (errno == EXDEV)      ||
                 (errno == EINVAL)     ||
                 (errno == ENOSYS)     ||
                 (errno == EOPNOTSUPP) ||
                 (errno == EPERM) )
            {
                D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_INFO,
                                       errno,
                                       "d_file_copy",
                                       NULL,
                                       "copy_file_range declined; using the "
                                       "portable path");

                return 1;
            }

            // an interrupted call is simply made again
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
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_FCOPYFILE)

/*
d_internal_ops_copy_native
  macOS fcopyfile(3) with COPYFILE_ALL, which brings extended attributes and
resource forks along -- metadata the portable path silently drops. It never
declines; any failure is a real one.
*/
static int
d_internal_ops_copy_native(
    int     _in,
    int     _out,
    int64_t _size
)
{
    (void)_size;

    // the whole file, metadata included, in one call
    if (fcopyfile(_in,
                  _out,
                  NULL,
                  COPYFILE_ALL) < 0)
    {
        return -1;
    }

    return 0;
}

#else

/*
d_internal_ops_copy_native
  A native engine was configured, but none is reachable on this target, so
this always declines and the caller takes the portable path.
*/
static int
d_internal_ops_copy_native(
    int     _in,
    int     _out,
    int64_t _size
)
{
    (void)_in;
    (void)_out;
    (void)_size;

    return 1;
}

#endif  // D_CFG_FILE_HAS_COPY_FILE_RANGE, D_CFG_FILE_HAS_FCOPYFILE

#endif  // D_INTERNAL_FILE_OPS_COPY_NATIVE == 1

#endif  // !D_FILE_BACKEND_IS_STDC

//==============================================================================
// 1.  OPERATIONS
//==============================================================================

/*
d_file_remove
  ISO C remove(), which is what makes it accept both kinds: POSIX remove()
calls rmdir() for a directory and unlink() otherwise.
*/
int
d_file_remove(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_remove",
                            NULL,
                            "path is NULL",
                            -1);

    const int result = remove(_path);

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_remove",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "remove failed");

        return -1;
    }

    return 0;
}

/*
d_file_unlink
  Removes the NAME, not necessarily the file -- which is why unlinking an open
file is a legitimate way to make a temporary that disappears on exit even if
the process is killed. The ISO C fallback is remove(), which cannot refuse a
directory.
*/
int
d_file_unlink(
    const char* _path
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_unlink",
                            NULL,
                            "path is NULL",
                            -1);

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    const int result = _unlink(_path);
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    const int result = unlink(_path);
#else
    // ISO C has only remove(), which also takes directories -- so on this
    // backend d_file_unlink cannot keep its promise to refuse them
    const int result = remove(_path);
#endif

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_unlink",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "unlink failed");

        return -1;
    }

    return 0;
}

/*
d_file_rename
  POSIX rename replaces silently and Win32's never replaces, so each side is
made to honour _overwrite. Refusing is done here with an existence check --
not atomic with the rename, since there is no portable rename-if-absent -- and
replacing on Win32 goes through MoveFileEx, the only spelling that replaces
atomically. A cross-device failure is reported as EXDEV unless the build has
explicitly accepted a non-atomic copy and delete in its place.
*/
int
d_file_rename(
    const char* _old,
    const char* _new,
    int         _overwrite
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_old != NULL,
                            EINVAL,
                            "d_file_rename",
                            NULL,
                            "source path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_new != NULL,
                            EINVAL,
                            "d_file_rename",
                            _old,
                            "destination path is NULL",
                            -1);

    // POSIX rename replaces silently, so refusing has to be done here. This
    // check is NOT atomic with the rename below -- another process can create
    // _new in between -- but there is no portable rename-if-absent, and the
    // alternative is not offering the option at all.
    if (!_overwrite)
    {
        // an existing destination is refused
        if (d_file_exists(_new))
        {
            D_INTERNAL_FILE_FAIL(EEXIST,
                                 "d_file_rename",
                                 _new,
                                 "destination exists and overwrite was "
                                 "not requested",
                                 -1);
        }
    }

    int result = -1;

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    // Win32's rename() fails when the destination exists; MoveFileEx is the
    // only spelling that replaces atomically
    if (_overwrite)
    {
        // MoveFileEx sets no errno; name the likeliest cause
        if (!MoveFileExA(_old,
                         _new,
                         MOVEFILE_REPLACE_EXISTING))
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
        result = rename(_old,
                        _new);
    }
#else
    result = rename(_old,
                    _new);

    #if D_CFG_IS_ON(D_CFG_FILE_OPS_RENAME_CROSS_DEVICE)
    // the caller has explicitly accepted non-atomic movement across
    // filesystems; see the knob's note
    if ( (result != 0) &&
         (errno == EXDEV) )
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                               EXDEV,
                               "d_file_rename",
                               D_INTERNAL_FILE_NOTIFY_PATH(_old),
                               "cross-device; falling back to a NON-ATOMIC "
                               "copy+delete");

        // copy first, so a failure leaves the original in place
        if (d_file_copy(_old,
                        _new) != 0)
        {
            return -1;
        }

        // the copy landed but the original will not go away, so both names
        // now exist -- report it rather than claim success
        if (d_file_unlink(_old) != 0)
        {
            return -1;
        }

        return 0;
    }
    #endif
#endif

    // report the failure; errno is the platform's
    if (result != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_rename",
                               D_INTERNAL_FILE_NOTIFY_PATH(_old),
                               "rename failed");

        return -1;
    }

    return 0;
}

#if D_FILE_BACKEND_IS_STDC

/*
d_file_copy
  The copy is built on descriptors, which the ISO C backend does not have, so
this only reports ENOSYS.
*/
int
d_file_copy(
    const char* _src,
    const char* _dst
)
{
    (void)_src;
    (void)_dst;

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_copy",
                         NULL,
                         "no descriptors on the ISO C backend",
                         -1);
}

#else

/*
d_file_copy
  The source is opened first and then stat'd through its descriptor, so the
metadata describes exactly what is about to be copied. The destination is
created with the source's permission bits from the start, so it is never
briefly more permissive than its source; the native engine is tried where
configured, and its decline (1) hands over to the portable loop. The umask may
have trimmed the create, so the bits are restored afterwards unless the build
says otherwise. A failed close of the destination fails the copy, and any
failure removes the partial destination while keeping the errno that caused
it.
*/
int
d_file_copy(
    const char* _src,
    const char* _dst
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_src != NULL,
                            EINVAL,
                            "d_file_copy",
                            NULL,
                            "source path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_dst != NULL,
                            EINVAL,
                            "d_file_copy",
                            _src,
                            "destination path is NULL",
                            -1);

    const int in = d_file_open(_src,
                               O_RDONLY);

    // d_file_open has already reported the failure
    if (in < 0)
    {
        return -1;
    }

    struct d_stat_t st;

    // stat the DESCRIPTOR, not the path: the file is already open, so this
    // cannot describe something other than what is about to be copied
    if (d_file_stat_fd(in,
                       &st) != 0)
    {
        (void)d_file_close_fd(in);

        return -1;
    }

    // only a regular file's contents can be copied this way
    if (!S_ISREG(st.st_mode))
    {
        (void)d_file_close_fd(in);
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_file_copy",
                             _src,
                             "source is not a regular file",
                             -1);
    }

    int flags = O_WRONLY | O_CREAT | O_TRUNC;

#if D_CFG_IS_OFF(D_CFG_FILE_OPS_COPY_OVERWRITE)
    // O_EXCL is the atomic form of "fail if it exists"; a d_file_exists check
    // here would race
    flags |= O_EXCL;
#endif

    // create with the source's bits from the start where we can, so the file
    // is never briefly more permissive than its source
    const int out = d_file_open(_dst,
                                flags,
                                (int)(st.st_mode & 0777));

    // keep the open's errno across closing the source
    if (out < 0)
    {
        const int saved_errno = errno;

        (void)d_file_close_fd(in);
        errno = saved_errno;

        return -1;
    }

    int result = 1;

#if (D_INTERNAL_FILE_OPS_COPY_NATIVE == 1)
    // 1 means "the platform declined", which is not a failure
    result = d_internal_ops_copy_native(in,
                                        out,
                                        (int64_t)st.st_size);
#endif

    // the portable loop takes over whenever the platform declined
    if (result == 1)
    {
        result = d_internal_ops_copy_portable(in,
                                              out);
    }

#if D_CFG_IS_ON(D_CFG_FILE_OPS_COPY_PRESERVE_MODE)
    // the umask may have taken bits off the create above; put them back.
    // Copying a 0600 private key into a 0644 file is a security bug, and it
    // is what happens by default if nobody does this.
    if (result == 0)
    {
        // bits that cannot be restored are a warning, not a failed copy
        if (d_file_chmod(_dst,
                         st.st_mode & 0777) != 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                                   errno,
                                   "d_file_copy",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_dst),
                                   "could not preserve the source's "
                                   "permissions");
        }
    }
#endif

    int saved_errno = errno;

    // a close failure means buffered data never reached the file, so the
    // copy is incomplete however well the writes appeared to go
    if (d_file_close_fd(out) != 0)
    {
        result      = -1;
        saved_errno = errno;
    }

    (void)d_file_close_fd(in);
    errno = saved_errno;

    // never leave a half-written destination behind claiming to be a copy
    if (result != 0)
    {
        (void)d_file_remove(_dst);
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               saved_errno,
                               "d_file_copy",
                               D_INTERNAL_FILE_NOTIFY_PATH(_dst),
                               "copy failed; the partial destination was "
                               "removed");
        errno = saved_errno;

        return -1;
    }

    return 0;
}

#endif  // D_FILE_BACKEND_IS_STDC
