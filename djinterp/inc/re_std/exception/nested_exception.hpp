/***********************************************************************
* re_std                                             nested_exception.hpp
*
* nested_exception:
*   a mixin whose constructor captures current_exception(), enabling the
* "throw with nested" idiom for exception chaining. C++11, RTTI-backed
* (rethrow_nested() / nested_ptr() rely on the captured exception_ptr).
* Built on the exception_ptr facility, so re_std re-exports the std type
* on C++11+; no portable C++98 path exists.
*
*
* path:      /inc/djinterp/re_std/exception/nested_exception.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_EXCEPTION_NESTED_EXCEPTION_
#define DJINTERP_RE_STD_EXCEPTION_NESTED_EXCEPTION_ 1

#include "../../core/djinterp.hpp"
#include "exception_ptr.hpp"

#if ( D_ENV_LANG_IS_CPP11_OR_HIGHER && \
      D_ENV_CPP98_HAS_EXCEPTION )

    #include <exception>

namespace re_std
{
    // nested_exception
    //   class: using-declaration from std::nested_exception.
    using std::nested_exception;

} // namespace re_std

#endif // C++11+ && <exception>

#endif  // DJINTERP_RE_STD_EXCEPTION_NESTED_EXCEPTION_
