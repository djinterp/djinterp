/*******************************************************************************
* djinterp [re_std]                                          make_error_code.hpp
*
* the make_error_code factory (re-export):
*   builds an error_code from an errc value (and, where their headers are
*   included, from future_errc / io_errc). The mapping to generic_category()
*   is runtime-provided, so re_std re-exports std::make_error_code; only the
*   overloads whose enums are in scope participate in the using-set.
*
*
* path:      /inc/re_std/system_error/make_error_code.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SYSTEM_ERROR_MAKE_ERROR_CODE_HPP
#define RE_STD_SYSTEM_ERROR_MAKE_ERROR_CODE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <system_error>

namespace re_std
{

    // make_error_code
    //   function: re-export of std::make_error_code (errc overload).
    using ::std::make_error_code;

}  // re_std

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SYSTEM_ERROR_MAKE_ERROR_CODE_HPP
