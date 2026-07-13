/******************************************************************************
* djinterp [test]                                          test_invariant.hpp
*
*   Concurrent invariant monitoring for the DTest multithreading
* harness.  An invariant is a predicate that should hold at every
* observable instant of the program's execution.  This module
* spawns a background thread that polls the predicate at a
* configurable rate and counts violations.
*
*   Two complementary models are provided:
*
*     1. POLLING INVARIANT
*        A background thread evaluates a user-supplied predicate
*        at a fixed cadence.  Each false result is recorded with
*        timestamp.  Best for invariants that may briefly be false
*        and re-true themselves.
*
*     2. MONOTONIC GUARD
*        A specialized form for "value never decreases" or "value
*        never exceeds bound" - extremely common assertions about
*        atomic counters, sequence numbers, version stamps.  No
*        background thread; the guard wraps the value's accessor.
*
*   The monitor runs alongside the system under test for the duration
* of the test.  When start_monitoring() is called it spawns the
* watcher thread; stop_monitoring() joins.  The accumulated report
* is then converted to a basic_test for the test tree.
*
*   PORTABILITY:
*   Requires C++11 or later.
*
*
* TABLE OF CONTENTS
* =================
* I.    INVARIANT REPORT
* II.   POLLING INVARIANT MONITOR
* III.  MONOTONIC GUARD
* IV.   BOUNDED GUARD
* V.    INVARIANT SCOPE (RAII)
* VI.   FACTORY HELPERS
*
*
* path:      /inc/djinterp/test/sync/test_invariant.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_INVARIANT_
#define DJINTERP_TEST_INVARIANT_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>
    #include <chrono>
    #include <condition_variable>
    #include <functional>
    #include <mutex>
    #include <thread>
    #include <utility>
    #include <vector>
#endif

// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/sync/atomic.hpp"
#include "../test_common.hpp"
#include "../test_object.hpp"
#include "./test_thread.hpp"


NS_DJINTERP
NS_TEST


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// --- threadsafe foundation wrappers used by this module ---
using djinterp::atomic_size;

///////////////////////////////////////////////////////////////////////////////
///                I.   INVARIANT REPORT                                    ///
///////////////////////////////////////////////////////////////////////////////

// invariant_violation
//   struct: a single recorded invariant failure.
struct invariant_violation
{
    using time_point = test_thread::time_point;

    time_point  observed_at;
    std::string detail;

    invariant_violation()
        : observed_at(),
          detail()
    {}

    invariant_violation(
        time_point  _at,
        std::string _detail
    )
        : observed_at(_at),
          detail(static_cast<std::string&&>(_detail))
    {}
};


// invariant_report
//   struct: aggregate from a monitoring run.
struct invariant_report
{
    using size_type = std::size_t;

    size_type                          checks_performed;
    size_type                          violation_count;
    std::vector<invariant_violation>   violations;

    invariant_report()
        : checks_performed(0),
          violation_count(0),
          violations()
    {}

    bool
    success() const D_NOEXCEPT
    {
        return (violation_count == 0);
    }

    basic_test
    to_test_object(
        test_type_id _type_id,
        const char*  _name = "invariant"
    ) const
    {
        const bool passed = success();

        basic_test t(_type_id, passed);
        t.metadata().set("name", _name);
        t.metadata().set("message", passed
            ? "invariant held throughout monitored period"
            : "invariant violation(s) observed");

        return t;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  POLLING INVARIANT MONITOR                           ///
///////////////////////////////////////////////////////////////////////////////

// invariant_monitor
//   class: spawns a background thread that polls a
// predicate at a fixed cadence.  Each false result is
// recorded.  Stop the monitor explicitly; the report is
// available afterward.
//
// Example:
//   threadsafe_array<int, ...> arr;
//   invariant_monitor mon([&arr]() {
//       auto snap = arr.snapshot();
//       // size never exceeds capacity
//       return snap.size() <= snap.capacity();
//   });
//   mon.set_poll_interval(std::chrono::microseconds(100));
//
//   mon.start();
//   // ... run the workload ...
//   mon.stop();
//
//   if (!mon.report().success()) { /* fail the test */ }
class invariant_monitor
{
public:
    using clock_type    = test_thread::clock_type;
    using time_point    = test_thread::time_point;
    using duration_type = test_thread::duration_type;
    using predicate_fn  = std::function<bool()>;
    using detail_fn     = std::function<std::string()>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    invariant_monitor()
        : m_predicate(),
          m_detail(),
          m_poll_interval(std::chrono::milliseconds(1)),
          m_max_violations_recorded(64),
          m_thread(),
          m_stop_flag(false),
          m_running(false),
          m_report(),
          m_report_mutex()
    {}

    explicit invariant_monitor(
        predicate_fn _predicate
    )
        : m_predicate(static_cast<predicate_fn&&>(_predicate)),
          m_detail(),
          m_poll_interval(std::chrono::milliseconds(1)),
          m_max_violations_recorded(64),
          m_thread(),
          m_stop_flag(false),
          m_running(false),
          m_report(),
          m_report_mutex()
    {}

    invariant_monitor(const invariant_monitor&)            = delete;
    invariant_monitor& operator=(const invariant_monitor&) = delete;

    ~invariant_monitor()
    {
        if (m_running.load(std::memory_order_acquire))
        {
            stop();
        }
    }

    // -----------------------------------------------------------------
    //  configuration
    // -----------------------------------------------------------------

    // set_predicate
    //   sets the boolean predicate evaluated each tick.
    void
    set_predicate(
        predicate_fn _predicate
    )
    {
        m_predicate = static_cast<predicate_fn&&>(_predicate);

        return;
    }

    // set_detail_provider
    //   optional: when a violation occurs, this callable is
    // invoked to produce a human-readable detail string for
    // the report.
    void
    set_detail_provider(
        detail_fn _detail
    )
    {
        m_detail = static_cast<detail_fn&&>(_detail);

        return;
    }

    // set_poll_interval
    //   sets the cadence at which the predicate is
    // evaluated.  Smaller intervals catch more transient
    // violations but contend more with the system under
    // test.
    template<typename _Rep,
             typename _Period>
    void
    set_poll_interval(
        const std::chrono::duration<_Rep, _Period>& _d
    )
    {
        m_poll_interval =
            std::chrono::duration_cast<duration_type>(_d);

        return;
    }

    // set_max_violations_recorded
    //   caps the number of violation records kept in
    // memory.  The violation count itself is not capped -
    // only the per-violation detail records.
    void
    set_max_violations_recorded(
        std::size_t _max
    ) D_NOEXCEPT
    {
        m_max_violations_recorded = _max;

        return;
    }

    // -----------------------------------------------------------------
    //  lifecycle
    // -----------------------------------------------------------------

    // start
    //   spawns the monitor thread.  Has no effect if
    // already running or if no predicate is set.
    void
    start()
    {
        if (m_running.load(std::memory_order_acquire))
        {
            return;
        }

        if (!m_predicate)
        {
            return;
        }

        // reset state for the new run
        {
            std::lock_guard<std::mutex> lk(m_report_mutex);
            m_report = invariant_report();
        }

        m_stop_flag.store(false, std::memory_order_release);
        m_running.store(true, std::memory_order_release);

        m_thread = std::thread(
            [this]()
            {
                this->monitor_loop_();
            });

        return;
    }

    // stop
    //   signals the monitor to stop and joins the thread.
    void
    stop()
    {
        if (!m_running.load(std::memory_order_acquire))
        {
            return;
        }

        m_stop_flag.store(true, std::memory_order_release);

        if (m_thread.joinable())
        {
            m_thread.join();
        }

        m_running.store(false, std::memory_order_release);

        return;
    }

    // running
    bool
    running() const D_NOEXCEPT
    {
        return m_running.load(std::memory_order_acquire);
    }

    // -----------------------------------------------------------------
    //  manual checks
    // -----------------------------------------------------------------

    // check_now
    //   evaluates the predicate immediately on the calling
    // thread.  Records a violation if false.  Returns the
    // result of the predicate.
    bool
    check_now()
    {
        if (!m_predicate)
        {
            return true;
        }

        bool ok = false;

        try
        {
            ok = m_predicate();
        }
        catch (...)
        {
            ok = false;
        }

        record_check_(ok);

        return ok;
    }

    // -----------------------------------------------------------------
    //  reporting
    // -----------------------------------------------------------------

    // report
    //   returns a snapshot of the accumulated report.  Safe
    // to call while monitoring is active.
    invariant_report
    report() const
    {
        std::lock_guard<std::mutex> lk(m_report_mutex);

        return m_report;
    }

    // violation_count
    std::size_t
    violation_count() const
    {
        std::lock_guard<std::mutex> lk(m_report_mutex);

        return m_report.violation_count;
    }

    // success
    bool
    success() const
    {
        return (violation_count() == 0);
    }

private:
    // -----------------------------------------------------------------
    //  monitoring loop
    // -----------------------------------------------------------------

    void
    monitor_loop_()
    {
        while (!m_stop_flag.load(std::memory_order_acquire))
        {
            check_now();
            std::this_thread::sleep_for(m_poll_interval);
        }

        return;
    }

    // record_check_
    //   updates the report under lock.
    void
    record_check_(
        bool _ok
    )
    {
        std::lock_guard<std::mutex> lk(m_report_mutex);

        ++m_report.checks_performed;

        if (_ok)
        {
            return;
        }

        ++m_report.violation_count;

        if (m_report.violations.size() < m_max_violations_recorded)
        {
            std::string detail;

            if (m_detail)
            {
                try
                {
                    detail = m_detail();
                }
                catch (...)
                {}
            }

            m_report.violations.emplace_back(
                clock_type::now(),
                static_cast<std::string&&>(detail));
        }

        return;
    }

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

    predicate_fn          m_predicate;
    detail_fn             m_detail;
    duration_type         m_poll_interval;
    std::size_t           m_max_violations_recorded;
    std::thread           m_thread;
    std::atomic<bool>     m_stop_flag;
    std::atomic<bool>     m_running;
    invariant_report      m_report;
    mutable std::mutex    m_report_mutex;
};


///////////////////////////////////////////////////////////////////////////////
///                III. MONOTONIC GUARD                                     ///
///////////////////////////////////////////////////////////////////////////////

// monotonic_guard
//   class template: enforces "value never decreases" over
// time.  No background thread - the guard wraps a value
// and exposes update() / observe() that verify monotonicity
// as the value moves.
//
//   Internally uses atomic CAS to handle concurrent
// updates.  Returns false (and records a violation) if a
// decrement is attempted.
//
// Template parameter:
//   _Value: an arithmetic or comparable type with operator<.
//
// Example:
//   monotonic_guard<std::uint64_t> seq;
//
//   // worker:
//   std::uint64_t my_seq = next_sequence();
//   if (!seq.observe(my_seq)) { /* sequence went backward */ }
template<typename _Value = std::uint64_t>
class monotonic_guard
{
public:
    using value_type = _Value;
    using size_type  = std::size_t;

    monotonic_guard()
        : m_max_seen(value_type{}),
          m_violations(0),
          m_observations(0)
    {}

    explicit monotonic_guard(
        value_type _initial
    )
        : m_max_seen(_initial),
          m_violations(0),
          m_observations(0)
    {}

    monotonic_guard(const monotonic_guard&)            = delete;
    monotonic_guard& operator=(const monotonic_guard&) = delete;

    // -----------------------------------------------------------------
    //  observation
    // -----------------------------------------------------------------

    // observe
    //   records an observation of _v.  Returns true if _v
    // is greater than or equal to every previously observed
    // value, false (and increments the violation count)
    // otherwise.  Updates the running maximum atomically.
    bool
    observe(
        value_type _v
    )
    {
        m_observations.fetch_add(1, std::memory_order_relaxed);

        // CAS loop to update the maximum
        value_type current =
            m_max_seen.load(std::memory_order_acquire);

        while (true)
        {
            if (_v < current)
            {
                m_violations.fetch_add(
                    1, std::memory_order_relaxed);

                return false;
            }

            // _v >= current; try to update
            if (m_max_seen.compare_exchange_weak(
                    current,
                    _v,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire))
            {
                return true;
            }

            // CAS failed: 'current' has been updated to
            // the actual value; loop again
        }
    }

    // -----------------------------------------------------------------
    //  inspection
    // -----------------------------------------------------------------

    value_type
    max_seen() const D_NOEXCEPT
    {
        return m_max_seen.load(std::memory_order_acquire);
    }

    size_type
    violations() const D_NOEXCEPT
    {
        return m_violations.load(std::memory_order_relaxed);
    }

    size_type
    observations() const D_NOEXCEPT
    {
        return m_observations.load(std::memory_order_relaxed);
    }

    bool
    success() const D_NOEXCEPT
    {
        return (violations() == 0);
    }

    basic_test
    to_test_object(
        test_type_id _type_id,
        const char*  _name = "monotonic"
    ) const
    {
        const bool passed = success();

        basic_test t(_type_id, passed);
        t.metadata().set("name", _name);
        t.metadata().set("message", passed
            ? "value remained monotonic"
            : "value decreased at least once");

        return t;
    }

private:
    std::atomic<value_type> m_max_seen;
    atomic_size             m_violations;
    atomic_size             m_observations;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  BOUNDED GUARD                                       ///
///////////////////////////////////////////////////////////////////////////////

// bounded_guard
//   class template: enforces "value stays within
// [low, high]" over time.  Like monotonic_guard, no
// background thread is involved - the guard is updated
// by callers.
//
// Example:
//   bounded_guard<int> usage(0, 0, capacity);
//
//   // worker (push):
//   usage.observe(usage.current() + 1);
//
//   // worker (pop):
//   usage.observe(usage.current() - 1);
//
//   if (!usage.success()) { /* somebody went out of bounds */ }
template<typename _Value = std::int64_t>
class bounded_guard
{
public:
    using value_type = _Value;
    using size_type  = std::size_t;

    bounded_guard()
        : m_value(value_type{}),
          m_low(value_type{}),
          m_high(value_type{}),
          m_violations(0),
          m_observations(0)
    {}

    bounded_guard(
        value_type _initial,
        value_type _low,
        value_type _high
    )
        : m_value(_initial),
          m_low(_low),
          m_high(_high),
          m_violations(0),
          m_observations(0)
    {}

    bounded_guard(const bounded_guard&)            = delete;
    bounded_guard& operator=(const bounded_guard&) = delete;

    // observe
    //   stores _v atomically.  Records a violation if _v
    // is out of [m_low, m_high].
    bool
    observe(
        value_type _v
    )
    {
        m_value.store(_v, std::memory_order_release);
        m_observations.fetch_add(1, std::memory_order_relaxed);

        bool in_bounds = ( (_v >= m_low) &&
                           (_v <= m_high) );

        if (!in_bounds)
        {
            m_violations.fetch_add(1, std::memory_order_relaxed);
        }

        return in_bounds;
    }

    value_type
    current() const D_NOEXCEPT
    {
        return m_value.load(std::memory_order_acquire);
    }

    value_type
    low() const D_NOEXCEPT
    {
        return m_low;
    }

    value_type
    high() const D_NOEXCEPT
    {
        return m_high;
    }

    size_type
    violations() const D_NOEXCEPT
    {
        return m_violations.load(std::memory_order_relaxed);
    }

    size_type
    observations() const D_NOEXCEPT
    {
        return m_observations.load(std::memory_order_relaxed);
    }

    bool
    success() const D_NOEXCEPT
    {
        return (violations() == 0);
    }

    basic_test
    to_test_object(
        test_type_id _type_id,
        const char*  _name = "bounded"
    ) const
    {
        const bool passed = success();

        basic_test t(_type_id, passed);
        t.metadata().set("name", _name);
        t.metadata().set("message", passed
            ? "value stayed within bounds"
            : "value escaped bounds");

        return t;
    }

private:
    std::atomic<value_type> m_value;
    value_type              m_low;
    value_type              m_high;
    atomic_size             m_violations;
    atomic_size             m_observations;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   INVARIANT SCOPE (RAII)                              ///
///////////////////////////////////////////////////////////////////////////////

// invariant_scope
//   class: RAII helper that starts an invariant_monitor on
// construction and stops it on destruction.  The user
// retrieves the report after the scope ends.
//
// Example:
//   {
//       invariant_scope scope([&q]() {
//           return q.size() <= q.capacity();
//       }, std::chrono::microseconds(50));
//
//       // ... run the workload ...
//
//   }   // scope destructor stops monitoring
//
//   // assert via scope.report()
class invariant_scope
{
public:
    using predicate_fn  = invariant_monitor::predicate_fn;
    using duration_type = invariant_monitor::duration_type;

    invariant_scope(
        predicate_fn _predicate
    )
        : m_monitor(static_cast<predicate_fn&&>(_predicate))
    {
        m_monitor.start();
    }

    template<typename _Rep,
             typename _Period>
    invariant_scope(
        predicate_fn                                _predicate,
        const std::chrono::duration<_Rep, _Period>& _interval
    )
        : m_monitor(static_cast<predicate_fn&&>(_predicate))
    {
        m_monitor.set_poll_interval(_interval);
        m_monitor.start();
    }

    invariant_scope(const invariant_scope&)            = delete;
    invariant_scope& operator=(const invariant_scope&) = delete;

    ~invariant_scope()
    {
        m_monitor.stop();
    }

    // report
    //   returns the accumulated report.  After the scope's
    // destructor has run, the report includes every check
    // performed during the scope's lifetime.
    invariant_report
    report() const
    {
        return m_monitor.report();
    }

    // monitor
    //   returns access to the underlying monitor (e.g. to
    // call check_now() or change cadence).
    invariant_monitor&
    monitor() D_NOEXCEPT
    {
        return m_monitor;
    }

private:
    invariant_monitor m_monitor;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  FACTORY HELPERS                                     ///
///////////////////////////////////////////////////////////////////////////////

// make_invariant_monitor
//   factory: returns a configured invariant_monitor.
template<typename _Predicate>
inline invariant_monitor
make_invariant_monitor(
    _Predicate&& _predicate
)
{
    return invariant_monitor(
        invariant_monitor::predicate_fn(
            static_cast<_Predicate&&>(_predicate)));
}

#endif  // C++11


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_INVARIANT_
