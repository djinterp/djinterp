/*******************************************************************************
* djinterp [re_std]                                         nested_exception.hpp
*
* nested_exception:
*   a mixin whose constructor captures current_exception(), enabling the
* "throw with nested" idiom for exception chaining. C++11, RTTI-backed
* (rethrow_nested() / nested_ptr() rely on the captured exception_ptr).
* Built on the exception_ptr facility, so re_std re-exports the std type
* on C++11+; no portable C++98 path exists.
*
*
* path:      /inc/re_std/exception/nested_exception.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXCEPTION_NESTED_EXCEPTION_HPP
#define RE_STD_EXCEPTION_NESTED_EXCEPTION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "exception_ptr.hpp"

#if ( RE_STD_LANG_IS_CPP11_OR_HIGHER && \
      RE_STD_HAS_EXCEPTIONS )

    // std
    #include <exception>

namespace re_std
{
    // nested_exception
    //   class: using-declaration from std::nested_exception.
    using std::nested_exception;

}  // re_std
#endif // C++11+ && <exception>

#endif  // RE_STD_EXCEPTION_NESTED_EXCEPTION_HPP
