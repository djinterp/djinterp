/*******************************************************************************
* djinterp [re_std]                                                    floor.hpp
*
* chrono::floor for durations and time_points:
*   Converts to a coarser precision, always rounding TOWARD NEGATIVE
* INFINITY -- earlier in time, smaller in value, without exception.
*
*   THIS IS THE FUNCTION duration_cast SHOULD HAVE BEEN FOR MOST USES:
*   duration_cast truncates toward zero, so it rounds negative values UP:
*
*       duration_cast<seconds>(milliseconds(-1999))  ->  -1s
*       floor<seconds>(milliseconds(-1999))          ->  -2s
*
*   The second is what "which second does this fall in" means, and it is
* what a caller almost always wants when bucketing timestamps. The first
* produces a discontinuity at zero that shows up as a one-unit error in
* exactly the region a test with positive fixtures will never cover.
*
*   HOW IT WORKS: cast, then step back one tick if the cast overshot
* upwards. Because the cast only ever moves toward zero, the correction
* is needed only for negative values, and never more than one tick.
*
*   BACK-PORT: std added floor / ceil / round in C++17. re_std provides
* them from C++11 and constexpr from C++11 -- a six-year lead. The C++11
* form is expressed through internal helpers rather than a local
* variable, since relaxed constexpr bodies are C++14.
*
*
* path:      /inc/re_std/chrono/floor.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_FLOOR_HPP
#define RE_STD_CHRONO_FLOOR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./duration.hpp"
#include "./duration_cast.hpp"
#include "./duration_arithmetic.hpp"
#include "./duration_compare.hpp"
#include "./time_point.hpp"
#include "../type_traits/enable_if.hpp"


namespace re_std
{

namespace chrono
{

namespace internal
{

    // floor_adjust
    //   function: step back one tick if the truncating cast landed above
    // the true value. Exists so the C++11 form needs no local variable.
    template<typename To,
             typename Rep,
             typename Period>
    RE_STD_CONSTEXPR To floor_adjust(const To&                       _t,
                                 const duration<Rep, Period>&   _d)
    {
        return (_t > _d) ? To(_t.count() - 1) : _t;
    }

}  // internal

    // floor
    //   function: coarsen a duration, rounding toward negative infinity.
    template<typename To,
             typename Rep,
             typename Period>
    RE_STD_CONSTEXPR
    typename enable_if<internal::is_duration<To>::value, To>::type
    floor(const duration<Rep, Period>& _d)
    {
        return internal::floor_adjust(duration_cast<To>(_d), _d);
    }

    // floor
    //   function: coarsen a time_point, rounding toward the past. Unlike
    // time_point_cast, the direction does not depend on which side of the
    // epoch the point falls.
    template<typename To,
             typename Clock,
             typename Duration>
    RE_STD_CONSTEXPR
    typename enable_if< internal::is_duration<To>::value,
                        time_point<Clock, To> >::type
    floor(const time_point<Clock, Duration>& _t)
    {
        return time_point<Clock, To>(floor<To>(_t.time_since_epoch()));
    }

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_FLOOR_HPP
