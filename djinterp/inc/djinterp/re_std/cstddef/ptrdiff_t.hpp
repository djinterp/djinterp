/******************************************************************************
* djinterp [re_std]                                                ptrdiff_t.hpp
*
* ptrdiff_t typedef (identity-preserving re-export):
*   ptrdiff_t is the type of the difference of two pointers. Like size_t
* it is fixed by the implementation, not by the library, and cannot be
* portably spelled out. re_std re-exports it so that re_std::ptrdiff_t
* IS std::ptrdiff_t and iterator difference types interoperate with the
* rest of the toolchain without a conversion.
*
*
* path:      /inc/djinterp/re_std/cstddef/ptrdiff_t.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDDEF_PTRDIFF_T_
#define DJINTERP_RE_STD_CSTDDEF_PTRDIFF_T_ 1

// djinterp
#include "../../core/djinterp.hpp"

// std
//   permitted: fundamental types only.
#include <cstddef>


NS_RESTD

    // ptrdiff_t
    //   typedef: identity-preserving re-export of std::ptrdiff_t. The signed
    // result type of subtracting two pointers into the same array.
    using ::std::ptrdiff_t;

NS_END  // re_std


#endif  // DJINTERP_RE_STD_CSTDDEF_PTRDIFF_T_
