/*******************************************************************************
* djinterp [re_std]                                          system_category.hpp
*
* the system_category() accessor (re-export):
*   returns the reference to the program-wide system_category singleton (the
*   category for OS-level error codes). Like generic_category() it is a
*   runtime-provided, address-compared object, so re_std re-exports
*   std::system_category.
*
*
* path:      /inc/re_std/system_error/system_category.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SYSTEM_ERROR_SYSTEM_CATEGORY_HPP
#define RE_STD_SYSTEM_ERROR_SYSTEM_CATEGORY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

namespace re_std
{

    // system_category
    //   function: re-export of std::system_category (singleton accessor).
    using ::std::system_category;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SYSTEM_ERROR_SYSTEM_CATEGORY_HPP
