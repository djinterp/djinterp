/******************************************************************************
* djinterp [math]                                             interval.hpp
*
* Unified compile-time interval template.
*   This header provides a single, fully generic interval type parameterized
* by value type, bounds, boundary openness, optional discrete step, and
* size type. It subsumes the functionality of closed_interval,
* open_interval, and discrete_interval into one template and exposes
* compile-time conversion aliases between all interval configurations.
*
* TEMPLATE PARAMETERS:
*   _Type      - the value/element type (any arithmetic or ordered type)
*   _Lower     - the lower bound
*   _Upper     - the upper bound
*   _LeftOpen  - true if the left endpoint is excluded  (default: false)
*   _RightOpen - true if the right endpoint is excluded (default: false)
*   _Step      - discrete stride; 0 means continuous    (default: 0)
*   _SizeType  - unsigned type used for counts/indices  (default: size_t)
*
* BOUNDARY CONFIGURATIONS:
*   [a, b]   closed        _LeftOpen=false, _RightOpen=false  (default)
*   (a, b)   open          _LeftOpen=true,  _RightOpen=true
*   [a, b)   half-open-R   _LeftOpen=false, _RightOpen=true
*   (a, b]   half-open-L   _LeftOpen=true,  _RightOpen=false
*
* DISCRETE vs CONTINUOUS:
*   _Step == 0  => continuous (unit-stride iteration for integral types)
*   _Step >  0  => discrete   (stride-_Step iteration, alignment checks)
*
* CONVERSION TYPE ALIASES (nested):
*   as_closed, as_open, as_half_open_left, as_half_open_right,
*   as_continuous, with_step<S>, with_bounds<L,U>, with_size_type<S>
*
* FREE-STANDING CONVERSION METAFUNCTION:
*   interval_cast<_Target, _Source> - converts between interval configs
*
* STRUCTURAL INTERFACE (for interval_traits):
*   value_type, size_type, lower_bound, upper_bound, step,
*   is_left_open, is_right_open
*
* 
* path:      /inc/djinterp/math/interval/interval.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2024.04.23
******************************************************************************/

#ifndef DJINTERP_MATH_INTERVAL_
#define DJINTERP_MATH_INTERVAL_ 1

// std
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"
#include "../math.hpp"
#include "./closed_interval.hpp"
#include "./open_interval.hpp"
#include "./discrete_interval.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    UNIFIED INTERVAL
// ============================================================================

// interval
//   struct: compile-time interval over _Type with configurable bounds,
// boundary openness, discrete step, and size type. When _Step is 0 the
// interval is continuous; when _Step > 0 it is discrete. All member
// functions adapt their semantics to the boundary configuration
// automatically via if-constexpr dispatch.
template<typename _Type,
         _Type    _Lower,
         _Type    _Upper,
         bool     _LeftOpen  = false,
         bool     _RightOpen = false,
         _Type    _Step      = static_cast<_Type>(0),
         typename _SizeType  = std::size_t>
struct interval
{
private:
    // ---- internal constants -------------------------------------------------

    // effective inclusive lower bound
    static constexpr _Type m_eff_lower =
        _LeftOpen
            ? static_cast<_Type>(_Lower + 1)
            : _Lower;

    // effective inclusive upper bound
    static constexpr _Type m_eff_upper =
        _RightOpen
            ? static_cast<_Type>(_Upper - 1)
            : _Upper;

    // whether this is a discrete interval
    static constexpr bool m_is_discrete =
        (_Step > static_cast<_Type>(0));

public:
    // ---- type aliases -------------------------------------------------------

    using value_type = _Type;
    using size_type  = _SizeType;

    // ---- static constants ---------------------------------------------------

    static constexpr value_type lower_bound   = _Lower;
    static constexpr value_type upper_bound   = _Upper;
    static constexpr value_type step          = _Step;
    static constexpr bool       is_left_open  = _LeftOpen;
    static constexpr bool       is_right_open = _RightOpen;
    static constexpr bool       is_discrete   = m_is_discrete;

    static_assert(_Lower <= _Upper,
                  "interval: _Lower must be <= _Upper.");
    static_assert( (_Step == static_cast<_Type>(0)) ||
                   (_Step > static_cast<_Type>(0)),
                  "interval: _Step must be >= 0 "
                  "(0 = continuous, > 0 = discrete).");


    // =========================================================================
    // II.   CONVERSION TYPE ALIASES
    // =========================================================================

    // as_closed
    //   type: this interval with both endpoints closed.
    // For integral types, adjusts bounds inward to match the effective
    // inclusive range. e.g. open (2,8) becomes closed [3,7].
    using as_closed = interval<_Type,
                               m_eff_lower,
                               m_eff_upper,
                               false,
                               false,
                               _Step,
                               _SizeType>;

    // as_open
    //   type: this interval with both endpoints open.
    // Widens bounds outward by one unit so that the interior matches.
    // e.g. closed [3,7] becomes open (2,8).
    using as_open = interval<_Type,
                             static_cast<_Type>(m_eff_lower - 1),
                             static_cast<_Type>(m_eff_upper + 1),
                             true,
                             true,
                             _Step,
                             _SizeType>;

    // as_half_open_right
    //   type: this interval as [effective_lower, effective_upper + 1).
    using as_half_open_right = interval<_Type,
                                        m_eff_lower,
                                        static_cast<_Type>(
                                            m_eff_upper + 1),
                                        false,
                                        true,
                                        _Step,
                                        _SizeType>;

    // as_half_open_left
    //   type: this interval as (effective_lower - 1, effective_upper].
    using as_half_open_left = interval<_Type,
                                       static_cast<_Type>(
                                           m_eff_lower - 1),
                                       m_eff_upper,
                                       true,
                                       false,
                                       _Step,
                                       _SizeType>;

    // as_continuous
    //   type: this interval with step removed (continuous).
    using as_continuous = interval<_Type,
                                   _Lower,
                                   _Upper,
                                   _LeftOpen,
                                   _RightOpen,
                                   static_cast<_Type>(0),
                                   _SizeType>;

    // with_step
    //   type: this interval with a specified discrete step.
    template<_Type _NewStep>
    using with_step = interval<_Type,
                               _Lower,
                               _Upper,
                               _LeftOpen,
                               _RightOpen,
                               _NewStep,
                               _SizeType>;

    // with_bounds
    //   type: this interval with different bounds but same configuration.
    template<_Type _NewLower,
             _Type _NewUpper>
    using with_bounds = interval<_Type,
                                 _NewLower,
                                 _NewUpper,
                                 _LeftOpen,
                                 _RightOpen,
                                 _Step,
                                 _SizeType>;

    // with_size_type
    //   type: this interval with a different size type.
    template<typename _NewSizeType>
    using with_size_type = interval<_Type,
                                    _Lower,
                                    _Upper,
                                    _LeftOpen,
                                    _RightOpen,
                                    _Step,
                                    _NewSizeType>;

    // to_closed_interval
    //   type: equivalent closed_interval from closed_interval.hpp.
    using to_closed_interval = closed_interval<_Type,
                                               m_eff_lower,
                                               m_eff_upper,
                                               _SizeType>;

    // to_open_interval
    //   type: equivalent open_interval from open_interval.hpp.
    using to_open_interval = open_interval<_Type,
                                           static_cast<_Type>(
                                               m_eff_lower - 1),
                                           static_cast<_Type>(
                                               m_eff_upper + 1),
                                           _SizeType>;

    // to_discrete_interval
    //   type: equivalent discrete_interval from discrete_interval.hpp.
    // Uses _Step if discrete, otherwise defaults to step of 1.
    using to_discrete_interval = discrete_interval<
        _Type,
        m_eff_lower,
        m_eff_upper,
        m_is_discrete
            ? _Step
            : static_cast<_Type>(1),
        _SizeType>;


    // =========================================================================
    // III.  SIZE / ELEMENT COUNT
    // =========================================================================

    // size
    //   returns the number of values in the interval.
    // Adapts to boundary openness and discrete step automatically.
    // For continuous intervals, counts integral positions.
    // For discrete intervals, counts step-aligned positions.
    static constexpr size_type
    size
    () noexcept
    {
        // empty check
        if constexpr (m_eff_lower > m_eff_upper)
        {
            return size_type{0};
        }
        else if constexpr (m_is_discrete)
        {
            // discrete: floor((eff_upper - eff_lower) / step) + 1
            return static_cast<size_type>(
                (m_eff_upper - m_eff_lower) / _Step + 1
            );
        }
        else
        {
            // continuous integral count
            return static_cast<size_type>(
                m_eff_upper - m_eff_lower + 1
            );
        }
    }

    // is_empty
    //   returns true if the interval contains no values.
    static constexpr bool
    is_empty
    () noexcept
    {
        return (m_eff_lower > m_eff_upper);
    }


    // =========================================================================
    // IV.   CONTAINMENT
    // =========================================================================

    // contains
    //   checks whether _value lies within the interval respecting
    // boundary openness and, for discrete intervals, step alignment.
    static constexpr bool
    contains
    (
        const value_type& _value
    ) noexcept
    {
        // left bound check
        if constexpr (_LeftOpen)
        {
            if (_value <= _Lower)
            {
                return false;
            }
        }
        else
        {
            if (_value < _Lower)
            {
                return false;
            }
        }

        // right bound check
        if constexpr (_RightOpen)
        {
            if (_value >= _Upper)
            {
                return false;
            }
        }
        else
        {
            if (_value > _Upper)
            {
                return false;
            }
        }

        // discrete alignment check
        if constexpr (m_is_discrete)
        {
            return ((_value - m_eff_lower) % _Step == 0);
        }

        return true;
    }

    // contains_in_range
    //   checks whether _value lies within the bounds without requiring
    // step alignment. Equivalent to contains() for continuous intervals.
    static constexpr bool
    contains_in_range
    (
        const value_type& _value
    ) noexcept
    {
        // left bound check
        bool left_ok = _LeftOpen
            ? (_value > _Lower)
            : (_value >= _Lower);

        // right bound check
        bool right_ok = _RightOpen
            ? (_value < _Upper)
            : (_value <= _Upper);

        return (left_ok && right_ok);
    }


    // =========================================================================
    // V.    CLAMPING
    // =========================================================================

    // clamp
    //   constrains _value to the nearest valid value in the interval.
    // For discrete intervals, snaps down to the nearest step-aligned
    // value within bounds.
    static constexpr value_type
    clamp
    (
        const value_type& _value
    ) noexcept
    {
        // below effective lower
        if (_value < m_eff_lower)
        {
            return m_eff_lower;
        }

        // above effective upper
        if (_value > m_eff_upper)
        {
            if constexpr (m_is_discrete)
            {
                // snap to largest step-aligned value
                value_type steps = (m_eff_upper - m_eff_lower) / _Step;

                return static_cast<value_type>(
                    m_eff_lower + steps * _Step
                );
            }
            else
            {
                return m_eff_upper;
            }
        }

        // within range
        if constexpr (m_is_discrete)
        {
            // snap down to nearest step-aligned value
            value_type offset = (_value - m_eff_lower) / _Step;

            return static_cast<value_type>(
                m_eff_lower + offset * _Step
            );
        }

        return _value;
    }

    // clamp_nearest
    //   constrains _value to the nearest discrete value, rounding to
    // whichever step-aligned value is closer. For continuous intervals,
    // behaves identically to clamp().
    static constexpr value_type
    clamp_nearest
    (
        const value_type& _value
    ) noexcept
    {
        if constexpr (!m_is_discrete)
        {
            return clamp(_value);
        }
        else
        {
            // below range
            if (_value < m_eff_lower)
            {
                return m_eff_lower;
            }

            // find the max step-aligned value
            value_type max_steps = (m_eff_upper - m_eff_lower) / _Step;
            value_type max_val   = static_cast<value_type>(
                m_eff_lower + max_steps * _Step
            );

            // above range
            if (_value > max_val)
            {
                return max_val;
            }

            // within range: find nearest
            value_type idx       = (_value - m_eff_lower) / _Step;
            value_type low_snap  = static_cast<value_type>(
                m_eff_lower + idx * _Step
            );
            value_type high_snap = static_cast<value_type>(
                low_snap + _Step
            );

            // choose the closer snap point
            if ( (high_snap <= m_eff_upper)               &&
                 ((_value - low_snap) > (high_snap - _value)) )
            {
                return high_snap;
            }

            return low_snap;
        }
    }


    // =========================================================================
    // VI.   NORMALIZATION
    // =========================================================================

    // normalize
    //   maps _value to [0.0, 1.0] relative to the full span.
    // Returns 0 for degenerate intervals where lower == upper.
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

    // normalize_effective
    //   maps _value to [0.0, 1.0] relative to the effective inclusive
    // span [m_eff_lower, m_eff_upper].
    template<typename _FloatType = double>
    static constexpr _FloatType
    normalize_effective
    (
        const value_type& _value
    ) noexcept
    {
        if constexpr (m_eff_lower == m_eff_upper)
        {
            return _FloatType{0};
        }
        else
        {
            return ( static_cast<_FloatType>(
                         _value - m_eff_lower) /
                     static_cast<_FloatType>(
                         m_eff_upper - m_eff_lower) );
        }
    }

    // normalize_discrete
    //   maps _value to [0.0, 1.0] relative to the discrete step count.
    // Step index 0 maps to 0.0; the last step maps to 1.0.
    // Falls back to normalize_effective for continuous intervals.
    template<typename _FloatType = double>
    static constexpr _FloatType
    normalize_discrete
    (
        const value_type& _value
    ) noexcept
    {
        if constexpr (!m_is_discrete)
        {
            return normalize_effective<_FloatType>(_value);
        }
        else
        {
            constexpr size_type count = size();

            if constexpr (count <= 1)
            {
                return _FloatType{0};
            }
            else
            {
                value_type step_index =
                    (_value - m_eff_lower) / _Step;

                return ( static_cast<_FloatType>(step_index) /
                         static_cast<_FloatType>(count - 1) );
            }
        }
    }


    // =========================================================================
    // VII.  DISCRETE ACCESS (enabled when _Step > 0)
    // =========================================================================

    // at
    //   returns the discrete value at the given step index.
    // Index 0 returns the effective lower bound.
    static constexpr value_type
    at
    (
        size_type _index
    ) noexcept
    {
        if constexpr (m_is_discrete)
        {
            return static_cast<value_type>(
                m_eff_lower +
                static_cast<_Type>(_index) * _Step
            );
        }
        else
        {
            // continuous: treat as unit-stride
            return static_cast<value_type>(
                m_eff_lower + static_cast<_Type>(_index)
            );
        }
    }

    // index_of
    //   returns the step index of _value, or size() if _value is not
    // a member of the interval.
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

        if constexpr (m_is_discrete)
        {
            return static_cast<size_type>(
                (_value - m_eff_lower) / _Step
            );
        }
        else
        {
            return static_cast<size_type>(
                _value - m_eff_lower
            );
        }
    }

    // last
    //   returns the largest valid value in the interval.
    // For discrete intervals, the largest step-aligned value.
    static constexpr value_type
    last
    () noexcept
    {
        if constexpr (m_is_discrete)
        {
            value_type steps =
                (m_eff_upper - m_eff_lower) / _Step;

            return static_cast<value_type>(
                m_eff_lower + steps * _Step
            );
        }
        else
        {
            return m_eff_upper;
        }
    }

    // first
    //   returns the smallest valid value in the interval.
    static constexpr value_type
    first
    () noexcept
    {
        return m_eff_lower;
    }


    // =========================================================================
    // VIII. OVERLAP DETECTION
    // =========================================================================

    // overlaps
    //   checks whether this interval overlaps with another interval
    // of the same value type. Compares effective inclusive ranges.
    template<_Type    _OtherLower,
             _Type    _OtherUpper,
             bool     _OtherLeftOpen  = false,
             bool     _OtherRightOpen = false,
             _Type    _OtherStep      = static_cast<_Type>(0),
             typename _OtherSizeType  = std::size_t>
    static constexpr bool
    overlaps
    (
        const interval<_Type,
                       _OtherLower,
                       _OtherUpper,
                       _OtherLeftOpen,
                       _OtherRightOpen,
                       _OtherStep,
                       _OtherSizeType>&
    ) noexcept
    {
        // compute effective bounds of the other interval
        constexpr _Type other_eff_lower =
            _OtherLeftOpen
                ? static_cast<_Type>(_OtherLower + 1)
                : _OtherLower;

        constexpr _Type other_eff_upper =
            _OtherRightOpen
                ? static_cast<_Type>(_OtherUpper - 1)
                : _OtherUpper;

        return !( (m_eff_upper < other_eff_lower) ||
                  (m_eff_lower > other_eff_upper) );
    }

    // overlaps (closed_interval interop)
    //   checks overlap with a closed_interval from closed_interval.hpp.
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
        return !( (m_eff_upper < _OtherLower) ||
                  (m_eff_lower > _OtherUpper) );
    }

    // overlaps (open_interval interop)
    //   checks overlap with an open_interval from open_interval.hpp.
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
        constexpr _Type other_eff_lower =
            static_cast<_Type>(_OtherLower + 1);
        constexpr _Type other_eff_upper =
            static_cast<_Type>(_OtherUpper - 1);

        return !( (m_eff_upper < other_eff_lower) ||
                  (m_eff_lower > other_eff_upper) );
    }

    // overlaps (discrete_interval interop)
    //   checks overlap with a discrete_interval from
    // discrete_interval.hpp.
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
        return !( (m_eff_upper < _OtherLower) ||
                  (m_eff_lower > _OtherUpper) );
    }


    // =========================================================================
    // IX.   VALIDATION
    // =========================================================================

    // is_valid
    //   returns true if the interval is well-formed.
    static constexpr bool
    is_valid
    () noexcept
    {
        if constexpr (m_is_discrete)
        {
            return ( (_Lower <= _Upper) &&
                     (_Step > static_cast<_Type>(0)) );
        }
        else
        {
            return (_Lower <= _Upper);
        }
    }

    // is_degenerate
    //   returns true if the interval contains exactly one value.
    static constexpr bool
    is_degenerate
    () noexcept
    {
        return (size() == 1);
    }


    // =========================================================================
    // X.    STRING REPRESENTATION
    // =========================================================================

    // to_string
    //   returns a human-readable representation of the interval.
    // Closed: [a, b], Open: (a, b), Half: [a, b) / (a, b]
    // Discrete: appends :step, e.g. [a:s:b]
    static std::string
    to_string
    ()
    {
        std::string result;

        // left bracket
        result += _LeftOpen ? "(" : "[";

        // body
        if constexpr (m_is_discrete)
        {
            result += std::to_string(_Lower);
            result += ":";
            result += std::to_string(_Step);
            result += ":";
            result += std::to_string(_Upper);
        }
        else
        {
            result += std::to_string(_Lower);
            result += ", ";
            result += std::to_string(_Upper);
        }

        // right bracket
        result += _RightOpen ? ")" : "]";

        return result;
    }


    // =========================================================================
    // XI.   ITERATOR
    // =========================================================================

    // iterator
    //   struct: forward iterator over the values in the interval.
    // Advances by _Step for discrete intervals, by 1 for continuous.
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
            if constexpr (m_is_discrete)
            {
                m_current = static_cast<value_type>(
                    m_current + _Step
                );
            }
            else
            {
                ++m_current;
            }

            return *this;
        }

        constexpr iterator operator++(int) noexcept
        {
            iterator tmp = *this;

            if constexpr (m_is_discrete)
            {
                m_current = static_cast<value_type>(
                    m_current + _Step
                );
            }
            else
            {
                ++m_current;
            }

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
            if constexpr (m_is_discrete)
            {
                // use < to catch overshoot past sentinel
                return (m_current < _other.m_current);
            }
            else
            {
                return (m_current != _other.m_current);
            }
        }
    };

    // begin
    //   returns an iterator to the first valid value.
    static constexpr iterator begin() noexcept
    {
        return iterator(m_eff_lower);
    }

    // end
    //   returns a past-the-end sentinel iterator.
    static constexpr iterator end() noexcept
    {
        if constexpr (m_is_discrete)
        {
            value_type steps =
                (m_eff_upper - m_eff_lower) / _Step;
            value_type past_end = static_cast<value_type>(
                m_eff_lower + (steps + 1) * _Step
            );

            return iterator(past_end);
        }
        else
        {
            return iterator(
                static_cast<value_type>(m_eff_upper + 1)
            );
        }
    }
};


// ============================================================================
// XII.  INTER-TYPE CONVERSION METAFUNCTION
// ============================================================================

NS_INTERNAL

    // interval_cast_helper
    //   helper: primary template (undefined).
    template<template<typename, auto, auto, auto...> typename _Target,
             typename                                        _Source>
    struct interval_cast_helper;

    // interval_cast_helper => closed_interval
    //   helper: converts any interval to closed_interval.
    template<typename _Source>
    struct interval_cast_to_closed
    {
    private:
        using vt = typename _Source::value_type;
        using st = typename _Source::size_type;

        static constexpr vt eff_lo =
            _Source::is_left_open
                ? static_cast<vt>(_Source::lower_bound + 1)
                : _Source::lower_bound;

        static constexpr vt eff_hi =
            _Source::is_right_open
                ? static_cast<vt>(_Source::upper_bound - 1)
                : _Source::upper_bound;

    public:
        using type = closed_interval<vt, eff_lo, eff_hi, st>;
    };

    // interval_cast_to_open
    //   helper: converts any interval to open_interval.
    template<typename _Source>
    struct interval_cast_to_open
    {
    private:
        using vt = typename _Source::value_type;
        using st = typename _Source::size_type;

        static constexpr vt eff_lo =
            _Source::is_left_open
                ? static_cast<vt>(_Source::lower_bound + 1)
                : _Source::lower_bound;

        static constexpr vt eff_hi =
            _Source::is_right_open
                ? static_cast<vt>(_Source::upper_bound - 1)
                : _Source::upper_bound;

    public:
        using type = open_interval<vt,
                                   static_cast<vt>(eff_lo - 1),
                                   static_cast<vt>(eff_hi + 1),
                                   st>;
    };

    // interval_cast_to_discrete
    //   helper: converts any interval to discrete_interval with a
    // given step. If the source has a step > 0, it is preserved;
    // otherwise defaults to 1.
    template<typename _Source>
    struct interval_cast_to_discrete
    {
    private:
        using vt = typename _Source::value_type;
        using st = typename _Source::size_type;

        static constexpr vt eff_lo =
            _Source::is_left_open
                ? static_cast<vt>(_Source::lower_bound + 1)
                : _Source::lower_bound;

        static constexpr vt eff_hi =
            _Source::is_right_open
                ? static_cast<vt>(_Source::upper_bound - 1)
                : _Source::upper_bound;

        static constexpr vt src_step = _Source::step;

        static constexpr vt resolved_step =
            (src_step > static_cast<vt>(0))
                ? src_step
                : static_cast<vt>(1);

    public:
        using type = discrete_interval<vt,
                                       eff_lo,
                                       eff_hi,
                                       resolved_step,
                                       st>;
    };

    // interval_cast_to_interval
    //   helper: converts any sub-module interval back to unified
    // interval, preserving all properties.
    template<typename _Source>
    struct interval_cast_to_interval
    {
    private:
        using vt = typename _Source::value_type;
        using st = typename _Source::size_type;

        static constexpr vt src_step = _Source::step;

    public:
        using type = interval<vt,
                              _Source::lower_bound,
                              _Source::upper_bound,
                              _Source::is_left_open,
                              _Source::is_right_open,
                              src_step,
                              st>;
    };

NS_END  // internal

// to_closed_interval_t
//   type: converts any interval-like type to closed_interval.
template<typename _Source>
using to_closed_interval_t =
    typename internal::interval_cast_to_closed<_Source>::type;

// to_open_interval_t
//   type: converts any interval-like type to open_interval.
template<typename _Source>
using to_open_interval_t =
    typename internal::interval_cast_to_open<_Source>::type;

// to_discrete_interval_t
//   type: converts any interval-like type to discrete_interval.
template<typename _Source>
using to_discrete_interval_t =
    typename internal::interval_cast_to_discrete<_Source>::type;

// to_interval_t
//   type: converts any sub-module interval to the unified interval.
template<typename _Source>
using to_interval_t =
    typename internal::interval_cast_to_interval<_Source>::type;


// ============================================================================
// XIII. CONVENIENCE TYPE ALIASES
// ============================================================================

// --- closed (default) -------------------------------------------------------

// int_interval
//   type: closed continuous interval over int.
template<int _Lower,
         int _Upper>
using int_interval = interval<int, _Lower, _Upper>;

// index_interval
//   type: closed continuous interval over std::size_t.
template<std::size_t _Lower,
         std::size_t _Upper>
using index_interval = interval<std::size_t, _Lower, _Upper>;

// char_interval
//   type: closed continuous interval over char.
template<char _Lower,
         char _Upper>
using char_interval = interval<char, _Lower, _Upper>;

// uint8_interval
//   type: closed continuous interval over uint8_t.
template<std::uint8_t _Lower,
         std::uint8_t _Upper>
using uint8_interval = interval<std::uint8_t, _Lower, _Upper>;

// int8_interval
//   type: closed continuous interval over int8_t.
template<std::int8_t _Lower,
         std::int8_t _Upper>
using int8_interval = interval<std::int8_t, _Lower, _Upper>;

// uint16_interval
//   type: closed continuous interval over uint16_t.
template<std::uint16_t _Lower,
         std::uint16_t _Upper>
using uint16_interval = interval<std::uint16_t, _Lower, _Upper>;

// int16_interval
//   type: closed continuous interval over int16_t.
template<std::int16_t _Lower,
         std::int16_t _Upper>
using int16_interval = interval<std::int16_t, _Lower, _Upper>;

// uint32_interval
//   type: closed continuous interval over uint32_t.
template<std::uint32_t _Lower,
         std::uint32_t _Upper>
using uint32_interval = interval<std::uint32_t, _Lower, _Upper>;

// int32_interval
//   type: closed continuous interval over int32_t.
template<std::int32_t _Lower,
         std::int32_t _Upper>
using int32_interval = interval<std::int32_t, _Lower, _Upper>;

// uint64_interval
//   type: closed continuous interval over uint64_t.
template<std::uint64_t _Lower,
         std::uint64_t _Upper>
using uint64_interval = interval<std::uint64_t, _Lower, _Upper>;

// int64_interval
//   type: closed continuous interval over int64_t.
template<std::int64_t _Lower,
         std::int64_t _Upper>
using int64_interval = interval<std::int64_t, _Lower, _Upper>;

// short_interval
//   type: closed continuous interval over short.
template<short _Lower,
         short _Upper>
using short_interval = interval<short, _Lower, _Upper>;

// long_interval
//   type: closed continuous interval over long.
template<long _Lower,
         long _Upper>
using long_interval = interval<long, _Lower, _Upper>;

// long_long_interval
//   type: closed continuous interval over long long.
template<long long _Lower,
         long long _Upper>
using long_long_interval = interval<long long, _Lower, _Upper>;

// bool_interval
//   type: closed continuous interval over bool.
template<bool _Lower,
         bool _Upper>
using bool_interval = interval<bool, _Lower, _Upper>;

// --- open -------------------------------------------------------------------

// int_open
//   type: open continuous interval over int.
template<int _Lower,
         int _Upper>
using int_open = interval<int, _Lower, _Upper, true, true>;

// index_open
//   type: open continuous interval over std::size_t.
template<std::size_t _Lower,
         std::size_t _Upper>
using index_open = interval<std::size_t, _Lower, _Upper, true, true>;

// char_open
//   type: open continuous interval over char.
template<char _Lower,
         char _Upper>
using char_open = interval<char, _Lower, _Upper, true, true>;

// int32_open
//   type: open continuous interval over int32_t.
template<std::int32_t _Lower,
         std::int32_t _Upper>
using int32_open = interval<std::int32_t, _Lower, _Upper, true, true>;

// uint32_open
//   type: open continuous interval over uint32_t.
template<std::uint32_t _Lower,
         std::uint32_t _Upper>
using uint32_open =
    interval<std::uint32_t, _Lower, _Upper, true, true>;

// int64_open
//   type: open continuous interval over int64_t.
template<std::int64_t _Lower,
         std::int64_t _Upper>
using int64_open =
    interval<std::int64_t, _Lower, _Upper, true, true>;

// uint64_open
//   type: open continuous interval over uint64_t.
template<std::uint64_t _Lower,
         std::uint64_t _Upper>
using uint64_open =
    interval<std::uint64_t, _Lower, _Upper, true, true>;

// --- half-open [a, b) -------------------------------------------------------

// int_half_open
//   type: half-open-right continuous interval over int.
template<int _Lower,
         int _Upper>
using int_half_open = interval<int, _Lower, _Upper, false, true>;

// index_half_open
//   type: half-open-right continuous interval over std::size_t.
template<std::size_t _Lower,
         std::size_t _Upper>
using index_half_open =
    interval<std::size_t, _Lower, _Upper, false, true>;

// int32_half_open
//   type: half-open-right continuous interval over int32_t.
template<std::int32_t _Lower,
         std::int32_t _Upper>
using int32_half_open =
    interval<std::int32_t, _Lower, _Upper, false, true>;

// int64_half_open
//   type: half-open-right continuous interval over int64_t.
template<std::int64_t _Lower,
         std::int64_t _Upper>
using int64_half_open =
    interval<std::int64_t, _Lower, _Upper, false, true>;

// --- discrete ---------------------------------------------------------------

// int_stepped
//   type: closed discrete interval over int.
template<int _Lower,
         int _Upper,
         int _Step = 1>
using int_stepped =
    interval<int, _Lower, _Upper, false, false, _Step>;

// index_stepped
//   type: closed discrete interval over std::size_t.
template<std::size_t _Lower,
         std::size_t _Upper,
         std::size_t _Step = 1>
using index_stepped =
    interval<std::size_t, _Lower, _Upper, false, false, _Step>;

// int32_stepped
//   type: closed discrete interval over int32_t.
template<std::int32_t _Lower,
         std::int32_t _Upper,
         std::int32_t _Step = 1>
using int32_stepped =
    interval<std::int32_t, _Lower, _Upper, false, false, _Step>;

// int64_stepped
//   type: closed discrete interval over int64_t.
template<std::int64_t _Lower,
         std::int64_t _Upper,
         std::int64_t _Step = 1>
using int64_stepped =
    interval<std::int64_t, _Lower, _Upper, false, false, _Step>;

// uint32_stepped
//   type: closed discrete interval over uint32_t.
template<std::uint32_t _Lower,
         std::uint32_t _Upper,
         std::uint32_t _Step = 1>
using uint32_stepped =
    interval<std::uint32_t, _Lower, _Upper, false, false, _Step>;

// uint64_stepped
//   type: closed discrete interval over uint64_t.
template<std::uint64_t _Lower,
         std::uint64_t _Upper,
         std::uint64_t _Step = 1>
using uint64_stepped =
    interval<std::uint64_t, _Lower, _Upper, false, false, _Step>;

// char_stepped
//   type: closed discrete interval over char.
template<char _Lower,
         char _Upper,
         char _Step = 1>
using char_stepped =
    interval<char, _Lower, _Upper, false, false, _Step>;




// ============================================================================
// XIV.  INTERVAL TRAITS   (folded in from interval_traits.hpp -- task #1)
// ============================================================================
// Structural SFINAE detection of interval types and their properties. These
// previously lived in a standalone interval_traits.hpp; they now reside in the
// interval primary. The unqualified project void_t is resolved to std::void_t
// (this module already requires C++17 through its use of if constexpr).

// ============================================================================
// I.    DETECTION HELPERS
// ============================================================================

NS_INTERNAL

    // has_lower_bound
    //   helper: detects lower_bound static member.
    template<typename _Type,
             typename = void>
    struct has_lower_bound : std::false_type
    {};

    template<typename _Type>
    struct has_lower_bound<_Type, std::void_t<decltype(_Type::lower_bound)>>
        : std::true_type
    {};

    // has_upper_bound
    //   helper: detects upper_bound static member.
    template<typename _Type,
             typename = void>
    struct has_upper_bound : std::false_type
    {};

    template<typename _Type>
    struct has_upper_bound<_Type, std::void_t<decltype(_Type::upper_bound)>>
        : std::true_type
    {};

    // has_is_left_open
    //   helper: detects is_left_open static bool member.
    template<typename _Type,
             typename = void>
    struct has_is_left_open : std::false_type
    {};

    template<typename _Type>
    struct has_is_left_open<_Type, std::enable_if_t<
        ( std::is_same<decltype(_Type::is_left_open),
                       const bool>::value ||
          std::is_same<decltype(_Type::is_left_open),
                       bool>::value )
    >> : std::true_type
    {};

    // has_is_right_open
    //   helper: detects is_right_open static bool member.
    template<typename _Type,
             typename = void>
    struct has_is_right_open : std::false_type
    {};

    template<typename _Type>
    struct has_is_right_open<_Type, std::enable_if_t<
        ( std::is_same<decltype(_Type::is_right_open),
                       const bool>::value ||
          std::is_same<decltype(_Type::is_right_open),
                       bool>::value )
    >> : std::true_type
    {};

    // has_step
    //   helper: detects step static member (for discrete intervals).
    template<typename _Type,
             typename = void>
    struct has_step : std::false_type
    {};

    template<typename _Type>
    struct has_step<_Type, std::void_t<decltype(_Type::step)>>
        : std::true_type
    {};

    // interval_structural_check
    //   helper: combines all structural requirements for interval
    // detection.
    template<typename _Type,
             typename = void>
    struct interval_structural_check : std::false_type
    {};

    template<typename _Type>
    struct interval_structural_check<_Type, std::enable_if_t<
        ( has_lower_bound<_Type>::value  &&
          has_upper_bound<_Type>::value  &&
          has_is_left_open<_Type>::value &&
          has_is_right_open<_Type>::value )
    >> : std::true_type
    {};

    // discrete_interval_structural_check
    //   helper: combines structural requirements for discrete interval
    // detection.
    template<typename _Type,
             typename = void>
    struct discrete_interval_structural_check : std::false_type
    {};

    template<typename _Type>
    struct discrete_interval_structural_check<_Type, std::enable_if_t<
        ( interval_structural_check<_Type>::value &&
          has_step<_Type>::value )
    >> : std::true_type
    {};

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
{};

template<typename _Type>
struct is_interval<_Type, std::enable_if_t<
    internal::interval_structural_check<_Type>::value
>> : std::true_type
{};

// is_discrete_interval
//   trait: checks if _Type is a discrete interval type.
// A discrete interval is an interval that additionally has a step
// static member.
template<typename _Type,
         typename = void>
struct is_discrete_interval : std::false_type
{};

template<typename _Type>
struct is_discrete_interval<_Type, std::enable_if_t<
    internal::discrete_interval_structural_check<_Type>::value
>> : std::true_type
{};

// is_continuous_interval
//   trait: checks if _Type is a non-discrete interval.
template<typename _Type,
         typename = void>
struct is_continuous_interval : std::false_type
{};

template<typename _Type>
struct is_continuous_interval<_Type, std::enable_if_t<
    ( is_interval<_Type>::value &&
      !is_discrete_interval<_Type>::value )
>> : std::true_type
{};


// ============================================================================
// III.  BOUNDARY TYPE DETECTION
// ============================================================================

NS_INTERNAL

    // is_closed_check
    //   helper: checks closed boundary condition.
    template<typename _Type,
             typename = void>
    struct is_closed_check : std::false_type
    {};

    template<typename _Type>
    struct is_closed_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          (!_Type::is_left_open)    &&
          (!_Type::is_right_open) )
    >> : std::true_type
    {};

    // is_open_check
    //   helper: checks open boundary condition.
    template<typename _Type,
             typename = void>
    struct is_open_check : std::false_type
    {};

    template<typename _Type>
    struct is_open_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          _Type::is_left_open       &&
          _Type::is_right_open )
    >> : std::true_type
    {};

    // is_half_open_check
    //   helper: checks half-open boundary condition.
    template<typename _Type,
             typename = void>
    struct is_half_open_check : std::false_type
    {};

    template<typename _Type>
    struct is_half_open_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          (_Type::is_left_open != _Type::is_right_open) )
    >> : std::true_type
    {};

NS_END  // internal

// is_closed
//   trait: checks if interval has both endpoints closed (inclusive).
template<typename _Type>
struct is_closed : internal::is_closed_check<_Type>
{};

// is_open
//   trait: checks if interval has both endpoints open (exclusive).
template<typename _Type>
struct is_open : internal::is_open_check<_Type>
{};

// is_half_open
//   trait: checks if interval has exactly one open endpoint.
template<typename _Type>
struct is_half_open : internal::is_half_open_check<_Type>
{};


// ============================================================================
// IV.   ENDPOINT DETECTION
// ============================================================================

NS_INTERNAL

    // left_open_check
    //   helper: checks left endpoint is open.
    template<typename _Type,
             typename = void>
    struct left_open_check : std::false_type
    {};

    template<typename _Type>
    struct left_open_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          _Type::is_left_open )
    >> : std::true_type
    {};

    // right_open_check
    //   helper: checks right endpoint is open.
    template<typename _Type,
             typename = void>
    struct right_open_check : std::false_type
    {};

    template<typename _Type>
    struct right_open_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          _Type::is_right_open )
    >> : std::true_type
    {};

    // left_closed_check
    //   helper: checks left endpoint is closed.
    template<typename _Type,
             typename = void>
    struct left_closed_check : std::false_type
    {};

    template<typename _Type>
    struct left_closed_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          (!_Type::is_left_open) )
    >> : std::true_type
    {};

    // right_closed_check
    //   helper: checks right endpoint is closed.
    template<typename _Type,
             typename = void>
    struct right_closed_check : std::false_type
    {};

    template<typename _Type>
    struct right_closed_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value &&
          (!_Type::is_right_open) )
    >> : std::true_type
    {};

NS_END  // internal

// is_left_open
//   trait: checks if interval has left endpoint open.
template<typename _Type>
struct is_left_open : internal::left_open_check<_Type>
{};

// is_right_open
//   trait: checks if interval has right endpoint open.
template<typename _Type>
struct is_right_open : internal::right_open_check<_Type>
{};

// is_left_closed
//   trait: checks if interval has left endpoint closed.
template<typename _Type>
struct is_left_closed : internal::left_closed_check<_Type>
{};

// is_right_closed
//   trait: checks if interval has right endpoint closed.
template<typename _Type>
struct is_right_closed : internal::right_closed_check<_Type>
{};


// ============================================================================
// V.    INTERVAL PROPERTY DETECTION
// ============================================================================

// is_bounded_interval
//   trait: checks if interval has finite bounds.
// Note: true for all compile-time intervals (bounds must be specified).
template<typename _Type>
struct is_bounded_interval : is_interval<_Type>
{};

NS_INTERNAL

    // empty_interval_check
    //   helper: checks if interval is empty.
    template<typename _Type,
             typename = void>
    struct empty_interval_check : std::false_type
    {};

    template<typename _Type>
    struct empty_interval_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value                         &&
          ((_Type::lower_bound > _Type::upper_bound)        ||
           ((_Type::lower_bound == _Type::upper_bound)      &&
            (_Type::is_left_open || _Type::is_right_open))) )
    >> : std::true_type
    {};

    // degenerate_interval_check
    //   helper: checks if interval contains exactly one element.
    template<typename _Type,
             typename = void>
    struct degenerate_interval_check : std::false_type
    {};

    template<typename _Type>
    struct degenerate_interval_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value                       &&
          (_Type::lower_bound == _Type::upper_bound)      &&
          (!_Type::is_left_open)                          &&
          (!_Type::is_right_open) )
    >> : std::true_type
    {};

NS_END  // internal

// is_empty_interval
//   trait: checks if interval contains no elements.
// Empty when: lower > upper, or lower == upper and either endpoint
// is open.
template<typename _Type>
struct is_empty_interval : internal::empty_interval_check<_Type>
{};

// is_degenerate_interval
//   trait: checks if interval contains exactly one element.
// Degenerate when: lower == upper and both endpoints are closed.
template<typename _Type>
struct is_degenerate_interval
    : internal::degenerate_interval_check<_Type>
{};

NS_INTERNAL

    // proper_interval_check
    //   helper: checks if interval is non-empty.
    template<typename _Type,
             typename = void>
    struct proper_interval_check : std::false_type
    {};

    template<typename _Type>
    struct proper_interval_check<_Type, std::enable_if_t<
        ( is_interval<_Type>::value       &&
          !is_empty_interval<_Type>::value )
    >> : std::true_type
    {};

NS_END  // internal

// is_proper_interval
//   trait: checks if interval is non-empty.
template<typename _Type>
struct is_proper_interval : internal::proper_interval_check<_Type>
{};


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
                                     std::void_t<typename _Type::value_type>>
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
                                    std::void_t<typename _Type::size_type>>
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
    {};

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
    {};

    // intervals_same_boundary_check
    //   helper: checks if two intervals have same boundary
    // configuration.
    template<typename _Interval1,
             typename _Interval2,
             typename = void>
    struct intervals_same_boundary_check : std::false_type
    {};

    template<typename _Interval1,
             typename _Interval2>
    struct intervals_same_boundary_check<_Interval1,
                                        _Interval2,
                                        std::enable_if_t<
        ( is_interval<_Interval1>::value                                &&
          is_interval<_Interval2>::value                                &&
          (_Interval1::is_left_open  == _Interval2::is_left_open)       &&
          (_Interval1::is_right_open == _Interval2::is_right_open) )
    >> : std::true_type
    {};

NS_END  // internal

// intervals_same_type
//   trait: checks if two intervals have the same value type.
template<typename _Interval1,
         typename _Interval2>
struct intervals_same_type
    : internal::intervals_same_type_check<_Interval1, _Interval2>
{};

// intervals_same_boundary_type
//   trait: checks if two intervals have the same boundary type
// (open/closed).
template<typename _Interval1,
         typename _Interval2>
struct intervals_same_boundary_type
    : internal::intervals_same_boundary_check<_Interval1, _Interval2>
{};


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


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_INTERVAL_