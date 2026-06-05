/***********************************************************************
* restd                                             rethrow_if_nested.hpp
*
* rethrow_if_nested:
*   if the argument's dynamic type has an accessible, unambiguous
* nested_exception base subobject, rethrows the exception it captured;
* otherwise does nothing. Uses a polymorphic dynamic_cast internally
* (RTTI), matching std. restd re-exports std::rethrow_if_nested on
* C++11+; reimplementing it would require restd::is_polymorphic /
* is_base_of / dynamic_cast plumbing not yet in restd's type_traits.
* No C++98 path — depends on the nested_exception facility.
*
*
* path:      /inc/djinterp/restd/exception/rethrow_if_nested.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.06.04
***********************************************************************/

#ifndef RESTD_EXCEPTION_RETHROW_IF_NESTED_
#define RESTD_EXCEPTION_RETHROW_IF_NESTED_ 1

#include "../djinterp.hpp"
#include "nested_exception.hpp"

#if ( D_ENV_LANG_IS_CPP11_OR_HIGHER && \
      D_ENV_CPP98_HAS_EXCEPTION )

    #include <exception>

namespace restd
{
    // rethrow_if_nested
    //   function: using-declaration from std::rethrow_if_nested.
    using std::rethrow_if_nested;

} // namespace restd

#endif // C++11+ && <exception>

#endif // RESTD_EXCEPTION_RETHROW_IF_NESTED_
