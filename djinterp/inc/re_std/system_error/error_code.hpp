/*******************************************************************************
* djinterp [re_std]                                               error_code.hpp
*
* the error_code value type (re-export):
*   error_code pairs an integer value with an error_category reference. Its
*   value depends on the runtime category singletons, so re_std re-exports
*   std::error_code (identity preserved). The relational and equality
*   operators are free functions in namespace std found by ADL on the
*   (std) operand type, so they keep working under the re_std spelling with
*   no re-declaration; operator<=> arrives from std on C++20+.
*
*
* path:      /inc/re_std/system_error/error_code.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SYSTEM_ERROR_ERROR_CODE_HPP
#define RE_STD_SYSTEM_ERROR_ERROR_CODE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

namespace re_std
{

    // error_code
    //   class: identity-preserving re-export of std::error_code.
    using ::std::error_code;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SYSTEM_ERROR_ERROR_CODE_HPP
