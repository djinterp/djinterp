/*******************************************************************************
* djinterp [math]                                          discrete_interval.hpp
*
* Compile-time discrete interval [lower, upper] with step.
*   A closed interval whose iterator advances by a configurable step size
* rather than by 1. The value type, bounds, step, and size type are all
* template parameters. Useful for representing arithmetic sequences,
* sampled ranges, and strided index sets.
*
* VALUES IN THE INTERVAL:
*   { Lower, Lower + Step, Lower + 2*Step, ... }
*   up to and including Upper (if Upper is reachable by the stride).
*
*   A face over the C core (c/math/interval.h), as closed_interval.hpp is.
* Everything compiles from C++98; the operations are constexpr from C++14.
*
* STRUCTURAL INTERFACE (for interval_traits):
*   - value_type, size_type
*   - static lower_bound, upper_bound, step
*   - static bool is_left_open  = false
*   - static bool is_right_open = false
*
*
* path:      /inc/djinterp/math/interval/discrete_interval.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.04.23
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DISCRETE INTERVAL
    -----------------
    1.  discrete_interval
    2.  Static member definitions
2.  TYPE ALIASES (C++11)
    --------------------
*/

#ifndef DJINTERP_MATH_INTERVAL_DISCRETE_INTERVAL_HPP
#define DJINTERP_MATH_INTERVAL_DISCRETE_INTERVAL_HPP 1

// std
#include <cstddef>                     // std::size_t
#include <string>                      // std::string
// djinterp
#include "../../djinterp.hpp"          // framework root
#include "./interval_common.hpp"       // internal::interval_kernel,
                                       // interval_iterator
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::int16_t ... uint64_t


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  DISCRETE INTERVAL
//==============================================================================


// 1.1    discrete_interval
//------------------------------------------------------------------------------
// discrete_interval
//   struct: compile-time discrete interval [Lower, Upper] with stride Step
// over Type. Endpoints are inclusive (closed). The members are Lower,
// Lower + Step, Lower + 2*Step, ..., up to and including Upper.
template<typename Type,
         Type     Lower,
         Type     Upper,
         Type     Step     = static_cast<Type>(1),
         typename SizeType = std::size_t>
struct discrete_interval
{
private:
    typedef internal::interval_kernel<Type> kernel;
    typedef typename kernel::core_type      core_type;

public:
    typedef Type                                 value_type;
    typedef SizeType                             size_type;
    typedef interval_iterator<discrete_interval> iterator;

    static D_CONSTEXPR_VAR value_type lower_bound   = Lower;
    static D_CONSTEXPR_VAR value_type upper_bound   = Upper;
    static D_CONSTEXPR_VAR value_type step          = Step;
    static D_CONSTEXPR_VAR bool       is_left_open  = false;
    static D_CONSTEXPR_VAR bool       is_right_open = false;

    D_STATIC_ASSERT((!(Upper < Lower)),
                    "discrete_interval: Lower must be <= Upper.");
    D_STATIC_ASSERT((static_cast<Type>(0) < Step),
                    "discrete_interval: Step must be > 0.");

    // size
    //   query: the number of members, floor((upper - lower) / step) + 1.
    static D_CONSTEXPR_CPP14 size_type
    size() D_NOEXCEPT
    {
        return static_cast<size_type>(kernel::count(m_core()));
    }

    // contains
    //   query: whether _value lies within [Lower, Upper] AND is a whole
    // number of steps past Lower.
    static D_CONSTEXPR_CPP14 bool
    contains(const value_type& _value) D_NOEXCEPT
    {
        return kernel::contains(m_core(), _value);
    }

    // contains_in_range
    //   query: whether _value lies within [Lower, Upper], on a step or not.
    static D_CONSTEXPR_CPP14 bool
    contains_in_range(const value_type& _value) D_NOEXCEPT
    {
        return kernel::contains_in_range(m_core(), _value);
    }

    // clamp
    //   transform: _value constrained to a member, rounding down to a step.
    static D_CONSTEXPR_CPP14 value_type
    clamp(const value_type& _value) D_NOEXCEPT
    {
        return kernel::clamp(m_core(), _value);
    }

    // clamp_nearest
    //   transform: _value constrained to the nearer member, a tie going down.
    static D_CONSTEXPR_CPP14 value_type
    clamp_nearest(const value_type& _value) D_NOEXCEPT
    {
        return kernel::clamp_nearest(m_core(), _value);
    }

    // is_valid
    //   query: whether the interval is well-formed (Lower <= Upper, Step > 0).
    static D_CONSTEXPR_CPP14 bool
    is_valid() D_NOEXCEPT
    {
        return ( (kernel::is_valid(m_core())) &&
                 (kernel::is_discrete(m_core())) );
    }

    // normalize
    //   transform: _value mapped to [0, 1] over the full span, as a double;
    // normalize<FloatType> in another precision.
    static D_CONSTEXPR_CPP14 double
    normalize(const value_type& _value) D_NOEXCEPT
    {
        return normalize<double>(_value);
    }

    template<typename FloatType>
    static D_CONSTEXPR_CPP14 FloatType
    normalize(const value_type& _value) D_NOEXCEPT
    {
        const struct d_math_ratio terms =
            kernel::normalize_terms(m_core(), _value);

        return ( static_cast<FloatType>(terms.numerator) /
                 static_cast<FloatType>(terms.denominator) );
    }

    // normalize_discrete
    //   transform: _value's step index mapped to [0, 1]: the first member is
    // 0, the last 1; as a double, or normalize_discrete<FloatType>.
    static D_CONSTEXPR_CPP14 double
    normalize_discrete(const value_type& _value) D_NOEXCEPT
    {
        return normalize_discrete<double>(_value);
    }

    template<typename FloatType>
    static D_CONSTEXPR_CPP14 FloatType
    normalize_discrete(const value_type& _value) D_NOEXCEPT
    {
        const struct d_math_ratio terms =
            kernel::normalize_discrete_terms(m_core(), _value);

        return ( static_cast<FloatType>(terms.numerator) /
                 static_cast<FloatType>(terms.denominator) );
    }

    // at
    //   access: the member _index steps past Lower.
    static D_CONSTEXPR_CPP14 value_type
    at(size_type _index) D_NOEXCEPT
    {
        return kernel::at(m_core(), static_cast<d_math_umax>(_index));
    }

    // index_of
    //   query: _value's step index, or size() if it is not a member.
    static D_CONSTEXPR_CPP14 size_type
    index_of(const value_type& _value) D_NOEXCEPT
    {
        return static_cast<size_type>(kernel::index_of(m_core(), _value));
    }

    // last
    //   query: the largest member.
    static D_CONSTEXPR_CPP14 value_type
    last() D_NOEXCEPT
    {
        return kernel::last(m_core());
    }

    // overlaps
    //   query: whether the ranges [Lower, Upper] and [OtherLower,
    // OtherUpper] overlap; the strides are not considered.
    template<Type OtherLower,
             Type OtherUpper,
             Type OtherStep>
    static D_CONSTEXPR_CPP14 bool
    overlaps(
        const discrete_interval<Type,
                                OtherLower,
                                OtherUpper,
                                OtherStep,
                                SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::overlaps(m_core(),
                                kernel::make(OtherLower,
                                             OtherUpper,
                                             D_INTERVAL_CLOSED,
                                             OtherStep));
    }

    // intersects
    //   query: whether the two intervals share a member: a value on both
    // strides within both ranges, found by the Chinese remainder theorem for
    // an integer type.
    template<Type OtherLower,
             Type OtherUpper,
             Type OtherStep>
    static D_CONSTEXPR_CPP14 bool
    intersects(
        const discrete_interval<Type,
                                OtherLower,
                                OtherUpper,
                                OtherStep,
                                SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::intersects(m_core(),
                                  kernel::make(OtherLower,
                                               OtherUpper,
                                               D_INTERVAL_CLOSED,
                                               OtherStep));
    }

    // to_string
    //   format: the interval as text, "[lower:step:upper]".
    static std::string
    to_string()
    {
        return kernel::to_string(m_core());
    }

    // begin / end
    //   iteration: over the members, by index.
    static D_CONSTEXPR iterator
    begin() D_NOEXCEPT
    {
        return iterator(0);
    }

    static D_CONSTEXPR_CPP14 iterator
    end() D_NOEXCEPT
    {
        return iterator(size());
    }

private:
    // m_core
    //   the interval as its kernel takes it.
    static D_CONSTEXPR_CPP14 core_type
    m_core() D_NOEXCEPT
    {
        return kernel::make(Lower, Upper, D_INTERVAL_CLOSED, Step);
    }
};

// 1.2    Static member definitions
//------------------------------------------------------------------------------
// As closed_interval.hpp's: definitions below C++17, redeclarations from it.
template<typename Type, Type Lower, Type Upper, Type Step, typename SizeType>
D_CONSTEXPR_VAR Type
    discrete_interval<Type, Lower, Upper, Step, SizeType>::lower_bound;
template<typename Type, Type Lower, Type Upper, Type Step, typename SizeType>
D_CONSTEXPR_VAR Type
    discrete_interval<Type, Lower, Upper, Step, SizeType>::upper_bound;
template<typename Type, Type Lower, Type Upper, Type Step, typename SizeType>
D_CONSTEXPR_VAR Type
    discrete_interval<Type, Lower, Upper, Step, SizeType>::step;
template<typename Type, Type Lower, Type Upper, Type Step, typename SizeType>
D_CONSTEXPR_VAR bool
    discrete_interval<Type, Lower, Upper, Step, SizeType>::is_left_open;
template<typename Type, Type Lower, Type Upper, Type Step, typename SizeType>
D_CONSTEXPR_VAR bool
    discrete_interval<Type, Lower, Upper, Step, SizeType>::is_right_open;


//==============================================================================
// 2.  TYPE ALIASES (C++11)
//==============================================================================
// Alias templates are C++11's; below it, spell discrete_interval<int, ...>.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// int_discrete_interval
//   type: discrete_interval over int.
template<int Lower,
         int Upper,
         int Step = 1>
using int_discrete_interval =
    discrete_interval<int, Lower, Upper, Step>;

// index_discrete_interval
//   type: discrete_interval over std::size_t.
template<std::size_t Lower,
         std::size_t Upper,
         std::size_t Step = 1>
using index_discrete_interval =
    discrete_interval<std::size_t, Lower, Upper, Step>;

// char_discrete_interval
//   type: discrete_interval over char.
template<char Lower,
         char Upper,
         char Step = 1>
using char_discrete_interval =
    discrete_interval<char, Lower, Upper, Step>;

// uint8_discrete_interval
//   type: discrete_interval over uint8_t.
template<re_std::uint8_t Lower,
         re_std::uint8_t Upper,
         re_std::uint8_t Step = 1>
using uint8_discrete_interval =
    discrete_interval<re_std::uint8_t, Lower, Upper, Step>;

// int64_discrete_interval
//   type: discrete_interval over int64_t.
template<re_std::int64_t Lower,
         re_std::int64_t Upper,
         re_std::int64_t Step = 1>
using int64_discrete_interval =
    discrete_interval<re_std::int64_t, Lower, Upper, Step>;

// uint64_discrete_interval
//   type: discrete_interval over uint64_t.
template<re_std::uint64_t Lower,
         re_std::uint64_t Upper,
         re_std::uint64_t Step = 1>
using uint64_discrete_interval =
    discrete_interval<re_std::uint64_t, Lower, Upper, Step>;

// int32_discrete_interval
//   type: discrete_interval over int32_t.
template<re_std::int32_t Lower,
         re_std::int32_t Upper,
         re_std::int32_t Step = 1>
using int32_discrete_interval =
    discrete_interval<re_std::int32_t, Lower, Upper, Step>;

// uint32_discrete_interval
//   type: discrete_interval over uint32_t.
template<re_std::uint32_t Lower,
         re_std::uint32_t Upper,
         re_std::uint32_t Step = 1>
using uint32_discrete_interval =
    discrete_interval<re_std::uint32_t, Lower, Upper, Step>;

// int16_discrete_interval
//   type: discrete_interval over int16_t.
template<re_std::int16_t Lower,
         re_std::int16_t Upper,
         re_std::int16_t Step = 1>
using int16_discrete_interval =
    discrete_interval<re_std::int16_t, Lower, Upper, Step>;

// uint16_discrete_interval
//   type: discrete_interval over uint16_t.
template<re_std::uint16_t Lower,
         re_std::uint16_t Upper,
         re_std::uint16_t Step = 1>
using uint16_discrete_interval =
    discrete_interval<re_std::uint16_t, Lower, Upper, Step>;

// short_discrete_interval
//   type: discrete_interval over short.
template<short Lower,
         short Upper,
         short Step = 1>
using short_discrete_interval =
    discrete_interval<short, Lower, Upper, Step>;

// long_discrete_interval
//   type: discrete_interval over long.
template<long Lower,
         long Upper,
         long Step = 1>
using long_discrete_interval =
    discrete_interval<long, Lower, Upper, Step>;

// long_long_discrete_interval
//   type: discrete_interval over long long.
template<long long Lower,
         long long Upper,
         long long Step = 1>
using long_long_discrete_interval =
    discrete_interval<long long, Lower, Upper, Step>;

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_INTERVAL_DISCRETE_INTERVAL_HPP
