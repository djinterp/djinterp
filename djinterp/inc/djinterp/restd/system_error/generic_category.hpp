/***********************************************************************
* restd                                             generic_category.hpp
*
* the generic_category() accessor (re-export):
*   returns the reference to the program-wide generic_category singleton
*   (the category for errc / portable conditions). The singleton is a
*   runtime-provided object compared by address, so restd re-exports
*   std::generic_category to preserve that one true identity.
*
*
* path:      /inc/restd/system_error/generic_category.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_SYSTEM_ERROR_GENERIC_CATEGORY_
#define RESTD_SYSTEM_ERROR_GENERIC_CATEGORY_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // generic_category
    //   function: re-export of std::generic_category (singleton accessor).
    using ::std::generic_category;

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_SYSTEM_ERROR_GENERIC_CATEGORY_
