/******************************************************************************
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
* path:      /inc/djinterp/re_std/chrono/time_point_arithmetic.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_TIME_POINT_ARITHMETIC_
#define DJINTERP_RE_STD_CHRONO_TIME_POINT_ARITHMETIC_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./time_point.hpp"
#include "./time_point_common_type.hpp"
#include "./duration_arithmetic.hpp"
#include "./duration_common_type.hpp"
#include "../type_traits/common_type.hpp"


NS_RESTD

namespace chrono
{

    // operator+
    //   function: advance a point by a length.
    template<typename _Clock, typename _Duration1,
             typename _Rep2,  typename _Period2>
    D_CONSTEXPR
    time_point<_Clock,
               typename common_type<_Duration1,
                                    duration<_Rep2, _Period2> >::type>
    operator+(const time_point<_Clock, _Duration1>& _t,
              const duration<_Rep2, _Period2>&      _d)
    {
        typedef typename common_type<_Duration1,
                                     duration<_Rep2, _Period2> >::type _CD;
        return time_point<_Clock, _CD>(_t.time_since_epoch() + _d);
    }

    // operator+
    //   function: the same, with the length on the left.
    template<typename _Rep1,  typename _Period1,
             typename _Clock, typename _Duration2>
    D_CONSTEXPR
    time_point<_Clock,
               typename common_type<duration<_Rep1, _Period1>,
                                    _Duration2>::type>
    operator+(const duration<_Rep1, _Period1>&      _d,
              const time_point<_Clock, _Duration2>& _t)
    {
        return _t + _d;
    }

    // operator-
    //   function: move a point back by a length.
    template<typename _Clock, typename _Duration1,
             typename _Rep2,  typename _Period2>
    D_CONSTEXPR
    time_point<_Clock,
               typename common_type<_Duration1,
                                    duration<_Rep2, _Period2> >::type>
    operator-(const time_point<_Clock, _Duration1>& _t,
              const duration<_Rep2, _Period2>&      _d)
    {
        typedef typename common_type<_Duration1,
                                     duration<_Rep2, _Period2> >::type _CD;
        return time_point<_Clock, _CD>(_t.time_since_epoch() - _d);
    }

    // operator-
    //   function: the distance between two points on the same clock.
    // Returns a duration, not a time_point.
    template<typename _Clock,
             typename _Duration1,
             typename _Duration2>
    D_CONSTEXPR
    typename common_type<_Duration1, _Duration2>::type
    operator-(const time_point<_Clock, _Duration1>& _lhs,
              const time_point<_Clock, _Duration2>& _rhs)
    {
        return _lhs.time_since_epoch() - _rhs.time_since_epoch();
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_TIME_POINT_ARITHMETIC_
