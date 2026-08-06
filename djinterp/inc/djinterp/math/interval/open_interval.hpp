/******************************************************************************
* djinterp [math]                                          open_interval.hpp
*
* Compile-time open interval (lower, upper).
*   Both endpoints are exclusive. The value type, bounds, and size type are
* all configurable via template parameters. Provides containment testing,
* overlap detection, clamping, normalization, and forward iteration over
* interior integral values.
*
* Also provides a runtime open interval (runtime_open_interval) for cases
* where the upper bound is not known until runtime.
*
* STRUCTURAL INTERFACE (for interval_traits):
*   - value_type, size_type
*   - static constexpr lower_bound, upper_bound
*   - static constexpr bool is_left_open  = true
*   - static constexpr bool is_right_open = true
*
* path:      /inc/djinterp/math/interval/open_interval.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2024.04.23
******************************************************************************/

#ifndef DJINTERP_MATH_OPEN_INTERVAL_
#define DJINTERP_MATH_OPEN_INTERVAL_ 1

// std
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    COMPILE-TIME OPEN INTERVAL
// ============================================================================

// open_interval
//   struct: compile-time open interval (_Lower, _Upper) over _Type.
// Both endpoints are exclusive. For integral types the interior values
// are {_Lower+1, _Lower+2, ..., _Upper-1}.
template<typename _Type,
         _Type    _Lower,
         _Type    _Upper,
         typename _SizeType = std::size_t>
struct open_interval
{
    // ---- type aliases -------------------------------------------------------

    using value_type      = _Type;
    using size_type       = _SizeType;

    // ---- static constants ---------------------------------------------------

    static constexpr value_type lower_bound   = _Lower;
    static constexpr value_type upper_bound   = _Upper;
    static constexpr value_type step          = static_cast<_Type>(0);
    static constexpr bool       is_left_open  = true;
    static constexpr bool       is_right_open = true;

    static_assert(_Lower < _Upper,
                  "open_interval: _Lower must be < _Upper for a "
                  "non-empty open interval.");

    // ---- static member functions --------------------------------------------

    // size
    //   returns the number of interior integral values.
    // For integral types this is max(0, upper - lower - 1).
    static constexpr size_type
    size
    () noexcept
    {
        // open interval excludes both endpoints
        if constexpr ((_Upper - _Lower) <= 1)
        {
            return size_type{0};
        }
        else
        {
            return static_cast<size_type>(_Upper - _Lower - 1);
        }
    }

    // contains
    //   checks whether _value lies within (_Lower, _Upper).
    // Both endpoints are excluded.
    static constexpr bool
    contains
    (
        const value_type& _value
    ) noexcept
    {
        return ( (_value > _Lower) &&
                 (_value < _Upper) );
    }

    // clamp
    //   constrains _value to the nearest interior value.
    // Values at or below _Lower map to _Lower+1; values at or above
    // _Upper map to _Upper-1.
    static constexpr value_type
    clamp
    (
        const value_type& _value
    ) noexcept
    {
        if (_value <= _Lower)
        {
            return static_cast<value_type>(_Lower + 1);
        }

        if (_value >= _Upper)
        {
            return static_cast<value_type>(_Upper - 1);
        }

        return _value;
    }

    // is_valid
    //   returns true if the interval is well-formed (_Lower < _Upper).
    static constexpr bool
    is_valid
    () noexcept
    {
        return (_Lower < _Upper);
    }

    // is_empty
    //   returns true if the interval contains no interior integral values.
    static constexpr bool
    is_empty
    () noexcept
    {
        return ((_Upper - _Lower) <= 1);
    }

    // normalize
    //   maps _value to [0.0, 1.0] relative to the open interval.
    // Normalization is relative to the full span (_Lower, _Upper).
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
    //   checks whether this interval overlaps another open_interval.
    template<_Type _OtherLower,
             _Type _OtherUpper>
    static constexpr bool
    overlaps
    (
        const open_interval<_Type,
                            _OtherLower,
                            _OtherUpper,
                            _SizeType>&
    ) noexcept
    {
        return !( (_Upper <= _OtherLower) ||
                  (_Lower >= _OtherUpper) );
    }

    // to_string
    //   returns a string representation: "(lower, upper)".
    static std::string
    to_string
    ()
    {
        return ( "(" + std::to_string(_Lower) +
                 ", " + std::to_string(_Upper) + ")" );
    }

    // ---- iterator -----------------------------------------------------------

    // iterator
    //   struct: forward iterator over interior integral values.
    // Iterates from _Lower+1 to _Upper-1 inclusive.
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

    // begin at _Lower+1 (first interior value)
    static constexpr iterator begin() noexcept
    {
        return iterator(static_cast<value_type>(_Lower + 1));
    }

    // end at _Upper (one past _Upper-1, the last interior value)
    static constexpr iterator end() noexcept
    {
        return iterator(_Upper);
    }
};


// ============================================================================
// II.   RUNTIME OPEN INTERVAL
// ============================================================================

// runtime_open_interval
//   struct: open interval with a compile-time lower bound and runtime
// upper bound. Useful when the upper bound is determined at runtime.
template<typename _Type,
         _Type    _Lower   = _Type{},
         typename _SizeType = std::size_t>
struct runtime_open_interval
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _Type;
    using size_type  = _SizeType;

    // ---- static constants ---------------------------------------------------

    static constexpr value_type lower_bound   = _Lower;
    static constexpr value_type step          = static_cast<_Type>(0);
    static constexpr bool       is_left_open  = true;
    static constexpr bool       is_right_open = true;

    // ---- construction -------------------------------------------------------

    constexpr explicit
    runtime_open_interval
    (
        value_type _end_val
    )
        : m_upper(_end_val)
    {
        // runtime check: upper must be > lower for open interval
        if (_end_val <= _Lower)
        {
            throw std::invalid_argument(
                "runtime_open_interval: upper bound must "
                "be > lower bound.");
        }
    }

    // ---- member functions ---------------------------------------------------

    // upper_bound
    //   returns the runtime upper bound.
    constexpr value_type
    get_upper_bound
    () const noexcept
    {
        return m_upper;
    }

    // size
    //   returns the number of interior integral values.
    constexpr size_type
    size
    () const noexcept
    {
        // open: excludes both endpoints
        value_type span = m_upper - _Lower;

        if (span <= 1)
        {
            return size_type{0};
        }

        return static_cast<size_type>(span - 1);
    }

    // contains
    //   checks whether _value lies within (_Lower, m_upper).
    constexpr bool
    contains
    (
        const value_type& _value
    ) const noexcept
    {
        return ( (_value > _Lower) &&
                 (_value < m_upper) );
    }

    // clamp
    //   constrains _value to the nearest interior value.
    constexpr value_type
    clamp
    (
        const value_type& _value
    ) const noexcept
    {
        if (_value <= _Lower)
        {
            return static_cast<value_type>(_Lower + 1);
        }

        if (_value >= m_upper)
        {
            return static_cast<value_type>(m_upper - 1);
        }

        return _value;
    }

    // overlaps
    //   checks whether this interval overlaps another runtime interval.
    bool
    overlaps
    (
        const runtime_open_interval& _other
    ) const noexcept
    {
        return !( (m_upper       <= _Lower) ||
                  (_Lower        >= _other.m_upper) );
    }

    // normalize
    //   maps _value to [0.0, 1.0] relative to the interval span.
    template<typename _FloatType = double>
    constexpr _FloatType
    normalize
    (
        const value_type& _value
    ) const noexcept
    {
        if (_Lower == m_upper)
        {
            return _FloatType{0};
        }

        return ( static_cast<_FloatType>(_value - _Lower) /
                 static_cast<_FloatType>(m_upper - _Lower) );
    }

    // to_string
    //   returns a string representation: "(lower, upper)".
    std::string
    to_string
    () const
    {
        return ( "(" + std::to_string(_Lower) +
                 ", " + std::to_string(m_upper) + ")" );
    }

    // ---- iterator -----------------------------------------------------------

    // iterator
    //   struct: forward iterator over interior integral values.
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

    constexpr iterator begin() const noexcept
    {
        return iterator(static_cast<value_type>(_Lower + 1));
    }

    constexpr iterator end() const noexcept
    {
        return iterator(m_upper);
    }

private:
    value_type m_upper;
};


// ============================================================================
// III.  TYPE ALIASES
// ============================================================================

// int_open_interval
//   type: open_interval over int.
template<int _Lower,
         int _Upper>
using int_open_interval = open_interval<int, _Lower, _Upper>;

// index_open_interval
//   type: open_interval over std::size_t.
template<std::size_t _Lower,
         std::size_t _Upper>
using index_open_interval = open_interval<std::size_t, _Lower, _Upper>;

// char_open_interval
//   type: open_interval over char.
template<char _Lower,
         char _Upper>
using char_open_interval = open_interval<char, _Lower, _Upper>;

// uint8_open_interval
//   type: open_interval over uint8_t.
template<std::uint8_t _Lower,
         std::uint8_t _Upper>
using uint8_open_interval = open_interval<std::uint8_t, _Lower, _Upper>;

// int64_open_interval
//   type: open_interval over int64_t.
template<std::int64_t _Lower,
         std::int64_t _Upper>
using int64_open_interval = open_interval<std::int64_t, _Lower, _Upper>;

// uint64_open_interval
//   type: open_interval over uint64_t.
template<std::uint64_t _Lower,
         std::uint64_t _Upper>
using uint64_open_interval = open_interval<std::uint64_t, _Lower, _Upper>;

// int32_open_interval
//   type: open_interval over int32_t.
template<std::int32_t _Lower,
         std::int32_t _Upper>
using int32_open_interval = open_interval<std::int32_t, _Lower, _Upper>;

// uint32_open_interval
//   type: open_interval over uint32_t.
template<std::uint32_t _Lower,
         std::uint32_t _Upper>
using uint32_open_interval = open_interval<std::uint32_t, _Lower, _Upper>;

// int16_open_interval
//   type: open_interval over int16_t.
template<std::int16_t _Lower,
         std::int16_t _Upper>
using int16_open_interval = open_interval<std::int16_t, _Lower, _Upper>;

// uint16_open_interval
//   type: open_interval over uint16_t.
template<std::uint16_t _Lower,
         std::uint16_t _Upper>
using uint16_open_interval = open_interval<std::uint16_t, _Lower, _Upper>;

// short_open_interval
//   type: open_interval over short.
template<short _Lower,
         short _Upper>
using short_open_interval = open_interval<short, _Lower, _Upper>;

// long_open_interval
//   type: open_interval over long.
template<long _Lower,
         long _Upper>
using long_open_interval = open_interval<long, _Lower, _Upper>;

// long_long_open_interval
//   type: open_interval over long long.
template<long long _Lower,
         long long _Upper>
using long_long_open_interval = open_interval<long long, _Lower, _Upper>;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_OPEN_INTERVAL_
