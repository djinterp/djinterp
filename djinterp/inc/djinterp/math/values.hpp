/******************************************************************************
* djinterp [math]                                                   values.hpp
*
* Compile-time function-value generation over an interval.
*   Harvested from the former math.hpp during consolidation. Samples an
* expression at the integral points of an interval and stores the results
* in a std::array, available at compile time.
*
* NOTE (interval API):
*   The original math.hpp implementation referenced an interval interface
* (first(), ::size, ::is_empty) that does not match the current interval
* headers. This version is adapted to the closed_interval / discrete_interval
* surface (static lower_bound, static size()). It assumes unit-step sampling
* from lower_bound; sampling a strided discrete_interval correctly requires
* dispatching through _Interval::at(index) and is left as a follow-up.
*
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
#include "../core/djinterp.hpp"
#include "./math.hpp"
#include "./expression.hpp"
#include "./interval.hpp"


NS_DJINTERP  // djinterp
NS_MATH      // math

// ===========================================================================
// I.   Value Generation
// ===========================================================================

NS_INTERNAL

    // values_helper
    //   helper: generates an array of evaluated function values from an
    // index sequence.
    template<typename _OutputType,
             typename _Function,
             typename _Interval,
             typename _Seq>
    struct values_helper;

    // values_helper (index-sequence specialization)
    //   helper: evaluates _Function at lower_bound + index for each index.
    template<typename       _OutputType,
             typename       _Function,
             typename       _Interval,
             std::size_t... _Indices>
    struct values_helper<_OutputType,
                         _Function,
                         _Interval,
                         std::index_sequence<_Indices...>>
    {
        using value_type = _OutputType;

        static constexpr std::size_t count = sizeof...(_Indices);

        static constexpr std::array<value_type, count>
        make() noexcept
        {
            return {{
                static_cast<value_type>(
                    _Function::evaluate(_Interval::lower_bound + _Indices)
                )...
            }};
        }
    };

NS_END  // internal

// values
//   struct: generates an array of function values over an interval,
// evaluating the function at each integral sample point.
template<typename _OutputType,
         typename _Function,
         typename _Interval>
struct values
{
    static_assert((_Interval::size() > 0),
                  "values: cannot generate values from an empty interval.");

    using value_type = _OutputType;

    static constexpr std::size_t count = _Interval::size();

private:
    using helper = internal::values_helper<
        _OutputType,
        _Function,
        _Interval,
        std::make_index_sequence<count>>;

public:
    static constexpr std::array<value_type, count> array = helper::make();

    static constexpr value_type
    at(std::size_t _index) noexcept
    {
        return array[_index];
    }

    static constexpr std::size_t
    size() noexcept
    {
        return count;
    }
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// values_v
//   variable template: array of function values over an interval.
template<typename _OutputType,
         typename _Function,
         typename _Interval>
inline constexpr auto values_v =
    values<_OutputType, _Function, _Interval>::array;
#endif


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_VALUES_