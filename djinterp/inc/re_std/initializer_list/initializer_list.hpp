/*******************************************************************************
* djinterp [re_std]                                         initializer_list.hpp
*
* the initializer_list class template (compiler-magic re-export):
*   std::initializer_list is the only type a brace-init-list ( { ... } )
*   is ever materialised as; the compiler synthesises it directly and it
*   cannot be reimplemented portably. re_std therefore surfaces it via an
*   identity-preserving using-declaration: re_std::initializer_list IS
*   std::initializer_list, so a braced list binds to either spelling and
*   the two interoperate. C++11 baseline; no C++98 path exists, as the
*   language cannot form the type before C++11.
*
*
* path:      /inc/re_std/initializer_list/initializer_list.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_INITIALIZER_LIST_INITIALIZER_LIST_HPP
#define RE_STD_INITIALIZER_LIST_INITIALIZER_LIST_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
//   <initializer_list> is one of the few standard headers re_std is permitted
// to include directly (the type is compiler-provided and unimplementable).
// std
#include <initializer_list>

namespace re_std
{

    // initializer_list
    //   type: identity-preserving re-export of std::initializer_list. The
    // compiler only ever materialises a brace-init-list as
    // std::initializer_list, so re_std::initializer_list IS that same type.
    using ::std::initializer_list;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_INITIALIZER_LIST_INITIALIZER_LIST_HPP
