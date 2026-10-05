/*******************************************************************************
* djinterp [re_std]                                                   endian.hpp
*
* endian header:
*   The three-value scoped enum describing byte order:
*
*     endian::little   endian::big   endian::native
*
*   native compares equal to little on a little-endian platform, equal
* to big on a big-endian one, and equal to NEITHER on a mixed-endian
* platform -- which is the only portable way to detect the mixed case,
* and the reason the enum has three enumerators rather than two.
*
*   DETECTION:
*   Uses the compiler's __BYTE_ORDER__ when it is available (GCC,
* Clang, and anything else that follows them). MSVC does not define it,
* so the fallback treats every MSVC target as little-endian, which is
* true of every architecture MSVC currently supports. Where neither is
* available, native is given a distinct value so that it equals neither
* little nor big -- the mixed-endian answer, which is honest, rather
* than guessing little and being silently wrong.
*
*   RE_STD_HAS_ENDIAN_DETECTION reports whether the answer came from
* the compiler or from the fallback.
*
*   PORTABILITY:
*   std added endian in C++20; re_std back-ports it to C++11, where
* scoped enums arrive.
*
*
* path:      /inc/re_std/bit/endian.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_BIT_ENDIAN_HPP
#define RE_STD_BIT_ENDIAN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


namespace re_std
{


// ===========================================================================
// 0.   DETECTION
// ===========================================================================

#ifndef RE_STD_HAS_ENDIAN_DETECTION
    #if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&        \
        defined(__ORDER_BIG_ENDIAN__)
        #define RE_STD_HAS_ENDIAN_DETECTION   1
    #elif defined(RE_STD_COMPILER_MSVC)
        #define RE_STD_HAS_ENDIAN_DETECTION   1
    #else
        #define RE_STD_HAS_ENDIAN_DETECTION   0
    #endif
#endif


// ===========================================================================
// I.   ENDIAN
// ===========================================================================

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&            \
    defined(__ORDER_BIG_ENDIAN__)

// endian
//   enum: byte order, taken from the compiler's own answer.
enum class endian
{
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
};

#elif defined(RE_STD_COMPILER_MSVC)

// endian
//   enum: MSVC does not define __BYTE_ORDER__, but every target it
// currently supports is little-endian.
enum class endian
{
    little = 0,
    big    = 1,
    native = 0
};

#else

// endian
//   enum: no detection available. native is given its own value so it
// equals neither little nor big -- the mixed-endian answer. Reporting
// "unknown" this way is preferable to guessing little.
enum class endian
{
    little = 0,
    big    = 1,
    native = 2
};

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_BIT_ENDIAN_HPP
