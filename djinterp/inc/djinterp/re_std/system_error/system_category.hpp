/***********************************************************************
* re_std                                             system_category.hpp
*
* the system_category() accessor (re-export):
*   returns the reference to the program-wide system_category singleton (the
*   category for OS-level error codes). Like generic_category() it is a
*   runtime-provided, address-compared object, so re_std re-exports
*   std::system_category.
*
*
* path:      /inc/djinterp/re_std/system_error/system_category.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef DJINTERP_RE_STD_SYSTEM_ERROR_SYSTEM_CATEGORY_
#define DJINTERP_RE_STD_SYSTEM_ERROR_SYSTEM_CATEGORY_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // system_category
    //   function: re-export of std::system_category (singleton accessor).
    using ::std::system_category;

NS_END  // re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_SYSTEM_ERROR_SYSTEM_CATEGORY_
