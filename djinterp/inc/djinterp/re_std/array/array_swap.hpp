/******************************************************************************
* djinterp [restd]                                               array_swap.hpp
*
* array swap specialization header:
*   Provides a non-member swap overload for restd::array. ADL-friendly;
* delegates to the array::swap member function (element-wise swap).
*
*   CONSTRAINT:
*   std::swap-for-array is constrained on is_swappable_v<_Type> from
* C++17. restd omits the constraint — _Type's swappability is
* enforced naturally at instantiation of the member swap (which
* uses copy-assign of _Type, requiring CopyAssignable). This is a
* slight relaxation vs std but avoids dragging in is_swappable
* infrastructure for a corner case rarely exercised in user code.
*
*   CONSTEXPR:
*   constexpr from C++20 (P1023, applied through to the member swap).
* Pre-C++20 the qualifier degrades to empty via D_CONSTEXPR_CPP20.
*
*
* path:      /inc/djinterp/re_std/array/array_swap.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RESTD_ARRAY_SWAP_
#define DJINTERP_RESTD_ARRAY_SWAP_ 1

#include <cstddef>

#include "../../core/djinterp.hpp"
#include "./array.hpp"


#ifndef D_CONSTEXPR_CPP20
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_CONSTEXPR_CPP20   constexpr
    #else
        #define D_CONSTEXPR_CPP20
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   swap (array specialization)
// ===========================================================================

// swap
//   function: exchanges the contents of two array<_Type, _Size>
// objects. Delegates to the array::swap member function (element-wise
// swap).
template<typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP20 void
swap(
    array<_Type, _Size>& _lhs,
    array<_Type, _Size>& _rhs
)
{
    _lhs.swap(_rhs);

    return;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ARRAY_SWAP_
