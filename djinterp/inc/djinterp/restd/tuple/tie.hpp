/******************************************************************************
* djinterp [restd]                                                       tie.hpp
*
* tie factory header:
*   Creates a tuple of lvalue references to its arguments. Used
* primarily to destructure a tuple into existing variables:
*
*     int a, b;
*     tie(a, b) = make_tuple(42, 17);
*     // now a == 42, b == 17
*
*   tie can also collaborate with the restd::ignore object to skip
* elements:
*
*     int a, c;
*     tie(a, ignore, c) = some_3_tuple;
*     // middle element discarded
*
*   PORTABILITY:
*   Requires variadic templates and rvalue references (C++11+).
*
*
* path:      /inc/djinterp/restd/tuple/tie.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TUPLE_TIE_
#define DJINTERP_RESTD_TUPLE_TIE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// djinterp
#include "./tuple.hpp"


NS_RESTD


// =============================================================================
// I.   TIE
// =============================================================================

// tie
//   function: creates a tuple<_Types&...> binding lvalue references
// to its arguments. Constexpr on C++14+.
template<typename... _Types>
D_CONSTEXPR
tuple<_Types&...>
tie(
    _Types&... _args
) D_NOEXCEPT
{
    return tuple<_Types&...>(_args...);
}


NS_END  // restd


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RESTD_TUPLE_TIE_
