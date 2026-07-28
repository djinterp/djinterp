/***********************************************************************
* restd                                              move_if_noexcept.hpp
*
* move-or-copy cast utility:
*   Returns either an rvalue reference (allowing move) or a const
* lvalue reference (forcing copy) depending on whether _Type's move
* constructor is noexcept and whether _Type is copy-constructible.
*
*   The rule: if T's move ctor can throw AND T has a copy ctor, return
* const T& -- forcing a copy that preserves the strong exception
* guarantee. Otherwise return T&&. This is the cast vector<T>::push_back
* uses to decide whether to move elements during reallocation.
*
*   STANDARD STATUS:
*   Introduced in C++11. Requires rvalue references and the
* is_nothrow_move_constructible / is_copy_constructible traits.
*
*
* path:      /inc/restd/utility/move_if_noexcept.hpp
* link(s):   TBA
* author(s): restd team                                  date: 2026.05.02
***********************************************************************/

#ifndef RESTD_UTILITY_MOVE_IF_NOEXCEPT_
#define RESTD_UTILITY_MOVE_IF_NOEXCEPT_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES \
    && D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

#include "../type_traits/conditional.hpp"
#include "../type_traits/is_nothrow_move_constructible.hpp"
#include "../type_traits/is_copy_constructible.hpp"

NS_RESTD

// =============================================================================
// MOVE_IF_NOEXCEPT
// =============================================================================

// move_if_noexcept
//   function: casts to rvalue ref if T's move is nothrow OR T has no
//   copy ctor; otherwise casts to const lvalue ref. Single-statement
//   body so constexpr-eligible from C++11.
template<typename _Type>
D_CONSTEXPR
typename conditional<
    !is_nothrow_move_constructible<_Type>::value
        && is_copy_constructible<_Type>::value,
    const _Type&,
    _Type&&
>::type
move_if_noexcept(_Type& _value) noexcept
{
    return static_cast<typename conditional<
        !is_nothrow_move_constructible<_Type>::value
            && is_copy_constructible<_Type>::value,
        const _Type&,
        _Type&&
    >::type>(_value);
}

NS_END  // restd

#endif  // rvalue refs && variadic templates

#endif  // RESTD_UTILITY_MOVE_IF_NOEXCEPT_
