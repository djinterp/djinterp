/******************************************************************************
* djinterp [re_std]                                              any_swap.hpp
*
* any swap specialization header:
*   Provides a non-member swap overload for re_std::any. This is the
* any-specific ADL swap; the master swap module (swap.hpp) is a separate,
* independent header.
*   The swap is implemented by delegating to the any::swap member function,
* which handles both SBO and heap storage paths.
*
*
* path:      /inc/djinterp/re_std/any/any_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.10
******************************************************************************/

#ifndef DJINTERP_RE_STD_ANY_SWAP_
#define DJINTERP_RE_STD_ANY_SWAP_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./any.hpp"


NS_RESTD


// ===========================================================================
// I.   swap (any specialization)
// ===========================================================================

// swap
//   function: exchanges the contents of two any objects. Delegates to
// the any::swap member function.
D_CONSTEXPR_INLINE void
swap(
    any& _lhs,
    any& _rhs
)
D_NOEXCEPT
{
    _lhs.swap(_rhs);

    return;
}


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ANY_SWAP_
