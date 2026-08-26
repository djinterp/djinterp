/***********************************************************************
* re_std                                        make_error_condition.hpp
*
* the make_error_condition factory (re-export):
*   builds an error_condition from an errc value, bound to
*   generic_category(). The mapping is runtime-provided, so re_std re-exports
*   std::make_error_condition.
*
*
* path:      /inc/djinterp/re_std/system_error/make_error_condition.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef DJINTERP_RE_STD_SYSTEM_ERROR_MAKE_ERROR_CONDITION_
#define DJINTERP_RE_STD_SYSTEM_ERROR_MAKE_ERROR_CONDITION_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // make_error_condition
    //   function: re-export of std::make_error_condition (errc overload).
    using ::std::make_error_condition;

NS_END  // re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_SYSTEM_ERROR_MAKE_ERROR_CONDITION_
