/***********************************************************************
* re_std                                            initializer_list.hpp
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
* path:      /inc/djinterp/re_std/initializer_list/initializer_list.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef DJINTERP_RE_STD_INITIALIZER_LIST_INITIALIZER_LIST_
#define DJINTERP_RE_STD_INITIALIZER_LIST_INITIALIZER_LIST_ 1

// djinterp
#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
//   <initializer_list> is one of the few standard headers re_std is permitted
// to include directly (the type is compiler-provided and unimplementable).
#include <initializer_list>

NS_RESTD

    // initializer_list
    //   type: identity-preserving re-export of std::initializer_list. The
    // compiler only ever materialises a brace-init-list as
    // std::initializer_list, so re_std::initializer_list IS that same type.
    using ::std::initializer_list;

NS_END  // re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_INITIALIZER_LIST_INITIALIZER_LIST_
