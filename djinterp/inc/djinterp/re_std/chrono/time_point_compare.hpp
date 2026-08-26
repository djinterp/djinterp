/******************************************************************************
* djinterp [re_std]                                       time_point_compare.hpp
*
* the six legacy time_point comparison operators:
*   ==, !=, <, <=, > and >= between two points on the SAME clock, at
* possibly different precisions.
*
*   Each compares the offsets from the shared epoch, so a millisecond
* point and a second point compare correctly against each other. Points
* on different clocks do not match these templates -- the single _Clock
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
* path:      /inc/djinterp/re_std/chrono/time_point_compare.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_TIME_POINT_COMPARE_
#define DJINTERP_RE_STD_CHRONO_TIME_POINT_COMPARE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./time_point.hpp"
#include "./duration_compare.hpp"


NS_RESTD

namespace chrono
{

    // operator==
    //   function: the same instant, whatever the precisions.
    template<typename _Clock, typename _Duration1, typename _Duration2>
    D_CONSTEXPR bool operator==(const time_point<_Clock, _Duration1>& _lhs,
                                const time_point<_Clock, _Duration2>& _rhs)
    {
        return _lhs.time_since_epoch() == _rhs.time_since_epoch();
    }

    // operator<
    //   function: earlier.
    template<typename _Clock, typename _Duration1, typename _Duration2>
    D_CONSTEXPR bool operator<(const time_point<_Clock, _Duration1>& _lhs,
                               const time_point<_Clock, _Duration2>& _rhs)
    {
        return _lhs.time_since_epoch() < _rhs.time_since_epoch();
    }

    // operator!=
    //   function: reflected through ==.
    template<typename _Clock, typename _Duration1, typename _Duration2>
    D_CONSTEXPR bool operator!=(const time_point<_Clock, _Duration1>& _lhs,
                                const time_point<_Clock, _Duration2>& _rhs)
    {
        return !(_lhs == _rhs);
    }

    // operator<=
    //   function: reflected through <.
    template<typename _Clock, typename _Duration1, typename _Duration2>
    D_CONSTEXPR bool operator<=(const time_point<_Clock, _Duration1>& _lhs,
                                const time_point<_Clock, _Duration2>& _rhs)
    {
        return !(_rhs < _lhs);
    }

    // operator>
    //   function: reflected through <.
    template<typename _Clock, typename _Duration1, typename _Duration2>
    D_CONSTEXPR bool operator>(const time_point<_Clock, _Duration1>& _lhs,
                               const time_point<_Clock, _Duration2>& _rhs)
    {
        return _rhs < _lhs;
    }

    // operator>=
    //   function: reflected through <.
    template<typename _Clock, typename _Duration1, typename _Duration2>
    D_CONSTEXPR bool operator>=(const time_point<_Clock, _Duration1>& _lhs,
                                const time_point<_Clock, _Duration2>& _rhs)
    {
        return !(_lhs < _rhs);
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_TIME_POINT_COMPARE_
