/*******************************************************************************
* djinterp [re_std]                                                nullptr_t.hpp
*
* nullptr_t typedef (identity-preserving re-export):
*   nullptr_t is `decltype(nullptr)` -- a type the compiler synthesises,
* with conversion rules built into the language rather than the library.
* It cannot be reimplemented: a hand-rolled empty struct would not be the
* type a bare `nullptr` has, so overloads taking re_std::nullptr_t would
* never be selected by an actual null pointer literal.
*
*   The re-export therefore preserves identity: re_std::nullptr_t IS
* std::nullptr_t IS decltype(nullptr), and the three spellings select the
* same overload.
*
*   C++11 FLOOR:
*   The type has no C++98 meaning -- the language cannot form it before
* C++11 -- so the header gates itself out below that tier rather than
* substituting a NULL-shaped stand-in with different conversion rules.
*
*
* path:      /inc/re_std/cstddef/nullptr_t.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CSTDDEF_NULLPTR_T_HPP
#define RE_STD_CSTDDEF_NULLPTR_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
//   permitted: fundamental types only.
// std
#include <cstddef>


namespace re_std
{

    // nullptr_t
    //   typedef: identity-preserving re-export of std::nullptr_t, the
    // compiler-synthesised type of the nullptr literal.
    using ::std::nullptr_t;

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CSTDDEF_NULLPTR_T_HPP
