/*******************************************************************************
* djinterp [c]                                                      file_space.c
*
* Implementation of the capacity query declared in file_space.h.
*   statvfs where the platform has it, GetDiskFreeSpaceEx on Windows, and an
* honest ENOSYS everywhere else: a made-up capacity is worse than an admitted
* absence, since the caller would act on it.
*
*
* path:      /src/djinterp/c/fs/file_space.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/fs/file_space.h"  // corresponding header
// std
#include <errno.h>   // errno, EINVAL, ENOENT, ENOSYS
#include <string.h>  // memset
// djinterp
#include "../../../../inc/djinterp/c/fs/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/config/c/fs/cfg_file_space.h"  // D_CFG_FILE_HAS_STATVFS
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint64_t
// posix
#if D_CFG_IS_ON(D_CFG_FILE_HAS_STATVFS)
    #include <sys/statvfs.h>  // statvfs, struct statvfs
#endif


//==============================================================================
// 2.  QUERY
//==============================================================================

/*
d_file_space
  f_frsize, not f_bsize: the block counts are in fragment units, and f_bsize is
the preferred I/O size, a different number that happens to be equal often
enough to hide the bug for years. On Windows, GetDiskFreeSpaceEx's first
out-parameter is quota-aware and is the analogue of f_bavail, not f_bfree --
its argument order invites getting that backwards.
  There is deliberately no emulation where neither exists. The output is
zeroed before anything can fail, so a caller who ignores the return code never
reads a plausible stale number.
*/
int
d_file_space(
    const char*       _path,
    struct d_space_t* _out
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_space",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_out != NULL,
                            EINVAL,
                            "d_file_space",
                            _path,
                            "output buffer is NULL",
                            -1);

    // zero first: a caller who ignores the return code must not read a number
    // that looks plausible and is not
    memset(_out,
           0,
           sizeof(*_out));

#if D_CFG_IS_ON(D_CFG_FILE_HAS_STATVFS)
    {
        struct statvfs vfs;

        // ask the filesystem that holds the path
        if (statvfs(_path,
                    &vfs) != 0)
        {
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   errno,
                                   "d_file_space",
                                   D_INTERNAL_FILE_NOTIFY_PATH(_path),
                                   "statvfs failed");

            return -1;
        }

        // f_frsize is the FRAGMENT size and is what the block counts are in.
        // f_bsize is the preferred I/O size and is a different number that
        // happens to be equal often enough to hide the bug for years.
        uint64_t unit = (uint64_t)vfs.f_frsize;

        // an implementation that reports no fragment size means f_bsize
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
        if (!GetDiskFreeSpaceExA(_path,
                                 &avail,
                                 &total,
                                 &total_free))
        {
            D_INTERNAL_FILE_SET_ERR(ENOENT);
            D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                                   ENOENT,
                                   "d_file_space",
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
                         "d_file_space",
                         _path,
                         "this target reports no filesystem capacity",
                         -1);
#endif
}
