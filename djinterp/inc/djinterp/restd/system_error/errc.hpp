/***********************************************************************
* restd                                                         errc.hpp
*
* the errc scoped enumeration (re-export):
*   errc is a C++11 scoped enumeration whose enumerators mirror the POSIX
*   errno constants and which is the canonical error_code_enum recognised by
*   make_error_code. It is meaningful only alongside the runtime category
*   machinery, so restd re-exports std::errc rather than back-porting a
*   struct-wrapper enum to C++98.
*
*
* path:      /inc/restd/system_error/errc.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_SYSTEM_ERROR_ERRC_
#define RESTD_SYSTEM_ERROR_ERRC_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // errc
    //   enum: identity-preserving re-export of the std::errc scoped enum.
    using ::std::errc;

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_SYSTEM_ERROR_ERRC_
