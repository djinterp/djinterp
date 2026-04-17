/******************************************************************************
* djinterp [restd]                                                swap_any.hpp
*
* any swap specialization header:
*   Provides a non-member swap overload for djinterp::stl::any. This is the
* any-specific ADL swap; the master swap module (swap.hpp) is a separate, 
* independent header.
*   The swap is implemented by delegating to the any::swap member function,
* which handles both SBO and heap storage paths.
*

* path:      /inc/djinterp/restd/any/swap_any.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.10
******************************************************************************/

#ifndef DJINTERP_RESTD_ANY_SWAP_
#define DJINTERP_RESTD_ANY_SWAP_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./any.hpp"


NS_DJINTERP
NS_RESTD


// =============================================================================
// I.   std::swap (any specialization)
// =============================================================================

// swap
//   function: exchanges the contents of two any objects. Delegates to
// the any::swap member function.
D_CONSTEXPR_INLINE void
swap
(
    any& _lhs,
    any& _rhs
)
noexcept
{
    _lhs.swap(_rhs);

    return;
}


NS_END  // restd
NS_END  // djinterp


#endif  // DJINTERP_RESTD_ANY_SWAP_