/******************************************************************************
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
* path:      /inc/djinterp/re_std/chrono/duration_common_type.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_DURATION_COMMON_TYPE_
#define DJINTERP_RE_STD_CHRONO_DURATION_COMMON_TYPE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration.hpp"
#include "../ratio/ratio.hpp"
#include "../type_traits/common_type.hpp"
#include "../cstdint/cstdint.hpp"


NS_RESTD

NS_INTERNAL

    // duration_common_ratio
    //   trait: the finest period that represents both _R1 and _R2
    // exactly -- gcd of the numerators over lcm of the denominators.
    template<typename _R1,
             typename _R2>
    struct duration_common_ratio
    {
    private:
        static const std::intmax_t s_gcd_num =
            ratio_gcd< ratio_abs<_R1::num>::value,
                       ratio_abs<_R2::num>::value >::value;

        static const std::intmax_t s_gcd_den =
            ratio_gcd<_R1::den, _R2::den>::value;

    public:
        // Divide before multiplying -- see the header comment.
        typedef ratio<s_gcd_num, (_R1::den / s_gcd_den) * _R2::den> type;
    };

NS_END  // internal


    // common_type< chrono::duration, chrono::duration >
    //   trait: specialisation. The common representation is the reps'
    // common type; the common period is the finest of the two.
    template<typename _Rep1,
             typename _Period1,
             typename _Rep2,
             typename _Period2>
    struct common_type< chrono::duration<_Rep1, _Period1>,
                        chrono::duration<_Rep2, _Period2> >
    {
        typedef chrono::duration<
                    typename common_type<_Rep1, _Rep2>::type,
                    typename internal::duration_common_ratio<
                        typename _Period1::type,
                        typename _Period2::type >::type
                > type;
    };

    // common_type< chrono::duration >
    //   trait: one-argument specialisation. Required because the primary
    // template's decay-based rule would produce the duration itself but
    // with an UNREDUCED period, so common_type<duration<int, ratio<2,4> > >
    // and duration<int, ratio<1,2> > would not agree. Normalising here
    // keeps the unary and binary forms consistent.
    template<typename _Rep,
             typename _Period>
    struct common_type< chrono::duration<_Rep, _Period> >
    {
        typedef chrono::duration<
                    typename common_type<_Rep>::type,
                    typename _Period::type > type;
    };

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_DURATION_COMMON_TYPE_
