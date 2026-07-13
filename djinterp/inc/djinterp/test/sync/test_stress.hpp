/******************************************************************************
* djinterp [test]                                              test_stress.hpp
*
*   Stress-test harness for the DTest multithreading module: drives a
* set of operations many times across many threads, with optional
* iteration limits, time bounds, randomized op-mix selection, and a
* reproducible RNG seed for re-runnable failures.
*
*   STRESS TESTING IS ABOUT VOLUME AND VARIATION.  A bug that
* manifests once in 10,000 interleavings will not be caught by a
* single deterministic run.  This harness multiplies a target
* operation by tens of thousands of executions across many threads,
* with each thread choosing its next operation by weighted random
* draw, and reports the result as a single test_object.
*
*   COMPONENTS:
*
*     stress_runner     - fixed-iteration stress driver
*     timed_stress      - time-bounded variant (run for N seconds)
*     chaos_runner      - randomized op-mix harness
*     stress_op         - describes a named operation with weight
*                         and callable for chaos_runner
*     stress_report     - per-op counters, total iterations, errors
*
*   REPRODUCIBILITY:
*   chaos_runner accepts a 64-bit seed.  Given the same seed and the
* same op set, two runs produce the same op-selection sequence
* (subject to scheduling - the threads interleave differently each
* time, but each thread's local op stream is deterministic).
*
*   PORTABILITY:
*   Requires C++11 or later for <thread> and <random>.  C++98/03
* code falls back to a sequential single-threaded driver: the same
* iteration count is performed but on the calling thread, with no
* randomness (each op runs in declaration order).
*
*
* TABLE OF CONTENTS
* =================
* I.    STRESS REPORT
* II.   STRESS RUNNER (FIXED ITERATIONS)
* III.  TIMED STRESS RUNNER
* IV.   STRESS OPERATION DESCRIPTOR
* V.    CHAOS RUNNER (WEIGHTED OP-MIX)
* VI.   FACTORY HELPERS
*
*
* path:      /inc/djinterp/test/sync/test_stress.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_STRESS_
#define DJINTERP_TEST_STRESS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>

// djinterp core first: defines the D_ENV_LANG_* gates the std block needs
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <atomic>
    #include <chrono>
    #include <functional>
    #include <random>
    #include <utility>
    #include <vector>
#endif
#include "../../core/sync/atomic.hpp"
#include "../../core/sync/condvar.hpp"
#include "../test_common.hpp"
#include "../test_object.hpp"
#include "./test_thread.hpp"
#include "./test_concurrent.hpp"


NS_DJINTERP
NS_TEST


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// --- threadsafe foundation wrappers used by this module ---
using djinterp::atomic_size;

///////////////////////////////////////////////////////////////////////////////
///                I.   STRESS REPORT                                       ///
///////////////////////////////////////////////////////////////////////////////

// stress_report
//   struct: aggregate result from a stress run.  Combines
// the underlying concurrent_run_report with stress-specific
// counters: total operations executed, per-thread totals,
// and (for chaos_runner) per-op counts.
struct stress_report
{
    using size_type     = std::size_t;
    using duration_type = test_thread::duration_type;

    concurrent_run_report   underlying;
    size_type               total_ops;
    std::vector<size_type>  ops_per_thread;
    std::vector<size_type>  ops_per_kind;     // chaos only
    std::vector<std::string> op_names;         // chaos only
    duration_type           wall_clock;

    stress_report()
        : underlying(),
          total_ops(0),
          ops_per_thread(),
          ops_per_kind(),
          op_names(),
          wall_clock(duration_type::zero())
    {}

    // success
    bool
    success() const D_NOEXCEPT
    {
        return underlying.success();
    }

    // ops_per_second
    //   returns the throughput of the run.  Returns zero if
    // wall_clock is zero (typically a no-op run).
    double
    ops_per_second() const D_NOEXCEPT
    {
        auto seconds =
            std::chrono::duration_cast<
                std::chrono::duration<double>>(wall_clock).count();

        if (seconds <= 0.0)
        {
            return 0.0;
        }

        return static_cast<double>(total_ops) / seconds;
    }

    // to_test_object
    basic_test
    to_test_object(
        test_type_id _type_id,
        const char*  _name = "stress_run"
    ) const
    {
        basic_test t(_type_id, success());
        t.metadata().set("name", _name);

        if (!success())
        {
            t.metadata().set("message", "stress run reported failure");
        }
        else
        {
            t.metadata().set("message", "stress run completed");
        }

        return t;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  STRESS RUNNER (FIXED ITERATIONS)                    ///
///////////////////////////////////////////////////////////////////////////////

// stress_runner
//   class: drives a single callable through a fixed number
// of iterations across N threads.  The callable receives
// (thread_id, iteration_index).
//
// Example:
//   threadsafe_counter<int, exclusive_lock_policy> counter;
//   stress_runner stress;
//   stress.set_thread_count(8);
//   stress.set_iterations(10000);
//   stress.set_operation([&counter](std::size_t /*tid*/, std::size_t /*iter*/) {
//       counter.increment();
//   });
//   auto report = stress.run();
//   // assert counter.value() == 8 * 10000
class stress_runner
{
public:
    using size_type     = std::size_t;
    using duration_type = test_thread::duration_type;
    using clock_type    = test_thread::clock_type;
    using operation_fn  =
        std::function<void(size_type /*tid*/,
                           size_type /*iter*/)>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    stress_runner()
        : m_thread_count(1),
          m_iterations(1000),
          m_operation(),
          m_join_timeout(duration_type::zero())
    {}

    stress_runner(const stress_runner&)            = delete;
    stress_runner& operator=(const stress_runner&) = delete;

    // -----------------------------------------------------------------
    //  configuration
    // -----------------------------------------------------------------

    // set_thread_count
    void
    set_thread_count(
        size_type _count
    ) D_NOEXCEPT
    {
        m_thread_count = _count;

        return;
    }

    // set_iterations
    //   sets the number of iterations each thread will
    // perform.  Total ops = thread_count * iterations.
    void
    set_iterations(
        size_type _iterations
    ) D_NOEXCEPT
    {
        m_iterations = _iterations;

        return;
    }

    // set_operation
    //   sets the per-iteration callable.
    void
    set_operation(
        operation_fn _op
    )
    {
        m_operation = static_cast<operation_fn&&>(_op);

        return;
    }

    // set_join_timeout
    template<typename _Rep,
             typename _Period>
    void
    set_join_timeout(
        const std::chrono::duration<_Rep, _Period>& _timeout
    )
    {
        m_join_timeout =
            std::chrono::duration_cast<duration_type>(_timeout);

        return;
    }

    // -----------------------------------------------------------------
    //  introspection
    // -----------------------------------------------------------------

    size_type     thread_count()  const D_NOEXCEPT { return m_thread_count;  }
    size_type     iterations()    const D_NOEXCEPT { return m_iterations;    }
    duration_type join_timeout()  const D_NOEXCEPT { return m_join_timeout;  }

    // -----------------------------------------------------------------
    //  execution
    // -----------------------------------------------------------------

    // run
    //   launches the configured workload and returns a
    // populated stress_report.
    stress_report
    run()
    {
        stress_report report;

        if ( (m_thread_count == 0) ||
             (m_iterations  == 0) ||
             (!m_operation) )
        {
            return report;
        }

        size_type iterations = m_iterations;
        size_type tcount     = m_thread_count;
        operation_fn op      = m_operation;

        // per-thread iteration counters (atomic so the
        // worker can update without false sharing concerns -
        // contention is acceptable here since these are
        // outside the tested code path)
        std::vector<atomic_size> per_thread(tcount);

        for (size_type i = 0; i < tcount; ++i)
        {
            per_thread[i].store(0, std::memory_order_relaxed);
        }

        concurrent_runner runner(tcount);
        runner.set_join_timeout(m_join_timeout);

        runner.set_worker(
            [iterations, op, &per_thread]
            (size_type _tid)
            {
                for (size_type i = 0; i < iterations; ++i)
                {
                    op(_tid, i);
                    per_thread[_tid].fetch_add(
                        1, std::memory_order_relaxed);
                }
            });

        auto wall_start = clock_type::now();

        report.underlying = runner.run();

        report.wall_clock = clock_type::now() - wall_start;

        // copy per-thread counts into the report
        report.ops_per_thread.reserve(tcount);
        for (size_type i = 0; i < tcount; ++i)
        {
            size_type c =
                per_thread[i].load(std::memory_order_relaxed);

            report.ops_per_thread.push_back(c);
            report.total_ops += c;
        }

        return report;
    }

private:
    size_type     m_thread_count;
    size_type     m_iterations;
    operation_fn  m_operation;
    duration_type m_join_timeout;
};


///////////////////////////////////////////////////////////////////////////////
///                III. TIMED STRESS RUNNER                                 ///
///////////////////////////////////////////////////////////////////////////////

// timed_stress
//   class: stress driver that runs the operation in a tight
// loop on each thread until a wall-clock budget elapses.
// Useful for "let it cook for 5 seconds and tell me what
// happened."
//
// Example:
//   timed_stress stress;
//   stress.set_thread_count(8);
//   stress.set_duration(std::chrono::seconds(5));
//   stress.set_operation([&q](std::size_t tid, std::size_t /*iter*/) {
//       if (tid % 2 == 0) q.push(42);
//       else              q.try_pop();
//   });
//   auto report = stress.run();
class timed_stress
{
public:
    using size_type     = std::size_t;
    using duration_type = test_thread::duration_type;
    using clock_type    = test_thread::clock_type;
    using operation_fn  = stress_runner::operation_fn;

    timed_stress()
        : m_thread_count(1),
          m_duration(std::chrono::seconds(1)),
          m_operation(),
          m_join_grace(duration_type::zero())
    {}

    timed_stress(const timed_stress&)            = delete;
    timed_stress& operator=(const timed_stress&) = delete;

    // -----------------------------------------------------------------
    //  configuration
    // -----------------------------------------------------------------

    void
    set_thread_count(
        size_type _count
    ) D_NOEXCEPT
    {
        m_thread_count = _count;

        return;
    }

    template<typename _Rep,
             typename _Period>
    void
    set_duration(
        const std::chrono::duration<_Rep, _Period>& _d
    )
    {
        m_duration =
            std::chrono::duration_cast<duration_type>(_d);

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

    // set_join_grace
    //   maximum time to wait beyond the duration for
    // workers to notice the stop signal and exit.
    template<typename _Rep,
             typename _Period>
    void
    set_join_grace(
        const std::chrono::duration<_Rep, _Period>& _g
    )
    {
        m_join_grace =
            std::chrono::duration_cast<duration_type>(_g);

        return;
    }

    // -----------------------------------------------------------------
    //  execution
    // -----------------------------------------------------------------

    // run
    //   launches the workload, runs for the configured
    // duration, and returns a populated stress_report.
    stress_report
    run()
    {
        stress_report report;

        if ( (m_thread_count == 0) ||
             (!m_operation) )
        {
            return report;
        }

        size_type    tcount   = m_thread_count;
        operation_fn op       = m_operation;
        auto         duration = m_duration;

        std::vector<atomic_size> per_thread(tcount);

        for (size_type i = 0; i < tcount; ++i)
        {
            per_thread[i].store(0, std::memory_order_relaxed);
        }

        std::atomic<bool> stop_flag(false);

        concurrent_runner runner(tcount);

        // join_timeout = duration + grace period
        if (m_join_grace != duration_type::zero())
        {
            runner.set_join_timeout(duration + m_join_grace);
        }

        runner.set_worker(
            [op, &per_thread, &stop_flag]
            (size_type _tid)
            {
                size_type iter = 0;

                while (!stop_flag.load(std::memory_order_acquire))
                {
                    op(_tid, iter);
                    per_thread[_tid].fetch_add(
                        1, std::memory_order_relaxed);
                    ++iter;
                }
            });

        // arm a watchdog thread that flips stop_flag when
        // the duration elapses.  The workers themselves run
        // on the main path through runner.run().
        std::thread watchdog(
            [&stop_flag, duration]()
            {
                std::this_thread::sleep_for(duration);
                stop_flag.store(true, std::memory_order_release);
            });

        auto wall_start = clock_type::now();

        report.underlying = runner.run();

        // ensure the flag is set even if the runner exited
        // earlier than the duration (e.g. a worker threw)
        stop_flag.store(true, std::memory_order_release);
        watchdog.join();

        report.wall_clock = clock_type::now() - wall_start;

        for (size_type i = 0; i < tcount; ++i)
        {
            size_type c =
                per_thread[i].load(std::memory_order_relaxed);

            report.ops_per_thread.push_back(c);
            report.total_ops += c;
        }

        return report;
    }

private:
    size_type     m_thread_count;
    duration_type m_duration;
    operation_fn  m_operation;
    duration_type m_join_grace;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  STRESS OPERATION DESCRIPTOR                         ///
///////////////////////////////////////////////////////////////////////////////

// stress_op
//   struct: describes a single named operation with a
// selection weight for use with chaos_runner.
//
// Fields:
//   name   - human-readable label, included in the report
//   weight - relative selection weight (positive)
//   action - the callable invoked for this op; receives
//            (thread_id, iteration_index)
struct stress_op
{
    using size_type    = std::size_t;
    using operation_fn =
        std::function<void(size_type /*tid*/,
                           size_type /*iter*/)>;

    std::string  name;
    unsigned     weight;
    operation_fn action;

    stress_op()
        : name(),
          weight(1),
          action()
    {}

    stress_op(
        std::string  _name,
        unsigned     _weight,
        operation_fn _action
    )
        : name(static_cast<std::string&&>(_name)),
          weight(_weight),
          action(static_cast<operation_fn&&>(_action))
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                V.   CHAOS RUNNER (WEIGHTED OP-MIX)                      ///
///////////////////////////////////////////////////////////////////////////////

// chaos_runner
//   class: stress driver where each thread, on each
// iteration, picks an operation at random from a weighted
// menu.  The selection is reproducible given the same
// initial seed.
//
//   Each thread uses its own std::mt19937_64 derived from
// the master seed plus the thread id, so per-thread op
// streams are deterministic.  The interleaving of those
// streams is, by design, non-deterministic.
//
// Example:
//   chaos_runner chaos;
//   chaos.set_thread_count(8);
//   chaos.set_iterations(50000);
//   chaos.set_seed(0xDEADBEEFu);
//   chaos.add_op("push",     3, [&](std::size_t,std::size_t) { q.push(0); });
//   chaos.add_op("pop",      2, [&](std::size_t,std::size_t) { q.try_pop(); });
//   chaos.add_op("size",     1, [&](std::size_t,std::size_t) { q.size(); });
//   auto report = chaos.run();
class chaos_runner
{
public:
    using size_type     = std::size_t;
    using duration_type = test_thread::duration_type;
    using clock_type    = test_thread::clock_type;
    using operation_fn  = stress_op::operation_fn;

    chaos_runner()
        : m_thread_count(1),
          m_iterations(1000),
          m_seed(0xC0FFEEu),
          m_ops(),
          m_join_timeout(duration_type::zero())
    {}

    chaos_runner(const chaos_runner&)            = delete;
    chaos_runner& operator=(const chaos_runner&) = delete;

    // -----------------------------------------------------------------
    //  configuration
    // -----------------------------------------------------------------

    void
    set_thread_count(
        size_type _count
    ) D_NOEXCEPT
    {
        m_thread_count = _count;

        return;
    }

    void
    set_iterations(
        size_type _iterations
    ) D_NOEXCEPT
    {
        m_iterations = _iterations;

        return;
    }

    // set_seed
    //   sets the master RNG seed for op selection.  Same
    // seed + same op set => same per-thread op streams.
    void
    set_seed(
        std::uint64_t _seed
    ) D_NOEXCEPT
    {
        m_seed = _seed;

        return;
    }

    // add_op
    //   appends a named op to the menu.
    void
    add_op(
        std::string  _name,
        unsigned     _weight,
        operation_fn _action
    )
    {
        m_ops.emplace_back(
            static_cast<std::string&&>(_name),
            _weight,
            static_cast<operation_fn&&>(_action));

        return;
    }

    // add_op (anonymous, default weight 1)
    void
    add_op(
        operation_fn _action
    )
    {
        m_ops.emplace_back(
            std::string("op"),
            1u,
            static_cast<operation_fn&&>(_action));

        return;
    }

    // op_count
    size_type
    op_count() const D_NOEXCEPT
    {
        return m_ops.size();
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
    //   launches the chaos workload.  Each thread iterates
    // m_iterations times, picking an op by weighted draw
    // each time.  Returns a populated stress_report with
    // per-op counts.
    stress_report
    run()
    {
        stress_report report;

        if ( (m_thread_count == 0) ||
             (m_iterations  == 0) ||
             (m_ops.empty()) )
        {
            return report;
        }

        size_type tcount     = m_thread_count;
        size_type iterations = m_iterations;
        std::uint64_t seed   = m_seed;
        const auto& ops      = m_ops;

        // per-thread iteration counts and per-op atomic
        // counts (op counts are global since op selection
        // is independent across threads but the totals
        // need to be aggregated)
        std::vector<atomic_size> per_thread(tcount);
        std::vector<atomic_size> per_op(ops.size());

        for (size_type i = 0; i < tcount; ++i)
        {
            per_thread[i].store(0, std::memory_order_relaxed);
        }

        for (size_type i = 0; i < ops.size(); ++i)
        {
            per_op[i].store(0, std::memory_order_relaxed);
        }

        // build cumulative weight table for fast O(log N)
        // selection - one shared, immutable copy
        std::vector<unsigned> cumulative;
        cumulative.reserve(ops.size());
        unsigned running = 0;

        for (const auto& op : ops)
        {
            running += (op.weight > 0) ? op.weight : 1u;
            cumulative.push_back(running);
        }

        unsigned total_weight = running;

        concurrent_runner runner(tcount);
        runner.set_join_timeout(m_join_timeout);

        runner.set_worker(
            [iterations, seed, &ops, &cumulative, total_weight,
             &per_thread, &per_op]
            (size_type _tid)
            {
                std::mt19937_64 rng(
                    seed ^ static_cast<std::uint64_t>(_tid));
                std::uniform_int_distribution<unsigned>
                    dist(0, total_weight - 1);

                for (size_type i = 0; i < iterations; ++i)
                {
                    unsigned roll = dist(rng);

                    // binary search the cumulative table
                    size_type lo = 0;
                    size_type hi = cumulative.size();

                    while (lo < hi)
                    {
                        size_type mid = lo + ((hi - lo) / 2);

                        if (cumulative[mid] <= roll)
                        {
                            lo = mid + 1;
                        }
                        else
                        {
                            hi = mid;
                        }
                    }

                    size_type op_idx = lo;

                    if (op_idx >= ops.size())
                    {
                        op_idx = ops.size() - 1;
                    }

                    ops[op_idx].action(_tid, i);

                    per_op[op_idx].fetch_add(
                        1, std::memory_order_relaxed);
                    per_thread[_tid].fetch_add(
                        1, std::memory_order_relaxed);
                }
            });

        auto wall_start = clock_type::now();

        report.underlying = runner.run();

        report.wall_clock = clock_type::now() - wall_start;

        // copy out per-thread totals
        for (size_type i = 0; i < tcount; ++i)
        {
            size_type c =
                per_thread[i].load(std::memory_order_relaxed);

            report.ops_per_thread.push_back(c);
            report.total_ops += c;
        }

        // copy out per-op totals and names
        for (size_type i = 0; i < ops.size(); ++i)
        {
            report.ops_per_kind.push_back(
                per_op[i].load(std::memory_order_relaxed));
            report.op_names.push_back(ops[i].name);
        }

        return report;
    }

private:
    size_type              m_thread_count;
    size_type              m_iterations;
    std::uint64_t          m_seed;
    std::vector<stress_op> m_ops;
    duration_type          m_join_timeout;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  FACTORY HELPERS                                     ///
///////////////////////////////////////////////////////////////////////////////

// make_stress
//   factory: returns a configured stress_runner.
template<typename _Op>
inline stress_runner
make_stress(
    std::size_t _thread_count,
    std::size_t _iterations,
    _Op&&       _op
)
{
    stress_runner runner;
    runner.set_thread_count(_thread_count);
    runner.set_iterations(_iterations);
    runner.set_operation(stress_runner::operation_fn(
        static_cast<_Op&&>(_op)));

    return runner;
}

// make_timed_stress
//   factory: returns a configured timed_stress.
template<typename _Op,
         typename _Rep,
         typename _Period>
inline timed_stress
make_timed_stress(
    std::size_t                                 _thread_count,
    const std::chrono::duration<_Rep, _Period>& _duration,
    _Op&&                                       _op
)
{
    timed_stress stress;
    stress.set_thread_count(_thread_count);
    stress.set_duration(_duration);
    stress.set_operation(timed_stress::operation_fn(
        static_cast<_Op&&>(_op)));

    return stress;
}

#endif  // C++11


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_STRESS_
