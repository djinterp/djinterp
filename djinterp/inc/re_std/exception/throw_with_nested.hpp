/*******************************************************************************
* djinterp [re_std]                                        throw_with_nested.hpp
*
* throw_with_nested:
*   throws an object that, when the argument type allows, derives from
* both the argument's decayed type and nested_exception (capturing the
* in-flight exception); otherwise throws the decayed argument as-is.
* [[noreturn]]. The selection logic relies on the same class/final/
* base-of trait analysis std performs internally, so re_std re-exports
* std::throw_with_nested on C++11+ rather than reimplementing it (which
* would pull in type-traits not yet in re_std). No C++98 path — it is
* built on nested_exception / current_exception.
*
*
* path:      /inc/re_std/exception/throw_with_nested.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXCEPTION_THROW_WITH_NESTED_HPP
#define RE_STD_EXCEPTION_THROW_WITH_NESTED_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "nested_exception.hpp"

#if ( RE_STD_LANG_IS_CPP11_OR_HIGHER && \
      RE_STD_HAS_EXCEPTIONS )

    // std
    #include <exception>

namespace re_std
{
    // throw_with_nested
    //   function: using-declaration from std::throw_with_nested.
    using std::throw_with_nested;

}  // re_std
#endif // C++11+ && <exception>

#endif  // RE_STD_EXCEPTION_THROW_WITH_NESTED_HPP
