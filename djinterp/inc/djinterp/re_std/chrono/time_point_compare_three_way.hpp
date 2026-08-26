/******************************************************************************
* djinterp [re_std]                             time_point_compare_three_way.hpp
*
* the time_point three-way comparison:
*   operator<=> between two points on the same clock. C++20 only -- the
* spaceship operator is a language feature with no back-port.
*
*   The comparison delegates to the durations' three-way comparison, so
* the ordering category is the representation's: strong_ordering for the
* integral reps the predefined clocks use.
*
*   The single _Clock parameter shared by both arguments keeps the
* mixed-clock case out, exactly as in the legacy operators.
*
*
* path:      /inc/djinterp/re_std/chrono/time_point_compare_three_way.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_TIME_POINT_COMPARE_THREE_WAY_
#define DJINTERP_RE_STD_CHRONO_TIME_POINT_COMPARE_THREE_WAY_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// std
//   required for the ordering types the builtin <=> yields.
#include <compare>

// djinterp
#include "./time_point.hpp"
#include "./duration_compare_three_way.hpp"


NS_RESTD

namespace chrono
{

    // operator<=>
    //   function: three-way comparison of two points on one clock.
    template<typename _Clock, typename _Duration1, typename _Duration2>
    D_CONSTEXPR auto operator<=>(const time_point<_Clock, _Duration1>& _lhs,
                                 const time_point<_Clock, _Duration2>& _rhs)
        -> decltype(_lhs.time_since_epoch() <=> _rhs.time_since_epoch())
    {
        return _lhs.time_since_epoch() <=> _rhs.time_since_epoch();
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_TIME_POINT_COMPARE_THREE_WAY_
