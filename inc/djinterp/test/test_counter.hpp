/******************************************************************************
* djinterp [test]                                             test_counter.hpp
*
*   A nestable, bounded counter for test instrumentation with optional
* event dispatch.  Wraps util::counter for value and bounds tracking,
* adds owning and non-owning child management, and fires events through
* an optional event_handler pointer.
*
*   When an event_handler is attached, the counter fires tag-typed events
* on increment, decrement, bound clamping, and reset.  When no handler
* is attached, operations proceed with zero event overhead.
*
*   EVENT TAGS:
*   Each test_counter instantiation defines nested event tag types
* compatible with the djinterp event system (event_traits, event_handler):
*     on_increment  — (value_type old, value_type new)
*     on_decrement  — (value_type old, value_type new)
*     on_limit      — (value_type clamped_value)
*     on_reset      — (value_type old)
*
*   NESTING:
*   Children may be owned (stored by value, lifetime-bound) or observed
* (external pointer, caller ensures lifetime).  add_child propagates
* the parent's event_handler to the new child by default.
*
*   PORTABILITY:
*   Requires C++11 or later.  Uses env.h for version detection and
* djinterp.hpp for namespace macros and constexpr support.
*
*
* TABLE OF CONTENTS
* =================
* I.    test_counter
*
*
* path:      /inc/djinterp/test/test_counter.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.08
******************************************************************************/

#ifndef DJINTERP_TEST_COUNTER_
#define DJINTERP_TEST_COUNTER_ 1

// std
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/util/counter/counter.hpp"
#include "../core/event/event_handler.hpp"


NS_DJINTERP
NS_TEST


// =============================================================================
// I.   test_counter
// =============================================================================

// test_counter
//   class: a bounded, nestable counter for test instrumentation
// with optional event dispatch.
//
//   Wraps util::counter<_ValueType> for core value tracking.
// Fires events through an optional event_handler pointer on
// increment, decrement, bound clamping, and reset.
//
// Example:
//   test_counter<int> c(0, 0, 100);
//   c.increment(5);       // value: 5
//   c.increment(200);     // value: 100 (clamped), returns false
//
// Example (with events):
//   event_handler eh;
//   test_counter<int> c(0, 0, 100, &eh);
//   eh.bind<test_counter<int>::on_increment>(
//       [](event_context& ctx, int old_v, int new_v) { ... });
//   c.increment(5);       // fires on_increment(0, 5)
template<typename _ValueType = std::int64_t>
class test_counter
{
    static_assert(std::is_arithmetic<_ValueType>::value,
                  "`_ValueType` must be an arithmetic type.");

private:
    using self_type     = test_counter<_ValueType>;
    using base_type     = util::counter<_ValueType>;
    using children_type = std::vector<self_type>;
    using observed_type = std::vector<self_type*>;

public:
    using value_type = _ValueType;
    using size_type  = std::size_t;

    // -----------------------------------------------------------------
    //  event tag types
    // -----------------------------------------------------------------

    // on_increment
    //   event: fired after a successful increment.
    // args: (old_value, new_value).
    struct on_increment
    {
        using args_type = std::tuple<value_type, value_type>;
        static const char* name() { return "test_counter::on_increment"; }
    };

    // on_decrement
    //   event: fired after a successful decrement.
    // args: (old_value, new_value).
    struct on_decrement
    {
        using args_type = std::tuple<value_type, value_type>;
        static const char* name() { return "test_counter::on_decrement"; }
    };

    // on_limit
    //   event: fired when an operation clamps to a bound.
    // args: (clamped_value).
    struct on_limit
    {
        using args_type = std::tuple<value_type>;
        static const char* name() { return "test_counter::on_limit"; }
    };

    // on_reset
    //   event: fired after a reset.
    // args: (old_value).
    struct on_reset
    {
        using args_type = std::tuple<value_type>;
        static const char* name() { return "test_counter::on_reset"; }
    };

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default
    test_counter()
        : m_counter(),
          m_handler(nullptr),
          m_children(),
          m_observed()
    {}

    // from initial value and bounds
    test_counter(
        value_type     _initial,
        value_type     _min     = std::numeric_limits<value_type>::lowest(),
        value_type     _max     = std::numeric_limits<value_type>::max(),
        event_handler* _handler = nullptr
    )
        : m_counter(_initial, _min, _max),
            m_handler(_handler),
            m_children(),
            m_observed()
    {}

    // -----------------------------------------------------------------
    //  operations
    // -----------------------------------------------------------------

    // increment
    //   increments the counter by _amount.  Returns false and
    // clamps to max if the operation would exceed the upper
    // bound.  Fires on_increment on any change; additionally
    // fires on_limit when clamped.
    bool
    increment(
        value_type _amount = value_type{1}
    )
    {
        value_type old_val = m_counter.value();
        bool       result  = m_counter.increment(_amount);
        value_type new_val = m_counter.value();

        // fire increment event if the value changed
        if (new_val != old_val)
        {
            emit<on_increment>(old_val, new_val);
        }

        // fire limit event if clamped
        if (!result)
        {
            emit<on_limit>(new_val);
        }

        return result;
    }

    // decrement
    //   decrements the counter by _amount.  Returns false and
    // clamps to min if the operation would exceed the lower
    // bound.  Fires on_decrement on any change; additionally
    // fires on_limit when clamped.
    bool
    decrement(
        value_type _amount = value_type{1}
    )
    {
        value_type old_val = m_counter.value();
        bool       result  = m_counter.decrement(_amount);
        value_type new_val = m_counter.value();

        // fire decrement event if the value changed
        if (new_val != old_val)
        {
            emit<on_decrement>(old_val, new_val);
        }

        // fire limit event if clamped
        if (!result)
        {
            emit<on_limit>(new_val);
        }

        return result;
    }

    // reset
    //   resets the counter to its initial value.  Does not
    // affect children or observed counters.  Fires on_reset.
    void
    reset()
    {
        value_type old_val = m_counter.value();

        m_counter.reset();

        emit<on_reset>(old_val);

        return;
    }

    // reset_all
    //   resets this counter and all owned children recursively.
    // Observed counters are not reset.
    void
    reset_all()
    {
        value_type old_val = m_counter.value();

        m_counter.reset();

        emit<on_reset>(old_val);

        for (auto& child : m_children)
        {
            child.reset_all();
        }

        return;
    }

    // -----------------------------------------------------------------
    //  accessors
    // -----------------------------------------------------------------

    // value
    //   returns the current counter value.
    value_type
    value() const D_NOEXCEPT
    {
        return m_counter.value();
    }

    // initial
    //   returns the initial value the counter was constructed
    // with.
    value_type
    initial() const D_NOEXCEPT
    {
        return m_counter.initial();
    }

    // min
    //   returns the lower bound.
    value_type
    min() const D_NOEXCEPT
    {
        return m_counter.min();
    }

    // max
    //   returns the upper bound.
    value_type
    max() const D_NOEXCEPT
    {
        return m_counter.max();
    }

    // at_min
    //   returns true if the counter is at or below its lower
    // bound.
    bool
    at_min() const D_NOEXCEPT
    {
        return m_counter.at_min();
    }

    // at_max
    //   returns true if the counter is at or above its upper
    // bound.
    bool
    at_max() const D_NOEXCEPT
    {
        return m_counter.at_max();
    }

    // -----------------------------------------------------------------
    //  children  (owning)
    // -----------------------------------------------------------------

    // add_child
    //   constructs and appends an owned child counter.
    // Inherits this counter's event_handler by default.
    // Returns a reference to the newly added child.
    self_type&
    add_child(
        value_type     _initial = value_type{0},
        value_type     _min     = std::numeric_limits<value_type>::lowest(),
        value_type     _max     = std::numeric_limits<value_type>::max()
    )
    {
        m_children.emplace_back(_initial,
                                _min,
                                _max,
                                m_handler);

        return m_children.back();
    }

    // child
    //   returns a reference to the owned child at _index.
    self_type&
    child(size_type _index)
    {
        return m_children[_index];
    }

    // child (const)
    const self_type&
    child(size_type _index) const
    {
        return m_children[_index];
    }

    // child_count
    //   returns the number of owned children.
    size_type
    child_count() const D_NOEXCEPT
    {
        return m_children.size();
    }

    // -----------------------------------------------------------------
    //  children  (non-owning / observed)
    // -----------------------------------------------------------------

    // observe
    //   registers a non-owning reference to an external counter.
    // The caller is responsible for ensuring the observed counter
    // outlives this counter.
    void
    observe(
        self_type& _target
    )
    {
        m_observed.push_back(&_target);

        return;
    }

    // observed
    //   returns a pointer to the observed counter at _index,
    // or nullptr if out of range.
    self_type*
    observed(
        size_type _index
    ) const
    {
        if (_index >= m_observed.size())
        {
            return nullptr;
        }

        return m_observed[_index];
    }

    // observed_count
    //   returns the number of observed (non-owning) children.
    size_type
    observed_count() const D_NOEXCEPT
    {
        return m_observed.size();
    }

    // -----------------------------------------------------------------
    //  event handler access
    // -----------------------------------------------------------------

    // handler
    //   returns the currently attached event_handler, or nullptr.
    event_handler*
    handler() const D_NOEXCEPT
    {
        return m_handler;
    }

    // set_handler
    //   attaches or detaches an event_handler.  Pass nullptr to
    // disable event dispatch.
    void
    set_handler(
        event_handler* _handler
    ) D_NOEXCEPT
    {
        m_handler = _handler;

        return;
    }

private:
    // -----------------------------------------------------------------
    //  event dispatch helper
    // -----------------------------------------------------------------

    // emit
    //   fires an event through the attached handler if one is
    // present.  No-op when m_handler is nullptr.
    template<typename    _Event,
             typename... _Args>
    void
    emit(
        _Args... _args
    )
    {
        if (m_handler)
        {
            m_handler->fire<_Event>(_args...);
        }

        return;
    }

    //  storage
    base_type      m_counter;
    event_handler* m_handler;
    children_type  m_children;
    observed_type  m_observed;
};


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_COUNTER_
