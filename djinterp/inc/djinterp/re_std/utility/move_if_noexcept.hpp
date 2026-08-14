/******************************************************************************
* re_std [utility]                                        move_if_noexcept.hpp
*
*   conditional move:
*   `move_if_noexcept(x)` returns an rvalue reference to x when moving it
* cannot throw, and a CONST LVALUE reference otherwise - so a container
* reallocating its buffer moves when that is safe and falls back to copying
* when a throwing move would leave it with a half-migrated, unrecoverable
* state.  This is the strong-exception-guarantee lever behind vector growth.
*
*   THE CONDITION IS DELIBERATELY ASYMMETRIC.
*   It yields T&& when the move is non-throwing OR when the type is not
* copyable at all.  The second half matters: a move-only type with a throwing
* move has no copy to fall back to, so refusing to move it would simply not
* compile.  std makes the same choice - correctness of the guarantee yields to
* the fact that no alternative exists.
*
*   STD IS C++11; re_std IS C++11.
*   Rvalue references are the whole mechanism, so this is a hard language
* ceiling - there is no meaningful C++98 form.  constexpr from C++11.
*
*
* path:      /inc/djinterp/re_std/utility/move_if_noexcept.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_UTILITY_MOVE_IF_NOEXCEPT_
#define RESTD_UTILITY_MOVE_IF_NOEXCEPT_ 1

// re_std
#include "../type_traits/type_traits.hpp"   // is_nothrow_move_constructible,
                                            // is_copy_constructible, conditional

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

NS_DJINTERP
NS_RESTD

// move_if_noexcept
//   function: cast to T&& when moving is non-throwing (or no copy exists),
// otherwise to const T&.
template<typename _Type>
D_NODISCARD D_CONSTEXPR
typename conditional<
        (   !is_nothrow_move_constructible<_Type>::value
         &&  is_copy_constructible<_Type>::value),
        const _Type&,
        _Type&&>::type
move_if_noexcept(_Type& value) D_NOEXCEPT
{
    return static_cast<
        typename conditional<
            (   !is_nothrow_move_constructible<_Type>::value
             &&  is_copy_constructible<_Type>::value),
            const _Type&,
            _Type&&>::type>(value);
}

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // RESTD_UTILITY_MOVE_IF_NOEXCEPT_
