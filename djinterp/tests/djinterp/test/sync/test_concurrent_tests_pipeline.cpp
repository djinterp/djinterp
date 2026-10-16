/*******************************************************************************
* djinterp [tests]                            test_concurrent_tests_pipeline.cpp
*
*   Section TU: pipeline_runner - phase registration and barrier-separated
* phase execution.
*
*   The ordering test leans on the pipeline's contract (phase body, then
* barrier): when any thread is inside phase p, every thread must already have
* finished phase p-1.  That is checked from inside the phase callables rather
* than by timing, so it is deterministic.
*
*
* path:      /tests/djinterp/test/sync/test_concurrent_tests_pipeline.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <mutex>
#include <set>
#include <vector>
// djinterp
#include "test_concurrent_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_cpipeline_construct()
{
    dt::pipeline_runner pipe(4);

    D_HC_CHECK(pipe.phase_count() == 0u);

    return true;
}

bool tests_cpipeline_add_phase()
{
    dt::pipeline_runner pipe(2);

    pipe.add_phase(
        dt::pipeline_runner::phase_fn(
            [](std::size_t, std::size_t) {}));
    D_HC_CHECK(pipe.phase_count() == 1u);

    pipe.add_phase(
        dt::pipeline_runner::phase_fn(
            [](std::size_t, std::size_t) {}));
    pipe.add_phase(
        dt::pipeline_runner::phase_fn(
            [](std::size_t, std::size_t) {}));
    D_HC_CHECK(pipe.phase_count() == 3u);

    return true;
}

bool tests_cpipeline_run_no_phases()
{
    dt::pipeline_runner pipe(3);

    const dt::concurrent_run_report rep = pipe.run();

    // no phases: every thread still spawns, runs an empty loop, and completes
    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(rep.thread_count == 3u);
    D_HC_CHECK(rep.completed_count == 3u);

    return true;
}

bool tests_cpipeline_run_all_phases()
{
    const std::size_t threads = 4;
    std::atomic<int>  invocations(0);

    dt::pipeline_runner pipe(threads);
    for (std::size_t p = 0; p < 3; ++p)
    {
        pipe.add_phase(
            dt::pipeline_runner::phase_fn(
                [&invocations](std::size_t, std::size_t)
                {
                    invocations.fetch_add(1, std::memory_order_acq_rel);
                }));
    }

    const dt::concurrent_run_report rep = pipe.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    // every phase runs once on every thread
    D_HC_CHECK(invocations.load(std::memory_order_acquire) == 12);

    return true;
}

bool tests_cpipeline_phase_indices()
{
    const std::size_t threads = 3;

    std::mutex            mx;
    std::set<std::size_t> seen_phase_0;
    std::set<std::size_t> seen_phase_1;
    std::atomic<bool>     mismatched(false);

    dt::pipeline_runner pipe(threads);

    pipe.add_phase(
        dt::pipeline_runner::phase_fn(
            [&mx, &seen_phase_0, &mismatched]
            (std::size_t _tid, std::size_t _phase)
            {
                if (_phase != 0u)
                {
                    mismatched.store(true, std::memory_order_release);
                }

                std::lock_guard<std::mutex> lk(mx);
                seen_phase_0.insert(_tid);
            }));

    pipe.add_phase(
        dt::pipeline_runner::phase_fn(
            [&mx, &seen_phase_1, &mismatched]
            (std::size_t _tid, std::size_t _phase)
            {
                if (_phase != 1u)
                {
                    mismatched.store(true, std::memory_order_release);
                }

                std::lock_guard<std::mutex> lk(mx);
                seen_phase_1.insert(_tid);
            }));

    const dt::concurrent_run_report rep = pipe.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(!mismatched.load(std::memory_order_acquire));
    // every thread visited both phases
    D_HC_CHECK(seen_phase_0.size() == threads);
    D_HC_CHECK(seen_phase_1.size() == threads);

    return true;
}

bool tests_cpipeline_barrier_ordering()
{
    // The pipeline runs a phase body then meets the barrier, so on entering
    // phase p every thread must already have completed phase p-1.
    const std::size_t threads = 4;
    const std::size_t phases  = 3;

    std::atomic<int>  entered[phases];
    for (std::size_t p = 0; p < phases; ++p)
    {
        entered[p].store(0, std::memory_order_release);
    }

    std::atomic<bool> violation(false);

    dt::pipeline_runner pipe(threads);

    for (std::size_t p = 0; p < phases; ++p)
    {
        pipe.add_phase(
            dt::pipeline_runner::phase_fn(
                [&entered, &violation, p, threads]
                (std::size_t /*tid*/, std::size_t /*phase*/)
                {
                    if (p > 0u)
                    {
                        const int prior =
                            entered[p - 1].load(std::memory_order_acquire);

                        if (prior != static_cast<int>(threads))
                        {
                            violation.store(true, std::memory_order_release);
                        }
                    }

                    entered[p].fetch_add(1, std::memory_order_acq_rel);
                }));
    }

    const dt::concurrent_run_report rep = pipe.run();

    D_HC_CHECK(rep.outcome == dt::concurrent_run_outcome::success);
    D_HC_CHECK(!violation.load(std::memory_order_acquire));

    for (std::size_t p = 0; p < phases; ++p)
    {
        D_HC_CHECK(entered[p].load(std::memory_order_acquire) ==
                   static_cast<int>(threads));
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
