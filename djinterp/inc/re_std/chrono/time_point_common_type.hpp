/******************************************************************************
* djinterp [re_std]                                   time_point_common_type.hpp
*
* the common_type specialisation for two time_points:
*   Two points on the SAME clock with different duration precisions have
* a common type: the same clock, with the durations' common type.
*
*   THE SPECIALISATION IS DELIBERATELY WRITTEN OVER ONE CLOCK PARAMETER:
*
*       common_type< time_point<_Clock, _Dur1>, time_point<_Clock, _Dur2> >
*
*   Both arguments name the same _Clock, so a pair of time_points from
* DIFFERENT clocks does not match this specialisation at all. It falls
* through to the primary template, which finds no conversion between them
* and so has no `type` member -- and because it has no member rather than
* a hard error, the failure is SFINAE-friendly: mixed-clock arithmetic
* removes itself from overload resolution instead of exploding inside the
* library. That is the mechanism behind "you cannot subtract a
* steady_clock reading from a system_clock reading".
*
*
* path:      /inc/djinterp/re_std/chrono/time_point_common_type.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_TIME_POINT_COMMON_TYPE_
#define DJINTERP_RE_STD_CHRONO_TIME_POINT_COMMON_TYPE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./time_point.hpp"
#include "./duration_common_type.hpp"
#include "../type_traits/common_type.hpp"


NS_RESTD

    // common_type< chrono::time_point, chrono::time_point >
    //   trait: specialisation for two points on the same clock.
    template<typename _Clock,
             typename _Duration1,
             typename _Duration2>
    struct common_type< chrono::time_point<_Clock, _Duration1>,
                        chrono::time_point<_Clock, _Duration2> >
    {
        typedef chrono::time_point<
                    _Clock,
                    typename common_type<_Duration1, _Duration2>::type > type;
    };

    // common_type< chrono::time_point >
    //   trait: one-argument form, normalising the duration for the same
    // reason the duration specialisation does.
    template<typename _Clock,
             typename _Duration>
    struct common_type< chrono::time_point<_Clock, _Duration> >
    {
        typedef chrono::time_point<
                    _Clock,
                    typename common_type<_Duration>::type > type;
    };

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_TIME_POINT_COMMON_TYPE_
