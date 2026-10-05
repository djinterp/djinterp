/*******************************************************************************
* djinterp [re_std]                                    time_point_arithmetic.hpp
*
* the time_point arithmetic operators:
*   The four operations the affine structure of a timeline permits, and
* no others:
*
*       time_point + duration  -> time_point      move along the timeline
*       duration + time_point  -> time_point      same, written the other way
*       time_point - duration  -> time_point      move backwards
*       time_point - time_point -> duration       how far apart
*
*   THE MISSING OPERATION IS THE INTERESTING ONE:
*   There is no `time_point + time_point`. Adding two positions is
* meaningless -- what would half past three plus half past four be? --
* and the standard does not define it, so neither does re_std. A time_point
* is a point in an affine space, a duration is a vector in it, and the
* operator set above is exactly what that structure allows.
*
*   Nor is there `duration - time_point`, for the same reason: subtracting
* a position from a length has no meaning, even though the token sequence
* looks symmetrical with the one above it.
*
*   SUBTRACTION IS WHERE THE CLOCK TAG EARNS ITS KEEP: both operands must
* name the same clock, or common_type finds no `type` and the overload
* removes itself. Subtracting a steady_clock reading from a system_clock
* reading is not a runtime surprise -- it does not compile.
*
*
* path:      /inc/re_std/chrono/time_point_arithmetic.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_TIME_POINT_ARITHMETIC_HPP
#define RE_STD_CHRONO_TIME_POINT_ARITHMETIC_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./time_point.hpp"
#include "./time_point_common_type.hpp"
#include "./duration_arithmetic.hpp"
#include "./duration_common_type.hpp"
#include "../type_traits/common_type.hpp"


namespace re_std
{

namespace chrono
{

    // operator+
    //   function: advance a point by a length.
    template<typename Clock, typename Duration1,
             typename Rep2,  typename Period2>
    RE_STD_CONSTEXPR
    time_point<Clock,
               typename common_type<Duration1,
                                    duration<Rep2, Period2> >::type>
    operator+(const time_point<Clock, Duration1>& _t,
              const duration<Rep2, Period2>&      _d)
    {
        typedef typename common_type<Duration1,
                                     duration<Rep2, Period2> >::type _CD;
        return time_point<Clock, _CD>(_t.time_since_epoch() + _d);
    }

    // operator+
    //   function: the same, with the length on the left.
    template<typename Rep1,  typename Period1,
             typename Clock, typename Duration2>
    RE_STD_CONSTEXPR
    time_point<Clock,
               typename common_type<duration<Rep1, Period1>,
                                    Duration2>::type>
    operator+(const duration<Rep1, Period1>&      _d,
              const time_point<Clock, Duration2>& _t)
    {
        return _t + _d;
    }

    // operator-
    //   function: move a point back by a length.
    template<typename Clock, typename Duration1,
             typename Rep2,  typename Period2>
    RE_STD_CONSTEXPR
    time_point<Clock,
               typename common_type<Duration1,
                                    duration<Rep2, Period2> >::type>
    operator-(const time_point<Clock, Duration1>& _t,
              const duration<Rep2, Period2>&      _d)
    {
        typedef typename common_type<Duration1,
                                     duration<Rep2, Period2> >::type _CD;
        return time_point<Clock, _CD>(_t.time_since_epoch() - _d);
    }

    // operator-
    //   function: the distance between two points on the same clock.
    // Returns a duration, not a time_point.
    template<typename Clock,
             typename Duration1,
             typename Duration2>
    RE_STD_CONSTEXPR
    typename common_type<Duration1, Duration2>::type
    operator-(const time_point<Clock, Duration1>& _lhs,
              const time_point<Clock, Duration2>& _rhs)
    {
        return _lhs.time_since_epoch() - _rhs.time_since_epoch();
    }

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_TIME_POINT_ARITHMETIC_HPP
