/*******************************************************************************
* djinterp [tests]                                   test_sync_tests_barrier.cpp
*
*   Section TU: test_barrier - reusable N-way barrier with an optional
* completion callable.  Phase accounting is asserted on a 1-way barrier (which
* trips without blocking); the meeting semantics are asserted with real
* threads.
*
*
* path:      /tests/djinterp/test/sync/test_sync_tests_barrier.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
// djinterp
#include "test_sync_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace {

// hy_spin_until
//   local helper: poll a predicate up to a deadline so a barrier's arrival
// count can be observed deterministically before the test acts on it.
template<typename _Pred>
bool hy_spin_until(_Pred _p, unsigned _budget_ms = 2000)
{
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(_budget_ms);

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (_p())
        {
            return true;
        }

        std::this_thread::yield();
    }

    return _p();
}

}  // anonymous namespace


bool tests_barrier_construct()
{
    dt::test_barrier barrier(3);

    D_HY_CHECK(barrier.expected() == 3u);
    D_HY_CHECK(barrier.arrived() == 0u);
    D_HY_CHECK(barrier.phase() == 0u);

    return true;
}

bool tests_barrier_single_way_advances()
{
    // a 1-way barrier is tripped by its single arrival, so it never blocks
    dt::test_barrier barrier(1);

    barrier.arrive_and_wait();

    D_HY_CHECK(barrier.phase() == 1u);
    D_HY_CHECK(barrier.arrived() == 0u);   // reset for the next phase

    return true;
}

bool tests_barrier_reusable()
{
    dt::test_barrier barrier(1);

    for (unsigned i = 0; i < 4; ++i)
    {
        barrier.arrive_and_wait();
    }

    D_HY_CHECK(barrier.phase() == 4u);
    D_HY_CHECK(barrier.arrived() == 0u);

    return true;
}

bool tests_barrier_completion_runs()
{
    std::atomic<int> hits(0);

    dt::test_barrier barrier(
        1,
        dt::test_barrier::completion_fn(
            [&hits]()
            {
                hits.fetch_add(1, std::memory_order_acq_rel);
            }));

    barrier.arrive_and_wait();
    D_HY_CHECK(hits.load(std::memory_order_acquire) == 1);

    barrier.arrive_and_wait();
    D_HY_CHECK(hits.load(std::memory_order_acquire) == 2);

    return true;
}

bool tests_barrier_arrive_returns_phase()
{
    // arrive_and_wait returns the phase index the caller arrived in
    dt::test_barrier barrier(1);

    D_HY_CHECK(barrier.arrive_and_wait() == 0u);
    D_HY_CHECK(barrier.arrive_and_wait() == 1u);
    D_HY_CHECK(barrier.arrive_and_wait() == 2u);

    return true;
}

bool tests_barrier_nway_mt()
{
    const unsigned n = 4;
    dt::test_barrier barrier(n);
    std::atomic<int> released(0);

    std::vector<std::thread> ts;
    for (unsigned i = 0; i < n; ++i)
    {
        ts.emplace_back(
            [&barrier, &released]()
            {
                const std::size_t p = barrier.arrive_and_wait();
                if (p == 0u)
                {
                    released.fetch_add(1, std::memory_order_acq_rel);
                }
            });
    }

    for (std::thread& t : ts)
    {
        t.join();
    }

    // every thread met in phase 0 and the barrier advanced exactly once
    D_HY_CHECK(released.load(std::memory_order_acquire) ==
               static_cast<int>(n));
    D_HY_CHECK(barrier.phase() == 1u);
    D_HY_CHECK(barrier.arrived() == 0u);

    return true;
}

bool tests_barrier_completion_once_per_phase_mt()
{
    const unsigned n      = 3;
    const unsigned rounds = 2;
    std::atomic<int> completions(0);

    dt::test_barrier barrier(
        n,
        dt::test_barrier::completion_fn(
            [&completions]()
            {
                completions.fetch_add(1, std::memory_order_acq_rel);
            }));

    std::vector<std::thread> ts;
    for (unsigned i = 0; i < n; ++i)
    {
        ts.emplace_back(
            [&barrier]()
            {
                for (unsigned r = 0; r < rounds; ++r)
                {
                    barrier.arrive_and_wait();
                }
            });
    }

    for (std::thread& t : ts)
    {
        t.join();
    }

    // exactly one completion per tripped phase, not per arrival
    D_HY_CHECK(completions.load(std::memory_order_acquire) ==
               static_cast<int>(rounds));
    D_HY_CHECK(barrier.phase() == rounds);

    return true;
}

bool tests_barrier_arrive_and_drop_mt()
{
    // Two workers wait at a 3-way barrier; the coordinator drops out, which
    // lowers the expected count to 2 and therefore completes the round.
    dt::test_barrier barrier(3);
    std::atomic<int> released(0);

    std::vector<std::thread> ts;
    for (unsigned i = 0; i < 2; ++i)
    {
        ts.emplace_back(
            [&barrier, &released]()
            {
                barrier.arrive_and_wait();
                released.fetch_add(1, std::memory_order_acq_rel);
            });
    }

    // both workers are parked before the drop
    D_HY_CHECK(hy_spin_until([&barrier]() {
        return barrier.arrived() == 2u;
    }));
    D_HY_CHECK(released.load(std::memory_order_acquire) == 0);

    barrier.arrive_and_drop();

    for (std::thread& t : ts)
    {
        t.join();
    }

    D_HY_CHECK(released.load(std::memory_order_acquire) == 2);
    D_HY_CHECK(barrier.expected() == 2u);   // count permanently lowered
    D_HY_CHECK(barrier.phase() == 1u);      // the round completed

    return true;
}


NS_END  // testing
NS_END  // djinterp
