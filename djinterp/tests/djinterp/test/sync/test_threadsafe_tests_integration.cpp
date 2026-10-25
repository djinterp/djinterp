/*******************************************************************************
* djinterp [tests]                         test_threadsafe_tests_integration.cpp
*
*   Section TU: short end-to-end flows spanning several submodules at once.
*
*   The per-module suites cover each module in isolation; these flows exist to
* catch the failures that only appear at the seams - a coordination primitive
* used from inside a runner's worker, a guard observing a concurrent run, a
* higher-level runner delegating to a lower-level one.  Every assertion here
* is deterministic: the counts are exact and the invariants are ones correct
* code cannot break.
*
*
* path:      /tests/djinterp/test/sync/test_threadsafe_tests_integration.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.10.23
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
// djinterp
#include "test_threadsafe_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_uintegration_thread_and_sync()
{
    // test_thread_group (test_thread) coordinated by a latch (test_sync)
    const std::size_t workers = 4;

    dt::test_latch       latch(workers);
    dt::test_thread_group group;
    std::atomic<int>     worked(0);

    for (std::size_t i = 0; i < workers; ++i)
    {
        group.emplace(
            i,
            dt::test_thread::worker_fn(
                [&latch, &worked](std::size_t)
                {
                    worked.fetch_add(1, std::memory_order_acq_rel);
                    latch.count_down();
                }));
    }

    group.start_all();

    latch.wait();   // every worker has passed its count_down

    D_HU_CHECK(latch.remaining() == 0u);
    D_HU_CHECK(latch.arrival_count() == workers);
    D_HU_CHECK(worked.load(std::memory_order_acquire) ==
               static_cast<int>(workers));

    group.join_all();

    D_HU_CHECK(group.all_completed());
    D_HU_CHECK(!group.any_failed());

    return true;
}

bool tests_uintegration_concurrent_and_invariant()
{
    // a bounded_guard (test_invariant) watching a concurrent run
    // (test_concurrent): every observation is in range by construction, so a
    // correct guard must record no violation
    const std::size_t threads = 4;
    const std::size_t iters   = 100;

    dt::bounded_guard<int> guard(0, 0, static_cast<int>(threads) - 1);

    dt::concurrent_runner runner(threads);
    runner.set_worker(
        dt::test_thread::worker_fn(
            [&guard](std::size_t _tid)
            {
                for (std::size_t i = 0; i < iters; ++i)
                {
                    guard.observe(static_cast<int>(_tid));
                }
            }));

    const dt::concurrent_run_report rep = runner.run();

    D_HU_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HU_CHECK(guard.observations() == (threads * iters));
    D_HU_CHECK(guard.violations() == 0u);
    D_HU_CHECK(guard.success());

    return true;
}

bool tests_uintegration_stress_over_concurrent()
{
    // stress_runner (test_stress) delegates to concurrent_runner
    // (test_concurrent) and surfaces its report
    std::atomic<long> hits(0);

    dt::stress_runner runner;
    runner.set_thread_count(3);
    runner.set_iterations(50);
    runner.set_operation(
        dt::stress_runner::operation_fn(
            [&hits](std::size_t, std::size_t)
            {
                hits.fetch_add(1, std::memory_order_relaxed);
            }));

    const dt::stress_report rep = runner.run();

    // the stress verdict is the wrapped concurrent verdict
    D_HU_CHECK(rep.success());
    D_HU_CHECK(rep.underlying.outcome ==
               dt::concurrent_run_outcome::success);
    D_HU_CHECK(rep.underlying.thread_count == 3u);
    D_HU_CHECK(rep.underlying.completed_count == 3u);

    // and the op accounting agrees with the work actually done
    D_HU_CHECK(rep.total_ops == 150u);
    D_HU_CHECK(hits.load(std::memory_order_relaxed) == 150);

    return true;
}

bool tests_uintegration_race_over_concurrent()
{
    // race_probe (test_race) drives concurrent_runner (test_concurrent); the
    // workload is correctly synchronised, so no evidence may be reported
    const std::size_t threads = 4;
    const std::size_t iters   = 50;
    const std::size_t runs    = 3;

    dt::atomic_size shared;

    dt::race_probe probe;
    probe.set_thread_count(threads);
    probe.set_iterations_per_thread(iters);
    probe.set_runs(runs);
    probe.set_setup(
        dt::race_probe::setup_fn(
            [&shared]()
            {
                shared.store(0);
            }));
    probe.set_operation(
        dt::race_probe::operation_fn(
            [&shared](std::size_t, std::size_t)
            {
                shared.fetch_add(1);
            }));
    probe.set_invariant(
        dt::race_probe::invariant_fn(
            [&shared]()
            {
                return shared.load() == (threads * iters);
            }));

    const dt::race_probe_report rep = probe.run();

    D_HU_CHECK(rep.runs_performed == runs);
    D_HU_CHECK(rep.runs_with_race_evidence == 0u);
    D_HU_CHECK(!rep.any_evidence);
    D_HU_CHECK(rep.success());
    D_HU_CHECK(rep.total_thread_iterations == (runs * threads * iters));

    return true;
}


NS_END  // testing
NS_END  // djinterp
