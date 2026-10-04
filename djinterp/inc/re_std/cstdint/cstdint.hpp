/******************************************************************************
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
* and are re-exported unconditionally. intptr_t / uintptr_t are optional
* (a machine need not have an integer wide enough to hold a pointer) and
* are gated the same way.
*
*   GRANULARITY -- A DOCUMENTED EXCEPTION:
*   Every public symbol normally gets its own header. These do not, for
* the same reason numbers.hpp holds all thirteen constants: granularity
* exists to narrow compile-time dependencies, and every typedef here has
* the identical dependency -- <cstdint> itself. Thirty-two headers each
* containing one using-declaration and the same #include would narrow
* nothing and cost thirty-two file opens.
*
*   C++11 FLOOR: <cstdint> is a C++11 header.
*
*
* path:      /inc/djinterp/re_std/cstdint/cstdint.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDINT_CSTDINT_
#define DJINTERP_RE_STD_CSTDINT_CSTDINT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
//   permitted: fundamental types only. This also brings the INT*_MIN /
// INT*_MAX / INT*_C limit and literal macros into scope; they are macros
// and therefore have no re_std:: spelling -- see the umbrella.
#include <cstdint>


NS_RESTD


// ===========================================================================
// I.   EXACT WIDTH  (optional -- gated per pair on its own limit macro)
// ===========================================================================

#ifdef INT8_MAX
    // int8_t / uint8_t
    //   typedef: exactly 8 bits, two's complement, no padding.
    using ::std::int8_t;
    using ::std::uint8_t;
#endif

#ifdef INT16_MAX
    // int16_t / uint16_t
    //   typedef: exactly 16 bits, two's complement, no padding.
    using ::std::int16_t;
    using ::std::uint16_t;
#endif

#ifdef INT32_MAX
    // int32_t / uint32_t
    //   typedef: exactly 32 bits, two's complement, no padding.
    using ::std::int32_t;
    using ::std::uint32_t;
#endif

#ifdef INT64_MAX
    // int64_t / uint64_t
    //   typedef: exactly 64 bits, two's complement, no padding.
    using ::std::int64_t;
    using ::std::uint64_t;
#endif


// ===========================================================================
// II.  LEAST WIDTH  (mandatory)
// ===========================================================================

    // int_least8_t / uint_least8_t
    //   typedef: smallest type with at least 8 bits.
    using ::std::int_least8_t;
    using ::std::uint_least8_t;

    // int_least16_t / uint_least16_t
    //   typedef: smallest type with at least 16 bits.
    using ::std::int_least16_t;
    using ::std::uint_least16_t;

    // int_least32_t / uint_least32_t
    //   typedef: smallest type with at least 32 bits.
    using ::std::int_least32_t;
    using ::std::uint_least32_t;

    // int_least64_t / uint_least64_t
    //   typedef: smallest type with at least 64 bits.
    using ::std::int_least64_t;
    using ::std::uint_least64_t;


// ===========================================================================
// III. FAST WIDTH  (mandatory)
// ===========================================================================

    // int_fast8_t / uint_fast8_t
    //   typedef: fastest type with at least 8 bits.
    using ::std::int_fast8_t;
    using ::std::uint_fast8_t;

    // int_fast16_t / uint_fast16_t
    //   typedef: fastest type with at least 16 bits.
    using ::std::int_fast16_t;
    using ::std::uint_fast16_t;

    // int_fast32_t / uint_fast32_t
    //   typedef: fastest type with at least 32 bits.
    using ::std::int_fast32_t;
    using ::std::uint_fast32_t;

    // int_fast64_t / uint_fast64_t
    //   typedef: fastest type with at least 64 bits.
    using ::std::int_fast64_t;
    using ::std::uint_fast64_t;


// ===========================================================================
// IV.  GREATEST WIDTH AND POINTER-SIZED
// ===========================================================================

    // intmax_t / uintmax_t
    //   typedef: the widest integer types the implementation supports.
    // re_std::ratio's non-type parameters are intmax_t, so this pair fixes
    // the range of every ratio and therefore of every chrono duration
    // period.
    using ::std::intmax_t;
    using ::std::uintmax_t;

#ifdef INTPTR_MAX
    // intptr_t / uintptr_t
    //   typedef: integers able to round-trip a void*. Optional -- a target
    // need not have an integer that wide.
    using ::std::intptr_t;
    using ::std::uintptr_t;
#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDINT_CSTDINT_
