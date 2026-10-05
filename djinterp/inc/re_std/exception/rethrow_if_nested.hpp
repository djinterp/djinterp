/*******************************************************************************
* djinterp [re_std]                                        rethrow_if_nested.hpp
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
* path:      /inc/re_std/exception/rethrow_if_nested.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXCEPTION_RETHROW_IF_NESTED_HPP
#define RE_STD_EXCEPTION_RETHROW_IF_NESTED_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "nested_exception.hpp"

#if ( RE_STD_LANG_IS_CPP11_OR_HIGHER && \
      RE_STD_HAS_EXCEPTIONS )

    // std
    #include <exception>

namespace re_std
{
    // rethrow_if_nested
    //   function: using-declaration from std::rethrow_if_nested.
    using std::rethrow_if_nested;

}  // re_std
#endif // C++11+ && <exception>

#endif  // RE_STD_EXCEPTION_RETHROW_IF_NESTED_HPP
