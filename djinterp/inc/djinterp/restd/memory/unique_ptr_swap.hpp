/***********************************************************************
* restd                                                 unique_ptr_swap.hpp
*
* non-member ADL swap overload for unique_ptr:
*   restd::swap(_lhs, _rhs) delegates to _lhs.swap(_rhs).
*
* this is the unique_ptr-specific swap; the generic restd::swap (in
* restd/utility/swap.hpp) is the fallback. ADL plus the standard
* two-step swap idiom (`using restd::swap; swap(a, b);`) routes calls
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
* path:      /inc/restd/memory/unique_ptr_swap.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_UNIQUE_PTR_SWAP_
#define RESTD_MEMORY_UNIQUE_PTR_SWAP_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/memory/unique_ptr.hpp"


namespace restd
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


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_UNIQUE_PTR_SWAP_
