/******************************************************************************
* djinterp [test]                                                test_race.hpp
*
*   Race-condition probing and linearization recording for the DTest
* multithreading harness.
*
*   We can't reliably "detect" data races without a runtime tool like
* TSAN; the standard says any racing read/write is undefined behavior
* and may produce any result.  But we CAN do three things that catch
* the bugs in practice:
*
*     1. RACE PROBES
*        Many threads hammer a non-atomic location for a long time.
*        If the final value, expected from a sequential reasoning,
*        does not match what was observed, a race occurred.  The
*        probe runs the harness many times to reduce false negatives.
*
*     2. ATOMICITY ASSERTIONS
*        Verify that an operation that the user claims is atomic
*        actually appears so to outside observers.  We snapshot the
*        observed state from many threads and check that no
*        intermediate state is ever visible.
*
*     3. LINEARIZATION RECORDING
*        Threads record (start_time, end_time, op_name, args, result)
*        for every operation.  Post-hoc, the recording can be
*        analyzed for serializability - does there exist a sequential
*        ordering of operations consistent with both the timestamps
*        and the observed results?
*
*   Components in this header:
*
*     race_probe        - drives N threads against a non-atomic value
*                         and reports observed final-value variance
*     atomicity_observer - captures intermediate-state snapshots
*     linearization_log  - append-only timestamped op log
*     consistency_check  - sequential-consistency post-hoc verifier
*
*   PORTABILITY:
*   Requires C++11 or later.
*
* TABLE OF CONTENTS
* =================
* I.    RACE PROBE
* II.   ATOMICITY OBSERVER
* III.  LINEARIZATION LOG
* IV.   CONSISTENCY CHECK
* V.    FACTORY HELPERS
*
*
* path:      /inc/djinterp/test/sync/test_race.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_RACE_
#define DJINTERP_TEST_RACE_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>
    #include <chrono>
    #include <functional>
    #include <mutex>
    #include <utility>
    #include <vector>
#endif  // 
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/atomic.hpp"
#include "../test_common.hpp"
#include "../test_object.hpp"
#include "../test_thread.hpp"
#include "../test_concurrent.hpp"


NS_DJINTERP
NS_TEST


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

///////////////////////////////////////////////////////////////////////////////
///                I.   RACE PROBE                                          ///
///////////////////////////////////////////////////////////////////////////////

// race_probe_report
//   struct: result of a race probe run.
struct race_probe_report
{
    using size_type = std::size_t;

    size_type runs_performed;
    size_type runs_with_race_evidence;
    size_type total_thread_iterations;
    bool      any_evidence;

    race_probe_report()
        : runs_performed(0),
          runs_with_race_evidence(0),
          total_thread_iterations(0),
          any_evidence(false)
    {}

    // probability_estimate
    //   returns runs_with_race_evidence / runs_performed,
    // a coarse estimate of how often the bug manifests
    // under this stress pattern.
    double
    probability_estimate() const D_NOEXCEPT
    {
        if (runs_performed == 0)
        {
            return 0.0;
        }

        return static_cast<double>(runs_with_race_evidence) /
               static_cast<double>(runs_performed);
    }

    bool
    success() const D_NOEXCEPT
    {
        return !any_evidence;
    }

    basic_test
    to_test_object(
        test_type_id _type_id,
        const char*  _name = "race_probe"
    ) const
    {
        return basic_test(
            _type_id,
            success(),
            _name,
            "no race evidence observed",
            "race evidence observed");
    }
};


// race_probe
//   class: drives a workload many times across many threads
// and checks that a user-supplied invariant holds at the
// end of every run.  If the invariant is violated, the run
// is counted as race evidence.
//
// Example: testing a non-atomic counter.
//
//   int counter = 0;
//   race_probe probe;
//   probe.set_thread_count(8);
//   probe.set_iterations_per_thread(1000);
//   probe.set_runs(20);
//   probe.set_setup([&counter]() { counter = 0; });
//   probe.set_operation([&counter](std::size_t, std::size_t) {
//       ++counter;   // racy on purpose
//   });
//   probe.set_invariant([&counter]() {
//       return (counter == 8 * 1000);
//   });
//   auto report = probe.run();
//   // For a non-atomic counter, expect any_evidence == true
class race_probe
{
public:
    using size_type     = std::size_t;
    using duration_type = test_thread::duration_type;
    using setup_fn      = std::function<void()>;
    using operation_fn  = std::function<void(size_type /*tid*/,
                                             size_type /*iter*/)>;
    using invariant_fn  = std::function<bool()>;

    race_probe()
        : m_thread_count(2),
          m_iterations(1000),
          m_runs(10),
          m_setup(),
          m_operation(),
          m_invariant(),
          m_join_timeout(duration_type::zero())
    {}

    race_probe(const race_probe&)            = delete;
    race_probe& operator=(const race_probe&) = delete;

    // -----------------------------------------------------------------
    //  configuration
    // -----------------------------------------------------------------

    void set_thread_count(size_type _c)        D_NOEXCEPT { m_thread_count = _c; }
    void set_iterations_per_thread(size_type _i) D_NOEXCEPT { m_iterations = _i; }
    void set_runs(size_type _r)                  D_NOEXCEPT { m_runs = _r;       }

    void
    set_setup(
        setup_fn _setup
    )
    {
        m_setup = static_cast<setup_fn&&>(_setup);

        return;
    }

    void
    set_operation(
        operation_fn _op
    )
    {
        m_operation = static_cast<operation_fn&&>(_op);

        return;
    }

    void
    set_invariant(
        invariant_fn _inv
    )
    {
        m_invariant = static_cast<invariant_fn&&>(_inv);

        return;
    }

    template<typename _Rep,
             typename _Period>
    void
    set_join_timeout(
        const std::chrono::duration<_Rep, _Period>& _t
    )
    {
        m_join_timeout =
            std::chrono::duration_cast<duration_type>(_t);

        return;
    }

    // -----------------------------------------------------------------
    //  execution
    // -----------------------------------------------------------------

    // run
    //   performs m_runs repetitions of the test workload,
    // checking the invariant after each.
    race_probe_report
    run()
    {
        race_probe_report report;

        if ( (m_thread_count == 0) ||
             (m_iterations  == 0) ||
             (m_runs        == 0) ||
             (!m_operation)         ||
             (!m_invariant) )
        {
            return report;
        }

        for (size_type r = 0; r < m_runs; ++r)
        {
            // setup before each run
            if (m_setup)
            {
                m_setup();
            }

            concurrent_runner runner(m_thread_count);
            runner.set_join_timeout(m_join_timeout);

            size_type iters = m_iterations;
            operation_fn op = m_operation;

            runner.set_worker(
                [iters, op]
                (size_type _tid)
                {
                    for (size_type i = 0; i < iters; ++i)
                    {
                        op(_tid, i);
                    }
                });

            auto cr = runner.run();

            ++report.runs_performed;
            report.total_thread_iterations +=
                m_thread_count * m_iterations;

            // a thread that threw is itself race evidence
            if (!cr.success())
            {
                ++report.runs_with_race_evidence;
                report.any_evidence = true;
                continue;
            }

            // check invariant on the main thread (no
            // contention by this point)
            bool ok = false;

            try
            {
                ok = m_invariant();
            }
            catch (...)
            {
                ok = false;
            }

            if (!ok)
            {
                ++report.runs_with_race_evidence;
                report.any_evidence = true;
            }
        }

        return report;
    }

private:
    size_type     m_thread_count;
    size_type     m_iterations;
    size_type     m_runs;
    setup_fn      m_setup;
    operation_fn  m_operation;
    invariant_fn  m_invariant;
    duration_type m_join_timeout;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  ATOMICITY OBSERVER                                  ///
///////////////////////////////////////////////////////////////////////////////

// atomicity_observer_report
//   struct: result of an atomicity observation.
struct atomicity_observer_report
{
    using size_type = std::size_t;

    size_type total_observations;
    size_type allowed_state_observations;
    size_type forbidden_state_observations;

    atomicity_observer_report()
        : total_observations(0),
          allowed_state_observations(0),
          forbidden_state_observations(0)
    {}

    bool
    success() const D_NOEXCEPT
    {
        return (forbidden_state_observations == 0);
    }

    basic_test
    to_test_object(
        test_type_id _type_id,
        const char*  _name = "atomicity"
    ) const
    {
        return basic_test(
            _type_id,
            success(),
            _name,
            "only allowed states observed",
            "intermediate state observed: not atomic");
    }
};


// atomicity_observer
//   class template: spawns N observer threads that
// repeatedly snapshot a value via a user-supplied reader,
// classify each snapshot via a user-supplied state
// classifier, and count occurrences of each state.
//
//   When the operation under test is supposed to be atomic,
// the user provides a state predicate that returns true
// for "valid intermediate-or-final" states.  Any false
// result indicates a torn read or partial update - i.e.
// non-atomicity.
//
// Template parameters:
//   _Snapshot: the snapshot type returned by the reader.
//
// Example: testing torn writes on a 64-bit field.
//
//   std::uint64_t field = 0;
//   atomicity_observer<std::uint64_t> obs;
//   obs.set_observer_count(4);
//   obs.set_observation_count(100000);
//   obs.set_reader([&field]() { return field; });
//   obs.set_allowed([](std::uint64_t v) {
//       return (v == 0 || v == 0xAAAA'AAAA'AAAA'AAAAull);
//   });
//   // ... a writer thread alternates field between the two values ...
//   auto report = obs.run_observers();
template<typename _Snapshot>
class atomicity_observer
{
public:
    using size_type     = std::size_t;
    using snapshot_type = _Snapshot;
    using reader_fn     = std::function<snapshot_type()>;
    using allowed_fn    = std::function<bool(const snapshot_type&)>;

    atomicity_observer()
        : m_observer_count(2),
          m_observations(10000),
          m_reader(),
          m_allowed(),
          m_join_timeout(test_thread::duration_type::zero())
    {}

    atomicity_observer(const atomicity_observer&)            = delete;
    atomicity_observer& operator=(const atomicity_observer&) = delete;

    // -----------------------------------------------------------------
    //  configuration
    // -----------------------------------------------------------------

    void set_observer_count(size_type _c)    D_NOEXCEPT { m_observer_count = _c; }
    void set_observation_count(size_type _o) D_NOEXCEPT { m_observations = _o;   }

    void
    set_reader(
        reader_fn _r
    )
    {
        m_reader = static_cast<reader_fn&&>(_r);

        return;
    }

    void
    set_allowed(
        allowed_fn _a
    )
    {
        m_allowed = static_cast<allowed_fn&&>(_a);

        return;
    }

    template<typename _Rep,
             typename _Period>
    void
    set_join_timeout(
        const std::chrono::duration<_Rep, _Period>& _t
    )
    {
        m_join_timeout =
            std::chrono::duration_cast<
                test_thread::duration_type>(_t);

        return;
    }

    // -----------------------------------------------------------------
    //  execution
    // -----------------------------------------------------------------

    // run_observers
    //   launches m_observer_count threads, each performing
    // m_observations snapshot-and-classify cycles.  This
    // is meant to be run alongside an external writer; the
    // user is responsible for orchestrating that.
    atomicity_observer_report
    run_observers()
    {
        atomicity_observer_report report;

        if ( (m_observer_count == 0) ||
             (m_observations  == 0)  ||
             (!m_reader)               ||
             (!m_allowed) )
        {
            return report;
        }

        std::atomic<size_type> total(0);
        std::atomic<size_type> allowed(0);
        std::atomic<size_type> forbidden(0);

        size_type   obs_count = m_observations;
        reader_fn   reader    = m_reader;
        allowed_fn  allowed_p = m_allowed;

        concurrent_runner runner(m_observer_count);
        runner.set_join_timeout(m_join_timeout);

        runner.set_worker(
            [obs_count, reader, allowed_p,
             &total, &allowed, &forbidden]
            (size_type /*tid*/)
            {
                for (size_type i = 0; i < obs_count; ++i)
                {
                    snapshot_type snap = reader();
                    bool ok            = allowed_p(snap);

                    total.fetch_add(1, std::memory_order_relaxed);

                    if (ok)
                    {
                        allowed.fetch_add(
                            1, std::memory_order_relaxed);
                    }
                    else
                    {
                        forbidden.fetch_add(
                            1, std::memory_order_relaxed);
                    }
                }
            });

        runner.run();

        report.total_observations =
            total.load(std::memory_order_relaxed);
        report.allowed_state_observations =
            allowed.load(std::memory_order_relaxed);
        report.forbidden_state_observations =
            forbidden.load(std::memory_order_relaxed);

        return report;
    }

private:
    size_type                       m_observer_count;
    size_type                       m_observations;
    reader_fn                       m_reader;
    allowed_fn                      m_allowed;
    test_thread::duration_type      m_join_timeout;
};


///////////////////////////////////////////////////////////////////////////////
///                III. LINEARIZATION LOG                                   ///
///////////////////////////////////////////////////////////////////////////////

// linearization_event
//   struct: a single recorded operation in the log.
struct linearization_event
{
    using size_type  = std::size_t;
    using clock_type = test_thread::clock_type;
    using time_point = test_thread::time_point;

    size_type     thread_id;
    std::string   op_name;
    std::string   args;     // freeform - caller chooses format
    std::string   result;   // freeform
    time_point    invoked_at;
    time_point    completed_at;

    linearization_event()
        : thread_id(0),
          op_name(),
          args(),
          result(),
          invoked_at(),
          completed_at()
    {}
};


// linearization_log
//   class: append-only timestamped log of operations.
// Threads call begin_event() before performing the
// operation and end_event() after.  The log records the
// invocation interval for each event.
//
//   The classic Linearizability test asks: does there
// exist a total order of these events, with each event
// placed at some point within its invoked..completed
// interval, that is consistent with the recorded results?
// Full linearizability checking is NP-hard in general but
// well-studied for individual data structures.  This log
// provides the raw data; the consistency_check class
// below offers a basic sequential-consistency verifier.
//
// Example:
//   linearization_log log;
//
//   // in worker:
//   auto h = log.begin_event(tid, "push", "42");
//   q.push(42);
//   log.end_event(h, "ok");
//
//   // post-hoc:
//   auto events = log.snapshot();
//   // ... feed to verifier ...
class linearization_log
{
public:
    using size_type  = std::size_t;
    using clock_type = test_thread::clock_type;
    using time_point = test_thread::time_point;

    // event_handle
    //   token returned by begin_event; pass to end_event.
    struct event_handle
    {
        size_type index;
    };

    linearization_log()
        : m_events(),
          m_mutex()
    {}

    linearization_log(const linearization_log&)            = delete;
    linearization_log& operator=(const linearization_log&) = delete;

    // -----------------------------------------------------------------
    //  recording
    // -----------------------------------------------------------------

    // begin_event
    //   records the invocation timestamp for an operation
    // and returns a handle to be passed to end_event.
    event_handle
    begin_event(
        size_type   _thread_id,
        std::string _op_name,
        std::string _args = std::string()
    )
    {
        linearization_event ev;
        ev.thread_id  = _thread_id;
        ev.op_name    = static_cast<std::string&&>(_op_name);
        ev.args       = static_cast<std::string&&>(_args);
        ev.invoked_at = clock_type::now();

        std::lock_guard<std::mutex> lk(m_mutex);
        m_events.push_back(static_cast<linearization_event&&>(ev));

        event_handle h;
        h.index = m_events.size() - 1;

        return h;
    }

    // end_event
    //   records the completion timestamp and result for an
    // event previously opened with begin_event.
    void
    end_event(
        event_handle _handle,
        std::string  _result = std::string()
    )
    {
        time_point now = clock_type::now();

        std::lock_guard<std::mutex> lk(m_mutex);

        if (_handle.index < m_events.size())
        {
            m_events[_handle.index].completed_at = now;
            m_events[_handle.index].result =
                static_cast<std::string&&>(_result);
        }

        return;
    }

    // record_event
    //   convenience: records a single instantaneous event
    // with both timestamps equal to the call time.
    void
    record_event(
        size_type   _thread_id,
        std::string _op_name,
        std::string _result = std::string()
    )
    {
        linearization_event ev;
        ev.thread_id    = _thread_id;
        ev.op_name      = static_cast<std::string&&>(_op_name);
        ev.result       = static_cast<std::string&&>(_result);
        ev.invoked_at   = clock_type::now();
        ev.completed_at = ev.invoked_at;

        std::lock_guard<std::mutex> lk(m_mutex);
        m_events.push_back(static_cast<linearization_event&&>(ev));

        return;
    }

    // -----------------------------------------------------------------
    //  inspection
    // -----------------------------------------------------------------

    // snapshot
    //   returns a copy of the events in the order they were
    // begun.  Safe to call from any thread.
    std::vector<linearization_event>
    snapshot() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_events;
    }

    // size
    size_type
    size() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        return m_events.size();
    }

    // clear
    void
    clear()
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_events.clear();

        return;
    }

private:
    std::vector<linearization_event> m_events;
    mutable std::mutex               m_mutex;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  CONSISTENCY CHECK                                   ///
///////////////////////////////////////////////////////////////////////////////

// consistency_check_report
//   struct: result of a sequential-consistency analysis.
struct consistency_check_report
{
    using size_type = std::size_t;

    size_type events_examined;
    size_type ordering_violations;
    size_type interval_violations;
    bool      consistent;

    consistency_check_report()
        : events_examined(0),
          ordering_violations(0),
          interval_violations(0),
          consistent(true)
    {}

    basic_test
    to_test_object(
        test_type_id _type_id,
        const char*  _name = "consistency"
    ) const
    {
        return basic_test(
            _type_id,
            consistent,
            _name,
            "events are sequentially consistent",
            "consistency violation observed");
    }
};


// consistency_check
//   class: runs a basic sequential-consistency check
// against a linearization_log.  Verifies two structural
// properties:
//
//     1. INTERVAL VALIDITY
//        For every event, completed_at >= invoked_at.
//
//     2. PER-THREAD PROGRAM ORDER
//        Within a single thread, op N+1's invoked_at
//        is >= op N's completed_at.  This catches log
//        corruption (same handle reused, etc.) and is
//        a necessary condition for sequential consistency.
//
//   Full linearizability checking is NP-hard and
// data-structure-specific; that is intentionally beyond
// the scope of this verifier.  The user can take the
// log snapshot and feed it to a specialized checker.
class consistency_check
{
public:
    using size_type = std::size_t;

    consistency_check_report
    check(
        const linearization_log& _log
    ) const
    {
        consistency_check_report report;

        auto events = _log.snapshot();

        report.events_examined = events.size();

        // pass 1: interval validity
        for (const auto& e : events)
        {
            if (e.completed_at < e.invoked_at)
            {
                ++report.interval_violations;
                report.consistent = false;
            }
        }

        // pass 2: per-thread program order
        // group events by thread id, then verify each group
        // is monotonic in (invoked_at, completed_at).
        std::vector<size_type> by_thread_last_completed;
        std::vector<size_type> thread_ids;

        // simple O(N*T) scan - fine for test diagnostics
        for (size_type i = 0; i < events.size(); ++i)
        {
            const auto& e = events[i];

            // find this thread's last completion time
            for (size_type j = i; j > 0; --j)
            {
                const auto& prev = events[j - 1];

                if (prev.thread_id == e.thread_id)
                {
                    if (e.invoked_at < prev.completed_at)
                    {
                        ++report.ordering_violations;
                        report.consistent = false;
                    }

                    break;
                }
            }
        }

        return report;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                V.   FACTORY HELPERS                                     ///
///////////////////////////////////////////////////////////////////////////////

// make_race_probe
//   factory: returns a configured race_probe.
template<typename _Op,
         typename _Inv>
inline race_probe
make_race_probe(
    std::size_t _thread_count,
    std::size_t _iterations,
    std::size_t _runs,
    _Op&&       _operation,
    _Inv&&      _invariant
)
{
    race_probe probe;
    probe.set_thread_count(_thread_count);
    probe.set_iterations_per_thread(_iterations);
    probe.set_runs(_runs);
    probe.set_operation(race_probe::operation_fn(
        static_cast<_Op&&>(_operation)));
    probe.set_invariant(race_probe::invariant_fn(
        static_cast<_Inv&&>(_invariant)));

    return probe;
}

#endif  // C++11


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_RACE_