/******************************************************************************
* djinterp [util]                                                    timer.hpp
*
* djinterp timer header:
*   This header provides a nestable timer with configurable clock and duration
* types. A timer can be started, stopped, queried for elapsed time, and
* optionally bounded by a maximum duration after which it is considered
* expired.
*
*   Timers support two nesting models:
*   - owning:      parent stores child timers by value (lifetime-bound)
*   - non-owning:  parent observes external timers via pointer (view)
*
*   PORTABILITY:
*   This header uses env.h for C++ version detection and djinterp.hpp for
* namespace macros. Requires C++17 or later.
*
*
* path:      /inc/djinterp/util/timer/timer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_UTILITY_TIMER_
#define DJINTERP_UTILITY_TIMER_ 1

#include <chrono>
#include <vector>
#include "../../djinterp.hpp"


NS_DJINTERP

// =========================================================================
// timer
//   class: a nestable timer with configurable clock, duration, and an
// optional maximum time limit.
//
//   The timer tracks elapsed wall-clock time between start/stop pairs.
// Successive start/stop cycles accumulate. If a maximum duration is
// set and the accumulated time meets or exceeds it, the timer is
// considered expired.
//
//   Template parameter `_Clock` must satisfy the Clock named
// requirement. `_Duration` must be a std::chrono::duration
// specialization.
// =========================================================================
template<typename _Clock    = std::chrono::steady_clock,
         typename _Duration = typename _Clock::duration>
class timer
{
private:
    using self_type      = timer<_Clock, _Duration>;
    using children_type  = std::vector<self_type>;
    using observed_type  = std::vector<self_type*>;
    using time_point     = std::chrono::time_point<_Clock, _Duration>;

public:
    using clock_type    = _Clock;
    using duration_type = _Duration;
    using rep_type      = typename _Duration::rep;
    using size_type     = std::size_t;

    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // timer()
    //   constructor: default-constructs a timer with no maximum limit.
    timer() D_NOEXCEPT
        : m_accumulated(_Duration::zero()),
          m_max(_Duration::zero()),
          m_start_point(),
          m_running(false),
          m_has_max(false),
          m_children(),
          m_observed()
    {
    }

    // timer(_max)
    //   constructor: constructs a timer with a maximum duration limit.
    // the timer is considered expired once accumulated time reaches
    // or exceeds `_max`.
    explicit timer(
        _Duration _max
    ) D_NOEXCEPT
        : m_accumulated(_Duration::zero()),
          m_max(_max),
          m_start_point(),
          m_running(false),
          m_has_max(true),
          m_children(),
          m_observed()
    {
    }

    // -----------------------------------------------------------------
    // operations
    // -----------------------------------------------------------------

    // start
    //   starts or resumes the timer. has no effect if already running
    // or if the timer is expired.
    void start() D_NOEXCEPT
    {
        if ( (m_running) ||
             (expired()) )
        {
            return;
        }

        m_start_point = _Clock::now();
        m_running     = true;

        return;
    }

    // stop
    //   stops the timer and accumulates elapsed time since the last
    // start. has no effect if not running.
    void stop() D_NOEXCEPT
    {
        if (!m_running)
        {
            return;
        }

        m_accumulated += std::chrono::duration_cast<_Duration>(
                             _Clock::now() - m_start_point);
        m_running      = false;

        return;
    }

    // reset
    //   stops the timer (if running) and clears all accumulated time.
    // does not affect children or observed timers.
    void reset() D_NOEXCEPT
    {
        m_running     = false;
        m_accumulated = _Duration::zero();

        return;
    }

    // reset_all
    //   resets this timer and all owned children recursively.
    // observed timers are not reset.
    void reset_all() D_NOEXCEPT
    {
        reset();

        for (auto& child : m_children)
        {
            child.reset_all();
        }

        return;
    }

    // -----------------------------------------------------------------
    // accessors
    // -----------------------------------------------------------------

    // elapsed
    //   returns the total accumulated duration. if the timer is
    // currently running, includes time since the last start.
    _Duration elapsed() const D_NOEXCEPT
    {
        if (m_running)
        {
            return m_accumulated
                   + std::chrono::duration_cast<_Duration>(
                         _Clock::now() - m_start_point);
        }

        return m_accumulated;
    }

    // max
    //   returns the maximum duration limit, or zero if no limit is set.
    _Duration max() const D_NOEXCEPT
    {
        return m_max;
    }

    // has_max
    //   returns true if a maximum duration limit has been set.
    bool has_max() const D_NOEXCEPT
    {
        return m_has_max;
    }

    // running
    //   returns true if the timer is currently running.
    bool running() const D_NOEXCEPT
    {
        return m_running;
    }

    // expired
    //   returns true if a maximum limit is set and the accumulated
    // time meets or exceeds it.
    bool expired() const D_NOEXCEPT
    {
        if (!m_has_max)
        {
            return false;
        }

        return (elapsed() >= m_max);
    }

    // remaining
    //   returns the time remaining before expiry, or zero if no
    // limit is set or the timer is already expired.
    _Duration remaining() const D_NOEXCEPT
    {
        if (!m_has_max)
        {
            return _Duration::zero();
        }

        auto rem = m_max - elapsed();

        if (rem < _Duration::zero())
        {
            return _Duration::zero();
        }

        return rem;
    }

    // -----------------------------------------------------------------
    // children (owning)
    // -----------------------------------------------------------------

    // add_child
    //   constructs and appends an owned child timer. returns a
    // reference to the newly added child.
    self_type& add_child()
    {
        m_children.emplace_back();

        return m_children.back();
    }

    // add_child(_max)
    //   constructs and appends an owned child timer with a maximum
    // duration limit. returns a reference to the newly added child.
    self_type& add_child(
        _Duration _max
    )
    {
        m_children.emplace_back(_max);

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
    //   registers a non-owning reference to an external timer.
    // the caller is responsible for ensuring the observed timer
    // outlives this timer.
    void observe(
        self_type& _target
    ) D_NOEXCEPT
    {
        m_observed.push_back(&_target);

        return;
    }

    // observed
    //   returns a pointer to the observed timer at `_index`,
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
    _Duration     m_accumulated;
    _Duration     m_max;
    time_point    m_start_point;
    bool          m_running;
    bool          m_has_max;
    children_type m_children;
    observed_type m_observed;
};


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_TIMER_