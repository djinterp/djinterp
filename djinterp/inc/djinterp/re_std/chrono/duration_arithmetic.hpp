/******************************************************************************
* djinterp [re_std]                                      duration_arithmetic.hpp
*
* the duration arithmetic operators:
*   +, -, *, / and % across mixed periods and against scalars.
*
*   MIXED-PERIOD ARITHMETIC GOES THROUGH THE COMMON TYPE:
*   `seconds(1) + milliseconds(5)` is not seconds and is not
* milliseconds -- it is the finest period that represents both exactly,
* which common_type computes. Both operands convert into it first, so the
* addition happens in one unit and nothing truncates. That is why this
* header depends on duration_common_type.hpp.
*
*   DIVIDING TWO DURATIONS YIELDS A NUMBER, NOT A DURATION:
*   seconds / seconds is a dimensionless ratio -- "how many times does
* this fit into that" -- so the result is the common REPRESENTATION type.
* This is the one operator in the set whose return type is not a
* duration, and it is dimensional analysis working correctly rather than
* an inconsistency.
*
*   THE SCALAR OVERLOADS MUST EXCLUDE DURATIONS EXPLICITLY:
*   `operator/(duration, Rep2)` and `operator/(duration, duration)` are
* both viable for `d / d` unless the first is constrained, and the
* scalar form would win by being a better match on the second argument.
* The enable_if on !is_duration<_Rep2> is what keeps the dimensional
* result from being silently replaced by a duration one.
*
*   MULTIPLICATION IS OFFERED BOTH WAYS ROUND (`d * 3` and `3 * d`)
* because scaling is commutative and requiring one order would be an
* arbitrary papercut. Division is not offered as `3 / d`: a number
* divided by a duration is a frequency, which this library has no type
* for.
*
*   CONSTEXPR: all of these are constexpr from C++11, matching std.
*
*
* path:      /inc/djinterp/re_std/chrono/duration_arithmetic.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_DURATION_ARITHMETIC_
#define DJINTERP_RE_STD_CHRONO_DURATION_ARITHMETIC_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration.hpp"
#include "./duration_common_type.hpp"
#include "../type_traits/common_type.hpp"
#include "../type_traits/enable_if.hpp"


NS_RESTD

namespace chrono
{

// ===========================================================================
// I.   DURATION AGAINST DURATION
// ===========================================================================

    // operator+
    //   function: sum, in the finest period that represents both operands.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR
    typename common_type< duration<_Rep1, _Period1>,
                          duration<_Rep2, _Period2> >::type
    operator+(const duration<_Rep1, _Period1>& _lhs,
              const duration<_Rep2, _Period2>& _rhs)
    {
        typedef typename common_type< duration<_Rep1, _Period1>,
                                      duration<_Rep2, _Period2> >::type _CD;
        return _CD(_CD(_lhs).count() + _CD(_rhs).count());
    }

    // operator-
    //   function: difference, in the finest period that represents both.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR
    typename common_type< duration<_Rep1, _Period1>,
                          duration<_Rep2, _Period2> >::type
    operator-(const duration<_Rep1, _Period1>& _lhs,
              const duration<_Rep2, _Period2>& _rhs)
    {
        typedef typename common_type< duration<_Rep1, _Period1>,
                                      duration<_Rep2, _Period2> >::type _CD;
        return _CD(_CD(_lhs).count() - _CD(_rhs).count());
    }

    // operator/
    //   function: how many times _rhs fits into _lhs. Returns the common
    // REPRESENTATION -- a dimensionless number, not a duration.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR
    typename common_type<_Rep1, _Rep2>::type
    operator/(const duration<_Rep1, _Period1>& _lhs,
              const duration<_Rep2, _Period2>& _rhs)
    {
        typedef typename common_type< duration<_Rep1, _Period1>,
                                      duration<_Rep2, _Period2> >::type _CD;
        return _CD(_lhs).count() / _CD(_rhs).count();
    }

    // operator%
    //   function: remainder, in the finest period that represents both.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR
    typename common_type< duration<_Rep1, _Period1>,
                          duration<_Rep2, _Period2> >::type
    operator%(const duration<_Rep1, _Period1>& _lhs,
              const duration<_Rep2, _Period2>& _rhs)
    {
        typedef typename common_type< duration<_Rep1, _Period1>,
                                      duration<_Rep2, _Period2> >::type _CD;
        return _CD(_CD(_lhs).count() % _CD(_rhs).count());
    }


// ===========================================================================
// II.  DURATION AGAINST A SCALAR
// ===========================================================================

    // operator*
    //   function: scale up. The period is unchanged; only the count moves.
    template<typename _Rep1, typename _Period, typename _Rep2>
    D_CONSTEXPR
    duration<typename common_type<_Rep1, _Rep2>::type, _Period>
    operator*(const duration<_Rep1, _Period>& _d, const _Rep2& _s)
    {
        typedef duration<typename common_type<_Rep1, _Rep2>::type,
                         _Period> _CD;
        return _CD(_CD(_d).count() * static_cast<typename _CD::rep>(_s));
    }

    // operator*
    //   function: scale up, scalar on the left. Same operation -- offered
    // because `3 * d` reads naturally and refusing it would be arbitrary.
    template<typename _Rep1, typename _Rep2, typename _Period>
    D_CONSTEXPR
    duration<typename common_type<_Rep1, _Rep2>::type, _Period>
    operator*(const _Rep1& _s, const duration<_Rep2, _Period>& _d)
    {
        return _d * _s;
    }

    // operator/
    //   function: scale down. Constrained on _Rep2 NOT being a duration,
    // which is what keeps duration / duration reaching the dimensional
    // overload above.
    template<typename _Rep1, typename _Period, typename _Rep2>
    D_CONSTEXPR
    typename enable_if<
        !internal::is_duration<_Rep2>::value,
        duration<typename common_type<_Rep1, _Rep2>::type, _Period> >::type
    operator/(const duration<_Rep1, _Period>& _d, const _Rep2& _s)
    {
        typedef duration<typename common_type<_Rep1, _Rep2>::type,
                         _Period> _CD;
        return _CD(_CD(_d).count() / static_cast<typename _CD::rep>(_s));
    }

    // operator%
    //   function: remainder against a scalar. Constrained for the same
    // reason as the scalar operator/.
    template<typename _Rep1, typename _Period, typename _Rep2>
    D_CONSTEXPR
    typename enable_if<
        !internal::is_duration<_Rep2>::value,
        duration<typename common_type<_Rep1, _Rep2>::type, _Period> >::type
    operator%(const duration<_Rep1, _Period>& _d, const _Rep2& _s)
    {
        typedef duration<typename common_type<_Rep1, _Rep2>::type,
                         _Period> _CD;
        return _CD(_CD(_d).count() % static_cast<typename _CD::rep>(_s));
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_DURATION_ARITHMETIC_
