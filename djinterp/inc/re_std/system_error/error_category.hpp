/*******************************************************************************
* djinterp [re_std]                                           error_category.hpp
*
* the error_category abstract base (re-export):
*   error_category is an abstract polymorphic base whose concrete instances
*   (generic_category(), system_category(), and user categories) are
*   runtime-provided singletons compared by address identity. That identity
*   cannot be reproduced portably, so re_std re-exports std::error_category;
*   re_std::error_category IS std::error_category.
*
*
* path:      /inc/re_std/system_error/error_category.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SYSTEM_ERROR_ERROR_CATEGORY_HPP
#define RE_STD_SYSTEM_ERROR_ERROR_CATEGORY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

namespace re_std
{

    // error_category
    //   class: identity-preserving re-export of the abstract category base.
    using ::std::error_category;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SYSTEM_ERROR_ERROR_CATEGORY_HPP
