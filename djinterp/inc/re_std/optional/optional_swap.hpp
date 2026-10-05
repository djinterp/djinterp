/*******************************************************************************
* djinterp [re_std]                                            optional_swap.hpp
*
* optional_swap swap specialization header:
*   non-member swap for optional<T>.
*
*   Constrained on move_constructible AND swappable, matching std.  Both are
* needed and for different reasons: the engaged/engaged case swaps the
* contained values (swappable), while the mixed case moves one across and
* destroys the other (move_constructible).  A type satisfying only one of the
* two would compile here and then fail inside the body on the other path.
*
*
* path:      /inc/re_std/optional/optional_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_OPTIONAL_OPTIONAL_SWAP_HPP
#define RE_STD_OPTIONAL_OPTIONAL_SWAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./optional.hpp"

namespace re_std
{

// swap
//   function: exchange the states of two optionals.
template<typename Type>
typename enable_if<   is_move_constructible<Type>::value
                   && is_swappable<Type>::value, void>::type
swap(optional<Type>& a, optional<Type>& b)
    RE_STD_NOEXCEPT_IF(noexcept(a.swap(b)))
{
    a.swap(b);
    return;
}

}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_OPTIONAL_OPTIONAL_SWAP_HPP
