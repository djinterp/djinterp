/*******************************************************************************
* djinterp [re_std]                                               tuple_swap.hpp
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
* path:      /inc/re_std/tuple/tuple_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_TUPLE_SWAP_HPP
#define RE_STD_TUPLE_TUPLE_SWAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// re_std
#include "./tuple.hpp"


namespace re_std
{


// =============================================================================
// I.   SWAP (TUPLE)
// =============================================================================

// swap
//   function: ADL-friendly swap for re_std::tuple. Forwards to the
// member swap.
template<typename... Types>
void
swap(
    tuple<Types...>& _a,
    tuple<Types...>& _b
)
    RE_STD_NOEXCEPT_IF(RE_STD_NOEXCEPT(_a.swap(_b)))
{
    _a.swap(_b);
    return;
}


}  // re_std


#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_TUPLE_SWAP_HPP
