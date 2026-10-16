/*******************************************************************************
* djinterp [tests]                           test_stress_tests_stress_runner.cpp
*
*   Section TU: stress_runner - configuration and the fixed-iteration
* workload.  Iteration counts are exact (threads x iterations), so the
* assertions here are all deterministic.
*
*
* path:      /tests/djinterp/test/sync/test_stress_tests_stress_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <vector>
// djinterp
#include "test_stress_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_srunner_defaults()
{
    dt::stress_runner runner;

    D_HS_CHECK(runner.thread_count() == 1u);
    D_HS_CHECK(runner.iterations() == 1000u);
    D_HS_CHECK(runner.join_timeout() ==
               dt::stress_runner::duration_type::zero());

    return true;
}

bool tests_srunner_setters()
{
    dt::stress_runner runner;

    runner.set_thread_count(4);
    runner.set_iterations(25);
    runner.set_join_timeout(std::chrono::milliseconds(500));

    D_HS_CHECK(runner.thread_count() == 4u);
    D_HS_CHECK(runner.iterations() == 25u);
    D_HS_CHECK(runner.join_timeout() >
               dt::stress_runner::duration_type::zero());

    return true;
}

bool tests_srunner_run_zero_threads()
{
    hs_tick sink;
    dt::stress_runner runner;
    runner.set_thread_count(0);
    runner.set_iterations(10);
    runner.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = runner.run();

    D_HS_CHECK(rep.total_ops == 0u);
    D_HS_CHECK(rep.ops_per_thread.empty());
    D_HS_CHECK(sink.calls.load(std::memory_order_relaxed) == 0);

    return true;
}

bool tests_srunner_run_zero_iterations()
{
    hs_tick sink;
    dt::stress_runner runner;
    runner.set_thread_count(2);
    runner.set_iterations(0);
    runner.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = runner.run();

    D_HS_CHECK(rep.total_ops == 0u);
    D_HS_CHECK(sink.calls.load(std::memory_order_relaxed) == 0);

    return true;
}

bool tests_srunner_run_without_operation()
{
    dt::stress_runner runner;
    runner.set_thread_count(2);
    runner.set_iterations(10);
    // no operation bound

    const dt::stress_report rep = runner.run();

    D_HS_CHECK(rep.total_ops == 0u);
    D_HS_CHECK(rep.ops_per_thread.empty());

    return true;
}

bool tests_srunner_run_total_ops()
{
    hs_tick sink;
    dt::stress_runner runner;
    runner.set_thread_count(4);
    runner.set_iterations(50);
    runner.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = runner.run();

    D_HS_CHECK(rep.total_ops == 200u);                            // 4 x 50
    D_HS_CHECK(sink.calls.load(std::memory_order_relaxed) == 200);

    return true;
}

bool tests_srunner_run_ops_per_thread()
{
    hs_tick sink;
    dt::stress_runner runner;
    runner.set_thread_count(3);
    runner.set_iterations(20);
    runner.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = runner.run();

    D_HS_CHECK(rep.ops_per_thread.size() == 3u);

    for (std::size_t i = 0; i < rep.ops_per_thread.size(); ++i)
    {
        D_HS_CHECK(rep.ops_per_thread[i] == 20u);
    }

    return true;
}

bool tests_srunner_run_iteration_indices()
{
    // each thread must see iteration indices 0..n-1 exactly once
    const std::size_t iters = 16;

    std::mutex mx;
    std::vector<std::set<std::size_t> > seen(2);

    dt::stress_runner runner;
    runner.set_thread_count(2);
    runner.set_iterations(iters);
    runner.set_operation(
        dt::stress_runner::operation_fn(
            [&mx, &seen](std::size_t _tid, std::size_t _iter)
            {
                std::lock_guard<std::mutex> lk(mx);
                seen[_tid].insert(_iter);
            }));

    runner.run();

    D_HS_CHECK(seen[0].size() == iters);
    D_HS_CHECK(seen[1].size() == iters);
    D_HS_CHECK(seen[0].count(0) == 1u);
    D_HS_CHECK(seen[0].count(iters - 1) == 1u);
    D_HS_CHECK(seen[1].count(0) == 1u);
    D_HS_CHECK(seen[1].count(iters - 1) == 1u);

    return true;
}

bool tests_srunner_run_thread_ids()
{
    std::mutex            mx;
    std::set<std::size_t> ids;

    dt::stress_runner runner;
    runner.set_thread_count(4);
    runner.set_iterations(5);
    runner.set_operation(
        dt::stress_runner::operation_fn(
            [&mx, &ids](std::size_t _tid, std::size_t /*iter*/)
            {
                std::lock_guard<std::mutex> lk(mx);
                ids.insert(_tid);
            }));

    runner.run();

    D_HS_CHECK(ids.size() == 4u);
    D_HS_CHECK(ids.count(0) == 1u);
    D_HS_CHECK(ids.count(3) == 1u);

    return true;
}

bool tests_srunner_run_success_outcome()
{
    hs_tick sink;
    dt::stress_runner runner;
    runner.set_thread_count(2);
    runner.set_iterations(10);
    runner.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = runner.run();

    D_HS_CHECK(rep.success());
    D_HS_CHECK(rep.underlying.outcome == dt::concurrent_run_outcome::success);
    D_HS_CHECK(rep.underlying.thread_count == 2u);

    return true;
}

bool tests_srunner_run_operation_throws()
{
    dt::stress_runner runner;
    runner.set_thread_count(2);
    runner.set_iterations(10);
    runner.set_operation(hs_throw_op());

    const dt::stress_report rep = runner.run();

    D_HS_CHECK(!rep.success());
    D_HS_CHECK(rep.underlying.outcome ==
               dt::concurrent_run_outcome::thread_threw);
    D_HS_CHECK(rep.underlying.failed_count == 2u);
    // the op threw on the first iteration, so no iteration completed
    D_HS_CHECK(rep.total_ops == 0u);

    return true;
}

bool tests_srunner_run_wall_clock()
{
    hs_tick sink;
    dt::stress_runner runner;
    runner.set_thread_count(2);
    runner.set_iterations(100);
    runner.set_operation(hs_bump_op(sink));

    const dt::stress_report rep = runner.run();

    D_HS_CHECK(rep.wall_clock >= dt::stress_report::duration_type::zero());
    D_HS_CHECK(rep.ops_per_second() >= 0.0);

    return true;
}

bool tests_srunner_run_repeatable()
{
    hs_tick sink;
    dt::stress_runner runner;
    runner.set_thread_count(2);
    runner.set_iterations(10);
    runner.set_operation(hs_bump_op(sink));

    const dt::stress_report first  = runner.run();
    const dt::stress_report second = runner.run();

    D_HS_CHECK(first.total_ops == 20u);
    D_HS_CHECK(second.total_ops == 20u);
    // each run reports its own counts; the shared sink accumulates both
    D_HS_CHECK(sink.calls.load(std::memory_order_relaxed) == 40);

    return true;
}


NS_END  // testing
NS_END  // djinterp
