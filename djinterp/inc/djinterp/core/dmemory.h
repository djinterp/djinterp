/*******************************************************************************
* djinterp [c]                                                         dmemory.h
*
* Cross-platform definition of <memory.h> module.
*
*
* path:      /inc/djinterp/c/dmemory.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.03
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_C_DMEMORY_H
#define DJINTERP_C_DMEMORY_H 1

// std
#include <stddef.h>      // for size_t
#include <stdlib.h>      // for malloc
#include <string.h>      // for memcpy
// djinterp
#include "./djinterp.h"


// compatibility constants and types

#ifndef EINVAL
    #define EINVAL 22   // invalid argument
#endif  // EINVAL

#ifndef ERANGE
    #define ERANGE 34   // result too large
#endif  // ERANGE

#ifndef errno_t
    typedef int errno_t;
#endif

#ifndef rsize_t
    typedef size_t rsize_t;
#endif

#ifndef RSIZE_MAX
    #define RSIZE_MAX SIZE_MAX
#endif

#ifndef EOVERFLOW
    #define EOVERFLOW 75
#endif



//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

void*   d_memcpy(void*       _destination,
                 const void* _source,
                 size_t      _amount);
int     d_memcpy_s(void*       _destination,
                   size_t      _destination_size,
                   const void* _source,
                   size_t      _amount);
void*   d_memdup_s(const void* _source,
                   size_t      _size);
void*   d_memset(void*  _ptr,
                 int    _value,
                 size_t _amount);
errno_t d_memset_s(void*   _destination,
                   rsize_t _destsz,
                   int     _ch,
                   rsize_t _count);



D_EXTERN_C_END

#endif  // DJINTERP_C_DMEMORY_H
