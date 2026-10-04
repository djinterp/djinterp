/******************************************************************************
* djinterp [re_std]                                                 byteswap.hpp
*
* byteswap header:
*   Reverses the order of the bytes in the object representation.
*
*     byteswap(uint32_t(0x11223344)) -> 0x44332211
*     byteswap(uint8_t(0x12))        -> 0x12   (nothing to reverse)
*
*   WIDER CONSTRAINT THAN THE REST OF THIS MODULE:
*   byteswap accepts any INTEGRAL type, not just the unsigned integer
* types -- signed types and the character types included. It is the one
* function here that does not use bit_enable.
*
*   The work is done on the corresponding unsigned type and cast back.
* Shifting a signed value would be implementation-defined for negatives
* before C++20, and the round trip through make_unsigned is exact.
*
*   A byte is CHAR_BIT bits, taken from numeric_limits rather than
* assumed to be 8, so the loop is correct on a platform where it is not.
*
*   PORTABILITY:
*   std added byteswap in C++23; re_std back-ports it to C++11 and is
* constexpr from C++11, where std is constexpr from C++23.
*
*
* path:      /inc/djinterp/re_std/bit/byteswap.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_BYTESWAP_
#define DJINTERP_RE_STD_BIT_BYTESWAP_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./bit_internal.hpp"
#include "../type_traits/is_integral.hpp"
#include "../type_traits/make_unsigned.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/remove_cv.hpp"


NS_RESTD


// ===========================================================================
// I.   INTERNAL
// ===========================================================================

NS_INTERNAL

    // bit_byte_bits / bit_byte_mask
    //   constants: a byte is CHAR_BIT bits, taken from numeric_limits
    // rather than assumed to be 8, so the walk is correct on a platform
    // where it is not.
    const int bit_byte_bits = numeric_limits<unsigned char>::digits;

    template<typename _U>
    struct bit_byte_mask
    {
        static const _U value =
            static_cast<_U>( (static_cast<_U>(1) << bit_byte_bits)
                             - static_cast<_U>(1) );
    };

    // byteswap_rec
    //   helper: pulls the low byte off _v and pushes it onto _acc, so the
    // first byte out becomes the most significant byte in. Recursive
    // rather than looped, to stay constexpr at C++11.
    template<typename _U>
    D_CONSTEXPR _U
    byteswap_rec(
        _U  _v,
        _U  _acc,
        int _n
    )
    {
        return (_n == 0)
            ? _acc
            : byteswap_rec<_U>(
                  static_cast<_U>(_v >> bit_byte_bits),
                  static_cast<_U>( static_cast<_U>(_acc << bit_byte_bits)
                                 | static_cast<_U>(_v & bit_byte_mask<_U>::value) ),
                  _n - 1);
    }

NS_END  // internal


// ===========================================================================
// II.  BYTESWAP
// ===========================================================================

// byteswap
//   function: reverses the bytes of an integral value. Accepts signed
// and character types too, unlike the rest of this module.
template<typename _T>
D_CONSTEXPR typename enable_if<
    is_integral<typename remove_cv<_T>::type>::value, _T >::type
byteswap(
    _T _v
) D_NOEXCEPT
{
    typedef typename make_unsigned<typename remove_cv<_T>::type>::type _U;
    return static_cast<_T>(
        internal::byteswap_rec<_U>(static_cast<_U>(_v),
                                   static_cast<_U>(0),
                                   static_cast<int>(sizeof(_T))));
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_BYTESWAP_
