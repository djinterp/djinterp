/*******************************************************************************
* djinterp [re_std]                                  is_error_condition_enum.hpp
*
* the is_error_condition_enum trait (re-export + _v back-port):
*   the customisation-point trait marking an enum as an error_condition enum
*   (specialised true for errc). re_std re-exports std::is_error_condition_enum
*   and back-ports the _v variable from std's C++17 to C++14 (variable
*   templates), computed from the C++11 trait's ::value.
*
*
* path:      /inc/re_std/system_error/is_error_condition_enum.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SYSTEM_ERROR_IS_ERROR_CONDITION_ENUM_HPP
#define RE_STD_SYSTEM_ERROR_IS_ERROR_CONDITION_ENUM_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

namespace re_std
{

    // is_error_condition_enum
    //   trait: re-export of std::is_error_condition_enum (user-specialisable).
    using ::std::is_error_condition_enum;

    // is_error_condition_enum_v (C++14+)
    //   variable: value alias; std ships it at C++17, re_std at C++14.
#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    template<typename Type>
    RE_STD_CONSTEXPR bool is_error_condition_enum_v =
        is_error_condition_enum<Type>::value;
#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SYSTEM_ERROR_IS_ERROR_CONDITION_ENUM_HPP
