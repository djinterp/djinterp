/******************************************************************************
* djinterp [math]                                        closed_interval.hpp
*
* Compile-time closed interval [lower, upper].
*   Both endpoints are inclusive. The value type, bounds, and size type are
* all configurable via template parameters. Provides containment testing,
* overlap detection, clamping, normalization, and forward iteration over
* all integral values in the range.
*
* STRUCTURAL INTERFACE (for interval_traits):
*   - value_type, size_type
*   - static constexpr lower_bound, upper_bound
*   - static constexpr bool is_left_open  = false
*   - static constexpr bool is_right_open = false
*
* path:      /inc/djinterp/math/interval/closed_interval.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2024.04.23
******************************************************************************/

#ifndef DJINTERP_MATH_CLOSED_INTERVAL_
#define DJINTERP_MATH_CLOSED_INTERVAL_ 1

// std
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    CLOSED INTERVAL
// ============================================================================

// closed_interval
//   struct: compile-time closed interval [_Lower, _Upper] over _Type.
// Both endpoints are inclusive. Supports iteration for integral types.
template<typename      _Type,
         _Type         _Lower,
         _Type         _Upper,
         typename      _SizeType = std::size_t>
struct closed_interval
{
    // ---- type aliases -------------------------------------------------------

    using value_type      = _Type;
    using size_type       = _SizeType;

    // ---- static constants ---------------------------------------------------

    static constexpr value_type lower_bound   = _Lower;
    static constexpr value_type upper_bound   = _Upper;
    static constexpr value_type step          = static_cast<_Type>(0);
    static constexpr bool       is_left_open  = false;
    static constexpr bool       is_right_open = false;

    static_assert(_Lower <= _Upper,
                  "closed_interval: _Lower must be <= _Upper.");

    // ---- static member functions --------------------------------------------

    // size
    //   returns the number of elements in the interval.
    // For integral types this is (upper - lower + 1).
    static constexpr size_type
    size
    () noexcept
    {
        return static_cast<size_type>(_Upper - _Lower + 1);
    }

    // contains
    //   checks whether _value lies within [_Lower, _Upper].
    static constexpr bool
    contains
    (
        const value_type& _value
    ) noexcept
    {
        return ( (_value >= _Lower) &&
                 (_value <= _Upper) );
    }

    // clamp
    //   constrains _value to lie within [_Lower, _Upper].
    static constexpr value_type
    clamp
    (
        const value_type& _value
    ) noexcept
    {
        if (_value < _Lower)
        {
            return _Lower;
        }

        if (_value > _Upper)
        {
            return _Upper;
        }

        return _value;
    }

    // is_valid
    //   returns true if the interval is well-formed (_Lower <= _Upper).
    static constexpr bool
    is_valid
    () noexcept
    {
        return (_Lower <= _Upper);
    }

    // normalize
    //   maps _value to the range [0.0, 1.0] relative to the interval.
    // Returns 0 for degenerate intervals where _Lower == _Upper.
    template<typename _FloatType = double>
    static constexpr _FloatType
    normalize
    (
        const value_type& _value
    ) noexcept
    {
        if constexpr (_Lower == _Upper)
        {
            return _FloatType{0};
        }
        else
        {
            return ( static_cast<_FloatType>(_value - _Lower) /
                     static_cast<_FloatType>(_Upper - _Lower) );
        }
    }

    // overlaps
    //   checks whether this interval overlaps with another closed_interval.
    template<_Type _OtherLower,
             _Type _OtherUpper>
    static constexpr bool
    overlaps
    (
        const closed_interval<_Type,
                              _OtherLower,
                              _OtherUpper,
                              _SizeType>&
    ) noexcept
    {
        return !( (_Upper < _OtherLower) ||
                  (_Lower > _OtherUpper) );
    }

    // to_string
    //   returns a string representation of the interval: "[lower, upper]".
    static std::string
    to_string
    ()
    {
        return ( "[" + std::to_string(_Lower) +
                 ", " + std::to_string(_Upper) + "]" );
    }

    // ---- iterator -----------------------------------------------------------

    // iterator
    //   struct: forward iterator over integral values in [_Lower, _Upper].
    struct iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = _Type;
        using pointer           = const _Type*;
        using reference         = const _Type&;

        value_type m_current;

        constexpr explicit iterator(value_type _val)
            : m_current(_val)
        {
        }

        constexpr value_type operator*() const noexcept
        {
            return m_current;
        }

        constexpr iterator& operator++() noexcept
        {
            ++m_current;

            return *this;
        }

        constexpr iterator operator++(int) noexcept
        {
            iterator tmp = *this;
            ++m_current;

            return tmp;
        }

        constexpr bool
        operator==
        (
            const iterator& _other
        ) const noexcept
        {
            return (m_current == _other.m_current);
        }

        constexpr bool
        operator!=
        (
            const iterator& _other
        ) const noexcept
        {
            return (m_current != _other.m_current);
        }
    };

    static constexpr iterator begin() noexcept
    {
        return iterator(_Lower);
    }

    static constexpr iterator end() noexcept
    {
        return iterator(static_cast<value_type>(_Upper + 1));
    }
};


// ============================================================================
// II.   TYPE ALIASES
// ============================================================================

// int_closed_interval
//   type: closed_interval over int.
template<int _Lower,
         int _Upper>
using int_closed_interval = closed_interval<int, _Lower, _Upper>;

// index_closed_interval
//   type: closed_interval over std::size_t.
template<std::size_t _Lower,
         std::size_t _Upper>
using index_closed_interval = closed_interval<std::size_t, _Lower, _Upper>;

// char_closed_interval
//   type: closed_interval over char.
template<char _Lower,
         char _Upper>
using char_closed_interval = closed_interval<char, _Lower, _Upper>;

// uint8_closed_interval
//   type: closed_interval over uint8_t.
template<std::uint8_t _Lower,
         std::uint8_t _Upper>
using uint8_closed_interval = closed_interval<std::uint8_t, _Lower, _Upper>;

// int64_closed_interval
//   type: closed_interval over int64_t.
template<std::int64_t _Lower,
         std::int64_t _Upper>
using int64_closed_interval = closed_interval<std::int64_t, _Lower, _Upper>;

// uint64_closed_interval
//   type: closed_interval over uint64_t.
template<std::uint64_t _Lower,
         std::uint64_t _Upper>
using uint64_closed_interval = closed_interval<std::uint64_t, _Lower, _Upper>;

// int32_closed_interval
//   type: closed_interval over int32_t.
template<std::int32_t _Lower,
         std::int32_t _Upper>
using int32_closed_interval = closed_interval<std::int32_t, _Lower, _Upper>;

// uint32_closed_interval
//   type: closed_interval over uint32_t.
template<std::uint32_t _Lower,
         std::uint32_t _Upper>
using uint32_closed_interval = closed_interval<std::uint32_t, _Lower, _Upper>;

// int16_closed_interval
//   type: closed_interval over int16_t.
template<std::int16_t _Lower,
         std::int16_t _Upper>
using int16_closed_interval = closed_interval<std::int16_t, _Lower, _Upper>;

// uint16_closed_interval
//   type: closed_interval over uint16_t.
template<std::uint16_t _Lower,
         std::uint16_t _Upper>
using uint16_closed_interval = closed_interval<std::uint16_t, _Lower, _Upper>;

// short_closed_interval
//   type: closed_interval over short.
template<short _Lower,
         short _Upper>
using short_closed_interval = closed_interval<short, _Lower, _Upper>;

// long_closed_interval
//   type: closed_interval over long.
template<long _Lower,
         long _Upper>
using long_closed_interval = closed_interval<long, _Lower, _Upper>;

// long_long_closed_interval
//   type: closed_interval over long long.
template<long long _Lower,
         long long _Upper>
using long_long_closed_interval = closed_interval<long long, _Lower, _Upper>;

// bool_closed_interval
//   type: closed_interval over bool.
template<bool _Lower,
         bool _Upper>
using bool_closed_interval = closed_interval<bool, _Lower, _Upper>;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_CLOSED_INTERVAL_
