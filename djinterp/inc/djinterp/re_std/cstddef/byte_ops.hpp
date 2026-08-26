/******************************************************************************
* djinterp [re_std]                                                 byte_ops.hpp
*
* the byte bit operations:
*   The eleven operators [cstddef.syn] gives byte -- <<, >>, |, &, ^, ~ and
* their five compound-assignment forms. There is deliberately no +, -, *,
* /, no comparison beyond the implicit enum ones, and no stream inserter:
* byte is storage, not a number.
*
*   THE PROMOTION TRAP IS THE WHOLE IMPLEMENTATION:
*   `static_cast<unsigned char>(b) << shift` does NOT produce an
* 8-bit result. The operand promotes to int first, so bits shifted past
* position 7 survive in the promoted value, and casting the result
* straight back to byte would be an out-of-range enum conversion. Every
* expression below therefore lands in unsigned char explicitly before it
* becomes a byte again. This is the same trap recorded as hard-won rule 6
* (rotl(uint8_t(0x80), 1) yielding 0x101), reached from the other
* direction.
*
*   WHY THE COMPOUND FORMS ARE D_CONSTEXPR_CPP14:
*   They mutate their reference parameter, and mutation inside a constexpr
* function needs the relaxed C++14 rules. On C++11 they remain ordinary
* inline functions -- usable, just not in a constant expression -- rather
* than being omitted. The explicit `inline` matters on that tier: with
* D_CONSTEXPR_CPP14 expanding to nothing, constexpr is no longer there to
* imply it, and a header-defined non-template function without it is an
* ODR violation the moment two translation units include this file.
*
*   C++11 FLOOR: follows byte.
*
*
* path:      /inc/djinterp/re_std/cstddef/byte_ops.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDDEF_BYTE_OPS_
#define DJINTERP_RE_STD_CSTDDEF_BYTE_OPS_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./byte.hpp"
#include "./to_integer.hpp"
#include "../type_traits/enable_if.hpp"
#include "../type_traits/is_integral.hpp"


NS_RESTD


// ===========================================================================
// I.   SHIFTS
// ===========================================================================

    // operator<<
    //   function: b shifted left by _shift bits, truncated to 8 bits.
    template<typename _IntType>
    D_CONSTEXPR
    typename enable_if<is_integral<_IntType>::value, byte>::type
    operator<<(byte _b, _IntType _shift) D_NOEXCEPT
    {
        return static_cast<byte>(static_cast<unsigned char>(
            static_cast<unsigned int>(_b) << _shift));
    }

    // operator>>
    //   function: b shifted right by _shift bits. The cast to unsigned int
    // keeps the shift logical -- byte's underlying type is unsigned, so no
    // sign bit can be replicated in.
    template<typename _IntType>
    D_CONSTEXPR
    typename enable_if<is_integral<_IntType>::value, byte>::type
    operator>>(byte _b, _IntType _shift) D_NOEXCEPT
    {
        return static_cast<byte>(static_cast<unsigned char>(
            static_cast<unsigned int>(_b) >> _shift));
    }

    // operator<<=
    //   function: shift-left in place. Returns the modified byte.
    template<typename _IntType>
    D_CONSTEXPR_CPP14 inline
    typename enable_if<is_integral<_IntType>::value, byte&>::type
    operator<<=(byte& _b, _IntType _shift) D_NOEXCEPT
    {
        return _b = _b << _shift;
    }

    // operator>>=
    //   function: shift-right in place. Returns the modified byte.
    template<typename _IntType>
    D_CONSTEXPR_CPP14 inline
    typename enable_if<is_integral<_IntType>::value, byte&>::type
    operator>>=(byte& _b, _IntType _shift) D_NOEXCEPT
    {
        return _b = _b >> _shift;
    }


// ===========================================================================
// II.  BITWISE
// ===========================================================================

    // operator|
    //   function: bitwise or.
    D_CONSTEXPR_INLINE byte operator|(byte _l, byte _r) D_NOEXCEPT
    {
        return static_cast<byte>(static_cast<unsigned char>(
            static_cast<unsigned int>(_l) | static_cast<unsigned int>(_r)));
    }

    // operator&
    //   function: bitwise and.
    D_CONSTEXPR_INLINE byte operator&(byte _l, byte _r) D_NOEXCEPT
    {
        return static_cast<byte>(static_cast<unsigned char>(
            static_cast<unsigned int>(_l) & static_cast<unsigned int>(_r)));
    }

    // operator^
    //   function: bitwise exclusive or.
    D_CONSTEXPR_INLINE byte operator^(byte _l, byte _r) D_NOEXCEPT
    {
        return static_cast<byte>(static_cast<unsigned char>(
            static_cast<unsigned int>(_l) ^ static_cast<unsigned int>(_r)));
    }

    // operator~
    //   function: bitwise complement. The mask to unsigned char is what
    // discards the promoted high bits ~ produced above bit 7.
    D_CONSTEXPR_INLINE byte operator~(byte _b) D_NOEXCEPT
    {
        return static_cast<byte>(static_cast<unsigned char>(
            ~static_cast<unsigned int>(_b)));
    }

    // operator|=
    //   function: bitwise or in place.
    D_CONSTEXPR_CPP14 inline byte& operator|=(byte& _l, byte _r) D_NOEXCEPT
    {
        return _l = _l | _r;
    }

    // operator&=
    //   function: bitwise and in place.
    D_CONSTEXPR_CPP14 inline byte& operator&=(byte& _l, byte _r) D_NOEXCEPT
    {
        return _l = _l & _r;
    }

    // operator^=
    //   function: bitwise exclusive or in place.
    D_CONSTEXPR_CPP14 inline byte& operator^=(byte& _l, byte _r) D_NOEXCEPT
    {
        return _l = _l ^ _r;
    }


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDDEF_BYTE_OPS_
