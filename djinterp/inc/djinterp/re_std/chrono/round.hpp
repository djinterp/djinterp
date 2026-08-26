/******************************************************************************
* djinterp [re_std]                                                    round.hpp
*
* chrono::round for durations and time_points:
*   Converts to a coarser precision, rounding to the NEAREST
* representable value.
*
*   TIES GO TO THE EVEN VALUE, NOT AWAY FROM ZERO:
*   round<seconds>(milliseconds(1500)) is 2s, and
* round<seconds>(milliseconds(2500)) is also 2s. This is banker's
* rounding, and it is what [time.duration.cast] specifies. The reason is
* statistical: always rounding halves up biases a long series of
* roundings upward, and for timestamp aggregation that bias accumulates
* into a visible drift. Ties-to-even has no such bias.
*
*   It does surprise people who expect the arithmetic they were taught,
* so it is worth stating rather than leaving to be discovered in a
* failing test.
*
*   FLOATING-POINT REPRESENTATIONS ARE EXCLUDED:
*   The target's rep must not be floating point -- the standard
* constrains it, and the constraint is meaningful: the tie test compares
* two distances for exact equality, which is not a question a
* floating-point count can answer reliably. The exclusion is SFINAE, so
* round<duration<double> > simply does not match rather than compiling
* into something unreliable.
*
*   IMPLEMENTED ON TOP OF floor, because the lower neighbour and the
* upper neighbour are floor's result and floor's result plus one tick.
*
*   BACK-PORT: C++17 in std, C++11 here. The C++11 form runs through
* internal helpers, since the natural body wants local variables.
*
*
* path:      /inc/djinterp/re_std/chrono/round.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_ROUND_
#define DJINTERP_RE_STD_CHRONO_ROUND_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration.hpp"
#include "./duration_arithmetic.hpp"
#include "./duration_compare.hpp"
#include "./floor.hpp"
#include "./time_point.hpp"
#include "./treat_as_floating_point.hpp"
#include "../type_traits/enable_if.hpp"


NS_RESTD

namespace chrono
{

NS_INTERNAL

    // round_tie
    //   function: resolve an exact tie toward the EVEN tick count.
    template<typename _To>
    D_CONSTEXPR _To round_tie(const _To& _lower, const _To& _upper)
    {
        return ((_lower.count() & 1) == 0) ? _lower : _upper;
    }

    // round_pick
    //   function: choose the nearer neighbour, deferring an exact tie to
    // round_tie.
    template<typename _To,
             typename _Diff>
    D_CONSTEXPR _To round_pick(const _To&   _lower,
                               const _To&   _upper,
                               const _Diff& _below,
                               const _Diff& _above)
    {
        return (_below == _above) ? round_tie(_lower, _upper)
                                  : ((_below < _above) ? _lower : _upper);
    }

    // round_from_floor
    //   function: given the lower neighbour, measure both distances and
    // pick. Split out so floor is evaluated once.
    template<typename _To,
             typename _Rep,
             typename _Period>
    D_CONSTEXPR _To round_from_floor(const _To&                     _lower,
                                     const duration<_Rep, _Period>& _d)
    {
        return round_pick(_lower,
                          _To(_lower.count() + 1),
                          _d - _lower,
                          _To(_lower.count() + 1) - _d);
    }

NS_END  // internal

    // round
    //   function: coarsen a duration to the nearest tick, ties to even.
    // Excluded for floating-point target representations.
    template<typename _To,
             typename _Rep,
             typename _Period>
    D_CONSTEXPR
    typename enable_if<
        internal::is_duration<_To>::value &&
        !treat_as_floating_point<typename _To::rep>::value,
        _To>::type
    round(const duration<_Rep, _Period>& _d)
    {
        return internal::round_from_floor(floor<_To>(_d), _d);
    }

    // round
    //   function: coarsen a time_point to the nearest tick, ties to even.
    template<typename _To,
             typename _Clock,
             typename _Duration>
    D_CONSTEXPR
    typename enable_if<
        internal::is_duration<_To>::value &&
        !treat_as_floating_point<typename _To::rep>::value,
        time_point<_Clock, _To> >::type
    round(const time_point<_Clock, _Duration>& _t)
    {
        return time_point<_Clock, _To>(round<_To>(_t.time_since_epoch()));
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_ROUND_
