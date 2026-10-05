/*******************************************************************************
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
* path:      /inc/re_std/cstddef/ptrdiff_t.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CSTDDEF_PTRDIFF_T_HPP
#define RE_STD_CSTDDEF_PTRDIFF_T_HPP 1

// std
//   permitted: fundamental types only.
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{

    // ptrdiff_t
    //   typedef: identity-preserving re-export of std::ptrdiff_t. The signed
    // result type of subtracting two pointers into the same array.
    using ::std::ptrdiff_t;

}  // re_std


#endif  // RE_STD_CSTDDEF_PTRDIFF_T_HPP
