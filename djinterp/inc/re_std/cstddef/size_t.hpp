/*******************************************************************************
* djinterp [re_std]                                                   size_t.hpp
*
* size_t typedef (identity-preserving re-export):
*   size_t is the type `sizeof` yields. The language, not the library,
* decides what that type is -- it is unsigned long on one LP64 target,
* unsigned long long on another, unsigned int on a 32-bit one -- and no
* portable definition can name it correctly on every platform. Writing
* `typedef unsigned long size_t;` would be right on Linux and wrong on
* Windows, and the mismatch would only surface at a link boundary.
*
*   re_std therefore re-exports it: re_std::size_t IS std::size_t, so a
* value of one binds to the other, overload resolution behaves the same
* way, and `sizeof(x)` has the re_std-spelled type without a conversion.
*
*   <cstddef> is one of the three standard headers the dependency rules
* permit (RE_STD_AGENT_README.md, "Dependency Rules" 3) precisely because
* of this: the fundamental types cannot be reimplemented, only surfaced.
*
*
* path:      /inc/re_std/cstddef/size_t.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CSTDDEF_SIZE_T_HPP
#define RE_STD_CSTDDEF_SIZE_T_HPP 1

// std
//   permitted: fundamental types only.
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{

    // size_t
    //   typedef: identity-preserving re-export of std::size_t. The result
    // type of sizeof; the implementation, not the library, fixes its width.
    using ::std::size_t;

}  // re_std


#endif  // RE_STD_CSTDDEF_SIZE_T_HPP
