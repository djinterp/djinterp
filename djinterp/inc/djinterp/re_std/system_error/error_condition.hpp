/***********************************************************************
* restd                                              error_condition.hpp
*
* the error_condition portable-condition type (re-export):
*   error_condition is the platform-independent counterpart of error_code,
*   also bound to the runtime category singletons, so restd re-exports
*   std::error_condition (identity preserved). Comparison operators arrive
*   via ADL on the std operand type, exactly as for error_code.
*
*
* path:      /inc/djinterp/re_std/system_error/error_condition.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_SYSTEM_ERROR_ERROR_CONDITION_
#define RESTD_SYSTEM_ERROR_ERROR_CONDITION_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // error_condition
    //   class: identity-preserving re-export of std::error_condition.
    using ::std::error_condition;

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_SYSTEM_ERROR_ERROR_CONDITION_
