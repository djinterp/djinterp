/*******************************************************************************
* djinterp [re_std]                                     duration_common_type.hpp
*
* the common_type specialisation for two durations:
*   Answers what type `milliseconds + microseconds` should have. The
* result must be able to represent both operands EXACTLY, since the whole
* point of the mixed-mode operators is that adding two durations never
* silently truncates.
*
*   THE PERIOD IS gcd(num) / lcm(den) -- AND THE ORDER IS NOT A TYPO:
*   The common period must be the coarsest tick that divides both, which
* means the SMALLEST period, which means taking the greatest common
* divisor of the numerators over the least common multiple of the
* denominators. The intuition runs backwards from the usual gcd/lcm
* pairing, and it is worth stating why: a smaller period is a finer tick,
* and only a finer tick can represent both inputs without loss.
*
*   For milli (1/1000) and micro (1/1000000) that gives
* gcd(1,1) / lcm(1000, 1000000) = 1/1000000 -- microseconds, the finer of
* the two. Correct.
*
*   THE LCM IS COMPUTED AS (a / gcd) * b, NEVER a * b / gcd:
*   The two are equal in exact arithmetic and are not equal in intmax_t.
* Multiplying first overflows for denominators as ordinary as
* 1000000000 and 1000000007, and the overflow is silent -- it produces a
* wrong period, not a compile error. Dividing first keeps every
* intermediate no larger than the answer.
*
*   The gcd machinery is re_std::internal::ratio_gcd from <ratio>, reused
* rather than reimplemented, and it is why <ratio> had to ship first.
*
*
* path:      /inc/re_std/chrono/duration_common_type.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef RE_STD_CHRONO_DURATION_COMMON_TYPE_HPP
#define RE_STD_CHRONO_DURATION_COMMON_TYPE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./duration.hpp"
#include "../ratio/ratio.hpp"
#include "../type_traits/common_type.hpp"
#include "../cstdint/cstdint.hpp"


namespace re_std
{

namespace internal
{

    // duration_common_ratio
    //   trait: the finest period that represents both R1 and R2
    // exactly -- gcd of the numerators over lcm of the denominators.
    template<typename R1,
             typename R2>
    struct duration_common_ratio
    {
    private:
        static const intmax_t s_gcd_num =
            ratio_gcd< ratio_abs<R1::num>::value,
                       ratio_abs<R2::num>::value >::value;

        static const intmax_t s_gcd_den =
            ratio_gcd<R1::den, R2::den>::value;

    public:
        // Divide before multiplying -- see the header comment.
        typedef ratio<s_gcd_num, (R1::den / s_gcd_den) * R2::den> type;
    };

}  // internal


    // common_type< chrono::duration, chrono::duration >
    //   trait: specialisation. The common representation is the reps'
    // common type; the common period is the finest of the two.
    template<typename Rep1,
             typename Period1,
             typename Rep2,
             typename Period2>
    struct common_type< chrono::duration<Rep1, Period1>,
                        chrono::duration<Rep2, Period2> >
    {
        typedef chrono::duration<
                    typename common_type<Rep1, Rep2>::type,
                    typename internal::duration_common_ratio<
                        typename Period1::type,
                        typename Period2::type >::type
                > type;
    };

    // common_type< chrono::duration >
    //   trait: one-argument specialisation. Required because the primary
    // template's decay-based rule would produce the duration itself but
    // with an UNREDUCED period, so common_type<duration<int, ratio<2,4> > >
    // and duration<int, ratio<1,2> > would not agree. Normalising here
    // keeps the unary and binary forms consistent.
    template<typename Rep,
             typename Period>
    struct common_type< chrono::duration<Rep, Period> >
    {
        typedef chrono::duration<
                    typename common_type<Rep>::type,
                    typename Period::type > type;
    };

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_DURATION_COMMON_TYPE_HPP
