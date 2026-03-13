/******************************************************************************
* djinterp [maths]                                      discrete_interval.hpp
*
* Compile-time discrete interval [lower, upper] with step.
*   A closed interval whose iterator advances by a configurable step size
* rather than by 1. The value type, bounds, step, and size type are all
* template parameters. Useful for representing arithmetic sequences,
* sampled ranges, and strided index sets.
*
* VALUES IN THE INTERVAL:
*   { _Lower, _Lower + _Step, _Lower + 2*_Step, ... }
*   up to and including _Upper (if _Upper is reachable by the stride).
*
* STRUCTURAL INTERFACE (for interval_traits):
*   - value_type, size_type
*   - static constexpr lower_bound, upper_bound
*   - static constexpr bool is_left_open  = false
*   - static constexpr bool is_right_open = false
*
* path:      /inc/maths/discrete_interval.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2024.04.23
******************************************************************************/

#ifndef DJINTERP_MATHS_DISCRETE_INTERVAL_
#define DJINTERP_MATHS_DISCRETE_INTERVAL_ 1

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include "../djinterp.h"


NS_DJINTERP
NS_MATHS

// ============================================================================
// I.    DISCRETE INTERVAL
// ============================================================================

// discrete_interval
//   struct: compile-time discrete interval [_Lower, _Upper] with stride
// _Step over _Type. Endpoints are inclusive (closed). The interval
// generates values {_Lower, _Lower+_Step, _Lower+2*_Step, ...} up to
// and including _Upper.
template<typename _Type,
         _Type    _Lower,
         _Type    _Upper,
         _Type    _Step     = static_cast<_Type>(1),
         typename _SizeType = std::size_t>
struct discrete_interval
{
    // ---- type aliases -------------------------------------------------------

    using value_type = _Type;
    using size_type  = _SizeType;

    // ---- static constants ---------------------------------------------------

    static constexpr value_type lower_bound   = _Lower;
    static constexpr value_type upper_bound   = _Upper;
    static constexpr value_type step          = _Step;
    static constexpr bool       is_left_open  = false;
    static constexpr bool       is_right_open = false;

    static_assert(_Lower <= _Upper,
                  "discrete_interval: _Lower must be <= _Upper.");
    static_assert(_Step > static_cast<_Type>(0),
                  "discrete_interval: _Step must be > 0.");

    // ---- static member functions --------------------------------------------

    // size
    //   returns the number of discrete values in the interval.
    // Computed as floor((upper - lower) / step) + 1.
    static constexpr size_type
    size
    () noexcept
    {
        return static_cast<size_type>(
            (_Upper - _Lower) / _Step + 1
        );
    }

    // contains
    //   checks whether _value lies within [_Lower, _Upper] AND is
    // reachable from _Lower by an integral number of steps.
    static constexpr bool
    contains
    (
        const value_type& _value
    ) noexcept
    {
        // bounds check
        if ( (_value < _Lower) ||
             (_value > _Upper) )
        {
            return false;
        }

        // stride alignment check
        return ((_value - _Lower) % _Step == 0);
    }

    // contains_in_range
    //   checks whether _value lies within [_Lower, _Upper] without
    // requiring stride alignment. Useful for general range queries.
    static constexpr bool
    contains_in_range
    (
        const value_type& _value
    ) noexcept
    {
        return ( (_value >= _Lower) &&
                 (_value <= _Upper) );
    }

    // clamp
    //   constrains _value to the nearest discrete value in the interval.
    // Rounds down to the nearest step-aligned value.
    static constexpr value_type
    clamp
    (
        const value_type& _value
    ) noexcept
    {
        // below range
        if (_value < _Lower)
        {
            return _Lower;
        }

        // above range: find the largest step-aligned value <= _Upper
        if (_value > _Upper)
        {
            value_type steps = (_Upper - _Lower) / _Step;

            return static_cast<value_type>(_Lower + steps * _Step);
        }

        // within range: snap to nearest step-aligned value (round down)
        value_type offset = (_value - _Lower) / _Step;

        return static_cast<value_type>(_Lower + offset * _Step);
    }

    // clamp_nearest
    //   constrains _value to the nearest discrete value, rounding to
    // whichever step-aligned value is closer.
    static constexpr value_type
    clamp_nearest
    (
        const value_type& _value
    ) noexcept
    {
        // below range
        if (_value < _Lower)
        {
            return _Lower;
        }

        // above range
        value_type max_steps = (_Upper - _Lower) / _Step;
        value_type max_val   = static_cast<value_type>(
            _Lower + max_steps * _Step
        );

        if (_value > max_val)
        {
            return max_val;
        }

        // within range: find nearest
        value_type offset    = (_value - _Lower) / _Step;
        value_type low_snap  = static_cast<value_type>(
            _Lower + offset * _Step
        );
        value_type high_snap = static_cast<value_type>(
            low_snap + _Step
        );

        // choose the closer snap point (if high_snap is still in range)
        if ( (high_snap <= _Upper) &&
             ((_value - low_snap) > (high_snap - _value)) )
        {
            return high_snap;
        }

        return low_snap;
    }

    // is_valid
    //   returns true if the interval is well-formed.
    static constexpr bool
    is_valid
    () noexcept
    {
        return ( (_Lower <= _Upper) &&
                 (_Step > static_cast<_Type>(0)) );
    }

    // normalize
    //   maps _value to [0.0, 1.0] relative to the full span.
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

    // normalize_discrete
    //   maps _value to [0.0, 1.0] relative to the discrete step count.
    // Step index 0 maps to 0.0; the last step maps to 1.0.
    template<typename _FloatType = double>
    static constexpr _FloatType
    normalize_discrete
    (
        const value_type& _value
    ) noexcept
    {
        constexpr size_type count = size();

        if constexpr (count <= 1)
        {
            return _FloatType{0};
        }
        else
        {
            value_type step_index = (_value - _Lower) / _Step;

            return ( static_cast<_FloatType>(step_index) /
                     static_cast<_FloatType>(count - 1) );
        }
    }

    // at
    //   returns the discrete value at the given step index.
    // Index 0 returns _Lower, index 1 returns _Lower+_Step, etc.
    static constexpr value_type
    at
    (
        size_type _index
    ) noexcept
    {
        return static_cast<value_type>(
            _Lower + static_cast<_Type>(_index) * _Step
        );
    }

    // index_of
    //   returns the step index of _value, or size() if _value is not
    // a member of the discrete interval.
    static constexpr size_type
    index_of
    (
        const value_type& _value
    ) noexcept
    {
        if (!contains(_value))
        {
            return size();
        }

        return static_cast<size_type>((_value - _Lower) / _Step);
    }

    // last
    //   returns the largest discrete value in the interval.
    static constexpr value_type
    last
    () noexcept
    {
        value_type steps = (_Upper - _Lower) / _Step;

        return static_cast<value_type>(_Lower + steps * _Step);
    }

    // overlaps
    //   checks whether the ranges [_Lower, _Upper] and
    // [_OtherLower, _OtherUpper] overlap (ignoring step alignment).
    template<_Type _OtherLower,
             _Type _OtherUpper,
             _Type _OtherStep>
    static constexpr bool
    overlaps
    (
        const discrete_interval<_Type,
                                _OtherLower,
                                _OtherUpper,
                                _OtherStep,
                                _SizeType>&
    ) noexcept
    {
        return !( (_Upper < _OtherLower) ||
                  (_Lower > _OtherUpper) );
    }

    // to_string
    //   returns a string representation: "[lower:step:upper]".
    static std::string
    to_string
    ()
    {
        return ( "[" + std::to_string(_Lower) +
                 ":" + std::to_string(_Step)  +
                 ":" + std::to_string(_Upper) + "]" );
    }

    // ---- iterator -----------------------------------------------------------

    // iterator
    //   struct: forward iterator that advances by _Step.
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
            m_current = static_cast<value_type>(
                m_current + _Step
            );

            return *this;
        }

        constexpr iterator operator++(int) noexcept
        {
            iterator tmp = *this;
            m_current = static_cast<value_type>(
                m_current + _Step
            );

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
            // use >= to catch overshoot past the sentinel
            return (m_current < _other.m_current);
        }
    };

    static constexpr iterator begin() noexcept
    {
        return iterator(_Lower);
    }

    static constexpr iterator end() noexcept
    {
        // sentinel: one step past the last valid value
        value_type steps   = (_Upper - _Lower) / _Step;
        value_type past_end = static_cast<value_type>(
            _Lower + (steps + 1) * _Step
        );

        return iterator(past_end);
    }
};


// ============================================================================
// II.   TYPE ALIASES
// ============================================================================

// int_discrete_interval
//   type: discrete_interval over int.
template<int _Lower,
         int _Upper,
         int _Step = 1>
using int_discrete_interval =
    discrete_interval<int, _Lower, _Upper, _Step>;

// index_discrete_interval
//   type: discrete_interval over std::size_t.
template<std::size_t _Lower,
         std::size_t _Upper,
         std::size_t _Step = 1>
using index_discrete_interval =
    discrete_interval<std::size_t, _Lower, _Upper, _Step>;

// char_discrete_interval
//   type: discrete_interval over char.
template<char _Lower,
         char _Upper,
         char _Step = 1>
using char_discrete_interval =
    discrete_interval<char, _Lower, _Upper, _Step>;

// uint8_discrete_interval
//   type: discrete_interval over uint8_t.
template<std::uint8_t _Lower,
         std::uint8_t _Upper,
         std::uint8_t _Step = 1>
using uint8_discrete_interval =
    discrete_interval<std::uint8_t, _Lower, _Upper, _Step>;

// int64_discrete_interval
//   type: discrete_interval over int64_t.
template<std::int64_t _Lower,
         std::int64_t _Upper,
         std::int64_t _Step = 1>
using int64_discrete_interval =
    discrete_interval<std::int64_t, _Lower, _Upper, _Step>;

// uint64_discrete_interval
//   type: discrete_interval over uint64_t.
template<std::uint64_t _Lower,
         std::uint64_t _Upper,
         std::uint64_t _Step = 1>
using uint64_discrete_interval =
    discrete_interval<std::uint64_t, _Lower, _Upper, _Step>;

// int32_discrete_interval
//   type: discrete_interval over int32_t.
template<std::int32_t _Lower,
         std::int32_t _Upper,
         std::int32_t _Step = 1>
using int32_discrete_interval =
    discrete_interval<std::int32_t, _Lower, _Upper, _Step>;

// uint32_discrete_interval
//   type: discrete_interval over uint32_t.
template<std::uint32_t _Lower,
         std::uint32_t _Upper,
         std::uint32_t _Step = 1>
using uint32_discrete_interval =
    discrete_interval<std::uint32_t, _Lower, _Upper, _Step>;

// int16_discrete_interval
//   type: discrete_interval over int16_t.
template<std::int16_t _Lower,
         std::int16_t _Upper,
         std::int16_t _Step = 1>
using int16_discrete_interval =
    discrete_interval<std::int16_t, _Lower, _Upper, _Step>;

// uint16_discrete_interval
//   type: discrete_interval over uint16_t.
template<std::uint16_t _Lower,
         std::uint16_t _Upper,
         std::uint16_t _Step = 1>
using uint16_discrete_interval =
    discrete_interval<std::uint16_t, _Lower, _Upper, _Step>;

// short_discrete_interval
//   type: discrete_interval over short.
template<short _Lower,
         short _Upper,
         short _Step = 1>
using short_discrete_interval =
    discrete_interval<short, _Lower, _Upper, _Step>;

// long_discrete_interval
//   type: discrete_interval over long.
template<long _Lower,
         long _Upper,
         long _Step = 1>
using long_discrete_interval =
    discrete_interval<long, _Lower, _Upper, _Step>;

// long_long_discrete_interval
//   type: discrete_interval over long long.
template<long long _Lower,
         long long _Upper,
         long long _Step = 1>
using long_long_discrete_interval =
    discrete_interval<long long, _Lower, _Upper, _Step>;


NS_END  // maths
NS_END  // djinterp


#endif  // DJINTERP_MATHS_DISCRETE_INTERVAL_
