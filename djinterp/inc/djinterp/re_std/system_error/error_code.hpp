/***********************************************************************
* restd                                                   error_code.hpp
*
* the error_code value type (re-export):
*   error_code pairs an integer value with an error_category reference. Its
*   value depends on the runtime category singletons, so restd re-exports
*   std::error_code (identity preserved). The relational and equality
*   operators are free functions in namespace std found by ADL on the
*   (std) operand type, so they keep working under the restd spelling with
*   no re-declaration; operator<=> arrives from std on C++20+.
*
*
* path:      /inc/restd/system_error/error_code.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_SYSTEM_ERROR_ERROR_CODE_
#define RESTD_SYSTEM_ERROR_ERROR_CODE_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // error_code
    //   class: identity-preserving re-export of std::error_code.
    using ::std::error_code;

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_SYSTEM_ERROR_ERROR_CODE_
