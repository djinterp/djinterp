/***********************************************************************
* restd                                           is_error_code_enum.hpp
*
* the is_error_code_enum trait (re-export + _v back-port):
*   the user customisation-point trait marking an enum as an error_code enum
*   (specialised true for errc). restd re-exports std::is_error_code_enum so
*   user and library specialisations are shared. std adds the _v variable at
*   C++17; restd back-ports it to C++14 (variable templates), computed from
*   the C++11 trait's ::value.
*
*
* path:      /inc/djinterp/re_std/system_error/is_error_code_enum.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_SYSTEM_ERROR_IS_ERROR_CODE_ENUM_
#define RESTD_SYSTEM_ERROR_IS_ERROR_CODE_ENUM_ 1

// djinterp
#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

NS_RESTD

    // is_error_code_enum
    //   trait: re-export of std::is_error_code_enum (user-specialisable).
    using ::std::is_error_code_enum;

    // is_error_code_enum_v (C++14+)
    //   variable: value alias; std ships it at C++17, restd at C++14.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_error_code_enum_v = is_error_code_enum<_Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

NS_END  // restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_SYSTEM_ERROR_IS_ERROR_CODE_ENUM_
