/******************************************************************************
* djinterp [re_std]                                                 bit_cast.hpp
*
* bit_cast header:
*   Reinterprets the object representation of _From as a _To of the
* same size. The defined way to do what a reinterpret_cast or a union
* punning trick only appears to do:
*
*     float  f = 1.0f;
*     unsigned u = bit_cast<unsigned>(f);   // 0x3F800000
*
*   WHY NOT reinterpret_cast:
*   Reading an object through a pointer to an unrelated type violates
* the strict-aliasing rule; the compiler is entitled to assume it never
* happens and to optimise on that basis. Union punning is UB in C++ as
* well (it is legal in C). bit_cast is the only spelling with defined
* behaviour, and on a compiler with the builtin it is also the only one
* usable in a constant expression.
*
*   CONSTEXPR IS NOT ACHIEVABLE WITHOUT THE COMPILER:
*   The fallback is std::memcpy, which is not constexpr before C++20
* and cannot be made so from library code -- reading one object's bytes
* as another's is precisely what the constant evaluator forbids. So
* bit_cast is constexpr exactly when __builtin_bit_cast exists, and
* D_RE_STD_HAS_BUILTIN_BIT_CAST reports which. This is unusual for
* re_std, which normally reaches the same constexpr-ness on every
* platform; it is not achievable here.
*
*   THE TRIVIALLY-COPYABLE CONSTRAINT IS A STOPGAP:
*   [bit.cast] requires both types to be trivially copyable.
* re_std::is_trivially_copyable is catalogued but NOT IMPLEMENTED -- it
* is one of the eight type_traits whose includes are commented out in
* type_traits.hpp -- so this header calls __is_trivially_copyable
* directly, behind its own detection macro. When that trait ships, this
* should switch to it and the local detection should be deleted. Where
* the intrinsic is unavailable the constraint is not enforced at all
* and only the size check remains; that is a known hole, not an
* oversight.
*
*   PORTABILITY:
*   std added bit_cast in C++20; re_std back-ports it to C++11.
*
*
* path:      /inc/djinterp/re_std/bit/bit_cast.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_BIT_BIT_CAST_
#define DJINTERP_RE_STD_BIT_BIT_CAST_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstring>

// djinterp
#include "../type_traits/enable_if.hpp"
#include "../type_traits/remove_cv.hpp"


NS_RESTD


// ===========================================================================
// 0.   DETECTION
// ===========================================================================

#ifndef D_RE_STD_HAS_BUILTIN_BIT_CAST
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_bit_cast)
            #define D_RE_STD_HAS_BUILTIN_BIT_CAST   1
        #else
            #define D_RE_STD_HAS_BUILTIN_BIT_CAST   0
        #endif
    #else
        #define D_RE_STD_HAS_BUILTIN_BIT_CAST       0
    #endif
#endif

// Local stand-in for re_std::is_trivially_copyable, which is catalogued
// but not implemented. Delete this block when that trait ships.
#ifndef D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE_INTRINSIC
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivially_copyable)
            #define D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE_INTRINSIC 1
        #else
            #define D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE_INTRINSIC 0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC) || defined(D_ENV_COMPILER_MSVC) )
        #define D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE_INTRINSIC     1
    #else
        #define D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE_INTRINSIC     0
    #endif
#endif


// ===========================================================================
// I.   BIT_CAST
// ===========================================================================

#if D_RE_STD_HAS_BUILTIN_BIT_CAST

// bit_cast
//   function: constexpr object-representation reinterpretation.
template<typename _To,
         typename _From>
D_CONSTEXPR _To
bit_cast(
    const _From& _from
) D_NOEXCEPT
{
    static_assert(sizeof(_To) == sizeof(_From),
        "re_std::bit_cast: source and destination must be the same size");
#if D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE_INTRINSIC
    static_assert(__is_trivially_copyable(_To) &&
                  __is_trivially_copyable(_From),
        "re_std::bit_cast: both types must be trivially copyable");
#endif
    return __builtin_bit_cast(_To, _from);
}

#else

// bit_cast
//   function: memcpy fallback. Correct, but NOT constexpr -- see the
// header note; this is not a limitation library code can lift.
template<typename _To,
         typename _From>
inline _To
bit_cast(
    const _From& _from
) D_NOEXCEPT
{
    static_assert(sizeof(_To) == sizeof(_From),
        "re_std::bit_cast: source and destination must be the same size");
#if D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE_INTRINSIC
    static_assert(__is_trivially_copyable(_To) &&
                  __is_trivially_copyable(_From),
        "re_std::bit_cast: both types must be trivially copyable");
#endif
    _To _to;
    std::memcpy(&_to, &_from, sizeof(_To));
    return _to;
}

#endif  // D_RE_STD_HAS_BUILTIN_BIT_CAST


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_BIT_BIT_CAST_
