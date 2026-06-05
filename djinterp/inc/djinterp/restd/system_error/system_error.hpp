/***********************************************************************
* restd                                                 system_error.hpp
*
* the system_error exception type (re-export):
*   system_error is the exception thrown for error_code-bearing failures;
*   it derives from std::runtime_error and carries an error_code. restd
*   re-exports std::system_error so that catch (std::system_error&) catches
*   a restd-thrown system_error and vice versa, and so it remains catchable
*   as restd::runtime_error / restd::exception.
*
*
* path:      /inc/restd/system_error/system_error.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_SYSTEM_ERROR_SYSTEM_ERROR_
#define RESTD_SYSTEM_ERROR_SYSTEM_ERROR_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // system_error
    //   class: identity-preserving re-export of std::system_error
    //   (derives from runtime_error; carries an error_code).
    using ::std::system_error;

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_SYSTEM_ERROR_SYSTEM_ERROR_
