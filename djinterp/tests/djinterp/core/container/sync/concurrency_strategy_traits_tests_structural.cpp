/******************************************************************************
* djinterp [test]         concurrency_strategy_traits_tests_structural.cpp
*
*   Section III - the structural member detectors used as the tag-less
* fallback.  Each detector is exercised on a type that has the member and one
* that lacks it.  Defines the "structural" block.
******************************************************************************/

#include "concurrency_strategy_traits_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_structural_locked_members
//   Verifies the lock-method detectors, which are probed on a const reference
//   (so a non-const-qualified read_lock() would not be seen).
//   Tests the following:
//   - has_read_lock_method_v true for a const read_lock(), false otherwise
//   - has_write_lock_method_v true for write_lock(), false otherwise
bool
tests_structural_locked_members()
{
    D_CS_CHECK(has_read_lock_method_v<st_locked_rw>);
    D_CS_CHECK(has_read_lock_method_v<st_locked_read_only>);
    D_CS_CHECK(has_read_lock_method_v<st_method_no_policy>);
    D_CS_CHECK(!has_read_lock_method_v<st_locked_write_only>);
    D_CS_CHECK(!has_read_lock_method_v<st_plain>);

    D_CS_CHECK(has_write_lock_method_v<st_locked_rw>);
    D_CS_CHECK(has_write_lock_method_v<st_locked_write_only>);
    D_CS_CHECK(!has_write_lock_method_v<st_locked_read_only>);
    D_CS_CHECK(!has_write_lock_method_v<st_plain>);
    return true;
}

// tests_structural_cow_members
//   Verifies the copy-on-write structural markers.
//   Tests the following:
//   - has_cow_state_type_v true for the cow_state_type alias, false otherwise
//   - has_snapshot_method_v true for a const snapshot(), false otherwise
bool
tests_structural_cow_members()
{
    D_CS_CHECK(has_cow_state_type_v<st_cow_state>);
    D_CS_CHECK(!has_cow_state_type_v<st_cow_snapshot>);
    D_CS_CHECK(!has_cow_state_type_v<st_plain>);

    D_CS_CHECK(has_snapshot_method_v<st_cow_snapshot>);
    D_CS_CHECK(!has_snapshot_method_v<st_cow_state>);
    D_CS_CHECK(!has_snapshot_method_v<st_plain>);
    return true;
}

// tests_structural_rcu_members
//   Verifies the RCU / epoch structural markers.
//   Tests the following:
//   - has_rcu_protected_type_v true for the rcu_protected_type alias
//   - has_epoch_type_v true for the epoch_counter_type alias
//   - both false for the empty type
bool
tests_structural_rcu_members()
{
    D_CS_CHECK(has_rcu_protected_type_v<st_rcu_protected>);
    D_CS_CHECK(!has_rcu_protected_type_v<st_rcu_epoch>);
    D_CS_CHECK(!has_rcu_protected_type_v<st_plain>);

    D_CS_CHECK(has_epoch_type_v<st_rcu_epoch>);
    D_CS_CHECK(!has_epoch_type_v<st_rcu_protected>);
    D_CS_CHECK(!has_epoch_type_v<st_plain>);
    return true;
}

// tests_structural_atomic_members
//   Verifies the two atomic structural markers.  The value_type-is-atomic
//   probe requires a value_type whose type exposes load(memory_order); the
//   indexed-load probe requires load(size_t) on the container itself.
//   Tests the following:
//   - is_atomic_container_v true for an atomic value_type (value probe)
//   - is_atomic_container_v true for a load(size_t) member (indexed probe)
//   - a plain (non-atomic) value_type is not atomic
//   - has_atomic_load_at_v tracks the load(size_t) member specifically
bool
tests_structural_atomic_members()
{
    // value_type-is-atomic path (probe reached via the predicate)
    D_CS_CHECK(is_atomic_container_v<st_atomic_value>);
    D_CS_CHECK(!is_atomic_container_v<st_value_plain>);

    // indexed-load path
    D_CS_CHECK(has_atomic_load_at_v<st_atomic_loadat>);
    D_CS_CHECK(!has_atomic_load_at_v<st_atomic_value>);
    D_CS_CHECK(!has_atomic_load_at_v<st_plain>);
    D_CS_CHECK(is_atomic_container_v<st_atomic_loadat>);
    return true;
}

// tests_structural_hazard_member
//   Verifies the hazard-domain structural marker.
//   Tests the following:
//   - has_hazard_domain_type_v true for the hazard_domain_type alias, false
//     otherwise
bool
tests_structural_hazard_member()
{
    D_CS_CHECK(has_hazard_domain_type_v<st_hazard>);
    D_CS_CHECK(!has_hazard_domain_type_v<st_plain>);
    D_CS_CHECK(!has_hazard_domain_type_v<st_cow_state>);
    return true;
}

NS_END  // testing
NS_END  // djinterp
