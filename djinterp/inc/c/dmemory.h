/******************************************************************************
* djinterp [core]                                                    dmemory.h
*
* Cross-platform definition of <memory.h> module.
*
*
* path:      \inc\dmemory.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.03.03
******************************************************************************/

#ifndef DJINTERP_MEMORY_
#define DJINTERP_MEMORY_ 1

#include <stddef.h>      // for size_t
#include <stdlib.h>      // for malloc
#include <string.h>      // for memcpy
#include ".\djinterp.h"


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


void*   d_memcpy(void*       _destination,
                 const void* _source,
                 size_t      _count);
int     d_memcpy_s(void*       _destination,
                   size_t      _destSize,
                   const void* _source,
                   size_t      _count);
void*   d_memdup_s(const void* _src,
                   size_t      _size);
void*   d_memset(void*  _ptr,
                 int    _value,
                 size_t _amount);
errno_t d_memset_s(void*   _destination,
                   rsize_t _destsz,
                   int     _ch,
                   rsize_t _count);


#endif  // DJINTERP_MEMORY_
