/*******************************************************************************
* djinterp [cpp][utility]                                            counter.hpp
*
* djinterp counter header:
*   This header provides a bounded, nestable counter with configurable value
* type, and the five operations every counter and timer in djinterp shares:
* increment, decrement, compare, merge and reset. A counter can also be
* queried for whether its minimum or maximum limit has been reached.
*   Its thread-safe forms are atomic_counter.hpp and mutex_counter.hpp; the
* bindings that move any of the three when an event fires are
* counter_event.hpp's. C's counterpart is c/util/counter/counter.h.
*
*   Counters support two nesting models:
*   - owning:      parent stores child counters by value (lifetime-bound)
*   - non-owning:  parent observes external counters via pointer (view)
*
* PORTABILITY
* -----------
*   This header uses env.h for C++ version detection and djinterp.hpp for
* namespace macros. Requires C++17 or later.
*
* HOW TO USE
* ----------
*     counter<int> hits;                  // at 0, unbounded
*     counter<int> slots(0, 0, 8);        // initial, min, max
*
*     hits.increment();
*     slots.decrement(2);                 // false: held at 0
*
* OPTIONS
* -------
*   The value type, and the initial value and bounds given to the constructor.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/util/counter/counter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.07
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_UTIL_COUNTER_COUNTER_HPP
#define DJINTERP_UTIL_COUNTER_COUNTER_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <limits>
#include <type_traits>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP


// counter
//   class: a bounded, nestable counter with configurable value type.
//
//   The counter maintains a current value within [min, max] bounds.
// Increment and decrement operations return false and clamp when
// a limit is reached. Children may be owned (stored by value) or
// observed (stored by pointer).
//
//   Template parameter `ValueType` must be an arithmetic type.
template<typename ValueType = re_std::int64_t>
class counter
{
    static_assert(std::is_arithmetic_v<ValueType>,
                  "`ValueType` must be an arithmetic type.");

private:
    using self_type     = counter<ValueType>;
    using children_type = std::vector<self_type>;
    using observed_type = std::vector<self_type*>;

public:
    using value_type = ValueType;
    using size_type  = std::size_t;

    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // counter()
    //   constructor: default-constructs a counter at zero with no bounds.
    constexpr counter() D_NOEXCEPT
        : m_value(value_type{0}),
          m_initial(value_type{0}),
          m_min(std::numeric_limits<value_type>::lowest()),
          m_max(std::numeric_limits<value_type>::max()),
          m_children(),
          m_observed()
    {
    }

    // counter(_initial, _min, _max)
    //   constructor: constructs a counter with an initial value and
    // optional min/max bounds.
    constexpr counter(
        value_type _initial,
        value_type _min = std::numeric_limits<value_type>::lowest(),
        value_type _max = std::numeric_limits<value_type>::max()
    ) D_NOEXCEPT
        : m_value(_initial),
          m_initial(_initial),
          m_min(_min),
          m_max(_max),
          m_children(),
          m_observed()
    {
    }

    // -----------------------------------------------------------------
    // operations
    // -----------------------------------------------------------------

    // increment
    //   increments the counter by `_amount`; a negative amount moves it
    // down. returns false and clamps to the bound it would pass: max for a
    // rise, min for a fall. the sum is compared through distance() or
    // falls_below() before it is formed, so a step that would overflow the
    // value type clamps instead.
    bool increment(
        value_type _amount = value_type{1}
    ) D_NOEXCEPT
    {
        // past the ceiling already, or a rise larger than the room left
        if ( (m_max < m_value) ||
             ( (value_type{0} < _amount) &&
               (distance(m_value,
                         m_max) < _amount) ) )
        {
            m_value = m_max;

            return false;
        }

        // only a signed type can step down
        if constexpr (std::is_signed_v<value_type>)
        {
            // a fall past the floor
            if ( (_amount < value_type{0}) &&
                 (falls_below(m_value,
                              _amount,
                              m_min)) )
            {
                m_value = m_min;

                return false;
            }
        }

        m_value += _amount;

        return true;
    }

    // decrement
    //   decrements the counter by `_amount`; a negative amount moves it
    // up. returns false and clamps to the bound it would pass: min for a
    // fall, max for a rise. as with increment, the difference is never
    // formed unless it fits.
    bool decrement(
        value_type _amount = value_type{1}
    ) D_NOEXCEPT
    {
        // below the floor already, or a fall larger than the room left
        if ( (m_value < m_min) ||
             ( (value_type{0} < _amount) &&
               (distance(m_min,
                         m_value) < _amount) ) )
        {
            m_value = m_min;

            return false;
        }

        // only a signed type can step up here
        if constexpr (std::is_signed_v<value_type>)
        {
            // a rise past the ceiling
            if ( (_amount < value_type{0}) &&
                 (rises_above(m_value,
                              _amount,
                              m_max)) )
            {
                m_value = m_max;

                return false;
            }
        }

        m_value -= _amount;

        return true;
    }

    // reset
    //   resets the counter to its initial value. does not affect
    // children or observed counters.
    void reset() D_NOEXCEPT
    {
        m_value = m_initial;

        return;
    }

    // reset_all
    //   resets this counter and all owned children recursively.
    // observed counters are not reset.
    void reset_all() D_NOEXCEPT
    {
        m_value = m_initial;

        for (auto& child : m_children)
        {
            child.reset_all();
        }

        return;
    }

    // compare
    //   three-way comparison by value: -1, 0 or 1 as this counter is below,
    // equal to or above `_other`. initial values and bounds play no part.
    constexpr int compare(
        const self_type& _other
    ) const D_NOEXCEPT
    {
        return (m_value < _other.m_value)
                   ? -1
                   : ( (_other.m_value < m_value) ? 1 : 0 );
    }

    // merge
    //   adds `_other`'s movement since its start (its value less its
    // initial value) to this counter, clamping as increment and decrement
    // do; returns false when clamped. a movement too large for the value
    // type is held at the type's range first -- a rise at max(), a fall at
    // lowest() -- and that is reported too, as the C counter's
    // d_counter_merge reports it. children are not merged. merging a counter
    // into itself adds its own movement once.
    //
    //   Movement, not value: two counters that start at 10 and each count 3
    // merge to 16, which is what one counter counting all 6 would hold.
    bool merge(
        const self_type& _other
    ) D_NOEXCEPT
    {
        // read both first: `_other` may be this counter
        const value_type value   = _other.m_value;
        const value_type initial = _other.m_initial;

        // moved up, or not at all; a rise too large is held at max()
        if (!(value < initial))
        {
            const bool is_exact = increment(distance(initial,
                                                     value));

            return ( (is_exact) &&
                     (distance_fits(initial,
                                    value)) );
        }

        // moved down, by a fall the type holds
        if (distance_fits(value,
                          initial))
        {
            return decrement(distance(value,
                                      initial));
        }

        // a larger fall is held at lowest(), the largest fall a signed
        // movement holds, and is exact only when it is that fall
        const bool is_exact =
            increment(std::numeric_limits<value_type>::lowest());

        return ( (is_exact) &&
                 (falls_by_lowest(value,
                                  initial)) );
    }

    // distance
    //   returns how far `_to` lies above `_from` (`_to` must not be below
    // `_from`). for signed integer types the distance may not fit, and is
    // held at the type's maximum instead of overflowing.
    static constexpr value_type distance(
        value_type _from,
        value_type _to
    ) D_NOEXCEPT
    {
        // a signed difference can overflow; saturate it
        if (!distance_fits(_from,
                           _to))
        {
            return std::numeric_limits<value_type>::max();
        }

        return static_cast<value_type>(_to - _from);
    }

    // distance_fits
    //   whether the distance from `_from` up to `_to` fits the value type,
    // so that distance returns it rather than the type's maximum. only a
    // signed integer difference can overflow.
    static constexpr bool distance_fits(
        value_type _from,
        value_type _to
    ) D_NOEXCEPT
    {
        if constexpr ( (std::is_integral_v<value_type>) &&
                       (std::is_signed_v<value_type>) )
        {
            return !( (_from < value_type{0}) &&
                      (_to > (std::numeric_limits<value_type>::max() +
                              _from)) );
        }

        return true;
    }

    // falls_by_lowest
    //   whether `_value` lies exactly -lowest() below `_initial`, for a fall
    // that distance_fits has refused: `_value` is then negative and
    // `_initial` is not, so neither side of the comparison overflows.
    static constexpr bool falls_by_lowest(
        value_type _value,
        value_type _initial
    ) D_NOEXCEPT
    {
        return ( (_initial - value_type{1}) ==
                 (_value + std::numeric_limits<value_type>::max()) );
    }

    // falls_below
    //   whether `_value` plus the negative `_amount` would leave the value
    // type or land below `_floor`. lowest() - _amount cannot overflow while
    // `_amount` is negative, so the sum is formed only once it is known to
    // fit.
    static constexpr bool falls_below(
        value_type _value,
        value_type _amount,
        value_type _floor
    ) D_NOEXCEPT
    {
        return ( (_value < (std::numeric_limits<value_type>::lowest() -
                            _amount)) ||
                 ((_value + _amount) < _floor) );
    }

    // rises_above
    //   whether `_value` minus the negative `_amount` would leave the value
    // type or land above `_ceiling`. max() + _amount cannot overflow while
    // `_amount` is negative, so the difference is formed only once it is
    // known to fit.
    static constexpr bool rises_above(
        value_type _value,
        value_type _amount,
        value_type _ceiling
    ) D_NOEXCEPT
    {
        return ( (_value > (std::numeric_limits<value_type>::max() +
                            _amount)) ||
                 (_ceiling < (_value - _amount)) );
    }

    // -----------------------------------------------------------------
    // accessors
    // -----------------------------------------------------------------

    // value
    //   returns the current counter value.
    constexpr value_type value() const D_NOEXCEPT
    {
        return m_value;
    }

    // initial
    //   returns the initial value the counter was constructed with.
    constexpr value_type initial() const D_NOEXCEPT
    {
        return m_initial;
    }

    // min
    //   returns the lower bound.
    constexpr value_type min() const D_NOEXCEPT
    {
        return m_min;
    }

    // max
    //   returns the upper bound.
    constexpr value_type max() const D_NOEXCEPT
    {
        return m_max;
    }

    // at_min
    //   returns true if the counter is at its lower bound.
    constexpr bool at_min() const D_NOEXCEPT
    {
        return (m_value <= m_min);
    }

    // at_max
    //   returns true if the counter is at its upper bound.
    constexpr bool at_max() const D_NOEXCEPT
    {
        return (m_value >= m_max);
    }

    // -----------------------------------------------------------------
    // children (owning)
    // -----------------------------------------------------------------

    // add_child
    //   constructs and appends an owned child counter. returns a
    // reference to the newly added child.
    self_type& add_child(
        value_type _initial = value_type{0},
        value_type _min     = std::numeric_limits<value_type>::lowest(),
        value_type _max     = std::numeric_limits<value_type>::max()
    )
    {
        m_children.emplace_back(_initial,
                                _min,
                                _max);

        return m_children.back();
    }

    // child
    //   returns a reference to the owned child at `_index`.
    self_type& child(
        size_type _index
    )
    {
        return m_children[_index];
    }

    // child (const)
    //   returns a const reference to the owned child at `_index`.
    const self_type& child(
        size_type _index
    ) const
    {
        return m_children[_index];
    }

    // child_count
    //   returns the number of owned children.
    size_type child_count() const D_NOEXCEPT
    {
        return m_children.size();
    }

    // -----------------------------------------------------------------
    // children (non-owning / observed)
    // -----------------------------------------------------------------

    // observe
    //   registers a non-owning reference to an external counter.
    // the caller is responsible for ensuring the observed counter
    // outlives this counter.
    void observe(
        self_type& _target
    ) D_NOEXCEPT
    {
        m_observed.push_back(&_target);

        return;
    }

    // observed
    //   returns a pointer to the observed counter at `_index`,
    // or nullptr if out of range.
    self_type* observed(
        size_type _index
    ) const D_NOEXCEPT
    {
        if (_index >= m_observed.size())
        {
            return nullptr;
        }

        return m_observed[_index];
    }

    // observed_count
    //   returns the number of observed (non-owning) children.
    size_type observed_count() const D_NOEXCEPT
    {
        return m_observed.size();
    }

private:
    value_type    m_value;
    value_type    m_initial;
    value_type    m_min;
    value_type    m_max;
    children_type m_children;
    observed_type m_observed;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_COUNTER_COUNTER_HPP
