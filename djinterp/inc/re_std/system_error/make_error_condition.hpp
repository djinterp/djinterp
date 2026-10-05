/*******************************************************************************
* djinterp [re_std]                                     make_error_condition.hpp
*
* the make_error_condition factory (re-export):
*   builds an error_condition from an errc value, bound to
*   generic_category(). The mapping is runtime-provided, so re_std re-exports
*   std::make_error_condition.
*
*
* path:      /inc/re_std/system_error/make_error_condition.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SYSTEM_ERROR_MAKE_ERROR_CONDITION_HPP
#define RE_STD_SYSTEM_ERROR_MAKE_ERROR_CONDITION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

namespace re_std
{

    // make_error_condition
    //   function: re-export of std::make_error_condition (errc overload).
    using ::std::make_error_condition;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SYSTEM_ERROR_MAKE_ERROR_CONDITION_HPP
