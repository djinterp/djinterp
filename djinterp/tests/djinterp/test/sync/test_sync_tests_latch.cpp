/*******************************************************************************
* djinterp [tests]                                     test_sync_tests_latch.cpp
*
*   Section TU: test_latch - one-shot countdown latch.  Countdown/arrival
* accounting is asserted single-threaded; release is asserted with real
* waiters on separate threads.
*
*
* path:      /tests/djinterp/test/sync/test_sync_tests_latch.cpp
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


bool tests_latch_construct()
{
    dt::test_latch latch(3);

    D_HY_CHECK(latch.remaining() == 3u);
    D_HY_CHECK(latch.initial_count() == 3u);
    D_HY_CHECK(latch.arrival_count() == 0u);
    D_HY_CHECK(!latch.try_wait());
    D_HY_CHECK(!latch.is_open());

    return true;
}

bool tests_latch_count_down_single()
{
    dt::test_latch latch(3);

    latch.count_down();

    D_HY_CHECK(latch.remaining() == 2u);
    D_HY_CHECK(latch.arrival_count() == 1u);

    return true;
}

bool tests_latch_count_down_amount()
{
    dt::test_latch latch(5);

    latch.count_down(3);

    D_HY_CHECK(latch.remaining() == 2u);
    D_HY_CHECK(latch.arrival_count() == 3u);

    return true;
}

bool tests_latch_reaches_zero_opens()
{
    dt::test_latch latch(2);

    latch.count_down();
    D_HY_CHECK(!latch.try_wait());

    latch.count_down();
    D_HY_CHECK(latch.remaining() == 0u);
    D_HY_CHECK(latch.try_wait());
    D_HY_CHECK(latch.is_open());

    return true;
}

bool tests_latch_count_down_clamped()
{
    dt::test_latch latch(2);

    latch.count_down(5);   // clamped to the 2 remaining

    D_HY_CHECK(latch.remaining() == 0u);
    D_HY_CHECK(latch.arrival_count() == 2u);   // not 5

    latch.count_down(3);   // already zero: stays clamped
    D_HY_CHECK(latch.remaining() == 0u);
    D_HY_CHECK(latch.arrival_count() == 2u);

    return true;
}

bool tests_latch_wait_returns_when_open()
{
    dt::test_latch latch(1);
    latch.count_down();

    latch.wait();   // already open: must return without blocking

    D_HY_CHECK(latch.try_wait());

    return true;
}

bool tests_latch_wait_for_timeout()
{
    dt::test_latch latch(1);   // never counted down

    const bool opened = latch.wait_for(hy_short());

    D_HY_CHECK(!opened);
    D_HY_CHECK(!latch.try_wait());

    return true;
}

bool tests_latch_wait_for_success()
{
    dt::test_latch latch(1);
    latch.count_down();

    const bool opened = latch.wait_for(hy_short());

    D_HY_CHECK(opened);

    return true;
}

bool tests_latch_arrival_count_tracks()
{
    dt::test_latch latch(4);

    latch.count_down();
    latch.count_down(2);
    D_HY_CHECK(latch.arrival_count() == 3u);
    D_HY_CHECK(latch.remaining() == 1u);

    latch.count_down();
    D_HY_CHECK(latch.arrival_count() == 4u);
    D_HY_CHECK(latch.try_wait());

    return true;
}

bool tests_latch_arrive_and_wait_single()
{
    dt::test_latch latch(1);

    latch.arrive_and_wait();   // last arriver: opens, does not block

    D_HY_CHECK(latch.remaining() == 0u);
    D_HY_CHECK(latch.try_wait());
    D_HY_CHECK(latch.arrival_count() == 1u);

    return true;
}

bool tests_latch_is_open_alias()
{
    dt::test_latch latch(1);
    D_HY_CHECK(latch.is_open() == latch.try_wait());   // both false

    latch.count_down();
    D_HY_CHECK(latch.is_open() == latch.try_wait());   // both true

    return true;
}

bool tests_latch_blocks_until_zero_mt()
{
    dt::test_latch latch(1);
    std::atomic<bool> passed(false);

    std::thread waiter(
        [&latch, &passed]()
        {
            latch.wait();
            passed.store(true, std::memory_order_release);
        });

    // latch is closed, so the waiter cannot have passed yet regardless of
    // scheduling; a brief settle lets it actually reach wait().
    std::this_thread::sleep_for(hy_short());
    D_HY_CHECK(!passed.load(std::memory_order_acquire));

    latch.count_down();
    waiter.join();

    D_HY_CHECK(passed.load(std::memory_order_acquire));
    D_HY_CHECK(latch.try_wait());

    return true;
}

bool tests_latch_arrive_and_wait_mt()
{
    const unsigned n = 4;
    dt::test_latch latch(n);
    std::atomic<int> done(0);

    std::vector<std::thread> ts;
    for (unsigned i = 0; i < n; ++i)
    {
        ts.emplace_back(
            [&latch, &done]()
            {
                latch.arrive_and_wait();
                done.fetch_add(1, std::memory_order_acq_rel);
            });
    }

    for (std::thread& t : ts)
    {
        t.join();
    }

    D_HY_CHECK(done.load(std::memory_order_acquire) == static_cast<int>(n));
    D_HY_CHECK(latch.remaining() == 0u);
    D_HY_CHECK(latch.arrival_count() == n);

    return true;
}


NS_END  // testing
NS_END  // djinterp
