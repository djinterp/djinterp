/******************************************************************************
* djinterp [restd]                                          forward_as_tuple.hpp
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
* path:      /inc/djinterp/re_std/tuple/forward_as_tuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TUPLE_FORWARD_AS_TUPLE_
#define DJINTERP_RESTD_TUPLE_FORWARD_AS_TUPLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// djinterp
#include "./tuple.hpp"


NS_RESTD


// =============================================================================
// I.   FORWARD_AS_TUPLE
// =============================================================================

// forward_as_tuple
//   function: yields tuple<_Types&&...> bound to the forwarded
// arguments. The result captures lvalues as lvalue references and
// rvalues as rvalue references.
template<typename... _Types>
D_CONSTEXPR
tuple<_Types&&...>
forward_as_tuple(
    _Types&&... _args
) D_NOEXCEPT
{
    return tuple<_Types&&...>(static_cast<_Types&&>(_args)...);
}


NS_END  // restd


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RESTD_TUPLE_FORWARD_AS_TUPLE_
