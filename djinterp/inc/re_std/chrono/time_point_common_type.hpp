/*******************************************************************************
* djinterp [re_std]                                   time_point_common_type.hpp
*
* the common_type specialisation for two time_points:
*   Two points on the SAME clock with different duration precisions have
* a common type: the same clock, with the durations' common type.
*
*   THE SPECIALISATION IS DELIBERATELY WRITTEN OVER ONE CLOCK PARAMETER:
*
*       common_type< time_point<Clock, _Dur1>, time_point<Clock, _Dur2> >
*
*   Both arguments name the same Clock, so a pair of time_points from
* DIFFERENT clocks does not match this specialisation at all. It falls
* through to the primary template, which finds no conversion between them
* and so has no `type` member -- and because it has no member rather than
* a hard error, the failure is SFINAE-friendly: mixed-clock arithmetic
* removes itself from overload resolution instead of exploding inside the
* library. That is the mechanism behind "you cannot subtract a
* steady_clock reading from a system_clock reading".
*
*
* path:      /inc/re_std/chrono/time_point_common_type.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_TIME_POINT_COMMON_TYPE_HPP
#define RE_STD_CHRONO_TIME_POINT_COMMON_TYPE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./time_point.hpp"
#include "./duration_common_type.hpp"
#include "../type_traits/common_type.hpp"


namespace re_std
{

    // common_type< chrono::time_point, chrono::time_point >
    //   trait: specialisation for two points on the same clock.
    template<typename Clock,
             typename Duration1,
             typename Duration2>
    struct common_type< chrono::time_point<Clock, Duration1>,
                        chrono::time_point<Clock, Duration2> >
    {
        typedef chrono::time_point<
                    Clock,
                    typename common_type<Duration1, Duration2>::type > type;
    };

    // common_type< chrono::time_point >
    //   trait: one-argument form, normalising the duration for the same
    // reason the duration specialisation does.
    template<typename Clock,
             typename Duration>
    struct common_type< chrono::time_point<Clock, Duration> >
    {
        typedef chrono::time_point<
                    Clock,
                    typename common_type<Duration>::type > type;
    };

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_TIME_POINT_COMMON_TYPE_HPP
