/***********************************************************************
* restd                                              make_error_code.hpp
*
* the make_error_code factory (re-export):
*   builds an error_code from an errc value (and, where their headers are
*   included, from future_errc / io_errc). The mapping to generic_category()
*   is runtime-provided, so restd re-exports std::make_error_code; only the
*   overloads whose enums are in scope participate in the using-set.
*
*
* path:      /inc/restd/system_error/make_error_code.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_SYSTEM_ERROR_MAKE_ERROR_CODE_
#define RESTD_SYSTEM_ERROR_MAKE_ERROR_CODE_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // make_error_code
    //   function: re-export of std::make_error_code (errc overload).
    using ::std::make_error_code;

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_SYSTEM_ERROR_MAKE_ERROR_CODE_
