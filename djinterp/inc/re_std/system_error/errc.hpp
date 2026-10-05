/*******************************************************************************
* djinterp [re_std]                                                     errc.hpp
*
* the errc scoped enumeration (re-export):
*   errc is a C++11 scoped enumeration whose enumerators mirror the POSIX
*   errno constants and which is the canonical error_code_enum recognised by
*   make_error_code. It is meaningful only alongside the runtime category
*   machinery, so re_std re-exports std::errc rather than back-porting a
*   struct-wrapper enum to C++98.
*
*
* path:      /inc/re_std/system_error/errc.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SYSTEM_ERROR_ERRC_HPP
#define RE_STD_SYSTEM_ERROR_ERRC_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

namespace re_std
{

    // errc
    //   enum: identity-preserving re-export of the std::errc scoped enum.
    using ::std::errc;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SYSTEM_ERROR_ERRC_HPP
