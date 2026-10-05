/*******************************************************************************
* djinterp [re_std]                                         forward_as_tuple.hpp
*
* forward_as_tuple factory header:
*   Constructs a tuple of forwarding references to its arguments.
* Suitable for forwarding heterogeneous arguments to a function that
* accepts a tuple, preserving value categories exactly:
*
*     forward_as_tuple(1, x, foo())
*       -> tuple<int&&, X&, Foo&&>
*
*   Note that the resulting tuple may contain dangling references if
* it outlives the temporaries bound to its rvalue elements; per the
* standard, forward_as_tuple is intended for immediate consumption.
*
*   PORTABILITY:
*   Requires variadic templates and rvalue references (C++11+).
*
*
* path:      /inc/re_std/tuple/forward_as_tuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_FORWARD_AS_TUPLE_HPP
#define RE_STD_TUPLE_FORWARD_AS_TUPLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// re_std
#include "./tuple.hpp"


namespace re_std
{


// =============================================================================
// I.   FORWARD_AS_TUPLE
// =============================================================================

// forward_as_tuple
//   function: yields tuple<Types&&...> bound to the forwarded
// arguments. The result captures lvalues as lvalue references and
// rvalues as rvalue references.
template<typename... Types>
RE_STD_CONSTEXPR
tuple<Types&&...>
forward_as_tuple(
    Types&&... _args
) RE_STD_NOEXCEPT
{
    return tuple<Types&&...>(static_cast<Types&&>(_args)...);
}


}  // re_std


#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_FORWARD_AS_TUPLE_HPP
