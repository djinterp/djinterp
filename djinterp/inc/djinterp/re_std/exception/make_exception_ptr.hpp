/***********************************************************************
* restd                                            make_exception_ptr.hpp
*
* make_exception_ptr:
*   captures a copy of a given value as an exception_ptr (effectively
* `try { throw e; } catch (...) { return current_exception(); }`). It is
* a C++11 free function built directly on the exception_ptr facility, so
* restd re-exports std::make_exception_ptr on C++11+. No C++98 path —
* it depends on current_exception(), which is itself ABI-provided.
*
*
* path:      /inc/djinterp/re_std/exception/make_exception_ptr.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_EXCEPTION_MAKE_EXCEPTION_PTR_
#define RESTD_EXCEPTION_MAKE_EXCEPTION_PTR_ 1

#include "../djinterp.hpp"
#include "exception_ptr.hpp"

#if ( D_ENV_LANG_IS_CPP11_OR_HIGHER && \
      D_ENV_CPP98_HAS_EXCEPTION )

    #include <exception>

namespace restd
{
    // make_exception_ptr
    //   function: using-declaration from std::make_exception_ptr.
    using std::make_exception_ptr;

} // namespace restd

#endif // C++11+ && <exception>

#endif // RESTD_EXCEPTION_MAKE_EXCEPTION_PTR_
