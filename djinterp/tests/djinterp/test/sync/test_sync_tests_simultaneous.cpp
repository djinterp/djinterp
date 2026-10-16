/*******************************************************************************
* djinterp [tests]                              test_sync_tests_simultaneous.cpp
*
*   Section TU: simultaneous_start - the "all threads stand at the line, fire
* on my signal" helper.
*
*   Once signal() has fired, go() no longer blocks, so the coordinator-side
* accounting is asserted single-threaded; the release itself is asserted with
* real workers.  The multithreaded assertions need no sleeps: a worker cannot
* pass go() before the signal, so observing a zero work-counter after the
* quorum is reached is deterministic.
*
*
* path:      /tests/djinterp/test/sync/test_sync_tests_simultaneous.cpp
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


bool tests_simultaneous_construct()
{
    dt::simultaneous_start start(4);

    D_HY_CHECK(start.expected() == 4u);
    D_HY_CHECK(start.ready_count() == 0u);
    D_HY_CHECK(!start.fired());

    return true;
}

bool tests_simultaneous_ready_increments()
{
    dt::simultaneous_start start(3);

    start.ready();
    D_HY_CHECK(start.ready_count() == 1u);

    start.ready();
    start.ready();
    D_HY_CHECK(start.ready_count() == 3u);

    return true;
}

bool tests_simultaneous_wait_until_ready_immediate()
{
    dt::simultaneous_start start(2);

    start.ready();
    start.ready();

    start.wait_until_ready();   // quorum already met: returns at once

    D_HY_CHECK(start.ready_count() == 2u);

    return true;
}

bool tests_simultaneous_wait_until_ready_for_timeout()
{
    dt::simultaneous_start start(2);

    start.ready();   // one short of the quorum
    D_HY_CHECK(!start.wait_until_ready_for(hy_short()));

    start.ready();   // quorum reached
    D_HY_CHECK(start.wait_until_ready_for(hy_short()));

    return true;
}

bool tests_simultaneous_signal_sets_fired()
{
    dt::simultaneous_start start(1);

    D_HY_CHECK(!start.fired());

    start.signal();
    D_HY_CHECK(start.fired());

    return true;
}

bool tests_simultaneous_go_after_signal()
{
    dt::simultaneous_start start(1);
    start.signal();

    start.go();   // already signalled: must not block

    D_HY_CHECK(start.fired());

    return true;
}

bool tests_simultaneous_ready_and_go_after_signal()
{
    dt::simultaneous_start start(1);
    start.signal();

    start.ready_and_go();   // records readiness, then returns

    D_HY_CHECK(start.ready_count() == 1u);
    D_HY_CHECK(start.fired());

    return true;
}

bool tests_simultaneous_full_cycle_mt()
{
    const unsigned n = 4;
    dt::simultaneous_start start(n);
    std::atomic<int> worked(0);

    std::vector<std::thread> ts;
    for (unsigned i = 0; i < n; ++i)
    {
        ts.emplace_back(
            [&start, &worked]()
            {
                start.ready();
                start.go();
                worked.fetch_add(1, std::memory_order_acq_rel);
            });
    }

    start.wait_until_ready();

    // every worker has checked in but none can be past go() yet
    D_HY_CHECK(start.ready_count() == n);
    D_HY_CHECK(worked.load(std::memory_order_acquire) == 0);
    D_HY_CHECK(!start.fired());

    start.signal();

    for (std::thread& t : ts)
    {
        t.join();
    }

    D_HY_CHECK(worked.load(std::memory_order_acquire) ==
               static_cast<int>(n));

    return true;
}

bool tests_simultaneous_wait_then_signal_mt()
{
    const unsigned n = 3;
    dt::simultaneous_start start(n);
    std::atomic<int> worked(0);

    std::vector<std::thread> ts;
    for (unsigned i = 0; i < n; ++i)
    {
        ts.emplace_back(
            [&start, &worked]()
            {
                start.ready_and_go();
                worked.fetch_add(1, std::memory_order_acq_rel);
            });
    }

    start.wait_then_signal();   // blocks for the quorum, then fires

    for (std::thread& t : ts)
    {
        t.join();
    }

    D_HY_CHECK(start.fired());
    D_HY_CHECK(start.ready_count() == n);
    D_HY_CHECK(worked.load(std::memory_order_acquire) ==
               static_cast<int>(n));

    return true;
}


NS_END  // testing
NS_END  // djinterp
