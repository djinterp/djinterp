/***********************************************************************
* restd                                                 exception_ptr.hpp
*
* the exception_ptr facility:
*   exception_ptr (an opaque, shared-ownership handle to a captured
* exception), current_exception() (captures the exception currently
* being handled), and rethrow_exception() (re-raises a captured one).
* This trio is C++11 and is implemented entirely by the language
* runtime / Itanium-or-MS C++ ABI (__cxa_* machinery). There is no
* portable way to reimplement it, so restd re-exports the std symbols
* on C++11+ and ships nothing on C++98 — exactly the posture <new>
* takes toward operator new (runtime-provided, not reimplementable).
*
*
* path:      /inc/djinterp/re_std/exception/exception_ptr.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_EXCEPTION_EXCEPTION_PTR_
#define RESTD_EXCEPTION_EXCEPTION_PTR_ 1

#include "../djinterp.hpp"

#if ( D_ENV_LANG_IS_CPP11_OR_HIGHER && \
      D_ENV_CPP98_HAS_EXCEPTION )

    #include <exception>

namespace restd
{
    // exception_ptr
    //   type: using-declaration from std::exception_ptr. Opaque,
    //   copyable, null-comparable handle to a captured exception.
    using std::exception_ptr;

    // current_exception
    //   function: using-declaration from std::current_exception. Returns
    //   an exception_ptr to the exception being handled, or null.
    using std::current_exception;

    // rethrow_exception
    //   function: using-declaration from std::rethrow_exception.
    //   [[noreturn]]; re-raises the exception referenced by the handle.
    using std::rethrow_exception;

} // namespace restd

#endif // C++11+ && <exception>
// C++98 (or freestanding): exception_ptr is ABI-provided and has no
// portable reimplementation; the facility is intentionally absent.

#endif // RESTD_EXCEPTION_EXCEPTION_PTR_
