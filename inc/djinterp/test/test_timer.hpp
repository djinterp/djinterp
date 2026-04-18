/******************************************************************************
* djinterp [test]                                              test_timer.hpp
*
*   A nestable timer for test instrumentation with optional event dispatch.
* Wraps util::timer for elapsed-time tracking, adds owning and non-owning
* child management, and fires events through an optional event_handler
* pointer.
*
*   When an event_handler is attached, the timer fires tag-typed events
* on start, stop, expiry, and reset.  When no handler is attached,
* operations proceed with zero event overhead.
*
*   EVENT TAGS:
*   Each test_timer instantiation defines nested event tag types
* compatible with the djinterp event system (event_traits, event_handler):
*     on_start   — ()
*     on_stop    — (rep_type elapsed_count)
*     on_expire  — ()
*     on_reset   — (rep_type elapsed_count_before)
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
* path:      /inc/djinterp/test/test_timer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.08
******************************************************************************/

#ifndef DJINTERP_TEST_TIMER_
#define DJINTERP_TEST_TIMER_ 1

// std
#include <chrono>
#include <cstddef>
#include <tuple>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/util/timer/timer.hpp"
#include "../core/event/event_handler.hpp"


NS_DJINTERP
NS_TEST


// test_timer
//   class: a nestable timer for test instrumentation with
// optional event dispatch.
//
//   Wraps util::timer<_Clock, _Duration> for core time tracking.
// Fires events through an optional event_handler pointer on
// start, stop, expiry, and reset.
//
//   The timer tracks elapsed wall-clock time between start/stop
// pairs.  Successive start/stop cycles accumulate.  If a maximum
// duration is set and the accumulated time meets or exceeds it,
// the timer is considered expired.
//
// Example:
//   test_timer<> t;
//   t.start();
//   // ... work ...
//   t.stop();
//   auto ns = t.elapsed().count();
//
// Example (with events and max limit):
//   event_handler eh;
//   test_timer<> t(std::chrono::seconds(30), &eh);
//   eh.bind<test_timer<>::on_stop>(
//       [](event_context& ctx, test_timer<>::rep_type ns) { ... });
//   t.start();
//   t.stop();  // fires on_stop with elapsed count
template<typename _Clock    = std::chrono::steady_clock,
         typename _Duration = typename _Clock::duration>
class test_timer
{
private:
    using self_type     = test_timer<_Clock, _Duration>;
    using base_type     = timer<_Clock, _Duration>;
    using children_type = std::vector<self_type>;
    using observed_type = std::vector<self_type*>;

public:
    using clock_type    = _Clock;
    using duration_type = _Duration;
    using rep_type      = typename _Duration::rep;
    using size_type     = std::size_t;

    // -----------------------------------------------------------------
    //  event tag types
    // -----------------------------------------------------------------

    // on_start
    //   event: fired when the timer begins running.
    // args: none.
    struct on_start
    {
        using args_type = std::tuple<>;

        static const char* name() 
        { 
            return "test_timer::on_start"; 
        }
    };

    // on_stop
    //   event: fired when the timer is stopped.
    // args: (elapsed_count) — accumulated duration count at stop.
    struct on_stop
    {
        using args_type = std::tuple<rep_type>;

        static const char* name() 
        { 
            return "test_timer::on_stop";
        }
    };

    // on_expire
    //   event: fired when the timer reaches its maximum limit.
    // args: none.
    struct on_expire
    {
        using args_type = std::tuple<>;

        static const char* name() 
        { 
            return "test_timer::on_expire"; 
        }
    };

    // on_reset
    //   event: fired after a reset.
    // args: (elapsed_count_before) — accumulated count before reset.
    struct on_reset
    {
        using args_type = std::tuple<rep_type>;

        static const char* name() 
        { 
            return "test_timer::on_reset"; 
        }
    };

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default (no max limit)
    test_timer()
        : m_timer(),
          m_handler(nullptr),
          m_children(),
          m_observed()
    {}

    // from max duration and optional handler
    explicit test_timer(
        _Duration      _max,
        event_handler* _handler = nullptr
    )
        : m_timer(_max),
            m_handler(_handler),
            m_children(),
            m_observed()
    {}

    // handler-only (no max limit)
    explicit test_timer(
        event_handler* _handler
    )
        : m_timer(),
            m_handler(_handler),
            m_children(),
            m_observed()
    {}

    // -----------------------------------------------------------------
    //  operations
    // -----------------------------------------------------------------

    // start
    //   starts or resumes the timer.  Has no effect if already
    // running or if the timer is expired.  Fires on_start when
    // the timer transitions to the running state.
    void
    start()
    {
        bool was_running = m_timer.running();

        m_timer.start();

        // fire only if we actually transitioned to running
        if ( (!was_running) &&
             (m_timer.running()) )
        {
            emit<on_start>();
        }

        return;
    }

    // stop
    //   stops the timer and accumulates elapsed time since the
    // last start.  Has no effect if not running.  Fires on_stop
    // with the accumulated elapsed count.  Additionally fires
    // on_expire if the timer has reached its maximum.
    void
    stop()
    {
        if (!m_timer.running())
        {
            return;
        }

        m_timer.stop();

        emit<on_stop>(m_timer.elapsed().count());

        // check for expiry after stop
        if (m_timer.expired())
        {
            emit<on_expire>();
        }

        return;
    }

    // reset
    //   stops the timer (if running) and clears all accumulated
    // time.  Does not affect children or observed timers.
    // Fires on_reset with the elapsed count before clearing.
    void
    reset()
    {
        rep_type old_count = m_timer.elapsed().count();

        m_timer.reset();

        emit<on_reset>(old_count);

        return;
    }

    // reset_all
    //   resets this timer and all owned children recursively.
    // Observed timers are not reset.
    void
    reset_all()
    {
        rep_type old_count = m_timer.elapsed().count();

        m_timer.reset();

        emit<on_reset>(old_count);

        for (auto& child : m_children)
        {
            child.reset_all();
        }

        return;
    }

    // -----------------------------------------------------------------
    //  accessors
    // -----------------------------------------------------------------

    // elapsed
    //   returns the total accumulated duration.  If the timer is
    // currently running, includes time since the last start.
    _Duration elapsed() const
    {
        return m_timer.elapsed();
    }

    // max
    //   returns the maximum duration limit, or zero if no limit
    // is set.
    _Duration max() const
    {
        return m_timer.max();
    }

    // has_max
    //   returns true if a maximum duration limit has been set.
    bool has_max() const
    {
        return m_timer.has_max();
    }

    // running
    //   returns true if the timer is currently running.
    bool running() const
    {
        return m_timer.running();
    }

    // expired
    //   returns true if a maximum limit is set and the
    // accumulated time meets or exceeds it.
    bool expired() const
    {
        return m_timer.expired();
    }

    // remaining
    //   returns the time remaining before expiry, or zero if
    // no limit is set or the timer is already expired.
    _Duration remaining() const
    {
        return m_timer.remaining();
    }

    // -----------------------------------------------------------------
    //  children  (owning)
    // -----------------------------------------------------------------

    // add_child (no max)
    //   constructs and appends an owned child timer with no
    // maximum limit.  Inherits this timer's event_handler by
    // default.  Returns a reference to the newly added child.
    self_type&
    add_child()
    {
        m_children.emplace_back(m_handler);

        return m_children.back();
    }

    // add_child (with max)
    //   constructs and appends an owned child timer with a
    // maximum duration limit.  Inherits this timer's
    // event_handler.
    self_type&
    add_child(
        _Duration _max
    )
    {
        m_children.emplace_back(_max, m_handler);

        return m_children.back();
    }

    // child
    //   returns a reference to the owned child at _index.
    self_type&
    child(
        size_type _index
    )
    {
        return m_children[_index];
    }

    // child (const)
    const self_type&
    child(
        size_type _index
    ) const
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
    //   registers a non-owning reference to an external timer.
    // The caller is responsible for ensuring the observed timer
    // outlives this timer.
    void
    observe(
        self_type& _target
    )
    {
        m_observed.push_back(&_target);

        return;
    }

    // observed
    //   returns a pointer to the observed timer at _index,
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
    template<typename _Event,
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

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    base_type      m_timer;
    event_handler* m_handler;
    children_type  m_children;
    observed_type  m_observed;
};


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TIMER_