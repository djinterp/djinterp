/*******************************************************************************
* djinterp [re_std]                                                     ceil.hpp
*
* chrono::ceil for durations and time_points:
*   Converts to a coarser precision, always rounding TOWARD POSITIVE
* INFINITY -- later in time, larger in value.
*
*       ceil<seconds>(milliseconds(1001))  ->  2s
*       ceil<seconds>(milliseconds(-1999)) -> -1s
*
*   THE NATURAL FUNCTION FOR TIMEOUTS AND DEADLINES:
*   A wait that is truncated is a wait that returns early, and a caller
* who asked to wait 1500 milliseconds and was given 1 second has been
* given the wrong answer in the direction that causes spurious timeouts.
* Rounding up is the safe direction for any "at least this long"
* quantity, which is why the standard's own wait_for overloads are
* specified in these terms.
*
*   HOW IT WORKS: cast, then step forward one tick if the cast undershot.
* The truncating cast moves toward zero, so the correction applies only
* to positive values, and never by more than one tick.
*
*   BACK-PORT: C++17 in std, C++11 here.
*
*
* path:      /inc/re_std/chrono/ceil.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_CEIL_HPP
#define RE_STD_CHRONO_CEIL_HPP 1

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

    // ceil_adjust
    //   function: step forward one tick if the truncating cast landed
    // below the true value.
    template<typename To,
             typename Rep,
             typename Period>
    RE_STD_CONSTEXPR To ceil_adjust(const To&                     _t,
                                const duration<Rep, Period>& _d)
    {
        return (_t < _d) ? To(_t.count() + 1) : _t;
    }

}  // internal

    // ceil
    //   function: coarsen a duration, rounding toward positive infinity.
    template<typename To,
             typename Rep,
             typename Period>
    RE_STD_CONSTEXPR
    typename enable_if<internal::is_duration<To>::value, To>::type
    ceil(const duration<Rep, Period>& _d)
    {
        return internal::ceil_adjust(duration_cast<To>(_d), _d);
    }

    // ceil
    //   function: coarsen a time_point, rounding toward the future.
    template<typename To,
             typename Clock,
             typename Duration>
    RE_STD_CONSTEXPR
    typename enable_if< internal::is_duration<To>::value,
                        time_point<Clock, To> >::type
    ceil(const time_point<Clock, Duration>& _t)
    {
        return time_point<Clock, To>(ceil<To>(_t.time_since_epoch()));
    }

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_CEIL_HPP
