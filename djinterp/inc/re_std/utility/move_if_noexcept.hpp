/*******************************************************************************
* djinterp [re_std]                                         move_if_noexcept.hpp
*
* move_if_noexcept function header:
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
* path:      /inc/re_std/utility/move_if_noexcept.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_UTILITY_MOVE_IF_NOEXCEPT_HPP
#define RE_STD_UTILITY_MOVE_IF_NOEXCEPT_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"   // is_nothrow_move_constructible,
                                            // is_copy_constructible, conditional

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

namespace re_std
{

// move_if_noexcept
//   function: cast to T&& when moving is non-throwing (or no copy exists),
// otherwise to const T&.
template<typename Type>
RE_STD_NODISCARD RE_STD_CONSTEXPR
typename conditional<
        (   !is_nothrow_move_constructible<Type>::value
         &&  is_copy_constructible<Type>::value),
        const Type&,
        Type&&>::type
move_if_noexcept(Type& value) RE_STD_NOEXCEPT
{
    return static_cast<
        typename conditional<
            (   !is_nothrow_move_constructible<Type>::value
             &&  is_copy_constructible<Type>::value),
            const Type&,
            Type&&>::type>(value);
}

}  // re_std
#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // floor, for now


#endif  // RE_STD_UTILITY_MOVE_IF_NOEXCEPT_HPP
