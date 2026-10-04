/******************************************************************************
* djinterp [re_std]                                             tuple_swap.hpp
*
* tuple swap header:
*   ADL-friendly non-member swap overload for re_std::tuple. Delegates
* to tuple's swap member.
*
*     tuple<int, char> a(1, 'x'), b(2, 'y');
*     swap(a, b);   // ADL picks re_std::swap(tuple&, tuple&)
*
*   PORTABILITY:
*   Requires variadic templates and rvalue references (C++11+).
*
*
* path:      /inc/djinterp/re_std/tuple/tuple_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_TUPLE_TUPLE_SWAP_
#define DJINTERP_RE_STD_TUPLE_TUPLE_SWAP_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// djinterp
#include "./tuple.hpp"


NS_RESTD


// =============================================================================
// I.   SWAP (TUPLE)
// =============================================================================

// swap
//   function: ADL-friendly swap for re_std::tuple. Forwards to the
// member swap.
template<typename... _Types>
void
swap(
    tuple<_Types...>& _a,
    tuple<_Types...>& _b
)
    D_NOEXCEPT_IF(D_NOEXCEPT(_a.swap(_b)))
{
    _a.swap(_b);
    return;
}


NS_END  // re_std


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RE_STD_TUPLE_TUPLE_SWAP_
