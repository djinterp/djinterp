/*******************************************************************************
* djinterp [core]                                                 file_space.hpp
*
* Filesystem capacity (roadmap Phase 9).
*   Three byte counts for the filesystem that holds a path -- and, exactly as
* file_space.h warns, the middle one is a trap:
*     capacity   -- total bytes on the filesystem.
*     free       -- unallocated bytes, INCLUDING the reserve only root may use.
*     available  -- bytes THIS user may actually claim. This is the one to use.
*   On a typical machine `free` and `available` differ by the root reserve --
* often gigabytes. Deciding "will this write fit" from `free` is how an
* unprivileged program convinces itself it has room it cannot touch, then
* fails at write time. Unless you are root, read `available`.
*   A VALUE, not a resource, and a free query over it -- so no ownership, no
* D_*_ kit, and a FLAT tier ladder.
*
*
* path:      /inc/djinterp/core/fs/file_space.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CAPACITY
    --------
    1.  Snapshot
         1.  space_info
    2.  Query
*/

#ifndef DJINTERP_FS_FILE_SPACE_HPP
#define DJINTERP_FS_FILE_SPACE_HPP 1

// std
#include <cerrno>  // EINVAL
// djinterp
#include "file_path.hpp"            // path
#include "file_common.hpp"          // error, the D_* kit
#include "../../c/fs/file_space.h"  // d_file_space, d_space_t
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


NS_DJINTERP


//==============================================================================
// 1.  CAPACITY
//==============================================================================


// 1.1    Snapshot
//------------------------------------------------------------------------------
// 1.1.1
// space_info
//   struct: a filesystem capacity snapshot, in bytes. Public members,
// mirroring std::filesystem::space_info -- a plain value with nothing to hide.
// See the banner on why `available`, not `free`, is the number to size a write
// against.
struct space_info
{
    uint64_t capacity;    // total bytes on the filesystem
    uint64_t free;        // unallocated, INCLUDING the root-only reserve
    uint64_t available;   // bytes this user may actually claim -- use this one

    /**
     * @brief Constructs an all-zero snapshot.
     */
    space_info(void)
        : capacity(0),
          free(0),
          available(0)
    {}
};

// 1.2    Query
//------------------------------------------------------------------------------
/**
 * @brief Reports the capacity of the filesystem holding a path.
 *
 * @note Any path ON the filesystem works -- a file, a directory, the mount
 *       point; the numbers describe the whole filesystem, not the path.
 *
 * @param[in]  _p   a path on the filesystem of interest.
 * @param[out] _ec  cleared on success; EINVAL for an invalid path, ENOENT for
 *                  one that does not exist, otherwise the platform's code.
 * @return the capacity, or a zeroed space_info on failure.
 */
inline space_info
space(
    const path& _p,
    error&      _ec
)
{
    space_info out;

    // an invalid path names nothing to measure
    if (!_p.valid())
    {
        _ec.assign(EINVAL);

        return out;
    }

    struct d_space_t s;

    // c/fs fills s, or reports through errno
    if (d_file_space(_p.c_str(),
                     &s) != 0)
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

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_FS_FILE_SPACE_HPP
