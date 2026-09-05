/******************************************************************************
* djinterp [c]                                                    file_space.c
*
* path:      /src/djinterp/c/fs/file_space.c
******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/c/fs/file_space.h"

#if D_CFG_IS_ON(D_CFG_FILE_HAS_STATVFS)
    // posix
    #include <sys/statvfs.h>
#endif


// II.   Query

/*
d_space
  Reports the capacity of the filesystem holding a path.
  The path names any existing file or directory ON the filesystem; the answer
describes the whole filesystem, not the path.

  free vs available -- the distinction this function exists to preserve:
    free       every unallocated byte.
    available  what an unprivileged process may actually claim.
  They diverge because of the root reserve (ext4 withholds 5% by default so a
full disk does not lock root out of repairing it), quotas, and container or
overlay limits. The gap is routinely an order of magnitude: on the machine
this was written on, /tmp reported 249 GiB free and 10 GiB available.
  Deciding "will my write fit" from `free` is how a program confidently runs
out of disk. Unless you are root, use `available`.

  There is deliberately no emulation. Where the platform will not answer, this
reports ENOSYS -- a made-up capacity is worse than an admitted absence, since
the caller would act on it.

Parameter(s):
  _path: any existing path on the filesystem of interest.
  _out:  receives the capacity; fully overwritten, and zeroed on failure so a
         caller who ignores the return code cannot read a stale number.
Return:
  0 on success, or -1 on failure with errno set.
*/
int
d_space
(
    const char*       _path,
    struct d_space_t* _out
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_space",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_out != NULL,
                            EINVAL,
                            "d_space",
                            _path,
                            "output buffer is NULL",
                            -1);

    // zero first: a caller who ignores the return code must not read a number
    // that looks plausible and is not
    memset(_out, 0, sizeof(*_out));

#if D_CFG_IS_ON(D_CFG_FILE_HAS_STATVFS)
    {
        struct statvfs vfs;
        uint64_t       unit;

        if (statvfs(_path, &vfs) != 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   errno,
                                   "d_space",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                   "statvfs failed");

            return -1;
        }

        // f_frsize is the FRAGMENT size and is what the block counts are in.
        // f_bsize is the preferred I/O size and is a different number that
        // happens to be equal often enough to hide the bug for years.
        unit = (uint64_t)vfs.f_frsize;

        if (unit == 0)
        {
            unit = (uint64_t)vfs.f_bsize;
        }

        _out->capacity  = (uint64_t)vfs.f_blocks * unit;
        _out->free      = (uint64_t)vfs.f_bfree  * unit;
        _out->available = (uint64_t)vfs.f_bavail * unit;

        return 0;
    }
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    {
        ULARGE_INTEGER avail;
        ULARGE_INTEGER total;
        ULARGE_INTEGER total_free;

        // the first out-parameter is quota-aware and is the ANALOGUE OF
        // f_bavail, not of f_bfree -- the argument order invites getting this
        // backwards
        if (!GetDiskFreeSpaceExA(_path, &avail, &total, &total_free))
        {
            D_INTERNAL_FILE_SET_ERR(ENOENT);
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   ENOENT,
                                   "d_space",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                   "GetDiskFreeSpaceEx failed");

            return -1;
        }

        _out->capacity  = (uint64_t)total.QuadPart;
        _out->free      = (uint64_t)total_free.QuadPart;
        _out->available = (uint64_t)avail.QuadPart;

        return 0;
    }
#else
    // no emulation: capacity cannot be derived from anything else here, and a
    // fabricated number would be acted upon
    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_space",
                         _path,
                         "this target reports no filesystem capacity",
                         -1);
#endif
}
