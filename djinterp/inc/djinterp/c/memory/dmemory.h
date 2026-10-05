/*******************************************************************************
* djinterp [c]                                                         dmemory.h
*
* Cross-platform memory operations and secure-memory compatibility.
*   Declares djinterp wrappers for raw memory copying, duplication, and
* filling, including bounded variants intended to provide a consistent C
* interface across supported platforms. Uses native implementations wherever
* possible.
*
* RATIONALE:
*   C compilers differ in their support for bounds-checked and secure
* memory operations:
* +---------------+--------+----------+--------+----------+--------+----------+
* | Env / libc    | memcpy | memcpy_s | memdup | memdup_s | memset | memset_s |
* +---------------+--------+----------+--------+----------+--------+----------+
* | ISO C90-C99   |  yes   |    no    |   no   |    no    |  yes   |    no    |
* | ISO C11-C23   |  yes   |   opt.   |   no   |    no    |  yes   |   opt.   |
* | MSVC / UCRT   |  yes   |   yes    |   no   |    no    |  yes   |    no    |
* | GCC / glibc   |  yes   |    no    |   no   |    no    |  yes   |    no    |
* | Clang / glibc |  yes   |    no    |   no   |    no    |  yes   |    no    |
* | GCC/MinGW-w64 |  yes   |   yes    |   no   |    no    |  yes   |    no    |
* | Clang/MinGW   |  yes   |   yes    |   no   |    no    |  yes   |    no    |
* | Apple libc    |  yes   |    no    |   no   |    no    |  yes   |   yes    |
* | Solaris libc  |  yes   |   yes    |   no   |    no    |  yes   |   yes    |
* +---------------+--------+----------+--------+----------+--------+----------+
*   Microsoft's UCRT provides functions such as `memcpy_s` but does not
* implement `memset_s`, while many GCC- and Clang-based environments do not
* provide the optional Annex K `_s` interfaces, `errno_t`, `rsize_t`, or
* `RSIZE_MAX`.
*   Platforms also expose different facilities for guaranteed non-elidable
* memory clearing. This module normalizes those differences behind
* a single djinterp interface, and declares none of Annex K's own names
* (section I says why).
*
* FUNCTIONS:
*   d_memcpy
*       - uniform djinterp wrapper around ISO C memcpy
*   d_memcpy_s
*       - portable bounds-checked memcpy
*       - adapts MS secure CRT / Annex K / manual implementation
*   d_memdup
*       - djinterp extension absent from ISO C
*   d_memdup_s
*       - djinterp extension absent from ISO C
*   d_memset
*       - uniform djinterp wrapper around ISO C memset
*   d_memset_s
*       - portable bounds-checked and possibly secure memset
*       - adapts secure-zero facilities depending on platform
*
*
* path:      /inc/djinterp/c/memory/dmemory.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.03
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_MEMORY_DMEMORY_H
#define DJINTERP_C_MEMORY_DMEMORY_H 1

// std
#include <errno.h>        // EINVAL, ERANGE, EOVERFLOW
#include <stddef.h>       // size_t
#include <stdlib.h>       // malloc
#include <string.h>       // memcpy, memset
// djinterp
#include "../djinterp.h"  // D_EXTERN_C_BEGIN, D_NODISCARD
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // SIZE_MAX


//==============================================================================
// I.   ERROR CODES AND LIMITS
//==============================================================================
//   The bounded functions report failure through <errno.h>'s codes. This header
// declares none of Annex K's names (errno_t, rsize_t, RSIZE_MAX): a typedef is
// invisible to the preprocessor, so no header can tell whether the C library
// already declared one, and declaring it twice is an error before C11 (macOS,
// MinGW-w64 and MSVC all declare both). The bounded functions take size_t and
// return int, the types those names stand for everywhere, and
// D_MEMORY_RSIZE_MAX stands in for RSIZE_MAX.

// EINVAL
//   constant: fallback for the error code of an invalid argument, for a C
// library whose <errno.h> lacks it (ISO C guarantees only ERANGE of the three).
#ifndef EINVAL
    #define EINVAL 22
#endif  // EINVAL

// EOVERFLOW
//   constant: fallback for the error code of a value too large for its type.
#ifndef EOVERFLOW
    #define EOVERFLOW 75
#endif  // EOVERFLOW

// D_MEMORY_RSIZE_MAX
//   constant: the largest size the bounded functions accept: RSIZE_MAX where
// the C library defines it, otherwise SIZE_MAX / 2, as Annex K recommends, so
// that a negative size converted to size_t is refused rather than obeyed.
#if defined(RSIZE_MAX)
    #define D_MEMORY_RSIZE_MAX RSIZE_MAX
#else
    #define D_MEMORY_RSIZE_MAX (SIZE_MAX >> 1)
#endif  // RSIZE_MAX


//==============================================================================
// II.  MEMORY OPERATIONS
//==============================================================================

D_EXTERN_C_BEGIN

// d_memcpy
//   function: copies _amount bytes from _source to _destination, as memcpy;
// returns _destination.
void*   d_memcpy(  void*       _destination,
                   const void* _source,
                   size_t      _amount);

// d_memcpy_s
//   function: copies _amount bytes when they fit in _destination_size, as
// Annex K's memcpy_s. Returns 0, or EINVAL for a null pointer, or ERANGE when
// the copy does not fit; after a failure with a destination, it is cleared.
int     d_memcpy_s(void*       _destination,
                   size_t      _destination_size,
                   const void* _source,
                   size_t      _amount);

// d_memdup
//   function: a malloc'd copy of _size bytes of _source, which the caller
// frees; NULL for a null _source or a failed allocation.
D_NODISCARD
void*   d_memdup(  const void* _source,
                   size_t      _size);

// d_memdup_s
//   function: d_memdup through d_memcpy_s; NULL as well for a zero _size.
D_NODISCARD
void*   d_memdup_s(const void* _source,
                   size_t      _size);

// d_memset
//   function: sets _amount bytes of _ptr to _value, as memset; returns _ptr.
void*   d_memset(  void*       _ptr,
                   int         _value,
                   size_t      _amount);

// d_memset_s
//   function: sets the first min(_count, _destination_size) bytes of
// _destination to _ch through volatile writes, which the compiler may not
// remove, as Annex K's memset_s. Returns 0, or EINVAL for a null destination
// or a size above D_MEMORY_RSIZE_MAX, or ERANGE when _count exceeds
// _destination_size, after filling the whole destination.
int     d_memset_s(void*       _destination,
                   size_t      _destination_size,
                   int         _ch,
                   size_t      _count);

D_EXTERN_C_END


#endif  // DJINTERP_C_MEMORY_DMEMORY_H
