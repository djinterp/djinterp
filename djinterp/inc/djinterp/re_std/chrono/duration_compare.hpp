/******************************************************************************
* djinterp [re_std]                                         duration_compare.hpp
*
* the six legacy duration comparison operators:
*   ==, !=, <, <=, > and >=, across mixed periods.
*
*   COMPARISON CONVERTS BOTH SIDES FIRST:
*   `seconds(1) == milliseconds(1000)` is TRUE. Both operands convert to
* the common period before the counts are compared, so the question asked
* is "are these the same length of time", not "are these the same value in
* the same unit". Anything else would make == disagree with - and the
* arithmetic operators.
*
*   ONLY == AND < CARRY LOGIC:
*   The other four reflect through them, which is the usual reason -- one
* place to be correct -- and one specific to durations: the conversion to
* the common type happens once per operator, so keeping the count of
* real implementations at two keeps the count of places a conversion
* could be written wrong at two.
*
*   PROVIDED ON EVERY TIER, INCLUDING C++20:
*   In C++20 std drops the four ordering operators and !=, synthesising
* them from operator<=> ([time.duration.comparisons]). re_std ships all
* six explicitly at every tier, matching the choice made for array and
* tuple: on C++11 through C++17 they are the only way to compare, and on
* C++20 they coexist with the three-way overload in
* duration_compare_three_way.hpp. A non-rewritten candidate is preferred
* over a rewritten one, so the explicit operators simply win and no
* ambiguity arises.
*
*   CONSTEXPR from C++11, matching std.
*
*
* path:      /inc/djinterp/re_std/chrono/duration_compare.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_DURATION_COMPARE_
#define DJINTERP_RE_STD_CHRONO_DURATION_COMPARE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration.hpp"
#include "./duration_common_type.hpp"
#include "../type_traits/common_type.hpp"


NS_RESTD

namespace chrono
{

    // operator==
    //   function: same length of time, whatever the periods.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR bool operator==(const duration<_Rep1, _Period1>& _lhs,
                                const duration<_Rep2, _Period2>& _rhs)
    {
        typedef typename common_type< duration<_Rep1, _Period1>,
                                      duration<_Rep2, _Period2> >::type _CD;
        return _CD(_lhs).count() == _CD(_rhs).count();
    }

    // operator<
    //   function: shorter length of time.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR bool operator<(const duration<_Rep1, _Period1>& _lhs,
                               const duration<_Rep2, _Period2>& _rhs)
    {
        typedef typename common_type< duration<_Rep1, _Period1>,
                                      duration<_Rep2, _Period2> >::type _CD;
        return _CD(_lhs).count() < _CD(_rhs).count();
    }

    // operator!=
    //   function: reflected through ==.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR bool operator!=(const duration<_Rep1, _Period1>& _lhs,
                                const duration<_Rep2, _Period2>& _rhs)
    {
        return !(_lhs == _rhs);
    }

    // operator<=
    //   function: reflected through <.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR bool operator<=(const duration<_Rep1, _Period1>& _lhs,
                                const duration<_Rep2, _Period2>& _rhs)
    {
        return !(_rhs < _lhs);
    }

    // operator>
    //   function: reflected through <.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR bool operator>(const duration<_Rep1, _Period1>& _lhs,
                               const duration<_Rep2, _Period2>& _rhs)
    {
        return _rhs < _lhs;
    }

    // operator>=
    //   function: reflected through <.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR bool operator>=(const duration<_Rep1, _Period1>& _lhs,
                                const duration<_Rep2, _Period2>& _rhs)
    {
        return !(_lhs < _rhs);
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_DURATION_COMPARE_
