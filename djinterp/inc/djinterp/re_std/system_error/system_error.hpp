/***********************************************************************
* re_std                                                system_error.hpp
*
* the system_error exception type (re-export):
*   system_error is the exception thrown for error_code-bearing failures;
*   it derives from std::runtime_error and carries an error_code. re_std
*   re-exports std::system_error so that catch (std::system_error&) catches
*   a re_std-thrown system_error and vice versa, and so it remains catchable
*   as re_std::runtime_error / re_std::exception.
*
*
* path:      /inc/djinterp/re_std/system_error/system_error.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef DJINTERP_RE_STD_SYSTEM_ERROR_SYSTEM_ERROR_
#define DJINTERP_RE_STD_SYSTEM_ERROR_SYSTEM_ERROR_ 1

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

NS_END  // re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_SYSTEM_ERROR_SYSTEM_ERROR_
