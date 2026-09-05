/******************************************************************************
* djinterp [c]                                                    file_space.h
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
* path:      /inc/djinterp/c/fs/file_space.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    TYPES
      -----
      1.  d_space_t   (capacity / free / available)

II.   QUERY
      -----
      1.  d_space
*/

#ifndef DJINTERP_FILE_SPACE_
#define DJINTERP_FILE_SPACE_ 1

// djinterp
#include "./file_common.h"
#include "../../config/c/fs/cfg_file_space.h"


D_EXTERN_C_BEGIN


// I.    Types

// d_space_t
//   type: the capacity of the filesystem holding some path. Byte counts, not
// blocks -- a caller should not have to know what f_frsize is to use this.
//   The field order and meaning match std::filesystem::space_info
// deliberately, so the C++ layer is a copy rather than a translation.
struct d_space_t
{
    uint64_t capacity;   // total bytes on the filesystem
    uint64_t free;       // unallocated bytes, INCLUDING any root reserve
    uint64_t available;  // bytes this user may actually claim. Use this one.
};


// II.   Query
int d_space(const char*       _path,
            struct d_space_t* _out);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_SPACE_
