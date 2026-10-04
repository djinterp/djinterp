/******************************************************************************
* djinterp [re_std]                                                     rotr.hpp
*
* rotr header:
*   Rotates the bits of the value right by _s positions. Bits shifted
* off one end reappear at the other.
*
*   THE SHIFT COUNT IS AN int, AND MAY BE ANYTHING:
*   It is not reduced by the caller and it is not required to be
* positive. The standard defines the whole range:
*
*     r == 0   ->  the value unchanged
*     r  > 0   ->  rotate right by r
*     r  < 0   ->  rotl(v, -r)
*
*   where r is s % N. Reducing first is not an optimisation -- a
* raw shift by N or more is undefined behaviour, so a count of
* exactly N would be UB without it, and the r == 0 case must then be
* split out because it would otherwise shift by N in the other
* direction.
*
*   EVERY SUB-EXPRESSION IS CAST BACK TO _T:
*   For a type narrower than int, both operands promote before the
* shift, so the bits rotated off the top survive in the promoted
* value instead of wrapping. Casting each half back to _T truncates
* them away before the OR. Without those casts rotl(uint8_t(0x80), 1)
* gives 0x101, not 0x01.
*
*   PORTABILITY:
*   C++20 in std, back-ported to C++11 and constexpr from C++11.
*
*
* path:      /inc/djinterp/re_std/bit/rotr.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_ROTR_
#define DJINTERP_RE_STD_BIT_ROTR_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"


NS_RESTD


// ===========================================================================
// I.   ROTR
// ===========================================================================

NS_INTERNAL

    // rotr_reduced
    //   helper: the rotation proper, on an ALREADY-REDUCED count. Split
    // out because a C++11 constexpr function may contain only a single
    // return statement -- binding N to a local would push this to C++14
    // for no benefit.
    template<typename _T>
    D_CONSTEXPR _T
    rotr_reduced(
        _T  _v,
        int _r,
        int _N
    )
    {
        return (_r == 0)
            ? _v
            : ( (_r > 0)
                ? static_cast<_T>( static_cast<_T>(_v >> _r)
                                 | static_cast<_T>(_v << (_N - _r)) )
                : static_cast<_T>( static_cast<_T>(_v << -_r)
                                 | static_cast<_T>(_v >> (_N + _r)) ) );
    }

NS_END  // internal

// rotr
//   function: bitwise rotation by _s. The count is reduced modulo N
// first -- a raw shift by N or more is undefined behaviour, so this is
// correctness, not economy.
template<typename _T>
D_CONSTEXPR typename internal::bit_enable<_T>::type
rotr(
    _T  _v,
    int _s
) D_NOEXCEPT
{
    return internal::rotr_reduced<_T>(
        _v,
        _s % internal::bit_digits<_T>::value,
        internal::bit_digits<_T>::value);
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_ROTR_
