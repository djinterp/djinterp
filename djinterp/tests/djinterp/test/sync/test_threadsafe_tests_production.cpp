/*******************************************************************************
* djinterp [tests]                          test_threadsafe_tests_production.cpp
*
*   Section TU: the production threadsafe types the harness is built on arrive
* with the umbrella.
*
*   NAMESPACE NOTE: the production entities live in djinterp, not
* djinterp::test.  The test subframework re-exports exactly three of them into
* djinterp::test with using-declarations - atomic_size (test_stress, test_race,
* test_invariant), and portable_condvar + exclusive_lock_policy (test_sync,
* test_deadlock).  Everything else must be named through djinterp directly,
* and this TU spells that out deliberately: if a re-export is ever added or
* removed, the mismatch shows up here rather than in downstream code.
*
*
* path:      /tests/djinterp/test/sync/test_threadsafe_tests_production.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <chrono>
#include <mutex>
#include <type_traits>
// djinterp
#include "test_threadsafe_tests.hpp"


NS_DJINTERP
NS_TESTING

// dp names the PRODUCTION subframework, which the umbrella also pulls in.
namespace dp = ::djinterp;


bool tests_uprod_atomic_size()
{
    // re-exported into djinterp::test, so both spellings must resolve to the
    // same type
    D_HU_CHECK((std::is_same<dt::atomic_size, dp::atomic_size>::value));

    dt::atomic_size counter;
    D_HU_CHECK(counter.load() == 0u);

    counter.store(7);
    D_HU_CHECK(counter.load() == 7u);

    const std::size_t previous = counter.fetch_add(3);
    D_HU_CHECK(previous == 7u);
    D_HU_CHECK(counter.load() == 10u);

    // the seeded ctor is reachable too
    dt::atomic_size seeded(42);
    D_HU_CHECK(seeded.load() == 42u);

    return true;
}

bool tests_uprod_exclusive_lock_policy()
{
    typedef dt::exclusive_lock_policy policy;

    D_HU_CHECK((std::is_same<policy, dp::exclusive_lock_policy>::value));

    // the policy's nested vocabulary types resolve
    D_HU_CHECK((std::is_same<policy::mutex_type, std::mutex>::value));
    D_HU_CHECK(policy::is_threadsafe);
    D_HU_CHECK(!policy::is_shared);
    D_HU_CHECK(!policy::is_timed);

    // and the mutex is usable through the alias
    policy::mutex_type mx;
    {
        policy::write_lock_type lk(mx);
        D_HU_CHECK(lk.owns_lock());
    }

    return true;
}

bool tests_uprod_condvar_and_once()
{
    // portable_condvar is a POLICY TEMPLATE, not a plain class
    typedef dt::portable_condvar<dt::exclusive_lock_policy> condvar;
    typedef dt::exclusive_lock_policy                       policy;

    condvar            cv;
    policy::mutex_type mx;

    // a predicate that already holds returns true without blocking
    {
        policy::write_lock_type lk(mx);

        const bool ready =
            cv.wait_for(lk,
                        std::chrono::milliseconds(1),
                        []() { return true; });

        D_HU_CHECK(ready);
    }

    // notify on an unwaited condvar is a safe no-op
    cv.notify_one();
    cv.notify_all();

    // portable_once is NOT re-exported, so it is named through djinterp
    dp::portable_once once;

    int runs = 0;
    once.call([&runs]() { ++runs; });
    once.call([&runs]() { ++runs; });   // already run: must not run again

    D_HU_CHECK(runs == 1);

    return true;
}

bool tests_uprod_thread_helpers()
{
    // both helpers live in djinterp and are not re-exported into the test
    // namespace
    dp::d_thread_yield();               // callable, no observable state

    // hardware_concurrency is permitted to answer 0 when the value is not
    // computable, so only the type and callability are pinned here
    const unsigned cores = dp::hardware_concurrency();
    D_HU_CHECK(cores == cores);         // used, without asserting a floor

    return true;
}


NS_END  // testing
NS_END  // djinterp
