/*******************************************************************************
* djinterp [cpp][test]                                       test_concurrent.hpp
*
*   The DTest concurrent runner: a workhorse for executing a callable
* concurrently from N threads under controlled conditions.
*
*   The runner orchestrates the four awkward parts of writing a
* concurrent test by hand:
*
*     1. Starting all threads at once
*        Threads spawned through std::thread don't actually start
*        running together - there is unavoidable scheduling jitter.
*        The runner uses DSimultaneousStart to hold every worker at
*        a gate until they have all checked in, then fires the gate.
*
*     2. Joining with a timeout
*        Real concurrent bugs deadlock.  Joining without a timeout
*        means the test framework deadlocks alongside the bug.  The
*        runner enforces a hard timeout and reports surviving
*        threads as failures.
*
*     3. Exception aggregation
*        Each worker can throw independently.  The runner aggregates
*        every captured exception into a per-thread report and
*        produces a single DTestObject summarizing the run.
*
*     4. Per-thread result collection
*        Many concurrent tests compute a per-thread value (e.g.
*        local sum, observed sequence) that the assertion checks
*        post-hoc.  The runner provides a typed result vector keyed
*        by thread id.
*
* EXECUTION PATTERNS
* ------------------
*   The runner exposes several pre-canned patterns:
*
*     run_simultaneous   - every thread runs the same callable
*                          starting at the same instant
*     run_per_thread     - each thread gets its own callable,
*                          all start simultaneously
*     run_reader_writer  - N readers and M writers, started
*                          together, with reader/writer roles
*                          baked into the callable signature
*     run_pipeline       - phase-coupled execution where all
*                          threads progress together through
*                          phases via an internal barrier
*
* PORTABILITY
* -----------
*   Requires C++11 or later.  On C++98/03 the runner degrades to
* a sequential executor: every "thread" runs one after another on
* the calling thread.  Tests still complete and produce results;
* they just can't exercise concurrency.
*
* HOW TO USE
* ----------
*   Make a runner, give it a thread count and a worker, and run it; the report
* converts to a basic_test with to_test_object:
*
*     DConcurrentRunner runner = make_concurrent_runner(8, worker);
*
* OPTIONS
* -------
*   The thread count, the worker or phases, a simultaneous start and a join
* timeout, each set on the runner.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/cpp/modules/test/sync/test_concurrent.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.27
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RUN REPORT
      ----------

II.   CONCURRENT RUNNER
      -----------------

III.  EXECUTION PATTERN HELPERS
      -------------------------

IV.   FACTORY HELPERS
      ---------------
*/

#ifndef DJINTERP_TEST_SYNC_TEST_CONCURRENT_HPP
#define DJINTERP_TEST_SYNC_TEST_CONCURRENT_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/interval/closed_interval.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <exception>
#include <string>

// djinterp
#include "../../../djinterp.hpp"  // framework root: env, read by the gates below

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // std
    #include <atomic>
    #include <chrono>
    #include <functional>
    #include <utility>
    #include <vector>
#endif

// djinterp
#include "../../../sync/condvar.hpp"
#include "../test_common.hpp"
#include "../test_object.hpp"
#include "./test_thread.hpp"
#include "./test_sync.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   RUN REPORT                                          ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// DConcurrentRunOutcome
//   enum: top-level result of a concurrent run.
enum class DConcurrentRunOutcome
{
    success      = 0,  // every thread completed normally
    thread_threw = 1,  // at least one thread captured an exception
    timed_out    = 2,  // join deadline exceeded; some threads alive
    not_started  = 3,  // run() never called or aborted before launch
    unknown      = 4
};

// DThreadReport
//   struct: per-thread record produced by the runner.
struct DThreadReport
{
    using size_type     = std::size_t;
    using duration_type = test_thread::duration_type;

    size_type          id;
    std::string        name;
    DTestThreadState  final_state;
    bool               failed;
    std::string        exception_message;
    duration_type      elapsed;

    DThreadReport()
        : id(0),
          name(),
          final_state(DTestThreadState::idle),
          failed(false),
          exception_message(),
          elapsed(duration_type::zero())
    {}
};

// concurrent_run_report
//   struct: aggregate report from a single concurrent run.
struct concurrent_run_report
{
    using size_type     = std::size_t;
    using duration_type = test_thread::duration_type;

    DConcurrentRunOutcome      outcome;
    size_type                   thread_count;
    size_type                   completed_count;
    size_type                   failed_count;
    size_type                   surviving_count;
    duration_type               wall_clock;
    std::vector<DThreadReport>  threads;
    std::exception_ptr          first_exception;

    concurrent_run_report()
        : outcome(DConcurrentRunOutcome::not_started),
          thread_count(0),
          completed_count(0),
          failed_count(0),
          surviving_count(0),
          wall_clock(duration_type::zero()),
          threads(),
          first_exception(nullptr)
    {}

    // success
    //   convenience: true if every thread completed without
    // exception within the timeout.
    bool success() const D_NOEXCEPT
    {
        return (outcome == DConcurrentRunOutcome::success);
    }

    // to_test_object
    //   converts the report into a basic_test for inclusion
    // in a test tree.  The result encodes outcome as
    // pass/fail and the message string summarizes the run.
    basic_test to_test_object(
        DTestTypeId _type_id,
        const char* _name = "concurrent_run"
    ) const
    {
        basic_test t(_type_id, success());
        t.metadata().set("name", _name);

        if (success())
        {
            t.metadata().set("message", "all threads completed");
        }
        else if (outcome == DConcurrentRunOutcome::timed_out)
        {
            t.metadata().set("message", "timed out: surviving threads remain");
        }
        else if (outcome == DConcurrentRunOutcome::thread_threw)
        {
            t.metadata().set("message", "at least one thread threw");
        }
        else
        {
            t.metadata().set("message", "did not start or unknown failure");
        }

        return t;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  CONCURRENT RUNNER                                   ///
///////////////////////////////////////////////////////////////////////////////

// DConcurrentRunner
//   class: the central engine for running concurrent test
// workloads.  Thread count, optional join timeout, and
// per-thread workers are configured via setters; run()
// drives the whole orchestration and returns a populated
// concurrent_run_report.
//
// Example (homogeneous workload):
//   threadsafe_counter<int, exclusive_lock_policy> ctr;
//   DConcurrentRunner runner;
//   runner.set_thread_count(8);
//   runner.set_worker(
//       [&ctr](std::size_t /*tid*/) {
//           for (int i = 0; i < 1000; ++i) ctr.increment();
//       });
//   auto report = runner.run();
//   // assert ctr.value() == 8000
//
// Example (heterogeneous workload):
//   DConcurrentRunner runner;
//   runner.add_worker([](std::size_t) { /* reader */ });
//   runner.add_worker([](std::size_t) { /* writer */ });
//   runner.add_worker([](std::size_t) { /* watcher */ });
//   auto report = runner.run();
class DConcurrentRunner
{
public:
    using size_type     = std::size_t;
    using duration_type = test_thread::duration_type;
    using clock_type    = test_thread::clock_type;
    using worker_fn     = test_thread::worker_fn;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    DConcurrentRunner()
        : m_thread_count(0),
          m_homogeneous_worker(),
          m_heterogeneous_workers(),
          m_join_timeout(duration_type::zero()),
          m_use_simultaneous_start(true)
    {}

    explicit DConcurrentRunner(
        size_type _thread_count
    )
        : m_thread_count(_thread_count),
          m_homogeneous_worker(),
          m_heterogeneous_workers(),
          m_join_timeout(duration_type::zero()),
          m_use_simultaneous_start(true)
    {}

    DConcurrentRunner(
        const DConcurrentRunner&
    ) = delete;
    DConcurrentRunner& operator=(
        const DConcurrentRunner&
    ) = delete;
    DConcurrentRunner(
        DConcurrentRunner&&
    ) = default;
    DConcurrentRunner& operator=(
        DConcurrentRunner&&
    ) = default;

    // -----------------------------------------------------------------
    //  configuration
    // -----------------------------------------------------------------

    // set_thread_count
    //   sets the number of worker threads to launch when
    // running a homogeneous workload.  Has no effect when
    // heterogeneous workers are configured.
    void set_thread_count(
        size_type _count
    ) D_NOEXCEPT
    {
        m_thread_count = _count;

        return;
    }

    // thread_count
    size_type thread_count() const D_NOEXCEPT
    {
        if (!m_heterogeneous_workers.empty())
        {
            return m_heterogeneous_workers.size();
        }

        return m_thread_count;
    }

    // set_worker
    //   configures a single homogeneous worker.  The same
    // callable will be invoked on every thread.
    void set_worker(
        worker_fn _worker
    )
    {
        m_homogeneous_worker = static_cast<worker_fn&&>(_worker);

        return;
    }

    // add_worker
    //   appends a per-thread worker.  When at least one
    // worker is added this way, the heterogeneous path is
    // used and thread_count reports the number of added
    // workers.
    void add_worker(
        worker_fn _worker
    )
    {
        m_heterogeneous_workers.emplace_back(
            static_cast<worker_fn&&>(_worker));

        return;
    }

    // clear_workers
    //   resets both homogeneous and heterogeneous workers.
    void clear_workers()
    {
        m_homogeneous_worker = worker_fn();
        m_heterogeneous_workers.clear();

        return;
    }

    // set_join_timeout
    //   sets the maximum wall-clock duration to wait for
    // threads to join after the start signal.  Zero means
    // wait indefinitely.
    template<typename Rep,
             typename Period>
    void set_join_timeout(
        const std::chrono::duration<Rep, Period>& _timeout
    )
    {
        m_join_timeout =
            std::chrono::duration_cast<duration_type>(_timeout);

        return;
    }

    // join_timeout
    duration_type join_timeout() const D_NOEXCEPT
    {
        return m_join_timeout;
    }

    // set_simultaneous_start
    //   enables or disables the "all threads start at once"
    // behavior.  When disabled, threads start as soon as
    // they are spawned (with the usual scheduling jitter).
    void set_simultaneous_start(
        bool _enabled
    ) D_NOEXCEPT
    {
        m_use_simultaneous_start = _enabled;

        return;
    }

    // -----------------------------------------------------------------
    //  execution
    // -----------------------------------------------------------------

    // run
    //   launches the workers, waits for completion (or
    // timeout), and returns a populated report.  The
    // configured workers and counts must be valid; if
    // misconfigured, returns a report with outcome
    // not_started.
    concurrent_run_report run()
    {
        concurrent_run_report report;

        size_type total = thread_count();

        if (total == 0)
        {
            report.outcome = DConcurrentRunOutcome::not_started;

            return report;
        }

        // determine per-thread worker function
        auto resolve_worker =
            [this](size_type _i) -> worker_fn
            {
                if (!m_heterogeneous_workers.empty())
                {
                    return m_heterogeneous_workers[_i];
                }

                return m_homogeneous_worker;
            };

        // every worker resolved must be valid
        for (size_type i = 0; i < total; ++i)
        {
            if (!resolve_worker(i))
            {
                report.outcome = DConcurrentRunOutcome::not_started;

                return report;
            }
        }

        report.thread_count = total;

        // build the thread group
        DTestThreadGroup group;
        DSimultaneousStart start(total);

        // spawn workers - wrap each user worker with the
        // simultaneous-start handshake when enabled
        for (size_type i = 0; i < total; ++i)
        {
            worker_fn user_worker = resolve_worker(i);

            if (m_use_simultaneous_start)
            {
                worker_fn wrapped =
                    [&start, user_worker]
                    (size_type _id)
                    {
                        start.ready_and_go();

                        user_worker(_id);
                    };

                group.emplace(i, wrapped);
            }
            else
            {
                group.emplace(i, user_worker);
            }
        }

        auto wall_start = clock_type::now();

        group.start_all();

        // fire the simultaneous start signal once everyone
        // has reached the line
        if (m_use_simultaneous_start)
        {
            start.wait_until_ready();
            start.signal();
        }

        // join with optional timeout
        bool all_joined = true;

        if (m_join_timeout != duration_type::zero())
        {
            all_joined = group.try_join_all_for(m_join_timeout);
        }
        else
        {
            group.join_all();
        }

        report.wall_clock = clock_type::now() - wall_start;

        // aggregate per-thread state
        for (size_type i = 0; i < total; ++i)
        {
            test_thread& t = group.at(i);

            DThreadReport tr;
            tr.id          = t.id();
            tr.name        = t.name();
            tr.final_state = t.state();
            tr.failed      = t.failed();
            tr.elapsed     = t.elapsed();

            if (t.failed())
            {
                tr.exception_message = t.exception_what();
                ++report.failed_count;

                if (!report.first_exception)
                {
                    report.first_exception = t.exception();
                }
            }

            if (t.completed())
            {
                ++report.completed_count;
            }

            if (t.is_running())
            {
                ++report.surviving_count;
            }

            report.threads.push_back(
                static_cast<DThreadReport&&>(tr));
        }

        // derive the outcome
        if (!all_joined)
        {
            report.outcome = DConcurrentRunOutcome::timed_out;
        }
        else if (report.failed_count > 0)
        {
            report.outcome = DConcurrentRunOutcome::thread_threw;
        }
        else if (report.completed_count == total)
        {
            report.outcome = DConcurrentRunOutcome::success;
        }
        else
        {
            report.outcome = DConcurrentRunOutcome::unknown;
        }

        return report;
    }

private:
    size_type              m_thread_count;
    worker_fn              m_homogeneous_worker;
    std::vector<worker_fn> m_heterogeneous_workers;
    duration_type          m_join_timeout;
    bool                   m_use_simultaneous_start;
};


///////////////////////////////////////////////////////////////////////////////
///                III. EXECUTION PATTERN HELPERS                           ///
///////////////////////////////////////////////////////////////////////////////

// run_simultaneous
//   function: convenience pattern.  Runs _worker on
// _thread_count threads with simultaneous start, no
// timeout, returning the report.
template<typename Worker>
inline
concurrent_run_report run_simultaneous(
    std::size_t _thread_count,
    Worker&&    _worker
)
{
    DConcurrentRunner runner(_thread_count);
    runner.set_worker(test_thread::worker_fn(
        static_cast<Worker&&>(_worker)));

    return runner.run();
}

// run_simultaneous_with_timeout
//   function: like run_simultaneous but enforces a
// per-run join timeout.
template<typename Worker,
         typename Rep,
         typename Period>
inline
concurrent_run_report run_simultaneous_with_timeout(
    std::size_t                               _thread_count,
    const std::chrono::duration<Rep, Period>& _timeout,
    Worker&&                                  _worker
)
{
    DConcurrentRunner runner(_thread_count);

    runner.set_worker(test_thread::worker_fn(
        static_cast<Worker&&>(_worker)));
    runner.set_join_timeout(_timeout);

    return runner.run();
}

// run_reader_writer
//   function: launches _readers + _writers threads where
// the reader and writer callables are distinguished by
// the role parameter.
//
// Reader threads are assigned ids [0 .. _readers - 1];
// writer threads are assigned ids [_readers .. total - 1].
template<typename ReaderFn,
         typename WriterFn>
inline
concurrent_run_report run_reader_writer(
    std::size_t _readers,
    std::size_t _writers,
    ReaderFn&&  _reader,
    WriterFn&&  _writer
)
{
    DConcurrentRunner runner;

    test_thread::worker_fn reader_w =
        test_thread::worker_fn(static_cast<ReaderFn&&>(_reader));
    test_thread::worker_fn writer_w =
        test_thread::worker_fn(static_cast<WriterFn&&>(_writer));

    for (std::size_t i = 0; i < _readers; ++i)
    {
        runner.add_worker(reader_w);
    }

    for (std::size_t i = 0; i < _writers; ++i)
    {
        runner.add_worker(writer_w);
    }

    return runner.run();
}


// DPipelineRunner
//   class: phase-synchronized concurrent runner.  Every
// thread executes a sequence of phases; an internal
// barrier ensures all threads are inside the same phase
// at once.
//
//   This is the canonical pattern for testing producer/
// consumer or "hand-shake" protocols where lock-step
// execution is required.
//
// Example:
//   DPipelineRunner pipeline(4);
//   pipeline.add_phase([](std::size_t tid, std::size_t /*phase*/) {
//       // phase 0: setup
//   });
//   pipeline.add_phase([](std::size_t tid, std::size_t /*phase*/) {
//       // phase 1: contended access
//   });
//   pipeline.add_phase([](std::size_t tid, std::size_t /*phase*/) {
//       // phase 2: validate
//   });
//   auto report = pipeline.run();
class DPipelineRunner
{
public:
    using size_type    = std::size_t;
    using phase_fn     = std::function<void(size_type /*tid*/,
                                            size_type /*phase*/)>;

    explicit DPipelineRunner(
        size_type _thread_count
    )
        : m_thread_count(_thread_count),
          m_phases(),
          m_join_timeout(test_thread::duration_type::zero())
    {}

    DPipelineRunner(
        const DPipelineRunner&
    ) = delete;
    DPipelineRunner& operator=(
        const DPipelineRunner&
    ) = delete;

    // add_phase
    //   appends a phase callable.  All threads must complete
    // a phase before any of them progresses to the next.
    void add_phase(
        phase_fn _phase
    )
    {
        m_phases.emplace_back(static_cast<phase_fn&&>(_phase));

        return;
    }

    // phase_count
    size_type phase_count() const D_NOEXCEPT
    {
        return m_phases.size();
    }

    // set_join_timeout
    template<typename Rep,
             typename Period>
    void set_join_timeout(
        const std::chrono::duration<Rep, Period>& _timeout
    )
    {
        m_join_timeout =
            std::chrono::duration_cast<
                test_thread::duration_type>(_timeout);

        return;
    }

    // run
    //   executes all phases across all threads with a
    // barrier between each.  Returns a concurrent_run_report
    // covering the entire pipeline.
    concurrent_run_report run()
    {
        DConcurrentRunner runner(m_thread_count);

        runner.set_join_timeout(m_join_timeout);

        // shared barrier across all threads
        auto barrier =
            std::make_shared<test_barrier>(m_thread_count);

        const auto& phases = m_phases;

        runner.set_worker(
            [barrier, &phases]
            (std::size_t _tid)
            {
                for (std::size_t p = 0; p < phases.size(); ++p)
                {
                    phases[p](_tid, p);
                    barrier->arrive_and_wait();
                }
            });

        return runner.run();
    }

private:
    size_type                       m_thread_count;
    std::vector<phase_fn>           m_phases;
    test_thread::duration_type      m_join_timeout;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  FACTORY HELPERS                                     ///
///////////////////////////////////////////////////////////////////////////////

// make_concurrent_runner
//   factory: returns a configured DConcurrentRunner ready
// to run.
template<typename Worker>
inline
DConcurrentRunner make_concurrent_runner(
    std::size_t _thread_count,
    Worker&&    _worker
)
{
    DConcurrentRunner runner(_thread_count);
    runner.set_worker(test_thread::worker_fn(
        static_cast<Worker&&>(_worker)));

    return runner;
}


#else  // C++98/03 sequential fallback


// Minimal sequential stub: drives a worker pointer once.
struct concurrent_run_report
{
    bool success;
    concurrent_run_report() : success(true) {}
};

// In C++98 mode the runner is a no-op shell; user code that
// needs concurrency must compile under C++11+.

#endif  // C++11


NS_END  // test
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_TEST_SYNC_TEST_CONCURRENT_HPP
