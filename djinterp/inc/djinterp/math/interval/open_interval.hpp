/*******************************************************************************
* djinterp [math]                                              open_interval.hpp
*
* Compile-time open interval (lower, upper).
*   Both endpoints are exclusive. The value type, bounds, and size type are
* all configurable via template parameters. Provides containment testing,
* overlap detection, clamping, normalization, indexed access, and forward
* iteration over interior integral values.
*
*   Also provides a runtime open interval (runtime_open_interval) for cases
* where the upper bound is not known until runtime. An upper bound at or
* below the lower one makes it empty, which is_valid() reports; it does not
* throw, so the header serves builds without exceptions.
*
*   A face over the C core (c/math/interval.h), as closed_interval.hpp is.
* Everything compiles from C++98; the operations are constexpr from C++14.
*
* STRUCTURAL INTERFACE (for interval_traits):
*   - value_type, size_type
*   - static lower_bound, upper_bound, step (0)
*   - static bool is_left_open  = true
*   - static bool is_right_open = true
*
*
* path:      /inc/djinterp/math/interval/open_interval.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.04.23
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  COMPILE-TIME OPEN INTERVAL
    --------------------------
    1.  open_interval
    2.  Static member definitions
2.  RUNTIME OPEN INTERVAL
    ---------------------
    1.  runtime_open_interval
    2.  Static member definitions
3.  TYPE ALIASES (C++11)
    --------------------
*/

#ifndef DJINTERP_MATH_INTERVAL_OPEN_INTERVAL_HPP
#define DJINTERP_MATH_INTERVAL_OPEN_INTERVAL_HPP 1

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
// 1.  COMPILE-TIME OPEN INTERVAL
//==============================================================================


// 1.1    open_interval
//------------------------------------------------------------------------------
// open_interval
//   struct: compile-time open interval (Lower, Upper) over Type. Both
// endpoints are exclusive: for an integral type the members are Lower + 1 to
// Upper - 1.
template<typename Type,
         Type     Lower,
         Type     Upper,
         typename SizeType = std::size_t>
struct open_interval
{
private:
    typedef internal::interval_kernel<Type> kernel;
    typedef typename kernel::core_type      core_type;

public:
    typedef Type                             value_type;
    typedef SizeType                         size_type;
    typedef interval_iterator<open_interval> iterator;

    static D_CONSTEXPR_VAR value_type lower_bound   = Lower;
    static D_CONSTEXPR_VAR value_type upper_bound   = Upper;
    static D_CONSTEXPR_VAR value_type step          = static_cast<Type>(0);
    static D_CONSTEXPR_VAR bool       is_left_open  = true;
    static D_CONSTEXPR_VAR bool       is_right_open = true;

    D_STATIC_ASSERT((Lower < Upper),
                    "open_interval: Lower must be < Upper for a non-empty "
                    "open interval.");

    // size
    //   query: the number of interior values, max(0, upper - lower - 1).
    static D_CONSTEXPR_CPP14 size_type
    size() D_NOEXCEPT
    {
        return static_cast<size_type>(kernel::count(m_core()));
    }

    // contains
    //   query: whether _value lies within (Lower, Upper).
    static D_CONSTEXPR_CPP14 bool
    contains(const value_type& _value) D_NOEXCEPT
    {
        return kernel::contains(m_core(), _value);
    }

    // clamp
    //   transform: _value constrained to the nearest interior value: at or
    // below Lower it is Lower + 1, at or above Upper it is Upper - 1.
    static D_CONSTEXPR_CPP14 value_type
    clamp(const value_type& _value) D_NOEXCEPT
    {
        return kernel::clamp(m_core(), _value);
    }

    // is_valid
    //   query: whether the interval is well-formed (Lower < Upper).
    static D_CONSTEXPR_CPP14 bool
    is_valid() D_NOEXCEPT
    {
        return ( (kernel::is_valid(m_core())) &&
                 (Lower < Upper) );
    }

    // is_empty
    //   query: whether the interval holds no interior integral value.
    static D_CONSTEXPR_CPP14 bool
    is_empty() D_NOEXCEPT
    {
        return kernel::is_empty(m_core());
    }

    // at
    //   access: the interior value _index places past Lower + 1.
    static D_CONSTEXPR_CPP14 value_type
    at(size_type _index) D_NOEXCEPT
    {
        return kernel::at(m_core(), static_cast<d_math_umax>(_index));
    }

    // normalize
    //   transform: _value mapped to [0, 1] over the full span (Lower, Upper),
    // as a double; normalize<FloatType> in another precision.
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
    //   query: whether this interval overlaps another open_interval, as
    // intervals of the real line: bounds that only touch do not overlap.
    template<Type OtherLower,
             Type OtherUpper>
    static D_CONSTEXPR_CPP14 bool
    overlaps(
        const open_interval<Type,
                            OtherLower,
                            OtherUpper,
                            SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::overlaps_continuous(m_core(),
                                           kernel::make(OtherLower,
                                                        OtherUpper,
                                                        D_INTERVAL_OPEN,
                                                        static_cast<Type>(0)));
    }

    // intersects
    //   query: whether this interval shares a member with another
    // open_interval -- an interior value both hold, which bounds that only
    // just overlap on the real line may not give.
    template<Type OtherLower,
             Type OtherUpper>
    static D_CONSTEXPR_CPP14 bool
    intersects(
        const open_interval<Type,
                            OtherLower,
                            OtherUpper,
                            SizeType>& _other
    ) D_NOEXCEPT
    {
        (void)_other;

        return kernel::intersects(m_core(),
                                  kernel::make(OtherLower,
                                               OtherUpper,
                                               D_INTERVAL_OPEN,
                                               static_cast<Type>(0)));
    }

    // to_string
    //   format: the interval as text, "(lower, upper)".
    static std::string
    to_string()
    {
        return kernel::to_string(m_core());
    }

    // begin / end
    //   iteration: over the interior values, by index.
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
                            D_INTERVAL_OPEN,
                            static_cast<Type>(0));
    }
};

// 1.2    Static member definitions
//------------------------------------------------------------------------------
// As closed_interval.hpp's: definitions below C++17, redeclarations from it.
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR Type open_interval<Type, Lower, Upper, SizeType>::lower_bound;
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR Type open_interval<Type, Lower, Upper, SizeType>::upper_bound;
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR Type open_interval<Type, Lower, Upper, SizeType>::step;
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR bool
    open_interval<Type, Lower, Upper, SizeType>::is_left_open;
template<typename Type, Type Lower, Type Upper, typename SizeType>
D_CONSTEXPR_VAR bool
    open_interval<Type, Lower, Upper, SizeType>::is_right_open;


//==============================================================================
// 2.  RUNTIME OPEN INTERVAL
//==============================================================================


// 2.1    runtime_open_interval
//------------------------------------------------------------------------------
// runtime_open_interval
//   struct: open interval with a compile-time lower bound and a runtime upper
// bound, for when the upper bound is determined at runtime.
template<typename Type,
         Type     Lower    = static_cast<Type>(0),
         typename SizeType = std::size_t>
struct runtime_open_interval
{
private:
    typedef internal::interval_kernel<Type> kernel;
    typedef typename kernel::core_type      core_type;

public:
    typedef Type                                     value_type;
    typedef SizeType                                 size_type;
    typedef interval_iterator<runtime_open_interval> iterator;

    static D_CONSTEXPR_VAR value_type lower_bound   = Lower;
    static D_CONSTEXPR_VAR value_type step          = static_cast<Type>(0);
    static D_CONSTEXPR_VAR bool       is_left_open  = true;
    static D_CONSTEXPR_VAR bool       is_right_open = true;

    // runtime_open_interval
    //   constructor: the interval (Lower, _end_val). An _end_val at or below
    // Lower gives an empty interval, which is_valid() reports.
    D_CONSTEXPR explicit
    runtime_open_interval(
        value_type _end_val
    ) D_NOEXCEPT
        : m_upper(_end_val)
    {}

    // get_upper_bound
    //   query: the runtime upper bound.
    D_CONSTEXPR value_type
    get_upper_bound() const D_NOEXCEPT
    {
        return m_upper;
    }

    // is_valid
    //   query: whether the upper bound lies above Lower.
    D_CONSTEXPR_CPP14 bool
    is_valid() const D_NOEXCEPT
    {
        return (Lower < m_upper);
    }

    // size
    //   query: the number of interior values; 0 when invalid.
    D_CONSTEXPR_CPP14 size_type
    size() const D_NOEXCEPT
    {
        return static_cast<size_type>(kernel::count(m_core()));
    }

    // contains
    //   query: whether _value lies within (Lower, upper).
    D_CONSTEXPR_CPP14 bool
    contains(const value_type& _value) const D_NOEXCEPT
    {
        return kernel::contains(m_core(), _value);
    }

    // clamp
    //   transform: _value constrained to the nearest interior value.
    D_CONSTEXPR_CPP14 value_type
    clamp(const value_type& _value) const D_NOEXCEPT
    {
        return kernel::clamp(m_core(), _value);
    }

    // overlaps
    //   query: whether this interval overlaps another with the same lower
    // bound: whether both are non-empty.
    D_CONSTEXPR_CPP14 bool
    overlaps(const runtime_open_interval& _other) const D_NOEXCEPT
    {
        return kernel::overlaps_continuous(m_core(), _other.m_core());
    }

    // intersects
    //   query: whether this interval shares a member with another with the
    // same lower bound.
    D_CONSTEXPR_CPP14 bool
    intersects(const runtime_open_interval& _other) const D_NOEXCEPT
    {
        return kernel::intersects(m_core(), _other.m_core());
    }

    // normalize
    //   transform: _value mapped to [0, 1] over (Lower, upper), as a double;
    // normalize<FloatType> in another precision.
    D_CONSTEXPR_CPP14 double
    normalize(const value_type& _value) const D_NOEXCEPT
    {
        return normalize<double>(_value);
    }

    template<typename FloatType>
    D_CONSTEXPR_CPP14 FloatType
    normalize(const value_type& _value) const D_NOEXCEPT
    {
        const struct d_math_ratio terms =
            kernel::normalize_terms(m_core(), _value);

        return ( static_cast<FloatType>(terms.numerator) /
                 static_cast<FloatType>(terms.denominator) );
    }

    // at
    //   access: the interior value _index places past Lower + 1. It depends
    // on Lower alone, so the iterator reaches it without an object.
    static D_CONSTEXPR_CPP14 value_type
    at(size_type _index) D_NOEXCEPT
    {
        return kernel::at(kernel::make(Lower,
                                       Lower,
                                       D_INTERVAL_OPEN,
                                       static_cast<Type>(0)),
                          static_cast<d_math_umax>(_index));
    }

    // to_string
    //   format: the interval as text, "(lower, upper)".
    std::string
    to_string() const
    {
        return kernel::to_string(m_core());
    }

    // begin / end
    //   iteration: over the interior values, by index.
    D_CONSTEXPR iterator
    begin() const D_NOEXCEPT
    {
        return iterator(0);
    }

    D_CONSTEXPR_CPP14 iterator
    end() const D_NOEXCEPT
    {
        return iterator(size());
    }

private:
    // m_core
    //   the interval as its kernel takes it.
    D_CONSTEXPR_CPP14 core_type
    m_core() const D_NOEXCEPT
    {
        return kernel::make(Lower,
                            m_upper,
                            D_INTERVAL_OPEN,
                            static_cast<Type>(0));
    }

    value_type m_upper;
};

// 2.2    Static member definitions
//------------------------------------------------------------------------------
template<typename Type, Type Lower, typename SizeType>
D_CONSTEXPR_VAR Type runtime_open_interval<Type, Lower, SizeType>::lower_bound;
template<typename Type, Type Lower, typename SizeType>
D_CONSTEXPR_VAR Type runtime_open_interval<Type, Lower, SizeType>::step;
template<typename Type, Type Lower, typename SizeType>
D_CONSTEXPR_VAR bool
    runtime_open_interval<Type, Lower, SizeType>::is_left_open;
template<typename Type, Type Lower, typename SizeType>
D_CONSTEXPR_VAR bool
    runtime_open_interval<Type, Lower, SizeType>::is_right_open;


//==============================================================================
// 3.  TYPE ALIASES (C++11)
//==============================================================================
// Alias templates are C++11's; below it, spell open_interval<int, L, U>.


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// int_open_interval
//   type: open_interval over int.
template<int Lower,
         int Upper>
using int_open_interval = open_interval<int, Lower, Upper>;

// index_open_interval
//   type: open_interval over std::size_t.
template<std::size_t Lower,
         std::size_t Upper>
using index_open_interval = open_interval<std::size_t, Lower, Upper>;

// char_open_interval
//   type: open_interval over char.
template<char Lower,
         char Upper>
using char_open_interval = open_interval<char, Lower, Upper>;

// uint8_open_interval
//   type: open_interval over uint8_t.
template<re_std::uint8_t Lower,
         re_std::uint8_t Upper>
using uint8_open_interval = open_interval<re_std::uint8_t, Lower, Upper>;

// int64_open_interval
//   type: open_interval over int64_t.
template<re_std::int64_t Lower,
         re_std::int64_t Upper>
using int64_open_interval = open_interval<re_std::int64_t, Lower, Upper>;

// uint64_open_interval
//   type: open_interval over uint64_t.
template<re_std::uint64_t Lower,
         re_std::uint64_t Upper>
using uint64_open_interval = open_interval<re_std::uint64_t, Lower, Upper>;

// int32_open_interval
//   type: open_interval over int32_t.
template<re_std::int32_t Lower,
         re_std::int32_t Upper>
using int32_open_interval = open_interval<re_std::int32_t, Lower, Upper>;

// uint32_open_interval
//   type: open_interval over uint32_t.
template<re_std::uint32_t Lower,
         re_std::uint32_t Upper>
using uint32_open_interval = open_interval<re_std::uint32_t, Lower, Upper>;

// int16_open_interval
//   type: open_interval over int16_t.
template<re_std::int16_t Lower,
         re_std::int16_t Upper>
using int16_open_interval = open_interval<re_std::int16_t, Lower, Upper>;

// uint16_open_interval
//   type: open_interval over uint16_t.
template<re_std::uint16_t Lower,
         re_std::uint16_t Upper>
using uint16_open_interval = open_interval<re_std::uint16_t, Lower, Upper>;

// short_open_interval
//   type: open_interval over short.
template<short Lower,
         short Upper>
using short_open_interval = open_interval<short, Lower, Upper>;

// long_open_interval
//   type: open_interval over long.
template<long Lower,
         long Upper>
using long_open_interval = open_interval<long, Lower, Upper>;

// long_long_open_interval
//   type: open_interval over long long.
template<long long Lower,
         long long Upper>
using long_long_open_interval = open_interval<long long, Lower, Upper>;

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_INTERVAL_OPEN_INTERVAL_HPP
