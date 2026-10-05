/*******************************************************************************
* djinterp [re_std]                                          error_condition.hpp
*
* the error_condition portable-condition type (re-export):
*   error_condition is the platform-independent counterpart of error_code,
*   also bound to the runtime category singletons, so re_std re-exports
*   std::error_condition (identity preserved). Comparison operators arrive
*   via ADL on the std operand type, exactly as for error_code.
*
*
* path:      /inc/re_std/system_error/error_condition.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SYSTEM_ERROR_ERROR_CONDITION_HPP
#define RE_STD_SYSTEM_ERROR_ERROR_CONDITION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

namespace re_std
{

    // error_condition
    //   class: identity-preserving re-export of std::error_condition.
    using ::std::error_condition;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SYSTEM_ERROR_ERROR_CONDITION_HPP
