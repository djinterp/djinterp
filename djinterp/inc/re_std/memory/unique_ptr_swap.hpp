/***********************************************************************
* re_std                                                unique_ptr_swap.hpp
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
* state. Std uses is_swappable<_D> instead of is_move_assignable; we
* leave the constraint off here because is_swappable is itself C++17,
* and the pragmatic effect on user code is the same.
*
*
* path:      /inc/djinterp/re_std/memory/unique_ptr_swap.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_UNIQUE_PTR_SWAP_
#define DJINTERP_RE_STD_MEMORY_UNIQUE_PTR_SWAP_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/unique_ptr.hpp"


namespace re_std
{

// swap
//   function: ADL-friendly non-member swap for unique_ptr.
template<typename _T, typename _D>
D_CONSTEXPR_INLINE void swap
(
    unique_ptr<_T, _D>& _lhs,
    unique_ptr<_T, _D>& _rhs
) D_NOEXCEPT
{
    _lhs.swap(_rhs);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_UNIQUE_PTR_SWAP_
