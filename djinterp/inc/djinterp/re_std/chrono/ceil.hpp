/******************************************************************************
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
* path:      /inc/djinterp/re_std/chrono/ceil.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_CEIL_
#define DJINTERP_RE_STD_CHRONO_CEIL_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration.hpp"
#include "./duration_cast.hpp"
#include "./duration_arithmetic.hpp"
#include "./duration_compare.hpp"
#include "./time_point.hpp"
#include "../type_traits/enable_if.hpp"


NS_RESTD

namespace chrono
{

NS_INTERNAL

    // ceil_adjust
    //   function: step forward one tick if the truncating cast landed
    // below the true value.
    template<typename _To,
             typename _Rep,
             typename _Period>
    D_CONSTEXPR _To ceil_adjust(const _To&                     _t,
                                const duration<_Rep, _Period>& _d)
    {
        return (_t < _d) ? _To(_t.count() + 1) : _t;
    }

NS_END  // internal

    // ceil
    //   function: coarsen a duration, rounding toward positive infinity.
    template<typename _To,
             typename _Rep,
             typename _Period>
    D_CONSTEXPR
    typename enable_if<internal::is_duration<_To>::value, _To>::type
    ceil(const duration<_Rep, _Period>& _d)
    {
        return internal::ceil_adjust(duration_cast<_To>(_d), _d);
    }

    // ceil
    //   function: coarsen a time_point, rounding toward the future.
    template<typename _To,
             typename _Clock,
             typename _Duration>
    D_CONSTEXPR
    typename enable_if< internal::is_duration<_To>::value,
                        time_point<_Clock, _To> >::type
    ceil(const time_point<_Clock, _Duration>& _t)
    {
        return time_point<_Clock, _To>(ceil<_To>(_t.time_since_epoch()));
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_CEIL_
