/*******************************************************************************
* djinterp [re_std]                                                 any_swap.hpp
*
* any swap specialization header:
*   Provides a non-member swap overload for re_std::any. This is the
* any-specific ADL swap; the master swap module (swap.hpp) is a separate,
* independent header.
*   The swap is implemented by delegating to the any::swap member function,
* which handles both SBO and heap storage paths.
*
*
* path:      /inc/re_std/any/any_swap.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.10
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ANY_ANY_SWAP_HPP
#define RE_STD_ANY_ANY_SWAP_HPP 1

// any.hpp exists wherever `long long` does (decision 4.6), so this header
// does too: under ISO strict C++98 it is empty, as any.hpp is.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_HAS_LONG_LONG

// re_std
#include "./any.hpp"


namespace re_std
{


// ===========================================================================
// I.   swap (any specialization)
// ===========================================================================

// swap
//   function: exchanges the contents of two any objects. Delegates to
// the any::swap member function. Not constexpr, as std's is not: any
// manages its storage at run time.
RE_STD_INLINE void
swap(
    any& _lhs,
    any& _rhs
)
RE_STD_NOEXCEPT
{
    _lhs.swap(_rhs);

    return;
}


}  // re_std

#endif  // RE_STD_HAS_LONG_LONG


#endif  // RE_STD_ANY_ANY_SWAP_HPP
