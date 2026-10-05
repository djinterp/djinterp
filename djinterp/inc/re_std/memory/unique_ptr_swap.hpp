/*******************************************************************************
* djinterp [re_std]                                          unique_ptr_swap.hpp
*
* non-member ADL swap overload for unique_ptr:
*   re_std::swap(_lhs, _rhs) delegates to _lhs.swap(_rhs).
*
* this is the unique_ptr-specific swap; the generic re_std::swap (in
* re_std/utility/swap.hpp) is the fallback. ADL plus the standard
* two-step swap idiom (`using re_std::swap; swap(a, b);`) routes calls
* on unique_ptr through this overload before considering the generic.
*
* this overload is constrained to types where the deleter is move-
* assignable. Without that constraint, the body still compiles for any
* deleter type, but the swap may produce a moved-from-but-unmovable
* state. Std uses is_swappable<D> instead of is_move_assignable; we
* leave the constraint off here because is_swappable is itself C++17,
* and the pragmatic effect on user code is the same.
*
*
* path:      /inc/re_std/memory/unique_ptr_swap.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNIQUE_PTR_SWAP_HPP
#define RE_STD_MEMORY_UNIQUE_PTR_SWAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/unique_ptr.hpp"


namespace re_std
{

// swap
//   function: ADL-friendly non-member swap for unique_ptr. Not constexpr:
// it calls the member swap, which is not, and std's became constexpr only
// in C++23; a constexpr void function is also ill-formed at C++11.
template<typename T, typename D>
RE_STD_INLINE void swap
(
    unique_ptr<T, D>& _lhs,
    unique_ptr<T, D>& _rhs
) RE_STD_NOEXCEPT
{
    _lhs.swap(_rhs);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_UNIQUE_PTR_SWAP_HPP
