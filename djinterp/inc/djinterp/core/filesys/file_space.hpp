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
*   WIDTH. space_info follows the C layer's D_FILE_WIDTH, as d_space_t does:
* at 64 it counts bytes under std::filesystem's names; at 32 it counts KiB,
* rounded down, as capacity_kib, free_kib and available_kib, because 32 bits
* of bytes stop at 4 GiB where 32 bits of KiB reach 4 TiB. Code written for
* one unit fails to compile against the other rather than misread a number,
* and a filesystem too large for the 32-bit count fails with EOVERFLOW. Where
* a language level has had to narrow the width the rest of the build keeps at
* 64 (D_FILE_WIDTH_IS_NARROWED) the header publishes nothing, since no level
* changes what a shared type is.
*
*
* path:      /inc/djinterp/core/filesys/file_space.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.13
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

#ifndef DJINTERP_FILESYS_FILE_SPACE_HPP
#define DJINTERP_FILESYS_FILE_SPACE_HPP 1

// std
#include <cerrno>  // EINVAL
// djinterp
#include "file_path.hpp"                 // path
#include "file_common.hpp"               // error, the D_* kit
#include "../../c/filesys/file_space.h"  // d_file_space, d_space_t
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint64_t

// space_info carries a width; where this level has had to narrow it, it is
// absent rather than a different type (see the banner)
#if !D_FILE_WIDTH_IS_NARROWED


NS_DJINTERP


//==============================================================================
// 1.  CAPACITY
//==============================================================================


// 1.1    Snapshot
//------------------------------------------------------------------------------
// 1.1.1
// space_info
//   struct: a filesystem capacity snapshot -- in bytes at D_FILE_WIDTH 64, in
// KiB at 32 (see the banner). Public members, mirroring
// std::filesystem::space_info -- a plain value with nothing to hide. See the
// banner on why `available`, not `free`, is the number to size a write
// against.
#if (D_FILE_WIDTH == 64)
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

    /**
     * @brief Constructs a snapshot from the C layer's answer, in bytes.
     *
     * @param[in] _s  the filled capacity; copied.
     */
    explicit space_info(
        const struct d_space_t& _s
    )
        : capacity(_s.capacity),
          free(_s.free),
          available(_s.available)
    {}
};
#else
struct space_info
{
    uint32_t capacity_kib;    // total KiB on the filesystem
    uint32_t free_kib;        // unallocated, INCLUDING the root-only reserve
    uint32_t available_kib;   // KiB this user may actually claim -- use this

    /**
     * @brief Constructs an all-zero snapshot.
     */
    space_info(void)
        : capacity_kib(0),
          free_kib(0),
          available_kib(0)
    {}

    /**
     * @brief Constructs a snapshot from the C layer's answer, in KiB.
     *
     * @param[in] _s  the filled capacity; copied.
     */
    explicit space_info(
        const struct d_space_t& _s
    )
        : capacity_kib(_s.capacity_kib),
          free_kib(_s.free_kib),
          available_kib(_s.available_kib)
    {}
};
#endif  // D_FILE_WIDTH == 64

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
 *                  one that does not exist, EOVERFLOW at D_FILE_WIDTH 32 for
 *                  a filesystem of 4 TiB or more, otherwise the platform's
 *                  code.
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

    struct d_space_t s = d_space_t();

    // c/filesys fills s, or reports through errno
    if (d_file_space(_p.c_str(),
                     &s) != 0)
    {
        _ec = error::from_errno();

        return out;
    }

    _ec.clear();

    return space_info(s);
}


NS_END  // djinterp

#endif  // !D_FILE_WIDTH_IS_NARROWED

#endif  // DJINTERP_FILESYS_FILE_SPACE_HPP
