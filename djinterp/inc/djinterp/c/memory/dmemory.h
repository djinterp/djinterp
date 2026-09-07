/******************************************************************************
* djinterp [core]                                                    dmemory.h
*
*   Cross-platform memory operations and secure-memory compatibility.
*   Declares djinterp wrappers for raw memory copying, duplication, and
* filling, including bounded variants intended to provide a consistent C interface
* across supported platforms.
*   C compilers differ in their support for bounds-checked and secure
* memory operations. 
*   - Microsoft's UCRT provides functions such as memcpy_s but does not 
* implement memset_s, while many GCC- and Clang-based environments do not 
* provide the optional Annex K _s interfaces, errno_t, rsize_t, or RSIZE_MAX.
* Platforms also expose different facilities for guaranteed non-elidable 
* memory clearing. This module normalizes those differences behind
* a single djinterp interface.
*
*
* path:      /inc/djinterp/c/memory/dmemory.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.03.03
******************************************************************************/

#ifndef DJINTERP_C_MEMORY_
#define DJINTERP_C_MEMORY_ 1

// std
#include <errno.h>       // for EINVAL, ERANGE
#include <stddef.h>      // for size_t
#include <stdlib.h>      // for malloc
#include <string.h>      // for memcpy, memset
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
#endif  // errno_t

#ifndef rsize_t
    typedef size_t rsize_t;
#endif  // rsize_t

#ifndef RSIZE_MAX
    #define RSIZE_MAX SIZE_MAX
#endif  // RSIZE_MAX

#ifndef EOVERFLOW
    #define EOVERFLOW 75
#endif  // EOVERFLOW


//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN


// I.    memory operations
void*   d_memcpy(void*       _destination,
                 const void* _source,
                 size_t      _amount);
int     d_memcpy_s(void*       _destination,
                   size_t      _destination_size,
                   const void* _source,
                   size_t      _amount);
void*   d_memdup(const void* _source,
                 size_t      _size);
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


#endif  // DJINTERP_C_MEMORY_