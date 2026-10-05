/*******************************************************************************
* djinterp [re_std]                                                  cstdint.hpp
*
* the fixed-width integer typedefs (identity-preserving re-exports):
*   int8_t through uint64_t, the _least and _fast families, intmax_t /
* uintmax_t and intptr_t / uintptr_t -- surfaced in re_std:: so that code
* can spell them with one prefix.
*
*   THERE IS NOTHING TO IMPLEMENT HERE, AND THAT IS THE POINT:
*   Which builtin type is exactly 64 bits wide is an ABI fact, not a
* library choice. `typedef long int64_t;` is right on LP64 and wrong on
* Windows; `typedef long long int64_t;` is the reverse. Only the
* implementation knows, so re_std re-exports its answer and preserves
* identity -- re_std::int64_t IS std::int64_t, so the two spellings pick
* the same printf format macro and the same overload.
*
*   THE EXACT-WIDTH FAMILY IS OPTIONAL AND IS GATED AS SUCH:
*   intN_t / uintN_t exist only where the implementation has a type of
* exactly N bits with no padding -- [cstdint.syn] makes them conditional,
* and a machine with 9-bit bytes or 36-bit words genuinely lacks them.
* The standard guarantees the corresponding INTN_MAX macro is defined if
* and only if the typedef is, which gives an exact preprocessor test, so
* each pair is gated on its own macro rather than on a compiler
* whitelist. On a target without them the names are simply absent -- the
* header still compiles, which is the rule: degrade or omit, never error.
*
*   The _least and _fast families and intmax_t / uintmax_t are mandatory
* in the standard, but dstdint.h leaves a family out where this build
* cannot spell its type -- the 64-bit ones under ISO strict C++98 on a
* target that makes them `long long`, the _fast ones wherever it cannot
* tell which types the platform chose -- so each pair is gated on its
* limit macro as well. intptr_t / uintptr_t are optional (a machine need
* not have an integer wide enough to hold a pointer) and are gated the
* same way.
*
*   GRANULARITY -- A DOCUMENTED EXCEPTION:
*   Every public symbol normally gets its own header. These do not, for
* the same reason numbers.hpp holds all thirteen constants: granularity
* exists to narrow compile-time dependencies, and every typedef here has
* the identical dependency -- <cstdint> itself. Thirty-two headers each
* containing one using-declaration and the same #include would narrow
* nothing and cost thirty-two file opens.
*
*   EVERY LEVEL: the names come from dstdint.h, re_std's own <stdint.h>
* beside this file, and are re-exported from the global namespace, where
* <cstdint> also puts them, so re_std::int64_t is std::int64_t from C++11
* and the platform's int64_t below. A name dstdint.h leaves out is absent
* here too, gated on its limit macro.
*
*
* path:      /inc/re_std/cstdint/cstdint.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef RE_STD_CSTDINT_CSTDINT_HPP
#define RE_STD_CSTDINT_CSTDINT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./dstdint.h"    // the typedefs and their limit macros, at every
                          // level
//   dstdint.h, not <cstdint>: <cstdint> is a C++11 header, and dstdint.h is
// re_std's own answer at every level (decision 4.7; the owner's ruling of
// 2026.10.01) -- the platform's <stdint.h> where there is one, so the types
// are the platform's, and its own derivation otherwise. It also brings the
// INT*_MIN / INT*_MAX / INT*_C limit and literal macros into scope; they are
// macros and therefore have no re_std:: spelling -- see the umbrella.


namespace re_std
{


// ===========================================================================
// I.   EXACT WIDTH  (optional -- gated per pair on its own limit macro)
// ===========================================================================

#ifdef INT8_MAX
    // int8_t / uint8_t
    //   typedef: exactly 8 bits, two's complement, no padding.
    using ::int8_t;
    using ::uint8_t;
#endif

#ifdef INT16_MAX
    // int16_t / uint16_t
    //   typedef: exactly 16 bits, two's complement, no padding.
    using ::int16_t;
    using ::uint16_t;
#endif

#ifdef INT32_MAX
    // int32_t / uint32_t
    //   typedef: exactly 32 bits, two's complement, no padding.
    using ::int32_t;
    using ::uint32_t;
#endif

#ifdef INT64_MAX
    // int64_t / uint64_t
    //   typedef: exactly 64 bits, two's complement, no padding.
    using ::int64_t;
    using ::uint64_t;
#endif


// ===========================================================================
// II.  LEAST WIDTH  (mandatory in the standard; dstdint.h leaves a pair out
//      where the build cannot spell its type, so each is gated on its limit
//      macro)
// ===========================================================================

#ifdef INT_LEAST8_MAX
    // int_least8_t / uint_least8_t
    //   typedef: smallest type with at least 8 bits.
    using ::int_least8_t;
    using ::uint_least8_t;
#endif

#ifdef INT_LEAST16_MAX
    // int_least16_t / uint_least16_t
    //   typedef: smallest type with at least 16 bits.
    using ::int_least16_t;
    using ::uint_least16_t;
#endif

#ifdef INT_LEAST32_MAX
    // int_least32_t / uint_least32_t
    //   typedef: smallest type with at least 32 bits.
    using ::int_least32_t;
    using ::uint_least32_t;
#endif

#ifdef INT_LEAST64_MAX
    // int_least64_t / uint_least64_t
    //   typedef: smallest type with at least 64 bits. Absent under ISO
    // strict C++98 on a target whose 64-bit type is `long long`.
    using ::int_least64_t;
    using ::uint_least64_t;
#endif


// ===========================================================================
// III. FAST WIDTH  (mandatory in the standard; dstdint.h's own derivation
//      leaves them out, so each pair is gated on its limit macro)
// ===========================================================================

#ifdef INT_FAST8_MAX
    // int_fast8_t / uint_fast8_t
    //   typedef: fastest type with at least 8 bits.
    using ::int_fast8_t;
    using ::uint_fast8_t;
#endif

#ifdef INT_FAST16_MAX
    // int_fast16_t / uint_fast16_t
    //   typedef: fastest type with at least 16 bits.
    using ::int_fast16_t;
    using ::uint_fast16_t;
#endif

#ifdef INT_FAST32_MAX
    // int_fast32_t / uint_fast32_t
    //   typedef: fastest type with at least 32 bits.
    using ::int_fast32_t;
    using ::uint_fast32_t;
#endif

#ifdef INT_FAST64_MAX
    // int_fast64_t / uint_fast64_t
    //   typedef: fastest type with at least 64 bits.
    using ::int_fast64_t;
    using ::uint_fast64_t;
#endif


// ===========================================================================
// IV.  GREATEST WIDTH AND POINTER-SIZED
// ===========================================================================

    // intmax_t / uintmax_t
    //   typedef: the widest integer types the implementation supports.
    // re_std::ratio's non-type parameters are intmax_t, so this pair fixes
    // the range of every ratio and therefore of every chrono duration
    // period. Gated on INTMAX_MAX: dstdint.h leaves the pair out where the
    // platform's is a `long long` the build cannot use (ISO strict C++98).
#ifdef INTMAX_MAX
    using ::intmax_t;
    using ::uintmax_t;
#endif

#ifdef INTPTR_MAX
    // intptr_t / uintptr_t
    //   typedef: integers able to round-trip a void*. Optional -- a target
    // need not have an integer that wide.
    using ::intptr_t;
    using ::uintptr_t;
#endif


}  // re_std


#endif  // RE_STD_CSTDINT_CSTDINT_HPP
