/*******************************************************************************
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
*   The single Clock parameter shared by both arguments keeps the
* mixed-clock case out, exactly as in the legacy operators.
*
*
* path:      /inc/re_std/chrono/time_point_compare_three_way.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_TIME_POINT_COMPARE_THREE_WAY_HPP
#define RE_STD_CHRONO_TIME_POINT_COMPARE_THREE_WAY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// std
//   required for the ordering types the builtin <=> yields.
// std
#include <compare>

// re_std
#include "./time_point.hpp"
#include "./duration_compare_three_way.hpp"


namespace re_std
{

namespace chrono
{

    // operator<=>
    //   function: three-way comparison of two points on one clock.
    template<typename Clock, typename Duration1, typename Duration2>
    RE_STD_CONSTEXPR auto operator<=>(const time_point<Clock, Duration1>& _lhs,
                                 const time_point<Clock, Duration2>& _rhs)
        -> decltype(_lhs.time_since_epoch() <=> _rhs.time_since_epoch())
    {
        return _lhs.time_since_epoch() <=> _rhs.time_since_epoch();
    }

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER


#endif  // RE_STD_CHRONO_TIME_POINT_COMPARE_THREE_WAY_HPP
