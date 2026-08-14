/***********************************************************************
* restd                                         make_error_condition.hpp
*
* the make_error_condition factory (re-export):
*   builds an error_condition from an errc value, bound to
*   generic_category(). The mapping is runtime-provided, so restd re-exports
*   std::make_error_condition.
*
*
* path:      /inc/djinterp/re_std/system_error/make_error_condition.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_SYSTEM_ERROR_MAKE_ERROR_CONDITION_
#define RESTD_SYSTEM_ERROR_MAKE_ERROR_CONDITION_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // make_error_condition
    //   function: re-export of std::make_error_condition (errc overload).
    using ::std::make_error_condition;

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_SYSTEM_ERROR_MAKE_ERROR_CONDITION_
