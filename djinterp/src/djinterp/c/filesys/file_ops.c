/*******************************************************************************
* djinterp [c]                                                        file_ops.c
*
* Implementation of the whole-file operations declared in file_ops.h.
*   Removal and renaming are thin wrappers that settle the platforms'
* disagreements: POSIX rename always replaces, Win32 rename never does. The
* copy opens both ends as descriptors, refuses to copy a file onto itself,
* gives the destination the source's bits before its data, tries the
* platform's own engine, falls back to a buffered loop, and removes its
* destination on any failure.
*   MORE THAN POSIX. On Linux the engine is copy_file_range(2), which glibc
* declares from 2.27 and only under _GNU_SOURCE. Build this file with
* -D_GNU_SOURCE on Linux to have it -- a feature-test macro comes from the
* build, never from a source (the C guide's rule); without it
* D_CFG_FILE_HAS_COPY_FILE_RANGE is 0 and the copy takes the buffered loop,
* which is slower and just as correct.
*
*
* path:      /src/djinterp/c/filesys/file_ops.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.17
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_ops.h"  // corresponding header
// std
#include <errno.h>   // errno, EEXIST, EINTR, EXDEV and the other E* codes
#include <stddef.h>  // NULL, size_t
#include <stdio.h>   // remove, rename
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/filesys/file_desc.h"    // d_file_open, d_file_close_fd
#include "../../../../inc/djinterp/c/filesys/file_io.h"      // d_file_read_fd, d_file_write_full_fd
#include "../../../../inc/djinterp/c/filesys/file_seek.h"    // d_file_truncate_fd64
#include "../../../../inc/djinterp/c/filesys/file_stat.h"    // d_file_stat_fd64, d_file_chmod, d_file_exists
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_ops.h"  // D_INTERNAL_FILE_OPS_*
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t
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
  Copies by moving bytes through a buffer: the fallback that always works,
with no kernel offload and no filesystem cooperation, identical on every
target. The native engines are faster; this one is the definition of correct.
It runs from both descriptors' current offsets, so it can finish what an
engine started. The buffer comes from the filesys allocator rather than the
stack, since D_CFG_FILE_OPS_COPY_BUF_SIZE is the caller's to choose and a
thread's stack may be smaller. Only the non-ISO backends define it, because
only they have a d_file_copy that calls it.
*/
static int
d_internal_ops_copy_portable(
    int _in,
    int _out
)
{
    char* const buffer =
        (char*)d_internal_file_alloc(D_INTERNAL_FILE_OPS_COPY_BUF);

    // d_internal_file_alloc has already reported the failure
    if (!buffer)
    {
        return -1;
    }

    int result = 0;

    // move the file a buffer at a time until the source runs out
    for (;;)
    {
        const ssize_t got = d_file_read_fd(_in,
                                           buffer,
                                           D_INTERNAL_FILE_OPS_COPY_BUF);

        // a failed read ends the copy; the end of the file ends it well
        if (got <= 0)
        {
            result = (got < 0) ? -1 : 0;

            break;
        }

        // d_file_write_full_fd, not d_file_write_fd: a short write here is
        // normal, and losing the remainder would corrupt the copy silently
        if (d_file_write_full_fd(_out,
                                 buffer,
                                 (size_t)got) < 0)
        {
            result = -1;

            break;
        }
    }

    const int saved_errno = errno;

    d_internal_file_free(buffer);
    errno = saved_errno;

    return result;
}

#if ( (D_INTERNAL_FILE_OPS_COPY_NATIVE == 1) &&                                \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_COPY_FILE_RANGE)) )

// D_INTERNAL_OPS_CHUNK
//   constant: the most one copy_file_range call is asked to move. It runs to
// the end of the file, not to a size from fstat: that size would not fit a
// 32-bit size_t past 4 GiB, and procfs and its kind report 0 for files that
// are not empty.
#define D_INTERNAL_OPS_CHUNK ((size_t)1 << 30)

/*
d_internal_ops_copy_native
  Linux copy_file_range(2): the bytes never leave the kernel, and the
filesystem may service the request directly -- on btrfs or XFS as a reflink,
instant and consuming no space until one side is written. EXDEV (across
filesystems), EINVAL, ENOSYS, EOPNOTSUPP and EPERM mean "use the other path",
not "the copy failed", and return 1 so the caller falls back -- as does a
first call that moves nothing, since a file whose size the kernel cannot see
reads as empty here and not to read(). The fallback resumes where the kernel
stopped, from the descriptors' shared offsets. EINTR is retried.
*/
static int
d_internal_ops_copy_native(
    int _in,
    int _out
)
{
    int moved_any = 0;

    // let the kernel move the bytes until the source runs out
    for (;;)
    {
        const ssize_t moved = copy_file_range(_in,
                                              NULL,
                                              _out,
                                              NULL,
                                              D_INTERNAL_OPS_CHUNK,
                                              0);

        // the end of the file, or a file the kernel would not read
        if (moved == 0)
        {
            return moved_any ? 0 : 1;
        }

        // progress; ask for more
        if (moved > 0)
        {
            moved_any = 1;

            continue;
        }

        // an interrupted call is simply made again
        if (errno == EINTR)
        {
            continue;
        }

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

        return -1;
    }
}

#elif ( (D_INTERNAL_FILE_OPS_COPY_NATIVE == 1) &&                              \
        (D_CFG_IS_ON(D_CFG_FILE_HAS_FCOPYFILE)) )

/*
d_internal_ops_copy_native
  macOS fcopyfile(3) with COPYFILE_ALL, which brings extended attributes and
resource forks along -- metadata the portable path silently drops. It never
declines; any failure is a real one.
*/
static int
d_internal_ops_copy_native(
    int _in,
    int _out
)
{
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
  No engine in this build -- D_CFG_FILE_OPS_COPY_NATIVE is off, or the
platform has none here, as Windows has not -- so this always declines and the
portable loop does the copy.
*/
static int
d_internal_ops_copy_native(
    int _in,
    int _out
)
{
    (void)_in;
    (void)_out;

    return 1;
}

#endif  // D_INTERNAL_FILE_OPS_COPY_NATIVE, D_CFG_FILE_HAS_*

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

// D_INTERNAL_OPS_CREATE_BITS
//   constant: the bits a new destination is created with besides the
// source's. Write, on Windows: the CRT creates a file without it read-only,
// and a read-only file cannot be deleted, so a failed copy could not remove
// what it wrote. The source's read-only bit follows the data
// (d_internal_ops_restore).
#define D_INTERNAL_OPS_CREATE_BITS 0200

/*
d_internal_ops_same_file
  Whether two open descriptors are one file. The CRT reports no inode number,
so the handles are asked for their volume and file index instead. A file
that cannot be described is taken to be a different one.
*/
static int
d_internal_ops_same_file(
    int                      _in,
    const struct d_stat64_t* _in_stat,
    int                      _out
)
{
    BY_HANDLE_FILE_INFORMATION a;
    BY_HANDLE_FILE_INFORMATION b;

    // Windows' identity is in the handle, not in the CRT's stat
    (void)_in_stat;

    // either handle unable to say: assume two files
    if ( (!GetFileInformationByHandle((HANDLE)_get_osfhandle(_in),
                                      &a)) ||
         (!GetFileInformationByHandle((HANDLE)_get_osfhandle(_out),
                                      &b)) )
    {
        return 0;
    }

    return ( (a.dwVolumeSerialNumber == b.dwVolumeSerialNumber) &&
             (a.nFileIndexHigh == b.nFileIndexHigh)             &&
             (a.nFileIndexLow == b.nFileIndexLow) );
}

/*
d_internal_ops_restrict
  The bits a destination takes before any data reaches it: none on Windows.
Its one bit is read-only, which keeps nothing private -- who may read the file
is its ACL's business -- and set early it would keep a failed copy from
removing what it wrote. d_internal_ops_finish sets it once the data is in.
*/
static void
d_internal_ops_restrict(
    int      _out,
    uint32_t _mode
)
{
    (void)_out;
    (void)_mode;

    return;
}

/*
d_internal_ops_restore
  The source's bits, once the data is in: the read-only attribute, by path,
since the CRT has no fchmod.
*/
static void
d_internal_ops_restore(
    const char* _dst,
    uint32_t    _mode
)
{
    // bits that cannot be restored are a warning, not a failed copy
    if (d_file_chmod(_dst,
                     _mode & 0777) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                               errno,
                               "d_file_copy",
                               D_INTERNAL_FILE_NOTIFY_PATH(_dst),
                               "could not preserve the source's "
                               "permissions");
    }

    return;
}

#else

// D_INTERNAL_OPS_CREATE_BITS
//   constant: the bits a new destination is created with besides the
// source's: none on POSIX, where any file can be unlinked.
#define D_INTERNAL_OPS_CREATE_BITS 0

/*
d_internal_ops_same_file
  Whether two open descriptors are one file: the same device and inode
number. A destination that cannot be described is taken to be another file.
*/
static int
d_internal_ops_same_file(
    int                      _in,
    const struct d_stat64_t* _in_stat,
    int                      _out
)
{
    struct d_stat64_t st = {0};

    // the source is already described
    (void)_in;

    // a destination that cannot say: assume two files
    if (d_file_stat_fd64(_out,
                         &st) != 0)
    {
        return 0;
    }

    return ( (st.st_dev == _in_stat->st_dev) &&
             (st.st_ino == _in_stat->st_ino) );
}

/*
d_internal_ops_restrict
  The source's bits, on the descriptor, before any data reaches it. An
existing destination keeps its own bits through open, so without this a 0600
private key would be written into a 0644 file and only then made private --
readable by anyone for as long as the copy took.
*/
static void
d_internal_ops_restrict(
    int      _out,
    uint32_t _mode
)
{
    // bits that cannot be set are a warning, not a failed copy
    if (fchmod(_out,
               (mode_t)(_mode & 0777)) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                               errno,
                               "d_file_copy",
                               NULL,
                               "could not preserve the source's "
                               "permissions");
    }

    return;
}

/*
d_internal_ops_restore
  Nothing left to do on POSIX: d_internal_ops_restrict set the bits before
the data.
*/
static void
d_internal_ops_restore(
    const char* _dst,
    uint32_t    _mode
)
{
    (void)_dst;
    (void)_mode;

    return;
}

#endif  // D_CFG_FILE_HAS_WIN32

/*
d_internal_ops_open_dst
  Opens the destination for a copy of the file open on _in, and readies it:
created with the source's bits where it is new (and writable on Windows,
D_INTERNAL_OPS_CREATE_BITS), and refused when it exists and _overwrite is 0
(O_EXCL, the atomic form of "fail if it exists"). It is opened without
O_TRUNC and compared with the source first -- the same file under another
name, a hard link or a symlink, would otherwise be emptied before a byte of
it was read -- and only then given the source's bits and truncated. Nothing
on any failure path here has touched an existing file's data, so none of
them removes the destination.
*/
static int
d_internal_ops_open_dst(
    const char*              _dst,
    int                      _in,
    const struct d_stat64_t* _in_stat,
    int                      _overwrite
)
{
    const int flags = O_WRONLY | O_CREAT | (_overwrite ? 0 : O_EXCL);
    const int mode  = (int)( (_in_stat->st_mode & 0777) |
                             D_INTERNAL_OPS_CREATE_BITS );
    const int out   = d_file_open(_dst,
                                  flags,
                                  mode);

    // d_file_open has already reported the failure
    if (out < 0)
    {
        return -1;
    }

    // a copy onto itself would destroy the only copy
    if (d_internal_ops_same_file(_in,
                                 _in_stat,
                                 out))
    {
        (void)d_file_close_fd(out);
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_file_copy",
                             _dst,
                             "source and destination are the same file",
                             -1);
    }

    // the source's bits before the source's data, where the build keeps them
    if (D_CFG_IS_ON(D_CFG_FILE_OPS_COPY_PRESERVE_MODE))
    {
        d_internal_ops_restrict(out,
                                _in_stat->st_mode);
    }

    // only now is the old content let go
    if (d_file_truncate_fd64(out,
                             0) != 0)
    {
        const int saved_errno = errno;

        (void)d_file_close_fd(out);
        errno = saved_errno;

        return -1;
    }

    return out;
}

/*
d_internal_ops_check_src
  Describes the source through its open descriptor, not its path -- the file
is already open, so this cannot describe something other than what is about
to be copied -- and refuses anything but a regular file, whose contents are
all a copy can carry.
*/
static int
d_internal_ops_check_src(
    int                _in,
    struct d_stat64_t* _st,
    const char*        _src
)
{
    // named only in the notices, which a build may compile out
    (void)_src;

    // d_file_stat_fd64 has already reported the failure
    if (d_file_stat_fd64(_in,
                         _st) != 0)
    {
        return -1;
    }

    // only a regular file's contents can be copied this way
    if (!S_ISREG(_st->st_mode))
    {
        D_INTERNAL_FILE_FAIL(EINVAL,
                             "d_file_copy",
                             _src,
                             "source is not a regular file",
                             -1);
    }

    return 0;
}

/*
d_internal_ops_finish
  The end of a copy, whichever way it went: the source's bits where the
platform sets them only after the data (d_internal_ops_restore), then both
descriptors closed -- a failed close of the destination fails the copy, since
buffered data never reached the file -- and on failure the partial
destination removed, keeping the errno that caused it.
*/
static int
d_internal_ops_finish(
    const char* _dst,
    int         _in,
    int         _out,
    int         _result,
    uint32_t    _mode
)
{
    int result = _result;

    // the bits that wait for the data, where the build keeps them
    if ( (result == 0) &&
         (D_CFG_IS_ON(D_CFG_FILE_OPS_COPY_PRESERVE_MODE)) )
    {
        d_internal_ops_restore(_dst,
                               _mode);
    }

    int saved_errno = errno;

    // a close failure means buffered data never reached the file, so the
    // copy is incomplete however well the writes appeared to go
    if (d_file_close_fd(_out) != 0)
    {
        result      = -1;
        saved_errno = errno;
    }

    (void)d_file_close_fd(_in);
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

/*
d_internal_ops_copy
  d_file_copy, with whether to replace an existing destination as an argument,
so that d_file_rename's cross-device fallback can honour its own caller. The
source is checked through its descriptor (d_internal_ops_check_src) and the
destination readied (d_internal_ops_open_dst); the native engine is tried
where configured, and its decline (1) hands over to the portable loop, which
finishes from wherever the engine stopped.
*/
static int
d_internal_ops_copy(
    const char* _src,
    const char* _dst,
    int         _overwrite
)
{
    const int in = d_file_open(_src,
                               O_RDONLY);

    // d_file_open has already reported the failure
    if (in < 0)
    {
        return -1;
    }

    struct d_stat64_t st  = {0};
    const int         out = (d_internal_ops_check_src(in,
                                                      &st,
                                                      _src) == 0)
                                ? d_internal_ops_open_dst(_dst,
                                                          in,
                                                          &st,
                                                          _overwrite)
                                : -1;

    // either end that would not open releases the other, keeping its errno
    if (out < 0)
    {
        const int saved_errno = errno;

        (void)d_file_close_fd(in);
        errno = saved_errno;

        return -1;
    }

    // the platform's engine first; 1 means it declined, which is not a
    // failure
    int result = d_internal_ops_copy_native(in,
                                            out);

    // the portable loop takes over whenever the platform declined
    if (result == 1)
    {
        result = d_internal_ops_copy_portable(in,
                                              out);
    }

    return d_internal_ops_finish(_dst,
                                 in,
                                 out,
                                 result,
                                 st.st_mode);
}

#endif  // !D_FILE_BACKEND_IS_STDC

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_ops_move
  MoveFileEx, for both answers to _overwrite. Asked not to replace, it
refuses an existing destination by itself -- atomically, which POSIX cannot
do -- and with no MOVEFILE_COPY_ALLOWED it refuses another volume with EXDEV,
where the CRT's rename would copy behind the caller's back. Its error is
GetLastError's, translated.
*/
static int
d_internal_ops_move(
    const char* _old,
    const char* _new,
    int         _overwrite
)
{
    const DWORD flags = _overwrite ? (DWORD)MOVEFILE_REPLACE_EXISTING
                                   : (DWORD)0;

    // moved, or refused for a reason Win32 names
    if (MoveFileExA(_old,
                    _new,
                    flags))
    {
        return 0;
    }

    D_INTERNAL_FILE_SET_ERR(d_internal_file_errno_win32(GetLastError()));

    return -1;
}

/*
d_internal_ops_rmdir
  RemoveDirectoryA, with its error translated. On a symbolic link or a
junction it removes the link itself and leaves the target alone.
*/
static int
d_internal_ops_rmdir(
    const char* _path
)
{
    // removed, or refused for a reason Win32 names
    if (RemoveDirectoryA(_path))
    {
        return 0;
    }

    const int code = d_internal_file_errno_win32(GetLastError());

    D_INTERNAL_FILE_SET_ERR(code);
    (void)code;

    return -1;
}

/*
d_internal_ops_names_dir
  Whether a path names a directory, or a link to one, by its attributes.
*/
static int
d_internal_ops_names_dir(
    const char* _path
)
{
    const DWORD attrs = GetFileAttributesA(_path);

    return ( (attrs != INVALID_FILE_ATTRIBUTES) &&
             ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) );
}

/*
d_internal_ops_remove
  remove() for Windows, whose C runtime's remove takes files only: a
directory -- or a link to one, which goes without its target -- is removed as
rmdir removes one, as POSIX remove() would.
*/
static int
d_internal_ops_remove(
    const char* _path
)
{
    // a directory goes as a directory goes
    if (d_internal_ops_names_dir(_path))
    {
        return d_internal_ops_rmdir(_path);
    }

    return remove(_path);
}

/*
d_internal_ops_unlink
  The CRT's _unlink, which refuses a directory as unlink(2) does -- except a
symbolic link or a junction to one, which _unlink refuses too but unlink(2)
would remove as it removes any link: the link goes, and its target stays.
*/
static int
d_internal_ops_unlink(
    const char* _path
)
{
    // a link to a directory goes as a directory goes
    if ( (d_internal_ops_names_dir(_path)) &&
         (d_internal_file_is_link_win32(_path,
                                        NULL)) )
    {
        return d_internal_ops_rmdir(_path);
    }

    return _unlink(_path);
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)

/*
d_internal_ops_move
  rename, which replaces silently, so refusing is done here: a name that
exists -- a dangling symlink included, which a following stat would miss --
is refused. lstat is called directly, as a missing destination is the normal
case and no failure worth a notice. The check is NOT atomic with the rename:
another process can create _new in between, and there is no portable
rename-if-absent.
*/
static int
d_internal_ops_move(
    const char* _old,
    const char* _new,
    int         _overwrite
)
{
    struct stat st = {0};

    // an existing name, link or not, stays unless it may be replaced
    if ( (!_overwrite) &&
         (lstat(_new,
                &st) == 0) )
    {
        D_INTERNAL_FILE_FAIL(EEXIST,
                             "d_file_rename",
                             _new,
                             "destination exists and overwrite was not "
                             "requested",
                             -1);
    }

    return rename(_old,
                  _new);
}

/*
d_internal_ops_remove
  ISO C remove(), which POSIX makes take both kinds: rmdir() for a directory,
unlink() otherwise.
*/
static int
d_internal_ops_remove(
    const char* _path
)
{
    return remove(_path);
}

/*
d_internal_ops_unlink
  unlink(2), which refuses a directory and removes a link itself.
*/
static int
d_internal_ops_unlink(
    const char* _path
)
{
    return unlink(_path);
}

#else

/*
d_internal_ops_move
  ISO C rename, which leaves an existing destination to the implementation, so
refusing is done here with the only existence test ISO C has -- not atomic
with the rename, as on POSIX.
*/
static int
d_internal_ops_move(
    const char* _old,
    const char* _new,
    int         _overwrite
)
{
    // an existing destination stays unless it may be replaced
    if ( (!_overwrite) &&
         (d_file_exists(_new)) )
    {
        D_INTERNAL_FILE_FAIL(EEXIST,
                             "d_file_rename",
                             _new,
                             "destination exists and overwrite was not "
                             "requested",
                             -1);
    }

    return rename(_old,
                  _new);
}

/*
d_internal_ops_remove
  ISO C remove(), which leaves directories to the implementation.
*/
static int
d_internal_ops_remove(
    const char* _path
)
{
    return remove(_path);
}

/*
d_internal_ops_unlink
  ISO C has only remove(), which also takes directories -- so on this backend
d_file_unlink cannot keep its promise to refuse them.
*/
static int
d_internal_ops_unlink(
    const char* _path
)
{
    return remove(_path);
}

#endif  // D_CFG_FILE_HAS_WIN32, D_CFG_FILE_HAS_POSIX

#if ( (D_CFG_IS_ON(D_CFG_FILE_OPS_RENAME_CROSS_DEVICE)) &&                     \
      (!D_FILE_BACKEND_IS_STDC) )

#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

/*
d_internal_ops_is_plain
  Whether a path names a regular file in itself: not a directory, and not a
reparse point -- the CRT's stat follows those, so the attribute is asked.
*/
static int
d_internal_ops_is_plain(
    const char* _path
)
{
    const DWORD attrs = GetFileAttributesA(_path);

    return ( (attrs != INVALID_FILE_ATTRIBUTES)                 &&
             ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0)         &&
             ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) == 0) );
}

#else

/*
d_internal_ops_is_plain
  Whether a path names a regular file in itself, not through a link.
*/
static int
d_internal_ops_is_plain(
    const char* _path
)
{
    struct stat st = {0};

    return ( (lstat(_path,
                    &st) == 0) &&
             (S_ISREG(st.st_mode)) );
}

#endif  // D_CFG_FILE_HAS_WIN32

/*
d_internal_ops_move_across
  d_file_rename's fallback across filesystems, which the build has accepted
for what it is: a copy, then a delete, neither atomic nor instant. Only a
regular file is moved this way (d_internal_ops_is_plain). A copy of anything
else is not the same thing -- a symlink's copy would be its target's data,
and a directory has no copy here at all -- so those keep the EXDEV that
brought them here, and everything stays where it was. The copy honours the
caller's _overwrite, and goes first, so a failure leaves the original in
place.
*/
static int
d_internal_ops_move_across(
    const char* _old,
    const char* _new,
    int         _overwrite
)
{
    // anything but a regular file cannot be carried across
    if (!d_internal_ops_is_plain(_old))
    {
        D_INTERNAL_FILE_SET_ERR(EXDEV);

        return -1;
    }

    D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_WARN,
                           EXDEV,
                           "d_file_rename",
                           D_INTERNAL_FILE_NOTIFY_PATH(_old),
                           "cross-device; falling back to a NON-ATOMIC "
                           "copy+delete");

    // copy first, so a failure leaves the original in place
    if (d_internal_ops_copy(_old,
                            _new,
                            _overwrite) != 0)
    {
        return -1;
    }

    // the copy landed but the original will not go away, so both names now
    // exist -- report it rather than claim success
    return d_file_unlink(_old);
}

#else

/*
d_internal_ops_move_across
  d_file_rename's fallback across filesystems where the build has accepted
none -- D_CFG_FILE_OPS_RENAME_CROSS_DEVICE is off, or the ISO C backend has no
copy to fall back on -- so the EXDEV that brought the call here stands.
*/
static int
d_internal_ops_move_across(
    const char* _old,
    const char* _new,
    int         _overwrite
)
{
    (void)_old;
    (void)_new;
    (void)_overwrite;

    return -1;
}

#endif  // D_CFG_FILE_OPS_RENAME_CROSS_DEVICE && !D_FILE_BACKEND_IS_STDC

//==============================================================================
// 1.  OPERATIONS
//==============================================================================

/*
d_file_remove
  ISO C remove(), which is what makes it accept both kinds: POSIX remove()
calls rmdir() for a directory and unlink() otherwise, and on Windows, whose
remove() takes files only, d_internal_ops_remove does the same.
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

    const int result = d_internal_ops_remove(_path);

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

    const int result = d_internal_ops_unlink(_path);

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
made to honour _overwrite (d_internal_ops_move): Win32 refuses atomically,
POSIX by an existence check that is not. A cross-device failure is reported
as EXDEV on both, unless the build has explicitly accepted a non-atomic copy
and delete in its place (d_internal_ops_move_across).
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

    int result = d_internal_ops_move(_old,
                                     _new,
                                     _overwrite);

    // across filesystems, the build's fallback: a non-atomic copy and delete
    // where it has accepted one (see the knob's note), and EXDEV where not
    if ( (result != 0) &&
         (errno == EXDEV) )
    {
        result = d_internal_ops_move_across(_old,
                                            _new,
                                            _overwrite);
    }

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
  d_internal_ops_copy, replacing an existing destination as
D_CFG_FILE_OPS_COPY_OVERWRITE says.
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

    return d_internal_ops_copy(_src,
                               _dst,
                               D_CFG_IS_ON(D_CFG_FILE_OPS_COPY_OVERWRITE));
}

#endif  // D_FILE_BACKEND_IS_STDC
