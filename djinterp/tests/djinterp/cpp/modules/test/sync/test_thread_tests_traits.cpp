/*******************************************************************************
* djinterp [cpp][test]                              test_thread_tests_traits.cpp
*
*   Section TU: the folded threadsafe-testable SFINAE trait surface
* (djinterp::test::traits).  Each axis is asserted with a positive probe (the
* fixture carrying that marker) and the marker-free negative probe.
*
*   Note on the re-exported method probes (has_read_lock_method /
* has_write_lock_method / has_snapshot_method): these are defined upstream in
* concurrency_strategy_traits.hpp and re-exported by the trait header.  The
* fixtures here match the documented shape (read_lock() const, write_lock(),
* snapshot()); if an upstream probe is stricter than documented the composite
* below will flag it - which is the intent of a test.
*
*
* path:      /tests/djinterp/cpp/modules/test/sync/test_thread_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_thread_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace tr = ::djinterp::test::traits;


bool tests_traits_negative_all_false()
{
    D_HT_CHECK(!tr::has_thread_safety_level<plain_probe>::value);
    D_HT_CHECK(!tr::has_concurrency_strategy_tag<plain_probe>::value);
    D_HT_CHECK(!tr::has_lock_policy_type<plain_probe>::value);
    D_HT_CHECK(!tr::has_mutex_type<plain_probe>::value);
    D_HT_CHECK(!tr::has_cow_state_type<plain_probe>::value);
    D_HT_CHECK(!tr::has_rcu_protected_type<plain_probe>::value);
    D_HT_CHECK(!tr::has_hazard_domain_type<plain_probe>::value);
    D_HT_CHECK(!tr::has_try_lock_method<plain_probe>::value);
    D_HT_CHECK(!tr::has_atomic_load_method<plain_probe>::value);
    D_HT_CHECK(!tr::has_atomic_store_method<plain_probe>::value);
    D_HT_CHECK(!tr::has_atomic_interface<plain_probe>::value);
    D_HT_CHECK(!tr::has_publish_method<plain_probe>::value);
    D_HT_CHECK(!tr::is_threadsafe_testable<plain_probe>::value);
    D_HT_CHECK(!tr::thread_test_class<plain_probe>::is_threadsafe);

    return true;
}

bool tests_traits_has_thread_safety_level()
{
    D_HT_CHECK(tr::has_thread_safety_level<DLevelProbe>::value);
    D_HT_CHECK(!tr::has_thread_safety_level<plain_probe>::value);

    return true;
}

bool tests_traits_has_concurrency_strategy_tag()
{
    D_HT_CHECK(tr::has_concurrency_strategy_tag<DStrategyProbe>::value);
    D_HT_CHECK(!tr::has_concurrency_strategy_tag<plain_probe>::value);

    return true;
}

bool tests_traits_has_lock_policy_type()
{
    D_HT_CHECK(tr::has_lock_policy_type<DLockPolicyProbe>::value);
    D_HT_CHECK(!tr::has_lock_policy_type<plain_probe>::value);

    return true;
}

bool tests_traits_has_mutex_type()
{
    D_HT_CHECK(tr::has_mutex_type<DMutexProbe>::value);
    D_HT_CHECK(!tr::has_mutex_type<plain_probe>::value);

    return true;
}

bool tests_traits_has_cow_state_type()
{
    D_HT_CHECK(tr::has_cow_state_type<DCowStateProbe>::value);
    D_HT_CHECK(!tr::has_cow_state_type<plain_probe>::value);

    return true;
}

bool tests_traits_has_rcu_protected_type()
{
    D_HT_CHECK(tr::has_rcu_protected_type<DRcuProbe>::value);
    D_HT_CHECK(!tr::has_rcu_protected_type<plain_probe>::value);

    return true;
}

bool tests_traits_has_hazard_domain_type()
{
    D_HT_CHECK(tr::has_hazard_domain_type<DHazardProbe>::value);
    D_HT_CHECK(!tr::has_hazard_domain_type<plain_probe>::value);

    return true;
}

bool tests_traits_has_try_lock_method()
{
    D_HT_CHECK(tr::has_try_lock_method<DTryLockProbe>::value);
    D_HT_CHECK(!tr::has_try_lock_method<plain_probe>::value);

    return true;
}

bool tests_traits_has_atomic_load_store_cas()
{
    D_HT_CHECK(tr::has_atomic_load_method<atomic_probe>::value);
    D_HT_CHECK(tr::has_atomic_store_method<atomic_probe>::value);
    D_HT_CHECK(tr::has_atomic_compare_exchange_method<atomic_probe>::value);

    D_HT_CHECK(!tr::has_atomic_load_method<plain_probe>::value);
    D_HT_CHECK(!tr::has_atomic_store_method<plain_probe>::value);
    D_HT_CHECK(!tr::has_atomic_compare_exchange_method<plain_probe>::value);

    return true;
}

bool tests_traits_has_atomic_interface()
{
    D_HT_CHECK(tr::has_atomic_interface<atomic_probe>::value);
    D_HT_CHECK(tr::has_atomic_interface<DRichProbe>::value);
    D_HT_CHECK(!tr::has_atomic_interface<plain_probe>::value);
    // a type with only a strategy tag is not, by itself, an atomic interface
    D_HT_CHECK(!tr::has_atomic_interface<DStrategyProbe>::value);

    return true;
}

bool tests_traits_has_publish_and_cow_interface()
{
    D_HT_CHECK(tr::has_publish_method<DCowIfaceProbe>::value);
    D_HT_CHECK(tr::has_snapshot_method<DCowIfaceProbe>::value);
    D_HT_CHECK(tr::has_cow_interface<DCowIfaceProbe>::value);

    D_HT_CHECK(!tr::has_publish_method<plain_probe>::value);
    D_HT_CHECK(!tr::has_cow_interface<plain_probe>::value);

    return true;
}

bool tests_traits_full_lock_interface()
{
    D_HT_CHECK(tr::has_read_lock_method<DFullLockProbe>::value);
    D_HT_CHECK(tr::has_write_lock_method<DFullLockProbe>::value);
    D_HT_CHECK(tr::has_full_lock_interface<DFullLockProbe>::value);

    D_HT_CHECK(!tr::has_full_lock_interface<plain_probe>::value);
    // try_lock alone is not the full reader/writer interface
    D_HT_CHECK(!tr::has_full_lock_interface<DTryLockProbe>::value);

    return true;
}

bool tests_traits_is_rcu_testable()
{
    D_HT_CHECK(tr::is_rcu_testable<DRcuProbe>::value);
    D_HT_CHECK(!tr::is_rcu_testable<plain_probe>::value);
    D_HT_CHECK(!tr::is_rcu_testable<DHazardProbe>::value);

    return true;
}

bool tests_traits_is_hazard_testable()
{
    D_HT_CHECK(tr::is_hazard_testable<DHazardProbe>::value);
    D_HT_CHECK(!tr::is_hazard_testable<plain_probe>::value);
    D_HT_CHECK(!tr::is_hazard_testable<DRcuProbe>::value);

    return true;
}

bool tests_traits_is_cow_testable()
{
    // satisfied by the cow_state_type alias ...
    D_HT_CHECK(tr::is_cow_testable<DCowStateProbe>::value);
    // ... and by the snapshot+publish method interface
    D_HT_CHECK(tr::is_cow_testable<DCowIfaceProbe>::value);
    D_HT_CHECK(!tr::is_cow_testable<plain_probe>::value);

    return true;
}

bool tests_traits_is_locked_testable()
{
    // satisfied by the lock_policy_type alias ...
    D_HT_CHECK(tr::is_locked_testable<DLockPolicyProbe>::value);
    // ... and by the read_lock+write_lock interface
    D_HT_CHECK(tr::is_locked_testable<DFullLockProbe>::value);
    D_HT_CHECK(!tr::is_locked_testable<plain_probe>::value);

    return true;
}

bool tests_traits_is_lock_free_testable()
{
    D_HT_CHECK(tr::is_lock_free_testable<atomic_probe>::value);   // atomic iface
    D_HT_CHECK(tr::is_lock_free_testable<DRcuProbe>::value);      // rcu marker
    D_HT_CHECK(tr::is_lock_free_testable<DHazardProbe>::value);   // hazard marker
    D_HT_CHECK(!tr::is_lock_free_testable<plain_probe>::value);
    // a lock-policy type is lock-BASED, not lock-free
    D_HT_CHECK(!tr::is_lock_free_testable<DLockPolicyProbe>::value);

    return true;
}

bool tests_traits_is_threadsafe_testable()
{
    D_HT_CHECK(tr::is_threadsafe_testable<DLevelProbe>::value);
    D_HT_CHECK(tr::is_threadsafe_testable<DStrategyProbe>::value);
    D_HT_CHECK(tr::is_threadsafe_testable<DLockPolicyProbe>::value);
    D_HT_CHECK(tr::is_threadsafe_testable<DCowStateProbe>::value);
    D_HT_CHECK(tr::is_threadsafe_testable<DRcuProbe>::value);
    D_HT_CHECK(tr::is_threadsafe_testable<DHazardProbe>::value);
    D_HT_CHECK(tr::is_threadsafe_testable<atomic_probe>::value);
    D_HT_CHECK(tr::is_threadsafe_testable<DFullLockProbe>::value);

    D_HT_CHECK(!tr::is_threadsafe_testable<plain_probe>::value);

    return true;
}

bool tests_traits_thread_test_class_flags()
{
    typedef tr::thread_test_class<atomic_probe> atc;
    D_HT_CHECK(atc::has_load);
    D_HT_CHECK(atc::has_store);
    D_HT_CHECK(atc::has_compare_exchange);
    D_HT_CHECK(atc::has_atomic);
    D_HT_CHECK(atc::is_lock_free);
    D_HT_CHECK(atc::is_threadsafe);
    D_HT_CHECK(!atc::has_safety_level);
    D_HT_CHECK(!atc::is_locked);

    typedef tr::thread_test_class<DLockPolicyProbe> ltc;
    D_HT_CHECK(ltc::has_lock_policy);
    D_HT_CHECK(ltc::is_locked);
    D_HT_CHECK(ltc::is_threadsafe);
    D_HT_CHECK(!ltc::is_lock_free);
    D_HT_CHECK(!ltc::has_atomic);

    typedef tr::thread_test_class<plain_probe> ptc;
    D_HT_CHECK(!ptc::is_threadsafe);
    D_HT_CHECK(!ptc::has_atomic);
    D_HT_CHECK(!ptc::is_locked);
    D_HT_CHECK(!ptc::is_cow);
    D_HT_CHECK(!ptc::is_rcu);
    D_HT_CHECK(!ptc::is_hazard);

    return true;
}


NS_END  // testing
NS_END  // djinterp
