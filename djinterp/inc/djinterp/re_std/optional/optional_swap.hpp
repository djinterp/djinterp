/******************************************************************************
* re_std [optional]                                          optional_swap.hpp
*
*   non-member swap for optional<T>.
*
*   Constrained on move_constructible AND swappable, matching std.  Both are
* needed and for different reasons: the engaged/engaged case swaps the
* contained values (swappable), while the mixed case moves one across and
* destroys the other (move_constructible).  A type satisfying only one of the
* two would compile here and then fail inside the body on the other path.
*
* path:      /inc/djinterp/re_std/optional/optional_swap.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_OPTIONAL_SWAP_
#define DJINTERP_RE_STD_OPTIONAL_SWAP_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./optional.hpp"

NS_RESTD

// swap
//   function: exchange the states of two optionals.
template<typename _Type>
typename enable_if<   is_move_constructible<_Type>::value
                   && is_swappable<_Type>::value, void>::type
swap(optional<_Type>& a, optional<_Type>& b)
    D_NOEXCEPT_IF(noexcept(a.swap(b)))
{
    a.swap(b);
    return;
}

NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_OPTIONAL_SWAP_
