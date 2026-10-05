/*******************************************************************************
* djinterp [re_std]                                       time_point_compare.hpp
*
* the six legacy time_point comparison operators:
*   ==, !=, <, <=, > and >= between two points on the SAME clock, at
* possibly different precisions.
*
*   Each compares the offsets from the shared epoch, so a millisecond
* point and a second point compare correctly against each other. Points
* on different clocks do not match these templates -- the single Clock
* parameter appears in both arguments -- so mixed-clock comparison is a
* compile error rather than a comparison of unrelated epochs.
*
*   Only == and < carry logic; the other four reflect through them.
*
*   As with duration, all six are provided on every tier including C++20,
* where std synthesises them from operator<=>. See
* duration_compare.hpp's header for the reasoning; the three-way overload
* lives in time_point_compare_three_way.hpp.
*
*
* path:      /inc/re_std/chrono/time_point_compare.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_TIME_POINT_COMPARE_HPP
#define RE_STD_CHRONO_TIME_POINT_COMPARE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./time_point.hpp"
#include "./duration_compare.hpp"


namespace re_std
{

namespace chrono
{

    // operator==
    //   function: the same instant, whatever the precisions.
    template<typename Clock, typename Duration1, typename Duration2>
    RE_STD_CONSTEXPR bool operator==(const time_point<Clock, Duration1>& _lhs,
                                const time_point<Clock, Duration2>& _rhs)
    {
        return _lhs.time_since_epoch() == _rhs.time_since_epoch();
    }

    // operator<
    //   function: earlier.
    template<typename Clock, typename Duration1, typename Duration2>
    RE_STD_CONSTEXPR bool operator<(const time_point<Clock, Duration1>& _lhs,
                               const time_point<Clock, Duration2>& _rhs)
    {
        return _lhs.time_since_epoch() < _rhs.time_since_epoch();
    }

    // operator!=
    //   function: reflected through ==.
    template<typename Clock, typename Duration1, typename Duration2>
    RE_STD_CONSTEXPR bool operator!=(const time_point<Clock, Duration1>& _lhs,
                                const time_point<Clock, Duration2>& _rhs)
    {
        return !(_lhs == _rhs);
    }

    // operator<=
    //   function: reflected through <.
    template<typename Clock, typename Duration1, typename Duration2>
    RE_STD_CONSTEXPR bool operator<=(const time_point<Clock, Duration1>& _lhs,
                                const time_point<Clock, Duration2>& _rhs)
    {
        return !(_rhs < _lhs);
    }

    // operator>
    //   function: reflected through <.
    template<typename Clock, typename Duration1, typename Duration2>
    RE_STD_CONSTEXPR bool operator>(const time_point<Clock, Duration1>& _lhs,
                               const time_point<Clock, Duration2>& _rhs)
    {
        return _rhs < _lhs;
    }

    // operator>=
    //   function: reflected through <.
    template<typename Clock, typename Duration1, typename Duration2>
    RE_STD_CONSTEXPR bool operator>=(const time_point<Clock, Duration1>& _lhs,
                                const time_point<Clock, Duration2>& _rhs)
    {
        return !(_lhs < _rhs);
    }

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_TIME_POINT_COMPARE_HPP
