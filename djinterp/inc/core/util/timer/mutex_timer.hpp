/******************************************************************************
* djinterp [util]                                              mutex_timer.hpp
*
* Lock-policy-based thread-safe timer.
*   Wraps the base `timer<_Clock, _Duration>` with a configurable lock
* policy from the threadsafe module.  Every public operation
* acquires either a read lock (accessors) or a write lock (mutations)
* through the policy's RAII guards.
*
*   The underlying timer is accessed only through the lock — no public
* method exposes an unguarded reference to the internal state.
*
* LOCK ORDERING:
*   Parent locks are always acquired before child locks.  Children must
* not hold references back to the parent to avoid deadlock.
*
* NOTE: an atomic_timer module is not provided.  Timer state consists of
* three coupled fields (accumulated duration, start time_point, running
* flag) that cannot be updated atomically without either a lock or a
* double-width CAS.  Use mutex_timer for all concurrent timer access.
*
* SEE ALSO:
*   timer.hpp  — unsynchronized base
*
*   PORTABILITY:
*   Requires C++17 or later.
*
*
* path:      /inc/djinterp/util/timer/mutex_timer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_UTILITY_MUTEX_TIMER_
#define DJINTERP_UTILITY_MUTEX_TIMER_ 1

#include <chrono>
#include <vector>
#include "../../djinterp.hpp"
#include "../../sync/lock_policy.hpp"
#include "../../sync/lock_guard.hpp"
#include "./timer.hpp"


NS_DJINTERP
NS_UTIL


// =========================================================================
// mutex_timer
//   class: a thread-safe nestable timer with configurable lock policy.
//
//   Wraps `timer<_Clock, _Duration>` by composition.  Read operations
// acquire a shared lock (when the policy supports it); write operations
// acquire an exclusive lock.  Children are mutex_timers with the same
// clock, duration, and policy, each with their own mutex.
//
//   Template parameter `_Clock` must satisfy the Clock named requirement.
//   Template parameter `_Duration` must be a std::chrono::duration.
//   Template parameter `_Policy` must be a lock policy struct.
// =========================================================================
template<typename _Clock    = std::chrono::steady_clock,
         typename _Duration = typename _Clock::duration,
         typename _Policy   = threadsafe::default_lock_policy>
class mutex_timer
{
private:
    using self_type      = mutex_timer<_Clock, _Duration, _Policy>;
    using base_type      = timer<_Clock, _Duration>;
    using children_type  = std::vector<self_type>;
    using observed_type  = std::vector<self_type*>;
    using read_guard     = threadsafe::scoped_read_lock<_Policy>;
    using write_guard    = threadsafe::scoped_write_lock<_Policy>;

public:
    using clock_type       = _Clock;
    using duration_type    = _Duration;
    using rep_type         = typename _Duration::rep;
    using size_type        = std::size_t;
    using lock_policy_type = _Policy;
    using mutex_type       = typename _Policy::mutex_type;

    // --- policy descriptors ---
    static constexpr bool is_threadsafe = _Policy::is_threadsafe;
    static constexpr bool is_shared     = _Policy::is_shared;
    static constexpr bool is_timed      = _Policy::is_timed;

    // -----------------------------------------------------------------
    // constructors
    // -----------------------------------------------------------------

    // mutex_timer()
    //   constructor: default-constructs a timer with no maximum limit.
    mutex_timer()
        : m_timer(),
          m_children(),
          m_observed(),
          m_mutex()
    {
    }

    // mutex_timer(_max)
    //   constructor: constructs a timer with a maximum duration limit.
    // the timer is considered expired once accumulated time reaches
    // or exceeds `_max`.
    explicit mutex_timer(
            _Duration _max
        )
        : m_timer(_max),
          m_children(),
          m_observed(),
          m_mutex()
    {
    }

    // non-copyable (mutex is non-copyable)
    mutex_timer(const mutex_timer&)            = delete;
    mutex_timer& operator=(const mutex_timer&) = delete;

    // -----------------------------------------------------------------
    // operations (write-locked)
    // -----------------------------------------------------------------

    // start
    //   starts or resumes the timer. has no effect if already running
    // or if the timer is expired.
    void start()
    {
        write_guard guard(m_mutex);

        m_timer.start();

        return;
    }

    // stop
    //   stops the timer and accumulates elapsed time since the last
    // start. has no effect if not running.
    void stop()
    {
        write_guard guard(m_mutex);

        m_timer.stop();

        return;
    }

    // reset
    //   stops the timer (if running) and clears all accumulated time.
    // does not affect children or observed timers.
    void reset()
    {
        write_guard guard(m_mutex);

        m_timer.reset();

        return;
    }

    // reset_all
    //   resets this timer and all owned children recursively.
    // observed timers are not reset.
    //
    // NOTE: acquires this timer's write lock, then each child's
    // write lock in sequence (parent-before-child ordering).
    void reset_all()
    {
        write_guard guard(m_mutex);

        m_timer.reset();

        for (auto& child : m_children)
        {
            child.reset_all();
        }

        return;
    }

    // -----------------------------------------------------------------
    // try-operations (non-blocking)
    // -----------------------------------------------------------------

    // try_start
    //   attempts a non-blocking start. returns false if the lock
    // could not be acquired, if already running, or if expired.
    bool try_start()
    {
        threadsafe::scoped_try_lock<_Policy> guard(m_mutex);

        if (!guard.owns_lock())
        {
            return false;
        }

        if ( (m_timer.running()) ||
             (m_timer.expired()) )
        {
            return false;
        }

        m_timer.start();

        return true;
    }

    // try_stop
    //   attempts a non-blocking stop. returns false if the lock
    // could not be acquired or if the timer is not running.
    bool try_stop()
    {
        threadsafe::scoped_try_lock<_Policy> guard(m_mutex);

        if (!guard.owns_lock())
        {
            return false;
        }

        if (!m_timer.running())
        {
            return false;
        }

        m_timer.stop();

        return true;
    }

    // -----------------------------------------------------------------
    // accessors (read-locked)
    // -----------------------------------------------------------------

    // elapsed
    //   returns the total accumulated duration. if the timer is
    // currently running, includes time since the last start.
    _Duration elapsed() const
    {
        read_guard guard(m_mutex);

        return m_timer.elapsed();
    }

    // max
    //   returns the maximum duration limit, or zero if no limit is set.
    _Duration max() const
    {
        read_guard guard(m_mutex);

        return m_timer.max();
    }

    // has_max
    //   returns true if a maximum duration limit has been set.
    bool has_max() const
    {
        read_guard guard(m_mutex);

        return m_timer.has_max();
    }

    // running
    //   returns true if the timer is currently running.
    bool running() const
    {
        read_guard guard(m_mutex);

        return m_timer.running();
    }

    // expired
    //   returns true if a maximum limit is set and the accumulated
    // time meets or exceeds it.
    bool expired() const
    {
        read_guard guard(m_mutex);

        return m_timer.expired();
    }

    // remaining
    //   returns the time remaining before expiry, or zero if no
    // limit is set or the timer is already expired.
    _Duration remaining() const
    {
        read_guard guard(m_mutex);

        return m_timer.remaining();
    }

    // -----------------------------------------------------------------
    // children (owning)
    // -----------------------------------------------------------------

    // add_child
    //   constructs and appends an owned child timer with no limit.
    // returns a reference to the newly added child.
    //
    // WARNING: the returned reference is invalidated if the
    // children vector reallocates on a subsequent add_child call.
    // The child has its own mutex for independent operation.
    self_type& add_child()
    {
        write_guard guard(m_mutex);

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
        write_guard guard(m_mutex);

        m_children.emplace_back(_max);

        return m_children.back();
    }

    // child
    //   returns a reference to the owned child at `_index`.
    // the child has its own mutex; callers may operate on it
    // without holding the parent lock.
    self_type& child(
            size_type _index
        )
    {
        read_guard guard(m_mutex);

        return m_children[_index];
    }

    // child (const)
    //   returns a const reference to the owned child at `_index`.
    const self_type& child(
            size_type _index
        ) const
    {
        read_guard guard(m_mutex);

        return m_children[_index];
    }

    // child_count
    //   returns the number of owned children.
    size_type child_count() const
    {
        read_guard guard(m_mutex);

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
        )
    {
        write_guard guard(m_mutex);

        m_observed.push_back(&_target);

        return;
    }

    // observed
    //   returns a pointer to the observed timer at `_index`,
    // or nullptr if out of range.
    self_type* observed(
            size_type _index
        ) const
    {
        read_guard guard(m_mutex);

        if (_index >= m_observed.size())
        {
            return nullptr;
        }

        return m_observed[_index];
    }

    // observed_count
    //   returns the number of observed (non-owning) children.
    size_type observed_count() const
    {
        read_guard guard(m_mutex);

        return m_observed.size();
    }

    // -----------------------------------------------------------------
    // mutex access
    // -----------------------------------------------------------------

    // mutex
    //   returns a reference to the underlying mutex for external
    // synchronization or use with portable_condvar.
    mutex_type& mutex() const noexcept
    {
        return m_mutex;
    }

private:
    base_type          m_timer;
    children_type      m_children;
    observed_type      m_observed;
    mutable mutex_type m_mutex;
};


NS_END  // util
NS_END  // djinterp


#endif  // DJINTERP_UTILITY_MUTEX_TIMER_