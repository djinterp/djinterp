/*******************************************************************************
* djinterp [c]                                                      file_space.c
*
* Implementation of the capacity query declared in file_space.h.
*   statvfs where the platform has it, GetDiskFreeSpaceEx on Windows, and an
* honest ENOSYS everywhere else: a made-up capacity is worse than an admitted
* absence, since the caller would act on it. The platform's call is one helper
* per platform (d_internal_space_query).
*   The 64-bit twin is the implementation; the 32-bit one converts its bytes
* to KiB, rounding down, and fails with EOVERFLOW from 4 TiB.
*
*
* path:      /src/djinterp/c/filesys/file_space.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/filesys/file_space.h"  // corresponding header
// std
#include <errno.h>   // errno, EINVAL, ENOSYS, EOVERFLOW
#include <string.h>  // memset
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"  // D_INTERNAL_FILE_*
#include "../../../../inc/djinterp/c/filesys/file_path.h"    // d_path_dirname
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_space.h"  // D_CFG_FILE_HAS_STATVFS
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, uint64_t,
                                                     // UINT32_MAX
// posix
#if D_CFG_IS_ON(D_CFG_FILE_HAS_STATVFS)
    #include <sys/statvfs.h>  // statvfs, struct statvfs
#endif


// the large-file interface this source needs (file_common.h, 6.4.1)
D_INTERNAL_FILE_ASSERT_LARGE_FILES;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if D_CFG_IS_ON(D_CFG_FILE_HAS_STATVFS)

/*
d_internal_space_query
  statvfs, retried across interruptions like any other call. f_frsize, not
f_bsize: the block counts are in fragment units, and f_bsize is the preferred
I/O size, a different number that happens to be equal often enough to hide
the bug for years. An implementation that reports no fragment size means
f_bsize.
*/
static int
d_internal_space_query(
    const char*         _path,
    struct d_space64_t* _out
)
{
    struct statvfs vfs    = {0};
    int            result = -1;

    D_INTERNAL_FILE_RETRY_EINTR(result,
                                statvfs(_path,
                                        &vfs));

    // statvfs has set errno
    if (result != 0)
    {
        return -1;
    }

    const uint64_t unit = (vfs.f_frsize != 0)
                              ? (uint64_t)vfs.f_frsize
                              : (uint64_t)vfs.f_bsize;

    _out->capacity  = (uint64_t)vfs.f_blocks * unit;
    _out->free      = (uint64_t)vfs.f_bfree  * unit;
    _out->available = (uint64_t)vfs.f_bavail * unit;

    return 0;
}

#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)

// D_INTERNAL_SPACE_ASK
//   macro: GetDiskFreeSpaceEx in the form the build's path text takes: the W
// function where the build hands Windows UTF-16 (D_INTERNAL_FILE_WIDE).
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_SPACE_ASK GetDiskFreeSpaceExW
#else
    #define D_INTERNAL_SPACE_ASK GetDiskFreeSpaceExA
#endif  // D_INTERNAL_FILE_WIDE == 1

/*
d_internal_space_ask
  GetDiskFreeSpaceEx, whose first out-parameter is quota-aware and is the
ANALOGUE OF f_bavail, not of f_bfree -- the argument order invites getting
that backwards. 1 for an answer; 0 for a failure left in GetLastError; -1 for
a path the platform's text cannot carry, with errno set by the conversion.
*/
static int
d_internal_space_ask(
    const char*         _dir,
    struct d_space64_t* _out
)
{
    struct d_internal_file_native        held = {0};
    const d_internal_file_nchar_t* const dir =
        d_internal_file_native_of(&held,
                                  _dir);

    // the conversion has set errno
    if (!dir)
    {
        return -1;
    }

    ULARGE_INTEGER avail      = {0};
    ULARGE_INTEGER total      = {0};
    ULARGE_INTEGER total_free = {0};

    const BOOL     asked      = D_INTERNAL_SPACE_ASK(dir,
                                                     &avail,
                                                     &total,
                                                     &total_free);

    d_internal_file_native_release(&held);

    // GetLastError says why
    if (!asked)
    {
        return 0;
    }

    _out->capacity  = (uint64_t)total.QuadPart;
    _out->free      = (uint64_t)total_free.QuadPart;
    _out->available = (uint64_t)avail.QuadPart;

    return 1;
}

/*
d_internal_space_refuse
  A Win32 reason, as the errno statvfs would have set for it.
*/
static int
d_internal_space_refuse(
    DWORD _error
)
{
    const int code = d_internal_file_errno_win32(_error);

    // named only by errno, which a build may not report through
    D_INTERNAL_FILE_SET_ERR(code);
    (void)code;

    return -1;
}

/*
d_internal_space_query
  GetDiskFreeSpaceEx takes a directory where statvfs takes any path, and
refuses a file with ERROR_DIRECTORY; a file is measured by its directory,
which on Windows is on the same volume. Nothing broader is tried -- the
volume's root, say -- since that answers for a path that does not exist.
*/
static int
d_internal_space_query_in(
    const char*         _path,
    struct d_space64_t* _out,
    char*               _dir_room
)
{
    char* const dir = _dir_room;

    const int asked = d_internal_space_ask(_path,
                                           _out);

    // an answer, or a path errno already says is unusable
    if (asked != 0)
    {
        return (asked > 0) ? 0 : -1;
    }

    const DWORD error = GetLastError();

    // anything but a file is the path's own failure
    if (error != ERROR_DIRECTORY)
    {
        return d_internal_space_refuse(error);
    }

    // d_path_dirname has set errno for a path it cannot take apart
    if (!d_path_dirname(_path,
                        dir,
                        D_FILE_PATH_MAX))
    {
        return -1;
    }

    const int again = d_internal_space_ask(dir,
                                           _out);

    // the file's directory, on the file's volume
    if (again != 0)
    {
        return (again > 0) ? 0 : -1;
    }

    return d_internal_space_refuse(GetLastError());
}

/*
d_internal_space_query
  d_internal_space_query's work is d_internal_space_query_in; this keeps the
room it builds a path in, on the heap where that is 32 KiB
(d_internal_file_path).
*/
static int
d_internal_space_query(
    const char*         _path,
    struct d_space64_t* _out
)
{
    struct d_internal_file_path dir_held = {0};
    char* const dir = d_internal_file_path_room(&dir_held);

    // d_internal_file_path_room has set errno, and reported the failure
    if (!dir)
    {
        d_internal_file_path_release(&dir_held);

        return -1;
    }

    const int result = d_internal_space_query_in(_path,
                                                 _out,
                                                 dir);

    d_internal_file_path_release(&dir_held);

    return result;
}

#endif  // D_CFG_FILE_HAS_STATVFS, D_CFG_FILE_HAS_WIN32

//==============================================================================
// 2.  QUERY
//==============================================================================

#if (D_INTERNAL_FILE_SPACE_AVAILABLE == 1)

/*
d_file_space64
  The platform's answer (d_internal_space_query), in bytes. The output is
zeroed before anything can fail, so a caller who ignores the return code
never reads a plausible stale number.
*/
int
d_file_space64(
    const char*         _path,
    struct d_space64_t* _out
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_space64",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_out != NULL,
                            EINVAL,
                            "d_file_space64",
                            _path,
                            "output buffer is NULL",
                            -1);

    // zero first: a caller who ignores the return code must not read a number
    // that looks plausible and is not
    memset(_out,
           0,
           sizeof(*_out));

    // the filesystem that holds the path answers, or says why not
    if (d_internal_space_query(_path,
                               _out) != 0)
    {
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,
                               errno,
                               "d_file_space64",
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),
                               "the filesystem's capacity could not be read");

        return -1;
    }

    return 0;
}

#else

/*
d_file_space64
  There is deliberately no emulation where neither statvfs nor Windows is
available: capacity cannot be derived from anything else here, and a
fabricated number would be acted upon. The output is still zeroed.
*/
int
d_file_space64(
    const char*         _path,
    struct d_space64_t* _out
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_space64",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_out != NULL,
                            EINVAL,
                            "d_file_space64",
                            _path,
                            "output buffer is NULL",
                            -1);

    // zero first, so a caller who ignores the result reads no number at all
    memset(_out,
           0,
           sizeof(*_out));

    D_INTERNAL_FILE_FAIL(ENOSYS,
                         "d_file_space64",
                         _path,
                         "this target reports no filesystem capacity",
                         -1);
}

#endif  // D_INTERNAL_FILE_SPACE_AVAILABLE == 1

/*
d_file_space32
  d_file_space64, in KiB: 32 bits of bytes would stop at 4 GiB, 32 bits of
KiB reach 4 TiB. Rounding down never claims room that is not there. A
filesystem too large even for that fails whole, with EOVERFLOW, rather than
report a capacity that is wrong by a multiple of 4 TiB.
*/
int
d_file_space32(
    const char*         _path,
    struct d_space32_t* _out
)
{
    // parameter validation
    D_INTERNAL_FILE_REQUIRE(_path != NULL,
                            EINVAL,
                            "d_file_space32",
                            NULL,
                            "path is NULL",
                            -1);
    D_INTERNAL_FILE_REQUIRE(_out != NULL,
                            EINVAL,
                            "d_file_space32",
                            _path,
                            "output buffer is NULL",
                            -1);

    struct d_space64_t bytes = {0};

    // zero first, as the 64-bit twin does
    memset(_out,
           0,
           sizeof(*_out));

    // d_file_space64 has already reported the failure
    if (d_file_space64(_path,
                       &bytes) != 0)
    {
        return -1;
    }

    const uint64_t capacity_kib  = bytes.capacity  / 1024u;
    const uint64_t free_kib      = bytes.free      / 1024u;
    const uint64_t available_kib = bytes.available / 1024u;

    // a count that does not fit fails the whole answer
    if ( (capacity_kib  > (uint64_t)UINT32_MAX) ||
         (free_kib      > (uint64_t)UINT32_MAX) ||
         (available_kib > (uint64_t)UINT32_MAX) )
    {
        D_INTERNAL_FILE_FAIL(EOVERFLOW,
                             "d_file_space32",
                             _path,
                             "capacity exceeds 4 TiB, which d_space32_t "
                             "cannot count",
                             -1);
    }

    _out->capacity_kib  = (uint32_t)capacity_kib;
    _out->free_kib      = (uint32_t)free_kib;
    _out->available_kib = (uint32_t)available_kib;

    return 0;
}
