/*******************************************************************************
* djinterp [tests]                              test_invariant_tests_bounded.cpp
*
*   Section TU: bounded_guard<T> - enforcement that observed values stay
* inside an inclusive [low, high] range.
*
*
* path:      /tests/djinterp/test/sync/test_invariant_tests_bounded.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <thread>
#include <vector>
// djinterp
#include "test_invariant_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_ibounded_construct_range()
{
    dt::bounded_guard<int> guard(0, 100);

    D_HI_CHECK(guard.low() == 0);
    D_HI_CHECK(guard.high() == 100);
    D_HI_CHECK(guard.observations() == 0u);
    D_HI_CHECK(guard.violations() == 0u);
    D_HI_CHECK(guard.success());

    return true;
}

bool tests_ibounded_observe_inside()
{
    dt::bounded_guard<int> guard(0, 10);

    D_HI_CHECK(guard.observe(5));
    D_HI_CHECK(guard.observe(1));
    D_HI_CHECK(guard.observe(9));

    D_HI_CHECK(guard.violations() == 0u);
    D_HI_CHECK(guard.success());

    return true;
}

bool tests_ibounded_observe_at_bounds()
{
    dt::bounded_guard<int> guard(-5, 5);

    // both endpoints are inclusive
    D_HI_CHECK(guard.observe(-5));
    D_HI_CHECK(guard.observe(5));

    D_HI_CHECK(guard.violations() == 0u);

    return true;
}

bool tests_ibounded_observe_below()
{
    dt::bounded_guard<int> guard(0, 10);

    D_HI_CHECK(!guard.observe(-1));

    D_HI_CHECK(guard.violations() == 1u);
    D_HI_CHECK(!guard.success());

    return true;
}

bool tests_ibounded_observe_above()
{
    dt::bounded_guard<int> guard(0, 10);

    D_HI_CHECK(!guard.observe(11));

    D_HI_CHECK(guard.violations() == 1u);
    D_HI_CHECK(!guard.success());

    return true;
}

bool tests_ibounded_current_tracks()
{
    dt::bounded_guard<int> guard(0, 100);

    guard.observe(42);
    D_HI_CHECK(guard.current() == 42);

    guard.observe(7);
    D_HI_CHECK(guard.current() == 7);

    // an out-of-range observation is still the most recent one seen
    guard.observe(500);
    D_HI_CHECK(guard.current() == 500);
    D_HI_CHECK(guard.violations() == 1u);

    return true;
}

bool tests_ibounded_observation_count()
{
    dt::bounded_guard<int> guard(0, 10);

    guard.observe(1);     // ok
    guard.observe(-4);    // violation
    guard.observe(10);    // ok
    guard.observe(99);    // violation

    // every call counts, in range or not
    D_HI_CHECK(guard.observations() == 4u);
    D_HI_CHECK(guard.violations() == 2u);

    return true;
}

bool tests_ibounded_success_predicate()
{
    dt::bounded_guard<int> guard(0, 10);

    D_HI_CHECK(guard.success());

    guard.observe(5);
    D_HI_CHECK(guard.success());

    guard.observe(50);
    D_HI_CHECK(!guard.success());

    // once violated, later good observations do not clear the verdict
    guard.observe(5);
    D_HI_CHECK(!guard.success());

    return true;
}

bool tests_ibounded_concurrent_observers_mt()
{
    // every thread observes only in-range values, so a correct guard must
    // record no violations regardless of interleaving
    const std::size_t threads = 4;
    const std::size_t per     = 500;

    dt::bounded_guard<int> guard(0, 10);

    std::vector<std::thread> ts;
    for (std::size_t t = 0; t < threads; ++t)
    {
        ts.emplace_back(
            [&guard, t]()
            {
                for (std::size_t i = 0; i < per; ++i)
                {
                    guard.observe(static_cast<int>(t % 10));
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

    return true;
}


NS_END  // testing
NS_END  // djinterp
