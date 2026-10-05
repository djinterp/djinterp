/*******************************************************************************
* djinterp [re_std]                                             bit_internal.hpp
*
* bit module internals header:
*   The constraint every <bit> function shares, plus the recursive
* helpers the portable fallbacks are built from. No public symbols.
*
*   THE CONSTRAINT IS NARROWER THAN "UNSIGNED":
*   [bit.terms] restricts every counting and rotating function to the
* UNSIGNED INTEGER TYPES, which is exactly:
*
*     unsigned char, unsigned short, unsigned int,
*     unsigned long, unsigned long long
*
*   and deliberately NOT bool, char, char8_t, char16_t, char32_t or
* wchar_t -- even though several of those are unsigned on common
* platforms and all of them satisfy is_unsigned. Writing the constraint
* as is_unsigned would therefore accept bool and char, which the
* standard rejects, and the acceptance would be silently
* platform-dependent. So the list is spelled out as explicit
* specialisations instead of derived from a trait.
*
*   C++20 says this with a concept. Here it is an enable_if hook.
*
*   WIDTH COMES FROM numeric_limits, NOT sizeof * CHAR_BIT:
*   The standard defines N as numeric_limits<T>::digits, which is the
* count of VALUE bits. On a platform with padding bits the two differ,
* and every shift in this module would be wrong by the padding width.
*
*   RECURSION RATHER THAN LOOPS:
*   The helpers are recursive so they are constexpr from C++11 rather
* than C++14 -- a C++11 constexpr function may only be a single return
* statement. Depth is bounded by the operand width, so 64 frames at
* worst, all evaluated at compile time.
*
*
* path:      /inc/re_std/bit/bit_internal.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_BIT_INTERNAL_HPP
#define RE_STD_BIT_BIT_INTERNAL_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../type_traits/enable_if.hpp"
#include "../type_traits/true_type.hpp"
#include "../type_traits/false_type.hpp"
#include "../type_traits/remove_cv.hpp"
#include "../limits/numeric_limits.hpp"


namespace re_std
{


namespace internal
{

    // bit_uint
    //   trait: the standard's "unsigned integer type" set, spelled out.
    // A trait-derived test would wrongly admit bool and the char types.
    template<typename T> struct bit_uint            : false_type {};
    template<> struct bit_uint<unsigned char>        : true_type  {};
    template<> struct bit_uint<unsigned short>       : true_type  {};
    template<> struct bit_uint<unsigned int>         : true_type  {};
    template<> struct bit_uint<unsigned long>        : true_type  {};
    template<> struct bit_uint<unsigned long long>   : true_type  {};

    // bit_enable
    //   struct: alias hook: SFINAE guard used by every public function here.
    // Cv-stripped first, so a `const unsigned int` argument still binds.
    template<typename T,
             typename R = T>
    struct bit_enable
        : enable_if< bit_uint<typename remove_cv<T>::type>::value, R >
    {};

    // bit_digits
    //   constant: N, the number of VALUE bits. numeric_limits::digits
    // rather than sizeof * CHAR_BIT, so padding bits cannot skew it.
    template<typename T>
    struct bit_digits
    {
        static const int value = numeric_limits<T>::digits;
    };


    // bit_width_rec
    //   function: position of the highest set bit, 0 for zero. This is the
    // primitive the whole module is built on -- countl_zero, bit_floor
    // and bit_ceil are all one step away from it.
    template<typename T>
    RE_STD_CONSTEXPR int
    bit_width_rec(
        T _v
    )
    {
        return (_v == 0) ? 0 : (1 + bit_width_rec<T>(static_cast<T>(_v >> 1)));
    }

    // bit_ctz_rec
    //   function: count of trailing zeros. Caller guarantees _v != 0, which
    // is what keeps the recursion terminating.
    template<typename T>
    RE_STD_CONSTEXPR int
    bit_ctz_rec(
        T  _v,
        int _n
    )
    {
        return ((_v & static_cast<T>(1)) != 0)
            ? _n
            : bit_ctz_rec<T>(static_cast<T>(_v >> 1), _n + 1);
    }

    // bit_popcount_rec
    //   function: population count.
    template<typename T>
    RE_STD_CONSTEXPR int
    bit_popcount_rec(
        T  _v,
        int _acc
    )
    {
        return (_v == 0)
            ? _acc
            : bit_popcount_rec<T>(static_cast<T>(_v >> 1),
                                   _acc + static_cast<int>(_v & static_cast<T>(1)));
    }

}  // internal


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_BIT_INTERNAL_HPP
