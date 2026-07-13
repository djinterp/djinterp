/******************************************************************************
* djinterp [util]                                                  counter.hpp
*
* djinterp counter header:
*   This header provides a bounded, nestable counter with configurable value
* type. A counter can be incremented, decremented, or queried for whether its
* minimum or maximum limit has been reached.
*
*   Counters support two nesting models:
*   - owning:      parent stores child counters by value (lifetime-bound)
*   - non-owning:  parent observes external counters via pointer (view)
*
*   PORTABILITY:
*   This header uses env.h for C++ version detection and djinterp.hpp for
* namespace macros. Requires C++17 or later.
*
*
* path:      /inc/djinterp/util/counter/counter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_UTILITY_COUNTER_
#define DJINTERP_UTILITY_COUNTER_ 1

#include <cstdint>
#include <limits>
#include <vector>
#include "../../djinterp.hpp"


NS_DJINTERP


// counter
//   class: a bounded, nestable counter with configurable value type.
//
//   The counter maintains a current value within [min, max] bounds.
// Increment and decrement operations return false and clamp when
// a limit is reached. Children may be owned (stored by value) or
// observed (stored by pointer).
//
//   Template parameter `_ValueType` must be an arithmetic type.
template<typename _ValueType = std::int64_t>
class counter
{
    static_assert(std::is_arithmetic_v<_ValueType>,
                  "`_ValueType` must be an arithmetic type.");

private:
    using self_type     = counter<_ValueType>;
    using children_type = std::vector<self_type>;
    using observed_type = std::vector<self_type*>;

public:
    using value_type = _ValueType;
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
    //   increments the counter by `_amount`. returns false and clamps
    // to max if the operation would exceed the upper bound.
    bool increment(
		value_type _amount = value_type{1}
	) D_NOEXCEPT
    {
        if (m_value + _amount > m_max)
        {
            m_value = m_max;

            return false;
        }

        m_value += _amount;

        return true;
    }

    // decrement
    //   decrements the counter by `_amount`. returns false and clamps
    // to min if the operation would exceed the lower bound.
    bool decrement(
		value_type _amount = value_type{1}
	) D_NOEXCEPT
    {
        if (m_value - _amount < m_min)
        {
            m_value = m_min;

            return false;
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


#endif  // DJINTERP_UTILITY_COUNTER_