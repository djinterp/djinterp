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
* path:      /tests/djinterp/cpp/modules/test/sync/test_thread_tests_concepts.cpp
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
    D_HT_CHECK(tr::safety_level_aware_type<DLevelProbe>);
    D_HT_CHECK(!tr::safety_level_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_strategy_tagged()
{
    D_HT_CHECK(tr::strategy_tagged_type<DStrategyProbe>);
    D_HT_CHECK(tr::strategy_tagged_type<DRichProbe>);
    D_HT_CHECK(!tr::strategy_tagged_type<plain_probe>);

    return true;
}

bool tests_concepts_lock_policy_aware()
{
    D_HT_CHECK(tr::lock_policy_aware_type<DLockPolicyProbe>);
    D_HT_CHECK(!tr::lock_policy_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_mutex_aware()
{
    D_HT_CHECK(tr::mutex_aware_type<DMutexProbe>);
    D_HT_CHECK(!tr::mutex_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_cow_state_aware()
{
    D_HT_CHECK(tr::cow_state_aware_type<DCowStateProbe>);
    D_HT_CHECK(!tr::cow_state_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_rcu_protected_aware()
{
    D_HT_CHECK(tr::rcu_protected_aware_type<DRcuProbe>);
    D_HT_CHECK(!tr::rcu_protected_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_hazard_domain_aware()
{
    D_HT_CHECK(tr::hazard_domain_aware_type<DHazardProbe>);
    D_HT_CHECK(!tr::hazard_domain_aware_type<plain_probe>);

    return true;
}

bool tests_concepts_try_lockable()
{
    D_HT_CHECK(tr::try_lockable_type<DTryLockProbe>);
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
    D_HT_CHECK(!tr::atomic_capable_type<DStrategyProbe>);

    return true;
}

bool tests_concepts_publishable_and_cow_capable()
{
    D_HT_CHECK(tr::publishable_type<DCowIfaceProbe>);
    D_HT_CHECK(tr::snapshottable_type<DCowIfaceProbe>);
    D_HT_CHECK(tr::cow_capable_type<DCowIfaceProbe>);

    D_HT_CHECK(!tr::publishable_type<plain_probe>);
    D_HT_CHECK(!tr::cow_capable_type<plain_probe>);

    return true;
}

bool tests_concepts_lock_free_testable()
{
    D_HT_CHECK(tr::lock_free_testable_type<atomic_probe>);
    D_HT_CHECK(tr::lock_free_testable_type<DRcuProbe>);
    D_HT_CHECK(tr::lock_free_testable_type<DHazardProbe>);
    D_HT_CHECK(!tr::lock_free_testable_type<plain_probe>);
    D_HT_CHECK(!tr::lock_free_testable_type<DLockPolicyProbe>);

    return true;
}

bool tests_concepts_threadsafe_testable()
{
    D_HT_CHECK(tr::threadsafe_testable_type<DLevelProbe>);
    D_HT_CHECK(tr::threadsafe_testable_type<DStrategyProbe>);
    D_HT_CHECK(tr::threadsafe_testable_type<DLockPolicyProbe>);
    D_HT_CHECK(tr::threadsafe_testable_type<DCowStateProbe>);
    D_HT_CHECK(tr::threadsafe_testable_type<DRcuProbe>);
    D_HT_CHECK(tr::threadsafe_testable_type<DHazardProbe>);
    D_HT_CHECK(tr::threadsafe_testable_type<atomic_probe>);
    D_HT_CHECK(!tr::threadsafe_testable_type<plain_probe>);

    return true;
}

bool tests_concepts_classified_thread_test()
{
    // classified_thread_test_type follows thread_test_class<T>::is_threadsafe
    D_HT_CHECK(tr::classified_thread_test_type<atomic_probe>);
    D_HT_CHECK(tr::classified_thread_test_type<DLockPolicyProbe>);
    D_HT_CHECK(!tr::classified_thread_test_type<plain_probe>);

    return true;
}

bool tests_concepts_race_testable()
{
    D_HT_CHECK(tr::race_testable_type<DFullLockProbe>);   // reader/writer
    D_HT_CHECK(tr::race_testable_type<DCowIfaceProbe>);   // snapshot
    D_HT_CHECK(tr::race_testable_type<atomic_probe>);      // atomic load
    D_HT_CHECK(!tr::race_testable_type<plain_probe>);

    return true;
}

bool tests_concepts_fully_described()
{
    // DRichProbe has both an alias axis (strategy tag) and a method axis
    // (atomic interface), so it is fully described ...
    D_HT_CHECK(tr::fully_described_threadsafe_type<DRichProbe>);
    // ... whereas a type with only an alias axis is not
    D_HT_CHECK(!tr::fully_described_threadsafe_type<DLevelProbe>);
    D_HT_CHECK(!tr::fully_described_threadsafe_type<plain_probe>);

    return true;
}


NS_END  // testing
NS_END  // djinterp

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
