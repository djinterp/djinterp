/*******************************************************************************
* djinterp [tests]                            test_invariant_tests_monotonic.cpp
*
*   Section TU: monotonic_guard<T> - enforcement that observed values never
* decrease.  Non-strict: repeating the current maximum is legal.
*
*
* path:      /tests/djinterp/test/sync/test_invariant_tests_monotonic.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
// djinterp
#include "test_invariant_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_imonotonic_default_construct()
{
    dt::monotonic_guard<int> guard;

    D_HI_CHECK(guard.observations() == 0u);
    D_HI_CHECK(guard.violations() == 0u);
    D_HI_CHECK(guard.max_seen() == 0);      // value-initialised
    D_HI_CHECK(guard.success());

    return true;
}

bool tests_imonotonic_initial_construct()
{
    dt::monotonic_guard<int> guard(100);

    D_HI_CHECK(guard.max_seen() == 100);
    D_HI_CHECK(guard.observations() == 0u);
    D_HI_CHECK(guard.violations() == 0u);

    // anything under the seed is immediately a violation
    D_HI_CHECK(!guard.observe(99));
    D_HI_CHECK(guard.violations() == 1u);

    return true;
}

bool tests_imonotonic_observe_increasing()
{
    dt::monotonic_guard<int> guard;

    D_HI_CHECK(guard.observe(1));
    D_HI_CHECK(guard.observe(2));
    D_HI_CHECK(guard.observe(10));
    D_HI_CHECK(guard.observe(1000));

    D_HI_CHECK(guard.violations() == 0u);
    D_HI_CHECK(guard.max_seen() == 1000);
    D_HI_CHECK(guard.success());

    return true;
}

bool tests_imonotonic_observe_equal_allowed()
{
    dt::monotonic_guard<int> guard;

    D_HI_CHECK(guard.observe(5));
    D_HI_CHECK(guard.observe(5));   // equal is not a decrease
    D_HI_CHECK(guard.observe(5));

    D_HI_CHECK(guard.violations() == 0u);
    D_HI_CHECK(guard.max_seen() == 5);

    return true;
}

bool tests_imonotonic_observe_decreasing()
{
    dt::monotonic_guard<int> guard;

    D_HI_CHECK(guard.observe(10));
    D_HI_CHECK(!guard.observe(9));    // rejected
    D_HI_CHECK(guard.violations() == 1u);
    D_HI_CHECK(!guard.success());

    D_HI_CHECK(!guard.observe(0));    // rejected again
    D_HI_CHECK(guard.violations() == 2u);

    return true;
}

bool tests_imonotonic_max_seen_tracks()
{
    dt::monotonic_guard<int> guard;

    guard.observe(3);
    D_HI_CHECK(guard.max_seen() == 3);

    guard.observe(7);
    D_HI_CHECK(guard.max_seen() == 7);

    guard.observe(7);
    D_HI_CHECK(guard.max_seen() == 7);

    return true;
}

bool tests_imonotonic_max_unchanged_by_violation()
{
    dt::monotonic_guard<int> guard;

    guard.observe(50);
    D_HI_CHECK(guard.max_seen() == 50);

    D_HI_CHECK(!guard.observe(20));   // rejected
    D_HI_CHECK(guard.max_seen() == 50);   // the maximum did not regress

    // and the guard keeps working afterwards
    D_HI_CHECK(guard.observe(60));
    D_HI_CHECK(guard.max_seen() == 60);

    return true;
}

bool tests_imonotonic_observation_count()
{
    dt::monotonic_guard<int> guard;

    guard.observe(1);    // ok
    guard.observe(5);    // ok
    guard.observe(2);    // violation
    guard.observe(9);    // ok
    guard.observe(0);    // violation

    // every call counts, accepted or not
    D_HI_CHECK(guard.observations() == 5u);
    D_HI_CHECK(guard.violations() == 2u);

    return true;
}

bool tests_imonotonic_success_predicate()
{
    dt::monotonic_guard<int> guard;

    D_HI_CHECK(guard.success());

    guard.observe(10);
    D_HI_CHECK(guard.success());

    guard.observe(1);
    D_HI_CHECK(!guard.success());

    return true;
}

bool tests_imonotonic_concurrent_observers_mt()
{
    // The guard must tolerate concurrent observation without inventing
    // violations.  The "advance then observe" pair is taken under a mutex so
    // the FEED is provably non-decreasing in real time - otherwise a thread
    // could legally read its own ticket value after a faster thread had
    // already published a higher one, and the resulting violation would be an
    // artefact of the test rather than a fault in the guard.  The guard's own
    // CAS loop is still driven from every thread at once.
    const std::size_t threads = 4;
    const std::size_t per     = 500;

    dt::monotonic_guard<long> guard;
    std::mutex                feed_mutex;
    long                      ticket = 0;

    std::vector<std::thread> ts;
    for (std::size_t t = 0; t < threads; ++t)
    {
        ts.emplace_back(
            [&guard, &feed_mutex, &ticket]()
            {
                for (std::size_t i = 0; i < per; ++i)
                {
                    std::lock_guard<std::mutex> lk(feed_mutex);

                    ++ticket;
                    guard.observe(ticket);
                }
            });
    }

    for (std::size_t i = 0; i < ts.size(); ++i)
    {
        ts[i].join();
    }

    D_HI_CHECK(guard.observations() == (threads * per));
    D_HI_CHECK(guard.violations() == 0u);
    D_HI_CHECK(guard.success());
    D_HI_CHECK(guard.max_seen() ==
               static_cast<long>(threads * per));

    return true;
}


NS_END  // testing
NS_END  // djinterp
