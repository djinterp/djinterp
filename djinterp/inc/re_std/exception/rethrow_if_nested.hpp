/***********************************************************************
* re_std                                            rethrow_if_nested.hpp
*
* rethrow_if_nested:
*   if the argument's dynamic type has an accessible, unambiguous
* nested_exception base subobject, rethrows the exception it captured;
* otherwise does nothing. Uses a polymorphic dynamic_cast internally
* (RTTI), matching std. re_std re-exports std::rethrow_if_nested on
* C++11+; reimplementing it would require re_std::is_polymorphic /
* is_base_of / dynamic_cast plumbing not yet in re_std's type_traits.
* No C++98 path — depends on the nested_exception facility.
*
*
* path:      /inc/djinterp/re_std/exception/rethrow_if_nested.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_EXCEPTION_RETHROW_IF_NESTED_
#define DJINTERP_RE_STD_EXCEPTION_RETHROW_IF_NESTED_ 1

#include "../../core/djinterp.hpp"
#include "nested_exception.hpp"

#if ( D_ENV_LANG_IS_CPP11_OR_HIGHER && \
      D_ENV_CPP98_HAS_EXCEPTION )

    #include <exception>

namespace re_std
{
    // rethrow_if_nested
    //   function: using-declaration from std::rethrow_if_nested.
    using std::rethrow_if_nested;

} // namespace re_std

#endif // C++11+ && <exception>

#endif  // DJINTERP_RE_STD_EXCEPTION_RETHROW_IF_NESTED_
