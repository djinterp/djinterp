/*******************************************************************************
* djinterp [math]                                            closed_interval.hpp
*
* Compile-time closed interval [lower, upper].
*   Both endpoints are inclusive. The value type, bounds, and size type are
* all configurable via template parameters. Provides containment testing,
* overlap detection, clamping, normalization, indexed access, and forward
* iteration over all integral values in the range.
*
*   A face over the C core (c/math/interval.h): every operation forwards to
* internal::interval_kernel<Type> (interval_common.hpp), which is the C core's
* family for an integer type and the generic path for any other. Everything
* compiles from C++98; the operations are constexpr from C++14, where the
* kernels are.
*
* STRUCTURAL INTERFACE (for interval_traits):
*   - value_type, size_type
*   - static lower_bound, upper_bound, step (0)
*   - static bool is_left_open  = false
*   - static bool is_right_open = false
*
*
* path:      /inc/djinterp/math/interval/closed_interval.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.04.23
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CLOSED INTERVAL
    ---------------
    1.  closed_interval
    2.  Static member definitions
2.  TYPE ALIASES (C++11)
    --------------------
*/

#ifndef DJINTERP_MATH_INTERVAL_CLOSED_INTERVAL_HPP
#define DJINTERP_MATH_INTERVAL_CLOSED_INTERVAL_HPP 1

// std
#include <cstddef>                     // std::size_t
#include <string>                      // std::string
// djinterp
#include "../../djinterp.hpp"          // framework root
#include "./interval_common.hpp"       // internal::interval_kernel,
                                       // interval_iterator
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::int8_t ... uint64_t


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  CLOSED INTERVAL
//==============================================================================


// 1.1    closed_interval
//------------------------------------------------------------------------------
// closed_interval
//   struct: compile-time closed interval [Lower, Upper] over Type. Both
// endpoints are inclusive.
template<typename Type,
         Type     Lower,
         Type     Upper,
         typename SizeType = std::size_t>
struct closed_interval
{
private:
    typedef internal::interval_kernel<Type> kernel;
    typedef typename kernel::core_type      core_type;

public:
    typedef Type                               value_type;
    typedef SizeType                           size_type;
    typedef interval_iterator<closed_interval> iterator;

    static D_CONSTEXPR_VAR value_type lower_bound   = Lower;
    static D_CONSTEXPR_VAR value_type upper_bound   = Upper;
    static D_CONSTEXPR_VAR value_type step          = static_cast<Type>(0);
    static D_CONSTEXPR_VAR bool       is_left_open  = false;
    static D_CONSTEXPR_VAR bool       is_right_open = false;

    D_STATIC_ASSERT((!(Upper < Lower)),
                    "closed_interval: Lower must be <= Upper.");

    // size
    //   query: the number of values, upper - lower + 1.
    static D_CONSTEXPR_CPP14 size_type
    size() D_NOEXCEPT
    {
        return static_cast<size_type>(kernel::count(m_core()));
    }

    // contains
    //   query: whether _value lies within [Lower, Upper].
    static D_CONSTEXPR_CPP14 bool
    contains(const value_type& _value) D_NOEXCEPT
    {
        return kernel::contains(m_core(), _value);
    }

    // clamp
    //   transform: _value constrained to [Lower, Upper].
    static D_CONSTEXPR_CPP14 value_type
    clamp(const value_type& _value) D_NOEXCEPT
    {
        return kernel::clamp(m_core(), _value);
    }

    // is_valid
    //   query: whether the interval is well-formed (Lower <= Upper).
    static D_CONSTEXPR_CPP14 bool
    is_valid() D_NOEXCEPT
    {
        return kernel::is_valid(m_core());
    }

    // at
    //   access: the value _index places past Lower.
    static D_CONSTEXPR_CPP14 value_type
    at(size_type _index) D_NOEXCEPT
    {
        return kernel::at(m_core(), static_cast<d_math_umax>(_index));
    }

    // normalize
    //   transform: _value mapped to [0, 1] over [Lower, Upper] (0 when they
    // are equal), as a double; normalize<FloatType> in another precision.
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

    // overlaps
    //   query: whether this interval overlaps another closed_interval.
    template<Type OtherLower,
             Type OtherUpper>
    static D_CONSTEXPR_CPP14 bool
    overlaps(
        const closed_interval<Type,
                              OtherLower,
                              OtherUpper,
                              SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::overlaps(m_core(),
                                kernel::make(OtherLower,
                                             OtherUpper,
                                             D_INTERVAL_CLOSED,
                                             static_cast<Type>(0)));
    }

    // intersects
    //   query: whether this interval shares a member with another
    // closed_interval: for two continuous intervals, overlaps.
    template<Type OtherLower,
             Type OtherUpper>
    static D_CONSTEXPR_CPP14 bool
    intersects(
        const closed_interval<Type,
                              OtherLower,
                              OtherUpper,
                              SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::intersects(m_core(),
                                  kernel::make(OtherLower,
                                               OtherUpper,
                                               D_INTERVAL_CLOSED,
                                               static_cast<Type>(0)));
    }

    // to_string
    //   format: the interval as text, "[lower, upper]".
    static std::string
    to_string()
    {
        return kernel::to_string(m_core());
    }

    // begin / end
    //   iteration: over the values Lower to Upper, by index.
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
        return kernel::make(Lower,
                            Upper,
                            D_INTERVAL_CLOSED,
                            static_cast<Type>(0));
    }
};

// 1.2    Static member definitions
//------------------------------------------------------------------------------
// Below C++17 a static data member named where its address could be taken
// needs a definition outside its class; from C++17 it is inline already, and
// this is a redeclaration that changes nothing.
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR Type closed_interval<Type, Lower, Upper, SizeType>::lower_bound;
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR Type closed_interval<Type, Lower, Upper, SizeType>::upper_bound;
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR Type closed_interval<Type, Lower, Upper, SizeType>::step;
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR bool
    closed_interval<Type, Lower, Upper, SizeType>::is_left_open;
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR bool
    closed_interval<Type, Lower, Upper, SizeType>::is_right_open;


//==============================================================================
// 2.  TYPE ALIASES (C++11)
//==============================================================================
// Alias templates are C++11's; below it, spell closed_interval<int, L, U>.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// int_closed_interval
//   type: closed_interval over int.
template<int Lower,
         int Upper>
using int_closed_interval = closed_interval<int, Lower, Upper>;

// index_closed_interval
//   type: closed_interval over std::size_t.
template<std::size_t Lower,
         std::size_t Upper>
using index_closed_interval = closed_interval<std::size_t, Lower, Upper>;

// char_closed_interval
//   type: closed_interval over char.
template<char Lower,
         char Upper>
using char_closed_interval = closed_interval<char, Lower, Upper>;

// uint8_closed_interval
//   type: closed_interval over uint8_t.
template<re_std::uint8_t Lower,
         re_std::uint8_t Upper>
using uint8_closed_interval = closed_interval<re_std::uint8_t, Lower, Upper>;

// int64_closed_interval
//   type: closed_interval over int64_t.
template<re_std::int64_t Lower,
         re_std::int64_t Upper>
using int64_closed_interval = closed_interval<re_std::int64_t, Lower, Upper>;

// uint64_closed_interval
//   type: closed_interval over uint64_t.
template<re_std::uint64_t Lower,
         re_std::uint64_t Upper>
using uint64_closed_interval =
    closed_interval<re_std::uint64_t, Lower, Upper>;

// int32_closed_interval
//   type: closed_interval over int32_t.
template<re_std::int32_t Lower,
         re_std::int32_t Upper>
using int32_closed_interval = closed_interval<re_std::int32_t, Lower, Upper>;

// uint32_closed_interval
//   type: closed_interval over uint32_t.
template<re_std::uint32_t Lower,
         re_std::uint32_t Upper>
using uint32_closed_interval =
    closed_interval<re_std::uint32_t, Lower, Upper>;

// int16_closed_interval
//   type: closed_interval over int16_t.
template<re_std::int16_t Lower,
         re_std::int16_t Upper>
using int16_closed_interval = closed_interval<re_std::int16_t, Lower, Upper>;

// uint16_closed_interval
//   type: closed_interval over uint16_t.
template<re_std::uint16_t Lower,
         re_std::uint16_t Upper>
using uint16_closed_interval =
    closed_interval<re_std::uint16_t, Lower, Upper>;

// short_closed_interval
//   type: closed_interval over short.
template<short Lower,
         short Upper>
using short_closed_interval = closed_interval<short, Lower, Upper>;

// long_closed_interval
//   type: closed_interval over long.
template<long Lower,
         long Upper>
using long_closed_interval = closed_interval<long, Lower, Upper>;

// long_long_closed_interval
//   type: closed_interval over long long.
template<long long Lower,
         long long Upper>
using long_long_closed_interval = closed_interval<long long, Lower, Upper>;

// bool_closed_interval
//   type: closed_interval over bool.
template<bool Lower,
         bool Upper>
using bool_closed_interval = closed_interval<bool, Lower, Upper>;

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_INTERVAL_CLOSED_INTERVAL_HPP
