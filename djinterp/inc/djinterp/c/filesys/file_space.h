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
*   Two widths, as everywhere in filesys (file_common.h), with one difference
* the others do not have: the 32-bit twin counts KiB, not bytes, because 32
* bits of bytes stop at 4 GiB and a disk that small is rare, where 32 bits of
* KiB reach 4 TiB. The fields are renamed to match -- capacity_kib, free_kib,
* available_kib -- so code written for one unit fails to compile against the
* other rather than reading the wrong number.
*
*
* path:      /inc/djinterp/c/filesys/file_space.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Capacity
         1.  d_space64_t
         2.  d_space32_t
2.  QUERY
    -----
    1.  Filesystem capacity
3.  DEFAULT WIDTH
    -------------
    1.  Unsuffixed names
*/

#ifndef DJINTERP_C_FILESYS_FILE_SPACE_H
#define DJINTERP_C_FILESYS_FILE_SPACE_H 1

// djinterp
#include "./file_common.h"                          // foundation, D_EXTERN_C_*
#include "../../config/c/filesys/cfg_file_space.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint32_t, uint64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Capacity
//------------------------------------------------------------------------------
// 1.1.1
// d_space64_t
//   struct: the capacity of the filesystem holding some path. Byte counts,
// not blocks -- a caller should not have to know what f_frsize is to use
// this. The field order and meaning match std::filesystem::space_info
// deliberately, so the C++ layer is a copy rather than a translation.
#if D_FILE_64_IS_AVAILABLE
struct d_space64_t
{
    uint64_t capacity;   // total bytes on the filesystem
    uint64_t free;       // unallocated bytes, INCLUDING any root reserve
    uint64_t available;  // bytes this user may actually claim. Use this one.
};
#endif  // D_FILE_64_IS_AVAILABLE

// 1.1.2
// d_space32_t
//   struct: the same three numbers in KiB (1024 bytes), rounded down, so each
// reaches 4 TiB. Rounding down never claims room that is not there.
struct d_space32_t
{
    uint32_t capacity_kib;   // total KiB on the filesystem
    uint32_t free_kib;       // unallocated KiB, INCLUDING any root reserve
    uint32_t available_kib;  // KiB this user may actually claim. Use this one.
};


//==============================================================================
// 2.  QUERY
//==============================================================================


// 2.1    Filesystem capacity
//------------------------------------------------------------------------------
#if D_FILE_64_IS_AVAILABLE
/**
 * @brief Reports the capacity of the filesystem holding a path.
 *
 * @note The answer describes the whole filesystem, not the path. Unless the
 *       caller is root, `available` -- not `free` -- says whether a write
 *       will fit.
 * @note On Windows, whose call takes only a directory, a file is measured by
 *       the directory that holds it: the same volume.
 *
 * @param[in]  _path  any existing path on the filesystem of interest, a file
 *                    or a directory.
 * @param[out] _out   receives the capacity; zeroed on failure, so a caller
 *                    who ignores the result cannot read a stale number.
 * @return 0, or -1 on failure with errno set; ENOSYS where the platform
 *         reports no capacity, which is never emulated.
 */
int d_file_space64(const char*         _path,
                   struct d_space64_t* _out);
#endif  // D_FILE_64_IS_AVAILABLE
/**
 * @brief d_file_space64, in KiB.
 *
 * @param[in]  _path  any existing path on the filesystem of interest.
 * @param[out] _out   receives the capacity in KiB; zeroed on failure.
 * @return 0, or -1 on failure with errno set; EOVERFLOW for a filesystem of
 *         4 TiB or more, ENOSYS where the platform reports no capacity.
 */
int d_file_space32(const char*         _path,
                   struct d_space32_t* _out);


//==============================================================================
// 3.  DEFAULT WIDTH
//==============================================================================


// 3.1    Unsuffixed names
//------------------------------------------------------------------------------
// d_space_t
//   alias: the struct tag of the capacity twin at D_FILE_WIDTH (file_common.h,
// 3.2) -- bytes at 64, KiB at 32, under the field names that say which;
// d_file_space after it is the same alias for the call.
#if (D_FILE_WIDTH == 64)
    #define d_space_t    d_space64_t
    #define d_file_space d_file_space64
#else
    #define d_space_t    d_space32_t
    #define d_file_space d_file_space32
#endif  // D_FILE_WIDTH == 64


D_EXTERN_C_END


#endif  // DJINTERP_C_FILESYS_FILE_SPACE_H
