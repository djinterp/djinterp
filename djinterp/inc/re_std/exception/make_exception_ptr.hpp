/*******************************************************************************
* djinterp [re_std]                                       make_exception_ptr.hpp
*
* make_exception_ptr:
*   captures a copy of a given value as an exception_ptr (effectively
* `try { throw e; } catch (...) { return current_exception(); }`). It is
* a C++11 free function built directly on the exception_ptr facility, so
* re_std re-exports std::make_exception_ptr on C++11+. No C++98 path —
* it depends on current_exception(), which is itself ABI-provided.
*
*
* path:      /inc/re_std/exception/make_exception_ptr.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXCEPTION_MAKE_EXCEPTION_PTR_HPP
#define RE_STD_EXCEPTION_MAKE_EXCEPTION_PTR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "exception_ptr.hpp"

#if ( RE_STD_LANG_IS_CPP11_OR_HIGHER && \
      RE_STD_HAS_EXCEPTIONS )

    // std
    #include <exception>

namespace re_std
{
    // make_exception_ptr
    //   function: using-declaration from std::make_exception_ptr.
    using std::make_exception_ptr;

}  // re_std
#endif // C++11+ && <exception>

#endif  // RE_STD_EXCEPTION_MAKE_EXCEPTION_PTR_HPP
