/***********************************************************************
* re_std                                              error_category.hpp
*
* the error_category abstract base (re-export):
*   error_category is an abstract polymorphic base whose concrete instances
*   (generic_category(), system_category(), and user categories) are
*   runtime-provided singletons compared by address identity. That identity
*   cannot be reproduced portably, so re_std re-exports std::error_category;
*   re_std::error_category IS std::error_category.
*
*
* path:      /inc/djinterp/re_std/system_error/error_category.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef DJINTERP_RE_STD_SYSTEM_ERROR_ERROR_CATEGORY_
#define DJINTERP_RE_STD_SYSTEM_ERROR_ERROR_CATEGORY_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // error_category
    //   class: identity-preserving re-export of the abstract category base.
    using ::std::error_category;

NS_END  // re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_SYSTEM_ERROR_ERROR_CATEGORY_
