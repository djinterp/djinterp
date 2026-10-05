/*******************************************************************************
* djinterp [re_std]                                         generic_category.hpp
*
* the generic_category() accessor (re-export):
*   returns the reference to the program-wide generic_category singleton
*   (the category for errc / portable conditions). The singleton is a
*   runtime-provided object compared by address, so re_std re-exports
*   std::generic_category to preserve that one true identity.
*
*
* path:      /inc/re_std/system_error/generic_category.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SYSTEM_ERROR_GENERIC_CATEGORY_HPP
#define RE_STD_SYSTEM_ERROR_GENERIC_CATEGORY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

namespace re_std
{

    // generic_category
    //   function: re-export of std::generic_category (singleton accessor).
    using ::std::generic_category;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SYSTEM_ERROR_GENERIC_CATEGORY_HPP
