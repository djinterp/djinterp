/*******************************************************************************
* djinterp [c]                                                      file_space.h
*
* Filesystem capacity.
*   Three numbers, and the middle one is a trap. `free` is every unallocated
* byte on the filesystem; `available` is what THIS user may actually claim.
* They differ by the root reserve (ext4 keeps 5% back by default), by quotas,
* and by whatever an overlay or container decides -- and the gap is not small.
* On the machine this module was written on: free 249 GiB, available 10 GiB.
* A 24x difference. Deciding "will this write fit" from `free` is how a program
* confidently runs out of disk.
*   Unless you ARE root, you want `available`.
*
*
* path:      /inc/djinterp/c/fs/file_space.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Capacity
         1.  d_space_t
2.  QUERY
    -----
    1.  Filesystem capacity
*/

#ifndef DJINTERP_C_FS_FILE_SPACE_H
#define DJINTERP_C_FS_FILE_SPACE_H 1

// djinterp
#include "./file_common.h"                     // fs foundation, D_EXTERN_C_*
#include "../../config/c/fs/cfg_file_space.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Capacity
//------------------------------------------------------------------------------
// 1.1.1
// d_space_t
//   struct: the capacity of the filesystem holding some path. Byte counts,
// not blocks -- a caller should not have to know what f_frsize is to use
// this. The field order and meaning match std::filesystem::space_info
// deliberately, so the C++ layer is a copy rather than a translation.
struct d_space_t
{
    uint64_t capacity;   // total bytes on the filesystem
    uint64_t free;       // unallocated bytes, INCLUDING any root reserve
    uint64_t available;  // bytes this user may actually claim. Use this one.
};


//==============================================================================
// 2.  QUERY
//==============================================================================


// 2.1    Filesystem capacity
//------------------------------------------------------------------------------
/**
 * @brief Reports the capacity of the filesystem holding a path.
 *
 * @note The answer describes the whole filesystem, not the path. Unless the
 *       caller is root, `available` -- not `free` -- says whether a write
 *       will fit.
 *
 * @param[in]  _path  any existing path on the filesystem of interest.
 * @param[out] _out   receives the capacity; zeroed on failure, so a caller
 *                    who ignores the result cannot read a stale number.
 * @return 0, or -1 on failure with errno set; ENOSYS where the platform
 *         reports no capacity, which is never emulated.
 */
int d_file_space(const char*       _path,
                 struct d_space_t* _out);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_SPACE_H
