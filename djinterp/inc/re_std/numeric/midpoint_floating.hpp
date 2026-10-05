/*******************************************************************************
* djinterp [re_std]                                        midpoint_floating.hpp
*
* midpoint_floating algorithm header:
*   overflow-safe floating-point midpoint:
*   `midpoint(a, b)` for floating-point a and b returns the point half way
* between them, computed so that it is correct across the WHOLE range - not
* just where (a + b) happens not to overflow.
*
*   WHY (a + b) / 2 IS WRONG.
*   It overflows to infinity whenever a and b are both near the top of the
* range, even though the true midpoint is perfectly representable:
* midpoint(1e308, 1e308) must be 1e308, but (1e308 + 1e308) is inf and inf/2
* is inf.  The obvious repair, a/2 + b/2, fixes that but breaks the other end:
* halving a subnormal loses its low bit to underflow, so the result drifts.
*
*   THE THREE-CASE FORM.
*   Neither expression is right everywhere, so the implementation picks per
* call, on the magnitudes:
*
*     both |a|,|b| <= max/2   ->  (a + b) / 2   no overflow possible; exact
*     |a| < min*2             ->  a + b/2       a is tiny, halving it would
*                                               underflow, so halve only b
*     |b| < min*2             ->  a/2 + b       mirror image
*     otherwise               ->  a/2 + b/2     both large; halve first
*
*   This is the formulation from P0811R3 and matches what the major standard
* libraries ship.
*
*   STD IS C++20; re_std IS C++98 (constexpr from C++11).
*   Nothing here needs a language feature past C++98 - it is comparisons and
* arithmetic over numeric_limits constants.  RE_STD_CONSTEXPR lifts it to a
* constant expression from C++11, nine years ahead of std.  No <cmath>: the
* absolute value is taken by an internal helper so the module keeps re_std's
* no-standard-headers rule and stays constexpr on C++11, where std::fabs is
* not.
*
*   NaN propagates: every comparison against a NaN is false, so a NaN input
* falls to the a/2 + b/2 branch and yields NaN, matching std.
*
*
* path:      /inc/re_std/numeric/midpoint_floating.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_NUMERIC_MIDPOINT_FLOATING_HPP
#define RE_STD_NUMERIC_MIDPOINT_FLOATING_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../limits/limits"             // numeric_limits
#include "../type_traits/type_traits.hpp"   // enable_if, is_floating_point

namespace re_std
{

namespace internal
{

    // abs_fp
    //   function: |value| for floating-point types, without <cmath>.  Kept
    // internal because it deliberately does NOT handle -0.0 specially (the
    // caller only compares the result, never returns it) and is not the
    // general-purpose abs.
    template<typename Float>
    RE_STD_CONSTEXPR Float abs_fp(Float value) RE_STD_NOEXCEPT
    {
        return (value < static_cast<Float>(0)) ? -value : value;
    }

}  // internal


// midpoint (floating-point)
//   function: half way between a and b, computed without overflow at the top
// of the range or underflow at the bottom.
template<typename Float>
RE_STD_NODISCARD RE_STD_CONSTEXPR
typename enable_if<is_floating_point<Float>::value, Float>::type
midpoint(Float a, Float b) RE_STD_NOEXCEPT
{
    return (   internal::abs_fp(a) <= (numeric_limits<Float>::max)()
                                      / static_cast<Float>(2)
            && internal::abs_fp(b) <= (numeric_limits<Float>::max)()
                                      / static_cast<Float>(2) )
               // neither is large: the direct form cannot overflow
               ? (a + b) / static_cast<Float>(2)
         : ( internal::abs_fp(a) < (numeric_limits<Float>::min)()
                                   * static_cast<Float>(2) )
               // a is tiny; halving it would underflow, so halve only b
               ? a + b / static_cast<Float>(2)
         : ( internal::abs_fp(b) < (numeric_limits<Float>::min)()
                                   * static_cast<Float>(2) )
               // mirror image
               ? a / static_cast<Float>(2) + b
               // both large: halve before adding
               : a / static_cast<Float>(2) + b / static_cast<Float>(2);
}

}  // re_std

#endif  // floor, for now


#endif  // RE_STD_NUMERIC_MIDPOINT_FLOATING_HPP
