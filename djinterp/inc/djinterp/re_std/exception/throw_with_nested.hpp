/***********************************************************************
* restd                                             throw_with_nested.hpp
*
* throw_with_nested:
*   throws an object that, when the argument type allows, derives from
* both the argument's decayed type and nested_exception (capturing the
* in-flight exception); otherwise throws the decayed argument as-is.
* [[noreturn]]. The selection logic relies on the same class/final/
* base-of trait analysis std performs internally, so restd re-exports
* std::throw_with_nested on C++11+ rather than reimplementing it (which
* would pull in type-traits not yet in restd). No C++98 path — it is
* built on nested_exception / current_exception.
*
*
* path:      /inc/djinterp/re_std/exception/throw_with_nested.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_EXCEPTION_THROW_WITH_NESTED_
#define RESTD_EXCEPTION_THROW_WITH_NESTED_ 1

#include "../djinterp.hpp"
#include "nested_exception.hpp"

#if ( D_ENV_LANG_IS_CPP11_OR_HIGHER && \
      D_ENV_CPP98_HAS_EXCEPTION )

    #include <exception>

namespace restd
{
    // throw_with_nested
    //   function: using-declaration from std::throw_with_nested.
    using std::throw_with_nested;

} // namespace restd

#endif // C++11+ && <exception>

#endif // RESTD_EXCEPTION_THROW_WITH_NESTED_
