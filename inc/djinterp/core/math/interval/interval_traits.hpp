/******************************************************************************
* djinterp [maths]                                      interval_traits.hpp
*
* SFINAE type traits for interval types.
*   This header provides compile-time detection of interval types and their
* properties using pure structural SFINAE (no tag types required).
*
* NAMING CONVENTIONS:
*   is_<pattern>             - primary trait (inherits from bool_constant)
*   is_<pattern>_v           - variable template helper
*
* DETECTED PATTERNS:
*   - Interval detection (is_interval)
*   - Boundary type detection (is_open, is_closed, is_half_open)
*   - Endpoint detection (is_left_open, is_right_open, etc.)
*   - Interval property detection (is_bounded, is_empty, is_degenerate)
*   - Discrete interval detection (is_discrete_interval)
*
* STRUCTURAL REQUIREMENTS FOR INTERVALS:
*   An interval type must have:
*     - static constexpr lower_bound
*     - static constexpr upper_bound
*     - static constexpr bool is_left_open
*     - static constexpr bool is_right_open
*
*   A discrete interval additionally has:
*     - static constexpr step
*
* path:      /inc/maths/interval_traits.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2024.04.24
******************************************************************************/

#ifndef DJINTERP_MATHS_INTERVAL_TRAITS_
#define DJINTERP_MATHS_INTERVAL_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "../env.h"
#include "../cpp_features.h"
#include "../djinterp.h"


NS_DJINTERP
NS_MATHS

// ============================================================================
// I.    DETECTION HELPERS
// ============================================================================

NS_INTERNAL

    // has_lower_bound
    //   helper: detects lower_bound static member.
    template<typename _Type,
             typename = void>
    struct has_lower_bound : std::false_type
    {
    };

    template<typename _Type>
    struct has_lower_bound<_Type, void_t<decltype(_Type::lower_bound)>>
        : std::true_type
    {
    };

    // has_upper_bound
    //   helper: detects upper_bound static member.
    template<typename _Type,
             typename = void>
    struct has_upper_bound : std::false_type
    {
    };

    template<typename _Type>
    struct has_upper_bound<_Type, void_t<decltype(_Type::upper_bound)>>
        : std::true_type
    {
    };

    // has_is_left_open
    //   helper: detects is_left_open static bool member.
    template<typename _Type,
             typename = void>
    struct has_is_left_open : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_left_open<_Type, std::enable_if_t<
        ( std::is_same<decltype(_Type::is_left_open),
                       const bool>::value ||
          std::is_same<decltype(_Type::is_left_open),
                       bool>::value )
    >> : std::true_type
    {
    };

    // has_is_right_open
    //   helper: detects is_right_open static bool member.
    template<typename _Type,
             typename = void>
    struct has_is_right_open : std::false_type
    {
    };

    template<typename _Type>
    struct has_is_right_open<_Type, std::enable_if_t<
        ( std::is_same<decltype(_Type::is_right_open),
                       const bool>::value ||
          std::is_same<decltype(_Type::is_right_open),
                       bool>::value )
    >> : std::true_type
    {
    };

    // has_step
    //   helper: detects step static member (for discrete intervals).
    template<typename _Type,
             typename = void>
    struct has_step : std::false_type
    {
    };

    template<typename _Type>
    struct has_step<_Type, void_t<decltype(_Type::step)>>
        : std::true_type
    {
    };

    // interval_structural_check
    //   helper: combines all structural requirements for interval
    // detection.
    template<typename _Type,
             typename = void>
    struct interval_structural_check : std::false_type
    {
    };

    template<typename _Type>
    struct interval_structural_check<_Type, std::enable_if_t<
        ( has_lower_bound<_Type>::value  &&
          has_upper_bound<_Type>::value  &&
          has_is_left_open<_Type>::value &&
          has_is_right_open<_Type>::value )
    >> : std::true_type
    {
    };

    // discrete_interval_structural_check
    //   helper: combines structural requirements for discrete interval
    // detection.
    template<typename _Type,
             typename = void>
    struct discrete_interval_structural_check : std::false_type
    {
    };

    template<typename _Type>
    struct discrete_interval_structural_check<_Type, std::enable_if_t<
        ( interval_structural_check<_Type>::value &&
          has_step<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal


// ============================================================================
// II.   INTERVAL DETECTION
// ============================================================================

// is_interval
//   trait: checks if _Type is an interval type.
// An interval type must have lower_bound, upper_bound, is_left_open,
// and is_right_open static members.
template<typename _Type,
         typename = void>
struct is_interval : std::false_type
{
};

template<typename _Type>
struct is_interval<_Type, std::enable_if_t<
    internal::interval_structural_check<_Type>::value
>> : std::true_type
{
};

// is_discrete_interval
//   trait: checks if _Type is a discrete interval type.
// A discrete interval is an interval that additionally has a step
// static member.
template<typename _Type,
         typename = void>
struct is_discrete_interval : std::false_type
{
};

template<typename _Type>
struct is_discrete_interval<_Type, std::enable_if_t<
    internal::discrete_interval_structural_check<_Type>::value
>> : std::true_type
{
};

// is_continuous_interval
//   trait: checks if _Type is a non-discrete interval.
template<typename _Type,
         typename = void>
struct is_continuous_interval : std::false_type
{
};

template<typename _Type>
struct is_continuous_interval<_Type, std::enable_if_t<
    ( is_interval<_Type>::value &&
      !is_discrete_interval<_Type>::value )
>> : std::true_type
{
};


// ============================================================================
// III.  BOUNDARY TYPE DETECTION
// ============================================================================

NS_INTERNAL

    // is_closed_check
    //   helper: checks closed boundary condition.
    template<typename _Type,
             typename = void>
    struct is_closed_check : std::false_type
    {
    };

    template<typename _Type>
    struct is_closed_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          (!_Type::is_left_open)    &&
          (!_Type::is_right_open) )
    >> : std::true_type
    {
    };

    // is_open_check
    //   helper: checks open boundary condition.
    template<typename _Type,
             typename = void>
    struct is_open_check : std::false_type
    {
    };

    template<typename _Type>
    struct is_open_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          _Type::is_left_open       &&
          _Type::is_right_open )
    >> : std::true_type
    {
    };

    // is_half_open_check
    //   helper: checks half-open boundary condition.
    template<typename _Type,
             typename = void>
    struct is_half_open_check : std::false_type
    {
    };

    template<typename _Type>
    struct is_half_open_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          (_Type::is_left_open != _Type::is_right_open) )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_closed
//   trait: checks if interval has both endpoints closed (inclusive).
template<typename _Type>
struct is_closed : internal::is_closed_check<_Type>
{
};

// is_open
//   trait: checks if interval has both endpoints open (exclusive).
template<typename _Type>
struct is_open : internal::is_open_check<_Type>
{
};

// is_half_open
//   trait: checks if interval has exactly one open endpoint.
template<typename _Type>
struct is_half_open : internal::is_half_open_check<_Type>
{
};


// ============================================================================
// IV.   ENDPOINT DETECTION
// ============================================================================

NS_INTERNAL

    // left_open_check
    //   helper: checks left endpoint is open.
    template<typename _Type,
             typename = void>
    struct left_open_check : std::false_type
    {
    };

    template<typename _Type>
    struct left_open_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          _Type::is_left_open )
    >> : std::true_type
    {
    };

    // right_open_check
    //   helper: checks right endpoint is open.
    template<typename _Type,
             typename = void>
    struct right_open_check : std::false_type
    {
    };

    template<typename _Type>
    struct right_open_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          _Type::is_right_open )
    >> : std::true_type
    {
    };

    // left_closed_check
    //   helper: checks left endpoint is closed.
    template<typename _Type,
             typename = void>
    struct left_closed_check : std::false_type
    {
    };

    template<typename _Type>
    struct left_closed_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          (!_Type::is_left_open) )
    >> : std::true_type
    {
    };

    // right_closed_check
    //   helper: checks right endpoint is closed.
    template<typename _Type,
             typename = void>
    struct right_closed_check : std::false_type
    {
    };

    template<typename _Type>
    struct right_closed_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          (!_Type::is_right_open) )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_left_open
//   trait: checks if interval has left endpoint open.
template<typename _Type>
struct is_left_open : internal::left_open_check<_Type>
{
};

// is_right_open
//   trait: checks if interval has right endpoint open.
template<typename _Type>
struct is_right_open : internal::right_open_check<_Type>
{
};

// is_left_closed
//   trait: checks if interval has left endpoint closed.
template<typename _Type>
struct is_left_closed : internal::left_closed_check<_Type>
{
};

// is_right_closed
//   trait: checks if interval has right endpoint closed.
template<typename _Type>
struct is_right_closed : internal::right_closed_check<_Type>
{
};


// ============================================================================
// V.    INTERVAL PROPERTY DETECTION
// ============================================================================

// is_bounded_interval
//   trait: checks if interval has finite bounds.
// Note: true for all compile-time intervals (bounds must be specified).
template<typename _Type>
struct is_bounded_interval : is_interval<_Type>
{
};

NS_INTERNAL

    // empty_interval_check
    //   helper: checks if interval is empty.
    template<typename _Type,
             typename = void>
    struct empty_interval_check : std::false_type
    {
    };

    template<typename _Type>
    struct empty_interval_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value                         &&
          ((_Type::lower_bound > _Type::upper_bound)        ||
           ((_Type::lower_bound == _Type::upper_bound)      &&
            (_Type::is_left_open || _Type::is_right_open))) )
    >> : std::true_type
    {
    };

    // degenerate_interval_check
    //   helper: checks if interval contains exactly one element.
    template<typename _Type,
             typename = void>
    struct degenerate_interval_check : std::false_type
    {
    };

    template<typename _Type>
    struct degenerate_interval_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value                       &&
          (_Type::lower_bound == _Type::upper_bound)      &&
          (!_Type::is_left_open)                          &&
          (!_Type::is_right_open) )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_empty_interval
//   trait: checks if interval contains no elements.
// Empty when: lower > upper, or lower == upper and either endpoint
// is open.
template<typename _Type>
struct is_empty_interval : internal::empty_interval_check<_Type>
{
};

// is_degenerate_interval
//   trait: checks if interval contains exactly one element.
// Degenerate when: lower == upper and both endpoints are closed.
template<typename _Type>
struct is_degenerate_interval
    : internal::degenerate_interval_check<_Type>
{
};

NS_INTERNAL

    // proper_interval_check
    //   helper: checks if interval is non-empty.
    template<typename _Type,
             typename = void>
    struct proper_interval_check : std::false_type
    {
    };

    template<typename _Type>
    struct proper_interval_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value       &&
          !is_empty_interval<_Type>::value )
    >> : std::true_type
    {
    };

NS_END  // internal

// is_proper_interval
//   trait: checks if interval is non-empty.
template<typename _Type>
struct is_proper_interval : internal::proper_interval_check<_Type>
{
};


// ============================================================================
// VI.   INTERVAL TYPE EXTRACTION
// ============================================================================

NS_INTERNAL

    // interval_value_type_helper
    //   helper: extracts value_type from interval if present.
    template<typename _Type,
             typename = void>
    struct interval_value_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct interval_value_type_helper<_Type,
                                     void_t<typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    // interval_size_type_helper
    //   helper: extracts size_type from interval if present.
    template<typename _Type,
             typename = void>
    struct interval_size_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct interval_size_type_helper<_Type,
                                    void_t<typename _Type::size_type>>
    {
        using type = typename _Type::size_type;
    };

NS_END  // internal

// interval_value_type
//   trait: extracts the value type from an interval type.
template<typename _Type>
struct interval_value_type
{
    using type = typename internal::interval_value_type_helper<_Type>::type;
};

// interval_value_type_t
//   type: shorthand for interval_value_type<_Type>::type.
template<typename _Type>
using interval_value_type_t =
    typename interval_value_type<_Type>::type;

// interval_size_type
//   trait: extracts the size type from an interval type.
template<typename _Type>
struct interval_size_type
{
    using type = typename internal::interval_size_type_helper<_Type>::type;
};

// interval_size_type_t
//   type: shorthand for interval_size_type<_Type>::type.
template<typename _Type>
using interval_size_type_t =
    typename interval_size_type<_Type>::type;


// ============================================================================
// VII.  INTERVAL RELATIONSHIP TRAITS
// ============================================================================

NS_INTERNAL

    // intervals_same_type_check
    //   helper: checks if two intervals have compatible value types.
    template<typename _Interval1,
             typename _Interval2,
             typename = void>
    struct intervals_same_type_check : std::false_type
    {
    };

    template<typename _Interval1,
             typename _Interval2>
    struct intervals_same_type_check<_Interval1,
                                    _Interval2,
                                    std::enable_if_t<
        ( is_interval<_Interval1>::value &&
          is_interval<_Interval2>::value &&
          std::is_same<
              interval_value_type_t<_Interval1>,
              interval_value_type_t<_Interval2>
          >::value )
    >> : std::true_type
    {
    };

    // intervals_same_boundary_check
    //   helper: checks if two intervals have same boundary
    // configuration.
    template<typename _Interval1,
             typename _Interval2,
             typename = void>
    struct intervals_same_boundary_check : std::false_type
    {
    };

    template<typename _Interval1,
             typename _Interval2>
    struct intervals_same_boundary_check<_Interval1,
                                        _Interval2,
                                        std::enable_if_t<
        ( is_interval<_Interval1>::value                                 &&
          is_interval<_Interval2>::value                                 &&
          (_Interval1::is_left_open  == _Interval2::is_left_open)       &&
          (_Interval1::is_right_open == _Interval2::is_right_open) )
    >> : std::true_type
    {
    };

NS_END  // internal

// intervals_same_type
//   trait: checks if two intervals have the same value type.
template<typename _Interval1,
         typename _Interval2>
struct intervals_same_type
    : internal::intervals_same_type_check<_Interval1, _Interval2>
{
};

// intervals_same_boundary_type
//   trait: checks if two intervals have the same boundary type
// (open/closed).
template<typename _Interval1,
         typename _Interval2>
struct intervals_same_boundary_type
    : internal::intervals_same_boundary_check<_Interval1, _Interval2>
{
};


// ============================================================================
// VIII. VARIABLE TEMPLATES
// ============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_interval_v
    //   variable template: value helper for is_interval.
    template<typename _Type>
    inline constexpr bool is_interval_v =
        is_interval<_Type>::value;

    // is_discrete_interval_v
    //   variable template: value helper for is_discrete_interval.
    template<typename _Type>
    inline constexpr bool is_discrete_interval_v =
        is_discrete_interval<_Type>::value;

    // is_continuous_interval_v
    //   variable template: value helper for is_continuous_interval.
    template<typename _Type>
    inline constexpr bool is_continuous_interval_v =
        is_continuous_interval<_Type>::value;

    // is_closed_v
    //   variable template: value helper for is_closed.
    template<typename _Type>
    inline constexpr bool is_closed_v =
        is_closed<_Type>::value;

    // is_open_v
    //   variable template: value helper for is_open.
    template<typename _Type>
    inline constexpr bool is_open_v =
        is_open<_Type>::value;

    // is_half_open_v
    //   variable template: value helper for is_half_open.
    template<typename _Type>
    inline constexpr bool is_half_open_v =
        is_half_open<_Type>::value;

    // is_left_open_v
    //   variable template: value helper for is_left_open.
    template<typename _Type>
    inline constexpr bool is_left_open_v =
        is_left_open<_Type>::value;

    // is_right_open_v
    //   variable template: value helper for is_right_open.
    template<typename _Type>
    inline constexpr bool is_right_open_v =
        is_right_open<_Type>::value;

    // is_left_closed_v
    //   variable template: value helper for is_left_closed.
    template<typename _Type>
    inline constexpr bool is_left_closed_v =
        is_left_closed<_Type>::value;

    // is_right_closed_v
    //   variable template: value helper for is_right_closed.
    template<typename _Type>
    inline constexpr bool is_right_closed_v =
        is_right_closed<_Type>::value;

    // is_bounded_interval_v
    //   variable template: value helper for is_bounded_interval.
    template<typename _Type>
    inline constexpr bool is_bounded_interval_v =
        is_bounded_interval<_Type>::value;

    // is_empty_interval_v
    //   variable template: value helper for is_empty_interval.
    template<typename _Type>
    inline constexpr bool is_empty_interval_v =
        is_empty_interval<_Type>::value;

    // is_degenerate_interval_v
    //   variable template: value helper for is_degenerate_interval.
    template<typename _Type>
    inline constexpr bool is_degenerate_interval_v =
        is_degenerate_interval<_Type>::value;

    // is_proper_interval_v
    //   variable template: value helper for is_proper_interval.
    template<typename _Type>
    inline constexpr bool is_proper_interval_v =
        is_proper_interval<_Type>::value;

    // intervals_same_type_v
    //   variable template: value helper for intervals_same_type.
    template<typename _Interval1,
             typename _Interval2>
    inline constexpr bool intervals_same_type_v =
        intervals_same_type<_Interval1, _Interval2>::value;

    // intervals_same_boundary_type_v
    //   variable template: value helper for intervals_same_boundary_type.
    template<typename _Interval1,
             typename _Interval2>
    inline constexpr bool intervals_same_boundary_type_v =
        intervals_same_boundary_type<_Interval1, _Interval2>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // maths
NS_END  // djinterp


#endif  // DJINTERP_MATHS_INTERVAL_TRAITS_
