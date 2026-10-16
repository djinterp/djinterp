/*******************************************************************************
* djinterp [tests]                                test_thread_tests_concepts.cpp
*
*   Section TU: the folded C++20 concept layer (djinterp::test::traits).  The
* whole body is gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS - on a compiler
* without concepts this is an empty TU, matching the same-gated declarations
* and spec rows in the suite header.
*
*   Each concept is exercised as a bool constant over the synthetic probes,
* mirroring the trait section one axis at a time.
*
*
* path:      /tests/djinterp/test/sync/test_thread_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_thread_tests.hpp"

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP
NS_TESTING

namespace tr = ::djinterp::test::traits;


bool tests_concepts_negative()
{
    D_HT_CHECK(!tr::safety_level_aware_type<plain_probe>);
    D_HT_CHECK(!tr::strategy_tagged_type<plain_probe>);
    D_HT_CHECK(!tr::lock_policy_aware_type<plain_probe>);
    D_HT_CHECK(!tr::mutex_aware_type<plain_probe>);
    D_HT_CHECK(!tr::cow_state_aware_type<plain_probe>);
    D_HT_CHECK(!tr::rcu_protected_aware_type<plain_probe>);
    D_HT_CHECK(!tr::hazard_domain_aware_type<plain_probe>);
    D_HT_CHECK(!tr::try_lockable_type<plain_probe>);
    D_HT_CHECK(!tr::atomic_capable_type<plain_probe>);
    D_HT_CHECK(!tr::threadsafe_testable_type<plain_probe>);
    D_HT_CHECK(!tr::classified_thread_test_type<plain_probe>);

    return true;
}

bool tests_concepts_safety_level_aware()
{
    D_HT_CHECK(tr::safety_level_aware_type<level_probe>);
    D_HT_CHECK(!tr::safety_level_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_strategy_tagged()
{
    D_HT_CHECK(tr::strategy_tagged_type<strategy_probe>);
    D_HT_CHECK(tr::strategy_tagged_type<rich_probe>);
    D_HT_CHECK(!tr::strategy_tagged_type<plain_probe>);

    return true;
}

bool tests_concepts_lock_policy_aware()
{
    D_HT_CHECK(tr::lock_policy_aware_type<lock_policy_probe>);
    D_HT_CHECK(!tr::lock_policy_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_mutex_aware()
{
    D_HT_CHECK(tr::mutex_aware_type<mutex_probe>);
    D_HT_CHECK(!tr::mutex_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_cow_state_aware()
{
    D_HT_CHECK(tr::cow_state_aware_type<cow_state_probe>);
    D_HT_CHECK(!tr::cow_state_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_rcu_protected_aware()
{
    D_HT_CHECK(tr::rcu_protected_aware_type<rcu_probe>);
    D_HT_CHECK(!tr::rcu_protected_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_hazard_domain_aware()
{
    D_HT_CHECK(tr::hazard_domain_aware_type<hazard_probe>);
    D_HT_CHECK(!tr::hazard_domain_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_try_lockable()
{
    D_HT_CHECK(tr::try_lockable_type<try_lock_probe>);
    D_HT_CHECK(!tr::try_lockable_type<plain_probe>);

    return true;
}

bool tests_concepts_atomic_capable()
{
    D_HT_CHECK(tr::atomic_loadable_type<atomic_probe>);
    D_HT_CHECK(tr::atomic_storable_type<atomic_probe>);
    D_HT_CHECK(tr::atomic_cas_type<atomic_probe>);
    D_HT_CHECK(tr::atomic_capable_type<atomic_probe>);

    D_HT_CHECK(!tr::atomic_capable_type<plain_probe>);
    D_HT_CHECK(!tr::atomic_capable_type<strategy_probe>);

    return true;
}

bool tests_concepts_publishable_and_cow_capable()
{
    D_HT_CHECK(tr::publishable_type<cow_iface_probe>);
    D_HT_CHECK(tr::snapshottable_type<cow_iface_probe>);
    D_HT_CHECK(tr::cow_capable_type<cow_iface_probe>);

    D_HT_CHECK(!tr::publishable_type<plain_probe>);
    D_HT_CHECK(!tr::cow_capable_type<plain_probe>);

    return true;
}

bool tests_concepts_lock_free_testable()
{
    D_HT_CHECK(tr::lock_free_testable_type<atomic_probe>);
    D_HT_CHECK(tr::lock_free_testable_type<rcu_probe>);
    D_HT_CHECK(tr::lock_free_testable_type<hazard_probe>);
    D_HT_CHECK(!tr::lock_free_testable_type<plain_probe>);
    D_HT_CHECK(!tr::lock_free_testable_type<lock_policy_probe>);

    return true;
}

bool tests_concepts_threadsafe_testable()
{
    D_HT_CHECK(tr::threadsafe_testable_type<level_probe>);
    D_HT_CHECK(tr::threadsafe_testable_type<strategy_probe>);
    D_HT_CHECK(tr::threadsafe_testable_type<lock_policy_probe>);
    D_HT_CHECK(tr::threadsafe_testable_type<cow_state_probe>);
    D_HT_CHECK(tr::threadsafe_testable_type<rcu_probe>);
    D_HT_CHECK(tr::threadsafe_testable_type<hazard_probe>);
    D_HT_CHECK(tr::threadsafe_testable_type<atomic_probe>);
    D_HT_CHECK(!tr::threadsafe_testable_type<plain_probe>);

    return true;
}

bool tests_concepts_classified_thread_test()
{
    // classified_thread_test_type follows thread_test_class<T>::is_threadsafe
    D_HT_CHECK(tr::classified_thread_test_type<atomic_probe>);
    D_HT_CHECK(tr::classified_thread_test_type<lock_policy_probe>);
    D_HT_CHECK(!tr::classified_thread_test_type<plain_probe>);

    return true;
}

bool tests_concepts_race_testable()
{
    D_HT_CHECK(tr::race_testable_type<full_lock_probe>);   // reader/writer
    D_HT_CHECK(tr::race_testable_type<cow_iface_probe>);   // snapshot
    D_HT_CHECK(tr::race_testable_type<atomic_probe>);      // atomic load
    D_HT_CHECK(!tr::race_testable_type<plain_probe>);

    return true;
}

bool tests_concepts_fully_described()
{
    // rich_probe has both an alias axis (strategy tag) and a method axis
    // (atomic interface), so it is fully described ...
    D_HT_CHECK(tr::fully_described_threadsafe_type<rich_probe>);
    // ... whereas a type with only an alias axis is not
    D_HT_CHECK(!tr::fully_described_threadsafe_type<level_probe>);
    D_HT_CHECK(!tr::fully_described_threadsafe_type<plain_probe>);

    return true;
}


NS_END  // testing
NS_END  // djinterp

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
