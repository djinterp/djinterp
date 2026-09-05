/******************************************************************************
* djinterp [core]                                               file_space.hpp
*
*   Filesystem capacity (roadmap Phase 9). Three byte counts for the filesystem
* that holds a path -- and, exactly as file_space.h warns, the middle one is a
* trap.
*
*     capacity   -- total bytes on the filesystem.
*     free       -- unallocated bytes, INCLUDING the reserve only root may use.
*     available  -- bytes THIS user may actually claim. This is the one to use.
*
*   On a typical machine `free` and `available` differ by the root reserve --
* often gigabytes. Deciding "will this write fit" from `free` is how an
* unprivileged program convinces itself it has room it cannot touch, then fails
* at write time. Unless you are root, read `available`.
*
*   A VALUE, not a resource, and a free query over it -- so no ownership, no
* D_*_ kit, and a FLAT tier ladder.
*
*
* path:      /inc/djinterp/core/fs/file_space.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#ifndef DJINTERP_FS_FILE_SPACE_
#define DJINTERP_FS_FILE_SPACE_ 1

// std
#include <cerrno>                 // EINVAL
// djinterp
#include "file_path.hpp"
#include "file_common.hpp"
#include "../../c/fs/file_space.h"   // d_space, d_space_t


NS_DJINTERP

// space_info
//   struct: a filesystem capacity snapshot, byte counts. Public members,
// mirroring std::filesystem::space_info -- a plain value with nothing to hide.
// See the header banner on why `available`, not `free`, is the number to size a
// write against.
struct space_info
{
    uint64_t capacity;    // total bytes on the filesystem
    uint64_t free;        // unallocated, INCLUDING the root-only reserve
    uint64_t available;   // bytes this user may actually claim -- use this one

    space_info(void)
        : capacity(0)
        , free(0)
        , available(0)
    {}
};


// space
//   function: capacity of the filesystem holding _p. Any path ON the filesystem
// works -- a file, a directory, the mount point; the numbers describe the whole
// filesystem, not the path. Returns a zeroed space_info with _ec set on failure
// (an invalid path is EINVAL; a path that does not exist is ENOENT).
inline space_info
space(
    const path& _p,
    error&      _ec
)
{
    struct d_space_t s;
    space_info       out;

    if (!_p.valid())
    {
        _ec.assign(EINVAL);

        return out;
    }

    if (d_space(_p.c_str(), &s) != 0)
    {
        _ec = error::from_errno();

        return out;
    }

    out.capacity  = s.capacity;
    out.free      = s.free;
    out.available = s.available;

    _ec.clear();

    return out;
}

NS_END  // djinterp

#endif  // DJINTERP_FS_FILE_SPACE_
