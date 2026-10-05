/*******************************************************************************
* djinterp [re_std]                                               array_swap.hpp
*
* array swap specialization header:
*   Provides a non-member swap overload for re_std::array. ADL-friendly;
* delegates to the array::swap member function (element-wise swap).
*
*   CONSTRAINT:
*   std::swap-for-array is constrained on is_swappable_v<Type> from
* C++17. re_std omits the constraint — Type's swappability is
* enforced naturally at instantiation of the member swap (which
* uses copy-assign of Type, requiring CopyAssignable). This is a
* slight relaxation vs std but avoids dragging in is_swappable
* infrastructure for a corner case rarely exercised in user code.
*
*   CONSTEXPR:
*   constexpr from C++20 (P1023, applied through to the member swap).
* Pre-C++20 the qualifier degrades to empty via RE_STD_CONSTEXPR_CPP20.
*
*
* path:      /inc/re_std/array/array_swap.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ARRAY_ARRAY_SWAP_HPP
#define RE_STD_ARRAY_ARRAY_SWAP_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
// re_std
#include "./array.hpp"


namespace re_std
{


// ===========================================================================
// I.   swap (array specialization)
// ===========================================================================

// swap
//   function: exchanges the contents of two array<Type, Size>
// objects. Delegates to the array::swap member function (element-wise
// swap).
template<typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP20 void
swap(
    array<Type, Size>& _lhs,
    array<Type, Size>& _rhs
)
{
    _lhs.swap(_rhs);

    return;
}


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_ARRAY_ARRAY_SWAP_HPP
