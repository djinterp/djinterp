/*******************************************************************************
* djinterp [cpp][test]                                        test_invariant.hpp
*
*   Concurrent invariant monitoring for the DTest multithreading
* harness.  An invariant is a predicate that should hold at every
* observable instant of the program's execution.  This module
* spawns a background thread that polls the predicate at a
* configurable rate and counts violations.
*
*   Two complementary models are provided:
*
*     1. Polling invariant
*        A background thread evaluates a user-supplied predicate
*        at a fixed cadence.  Each false result is recorded with
*        timestamp.  Best for invariants that may briefly be false
*        and re-true themselves.
*
*     2. Monotonic guard
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
* PORTABILITY
* -----------
*   Requires C++11 or later.
*
* HOW TO USE
* ----------
*   Give a DInvariantMonitor a predicate and a poll interval, run the code under
* test, and read its report; DMonotonicGuard and DBoundedGuard check common
* invariants at each step.
*
* OPTIONS
* -------
*   The predicate, the poll interval, a detail provider and the most violations
* recorded, each set on the monitor.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/modules/test/sync/test_invariant.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.27
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INVARIANT REPORT
      ----------------

II.   POLLING INVARIANT MONITOR
      -------------------------

III.  MONOTONIC GUARD
      ---------------

IV.   BOUNDED GUARD
      -------------

V.    INVARIANT SCOPE (RAII)
      ----------------------

VI.   FACTORY HELPERS
      ---------------
*/

#ifndef DJINTERP_TEST_SYNC_TEST_INVARIANT_HPP
#define DJINTERP_TEST_SYNC_TEST_INVARIANT_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/interval/closed_interval.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <string>

// djinterp
#include "../../../djinterp.hpp"  // framework root: env, read by the gates below
// re_std
#include "../../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint64_t, int64_t

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // std
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
#include "../../../sync/atomic.hpp"
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

// DInvariantViolation
//   struct: a single recorded invariant failure.
struct DInvariantViolation
{
    using time_point = test_thread::time_point;

    time_point  observed_at;
    std::string detail;

    DInvariantViolation()
        : observed_at(),
          detail()
    {}

    DInvariantViolation(
        time_point  _at,
        std::string _detail
    )
        : observed_at(_at),
          detail(static_cast<std::string&&>(_detail))
    {}
};


// DInvariantReport
//   struct: aggregate from a monitoring run.
struct DInvariantReport
{
    using size_type = std::size_t;

    size_type                          checks_performed;
    size_type                          violation_count;
    std::vector<DInvariantViolation>   violations;

    DInvariantReport()
        : checks_performed(0),
          violation_count(0),
          violations()
    {}

    bool success() const D_NOEXCEPT
    {
        return (violation_count == 0);
    }

    basic_test to_test_object(
        DTestTypeId _type_id,
        const char* _name = "invariant"
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

// DInvariantMonitor
//   class: spawns a background thread that polls a
// predicate at a fixed cadence.  Each false result is
// recorded.  Stop the monitor explicitly; the report is
// available afterward.
//
// Example:
//   DThreadsafeArray<int, ...> arr;
//   DInvariantMonitor mon([&arr]() {
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
class DInvariantMonitor
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

    DInvariantMonitor()
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

    explicit DInvariantMonitor(
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

    DInvariantMonitor(
        const DInvariantMonitor&
    ) = delete;
    DInvariantMonitor& operator=(
        const DInvariantMonitor&
    ) = delete;

    ~DInvariantMonitor()
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
    void set_predicate(
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
    void set_detail_provider(
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
    template<typename Rep,
             typename Period>
    void set_poll_interval(
        const std::chrono::duration<Rep, Period>& _d
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
    void set_max_violations_recorded(
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
    void start()
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
            m_report = DInvariantReport();
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
    void stop()
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
    bool running() const D_NOEXCEPT
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
    bool check_now()
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
    DInvariantReport report() const
    {
        std::lock_guard<std::mutex> lk(m_report_mutex);

        return m_report;
    }

    // violation_count
    std::size_t violation_count() const
    {
        std::lock_guard<std::mutex> lk(m_report_mutex);

        return m_report.violation_count;
    }

    // success
    bool success() const
    {
        return (violation_count() == 0);
    }

private:
    // -----------------------------------------------------------------
    //  monitoring loop
    // -----------------------------------------------------------------

    void monitor_loop_()
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
    void record_check_(
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
    DInvariantReport      m_report;
    mutable std::mutex    m_report_mutex;
};


///////////////////////////////////////////////////////////////////////////////
///                III. MONOTONIC GUARD                                     ///
///////////////////////////////////////////////////////////////////////////////

// DMonotonicGuard
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
//   Value: an arithmetic or comparable type with operator<.
//
// Example:
//   DMonotonicGuard<re_std::uint64_t> seq;
//
//   // worker:
//   re_std::uint64_t my_seq = next_sequence();
//   if (!seq.observe(my_seq)) { /* sequence went backward */ }
template<typename Value = re_std::uint64_t>
class DMonotonicGuard
{
public:
    using value_type = Value;
    using size_type  = std::size_t;

    DMonotonicGuard()
        : m_max_seen(value_type{}),
          m_violations(0),
          m_observations(0)
    {}

    explicit DMonotonicGuard(
        value_type _initial
    )
        : m_max_seen(_initial),
          m_violations(0),
          m_observations(0)
    {}

    DMonotonicGuard(const DMonotonicGuard&)            = delete;
    DMonotonicGuard& operator=(const DMonotonicGuard&) = delete;

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
        DTestTypeId _type_id,
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

// DBoundedGuard
//   class template: enforces "value stays within
// [low, high]" over time.  Like DMonotonicGuard, no
// background thread is involved - the guard is updated
// by callers.
//
// Example:
//   DBoundedGuard<int> usage(0, 0, capacity);
//
//   // worker (push):
//   usage.observe(usage.current() + 1);
//
//   // worker (pop):
//   usage.observe(usage.current() - 1);
//
//   if (!usage.success()) { /* somebody went out of bounds */ }
template<typename Value = re_std::int64_t>
class DBoundedGuard
{
public:
    using value_type = Value;
    using size_type  = std::size_t;

    DBoundedGuard()
        : m_value(value_type{}),
          m_low(value_type{}),
          m_high(value_type{}),
          m_violations(0),
          m_observations(0)
    {}

    DBoundedGuard(
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

    DBoundedGuard(const DBoundedGuard&)            = delete;
    DBoundedGuard& operator=(const DBoundedGuard&) = delete;

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
        DTestTypeId _type_id,
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

// DInvariantScope
//   class: RAII helper that starts an DInvariantMonitor on
// construction and stops it on destruction.  The user
// retrieves the report after the scope ends.
//
// Example:
//   {
//       DInvariantScope scope([&q]() {
//           return q.size() <= q.capacity();
//       }, std::chrono::microseconds(50));
//
//       // ... run the workload ...
//
//   }   // scope destructor stops monitoring
//
//   // assert via scope.report()
class DInvariantScope
{
public:
    using predicate_fn  = DInvariantMonitor::predicate_fn;
    using duration_type = DInvariantMonitor::duration_type;

    DInvariantScope(
        predicate_fn _predicate
    )
        : m_monitor(static_cast<predicate_fn&&>(_predicate))
    {
        m_monitor.start();
    }

    template<typename Rep,
             typename Period>
    DInvariantScope(
        predicate_fn                                _predicate,
        const std::chrono::duration<Rep, Period>& _interval
    )
        : m_monitor(static_cast<predicate_fn&&>(_predicate))
    {
        m_monitor.set_poll_interval(_interval);
        m_monitor.start();
    }

    DInvariantScope(
        const DInvariantScope&
    ) = delete;
    DInvariantScope& operator=(
        const DInvariantScope&
    ) = delete;

    ~DInvariantScope()
    {
        m_monitor.stop();
    }

    // report
    //   returns the accumulated report.  After the scope's
    // destructor has run, the report includes every check
    // performed during the scope's lifetime.
    DInvariantReport report() const
    {
        return m_monitor.report();
    }

    // monitor
    //   returns access to the underlying monitor (e.g. to
    // call check_now() or change cadence).
    DInvariantMonitor& monitor() D_NOEXCEPT
    {
        return m_monitor;
    }

private:
    DInvariantMonitor m_monitor;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  FACTORY HELPERS                                     ///
///////////////////////////////////////////////////////////////////////////////

// make_invariant_monitor
//   factory: returns a configured DInvariantMonitor.
template<typename Predicate>
inline
DInvariantMonitor make_invariant_monitor(
    Predicate&& _predicate
)
{
    return DInvariantMonitor(
        DInvariantMonitor::predicate_fn(
            static_cast<Predicate&&>(_predicate)));
}

#endif  // C++11


NS_END  // test
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_TEST_SYNC_TEST_INVARIANT_HPP
