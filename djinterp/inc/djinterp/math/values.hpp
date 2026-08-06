/******************************************************************************
* djinterp [math]                                                   values.hpp
*
* Compile-time sampling of an expression.
*   Evaluates a value-holding expression (expression.hpp) at a sequence of
* points and stores the results in a std::array available at compile time.
* This replaces the former static-`evaluate` sampling: expressions are now
* instances called through operator(), so sampling takes an expression value.
*
*   sample<Count>(e, start, step)  - e at start, start+step, ... (Count points)
*   sample_over<Interval>(e)       - e at an interval's sample points
*
* path:      /inc/djinterp/math/values.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.04
******************************************************************************/

#ifndef DJINTERP_MATH_VALUES_
#define DJINTERP_MATH_VALUES_ 1

// std
#include <array>
#include <cstddef>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./expression.hpp"
#include "./interval.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    SAMPLING
// ============================================================================

NS_INTERNAL

    // sample_impl
    //   helper: builds the array by evaluating _e at start + i*step for each
    // index i in the pack (functor-free so it stays constexpr in C++14).
    template<typename _Expr, std::size_t... _Is>
    D_CONSTEXPR std::array<double, sizeof...(_Is)>
    sample_impl(const _Expr& _e, double _start, double _step,
                std::index_sequence<_Is...>)
    {
        return {{
            static_cast<double>(
                _e(_start + static_cast<double>(_Is) * _step))...
        }};
    }

NS_END  // internal

// sample
//   evaluates _e at _Count points beginning at _start, spaced by _step, and
// returns the results as a std::array<double, _Count>.
template<std::size_t _Count, typename _Expr>
D_CONSTEXPR std::array<double, _Count>
sample(const _Expr& _e, double _start, double _step = 1.0)
{
    return internal::sample_impl(_e, _start, _step,
                                 std::make_index_sequence<_Count>{});
}

// sample_over
//   evaluates _e at the sample points of an interval: lower_bound + i * step,
// where step is the interval's step when discrete and 1 otherwise, for each of
// the interval's size() positions.
template<typename _Interval, typename _Expr>
D_CONSTEXPR std::array<double, _Interval::size()>
sample_over(const _Expr& _e)
{
    static_assert(is_interval<_Interval>::value,
                  "sample_over: _Interval must be an interval type.");

    constexpr double step =
        (_Interval::step > static_cast<decltype(_Interval::step)>(0))
            ? static_cast<double>(_Interval::step)
            : 1.0;

    return internal::sample_impl(
        _e,
        static_cast<double>(_Interval::lower_bound),
        step,
        std::make_index_sequence<_Interval::size()>{});
}

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_VALUES_
