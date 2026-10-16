/*******************************************************************************
* djinterp [tests]                                   test_stress_tests_timed.cpp
*
*   Section TU: timed_stress - duration-bounded execution.  A timed run has no
* exact op count by construction, so the assertions here are on the
* invariants that DO hold: work happened, the window was honoured, and the
* per-thread vector is shaped correctly.
*
*
* path:      /tests/djinterp/test/sync/test_stress_tests_timed.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
// djinterp
#include "test_stress_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_stimed_run_zero_threads()
{
    hs_tick sink;
    dt::timed_stress stress;
    stress.set_thread_count(0);
    stress.set_duration(hs_timed_span());
    stress.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = stress.run();

    D_HS_CHECK(rep.total_ops == 0u);
    D_HS_CHECK(rep.ops_per_thread.empty());
    D_HS_CHECK(sink.calls.load(std::memory_order_relaxed) == 0);

    return true;
}

bool tests_stimed_run_without_operation()
{
    dt::timed_stress stress;
    stress.set_thread_count(2);
    stress.set_duration(hs_timed_span());
    // no operation bound

    const dt::stress_report rep = stress.run();

    D_HS_CHECK(rep.total_ops == 0u);
    D_HS_CHECK(rep.ops_per_thread.empty());

    return true;
}

bool tests_stimed_run_performs_work()
{
    hs_tick sink;
    dt::timed_stress stress;
    stress.set_thread_count(2);
    stress.set_duration(hs_timed_span());
    stress.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = stress.run();

    // the workers loop until the watchdog trips, so some work must have run
    D_HS_CHECK(rep.total_ops > 0u);
    D_HS_CHECK(sink.calls.load(std::memory_order_relaxed) > 0);
    // the sink and the report agree on how much
    D_HS_CHECK(static_cast<long>(rep.total_ops) ==
               sink.calls.load(std::memory_order_relaxed));

    return true;
}

bool tests_stimed_run_respects_duration()
{
    hs_tick sink;
    const std::chrono::milliseconds window = hs_timed_span();

    dt::timed_stress stress;
    stress.set_thread_count(1);
    stress.set_duration(window);
    stress.set_operation(hs_bump_op(sink));

    const std::chrono::steady_clock::time_point before =
        std::chrono::steady_clock::now();

    const dt::stress_report rep = stress.run();

    const std::chrono::steady_clock::duration observed =
        std::chrono::steady_clock::now() - before;

    // the watchdog sleeps the whole window before stopping the workers, so
    // the call cannot return early
    D_HS_CHECK(observed >= window);
    D_HS_CHECK(rep.wall_clock >= dt::stress_report::duration_type::zero());

    return true;
}

bool tests_stimed_run_ops_per_thread_sizes()
{
    hs_tick sink;
    dt::timed_stress stress;
    stress.set_thread_count(3);
    stress.set_duration(hs_timed_span());
    stress.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = stress.run();

    D_HS_CHECK(rep.ops_per_thread.size() == 3u);

    // the per-thread counts must sum to the reported total
    std::size_t sum = 0;
    for (std::size_t i = 0; i < rep.ops_per_thread.size(); ++i)
    {
        sum += rep.ops_per_thread[i];
    }

    D_HS_CHECK(sum == rep.total_ops);

    return true;
}

bool tests_stimed_run_success_outcome()
{
    hs_tick sink;
    dt::timed_stress stress;
    stress.set_thread_count(2);
    stress.set_duration(hs_timed_span());
    stress.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = stress.run();

    D_HS_CHECK(rep.success());
    D_HS_CHECK(rep.underlying.outcome == dt::concurrent_run_outcome::success);
    D_HS_CHECK(rep.underlying.thread_count == 2u);

    return true;
}

bool tests_stimed_run_join_grace()
{
    // a grace period converts the run's join into a bounded one; the workers
    // stop on the watchdog well inside it, so the run still completes clean
    hs_tick sink;
    dt::timed_stress stress;
    stress.set_thread_count(2);
    stress.set_duration(hs_timed_span());
    stress.set_join_grace(std::chrono::seconds(5));
    stress.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = stress.run();

    D_HS_CHECK(rep.success());
    D_HS_CHECK(rep.total_ops > 0u);

    return true;
}


NS_END  // testing
NS_END  // djinterp
