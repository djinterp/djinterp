/*******************************************************************************
* djinterp [cpp][math]                                                values.hpp
*
* Compile-time sampling of an expression.
*   Evaluates a value-holding expression (expression.hpp) at a sequence of
* points and stores the results in a std::array available at compile time.
* This replaces the former static-`evaluate` sampling: expressions are now
* instances called through operator(), so sampling takes an expression value.
*
*   sample<Count>(e, start, step)  - e at start, start+step, ... (Count points)
*   sample_over<Interval>(e)       - e at each of an interval's members
*
* LEVELS (step 30, 5b)
*   C++11 is the floor, as for expression.hpp, and the header is empty below
* it. sample is from C++11, a constant expression wherever its expression's
* evaluation is. sample_over is from C++14: its array's size is the
* interval's size(), and its points the interval's at(), constant
* expressions from C++14, where the C core's count is.
*
*
* path:      /inc/djinterp/cpp/modules/math/values.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.04
*                                                            revised: 2026.11.22
*******************************************************************************/

#ifndef DJINTERP_MATH_VALUES_HPP
#define DJINTERP_MATH_VALUES_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <array>    // std::array
#include <cstddef>  // std::size_t
// djinterp
#include "./expression.hpp"            // expressions
#include "./interval/interval.hpp"     // is_interval
// re_std
#include "../../../../re_std/cpp/utility/integer_sequence.hpp"       // index_
                                                                     // sequence
#include "../../../../re_std/cpp/utility/make_integer_sequence.hpp"  // make_
                                                                     // index_
                                                                     // sequence


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    SAMPLING
// ============================================================================

NS_INTERNAL

    // sample_impl
    //   helper: builds the array by evaluating _e at start + i*step for each
    // index i in the pack (functor-free, one return, so that it is a constant
    // expression from C++11).
    template<typename _Expr, std::size_t... _Is>
    D_CONSTEXPR std::array<double, sizeof...(_Is)>
    sample_impl(const _Expr& _e, double _start, double _step,
                re_std::index_sequence<_Is...>)
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
D_CONSTEXPR
std::array<double, _Count> sample(
    const _Expr& _e,
    double       _start,
    double       _step = 1.0
)
{
    return internal::sample_impl(_e, _start, _step,
                                 re_std::make_index_sequence<_Count>{});
}

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_INTERNAL

    // sample_members_impl
    //   helper: builds the array by evaluating _e at the interval's member
    // at(i) for each index i in the pack.
    template<typename _Interval, typename _Expr, std::size_t... _Is>
    D_CONSTEXPR std::array<double, sizeof...(_Is)>
    sample_members_impl(const _Expr& _e, re_std::index_sequence<_Is...>)
    {
        return {{
            static_cast<double>(
                _e(static_cast<double>(_Interval::at(_Is))))...
        }};
    }

NS_END  // internal

// sample_over
//   evaluates _e at each of an interval's members, at(0) to at(size() - 1):
// from its first member (past an open lower bound), by its step when
// discrete and by 1 otherwise.
template<typename _Interval, typename _Expr>
D_CONSTEXPR std::array<double, _Interval::size()>
sample_over(const _Expr& _e)
{
    static_assert(is_interval<_Interval>::value,
                  "sample_over: _Interval must be an interval type.");

    return internal::sample_members_impl<_Interval>(
        _e,
        re_std::make_index_sequence<_Interval::size()>{});
}

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_MATH_VALUES_HPP
