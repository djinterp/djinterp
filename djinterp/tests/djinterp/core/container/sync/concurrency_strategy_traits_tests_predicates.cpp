/******************************************************************************
* djinterp [test]         concurrency_strategy_traits_tests_predicates.cpp
*
*   Section IV - the five per-strategy predicates.  Each is checked on its tag
* path, on its structural path, and (where it matters) on the boundary where a
* related-but-different structure must NOT match.  Precedence gets its own
* test.  Defines the "predicates" block.
******************************************************************************/

#include "concurrency_strategy_traits_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_predicates_locked
//   Verifies is_locked_container and its is_mutex_container synonym.  Locked-
//   by-structure needs a lock policy AND a lock method, so neither half alone
//   suffices.
//   Tests the following:
//   - the locked tag classifies as locked
//   - policy + read_lock(), policy + write_lock() classify as locked
//   - policy alone, or a lock method alone, do NOT
//   - is_mutex_container_v agrees with is_locked_container_v
bool
tests_predicates_locked()
{
    D_CS_CHECK(is_locked_container_v<tag_locked>);
    D_CS_CHECK(is_locked_container_v<st_locked_rw>);
    D_CS_CHECK(is_locked_container_v<st_locked_read_only>);
    D_CS_CHECK(is_locked_container_v<st_locked_write_only>);

    D_CS_CHECK(!is_locked_container_v<st_policy_no_method>);
    D_CS_CHECK(!is_locked_container_v<st_method_no_policy>);
    D_CS_CHECK(!is_locked_container_v<st_plain>);
    D_CS_CHECK(!is_locked_container_v<tag_cow>);

    // the mutex synonym is the same trait under another spelling
    D_CS_CHECK(is_mutex_container_v<st_locked_rw>);
    D_CS_CHECK(is_mutex_container_v<st_locked_rw> ==
               is_locked_container_v<st_locked_rw>);
    D_CS_CHECK(is_mutex_container_v<st_plain> ==
               is_locked_container_v<st_plain>);
    return true;
}

// tests_predicates_cow
//   Verifies is_cow_container over its tag and both structural arms.
//   Tests the following:
//   - the cow tag classifies as cow
//   - a cow_state_type alias, or a snapshot() method, classifies as cow
//   - unrelated structure and the empty type do NOT
bool
tests_predicates_cow()
{
    D_CS_CHECK(is_cow_container_v<tag_cow>);
    D_CS_CHECK(is_cow_container_v<st_cow_state>);
    D_CS_CHECK(is_cow_container_v<st_cow_snapshot>);

    D_CS_CHECK(!is_cow_container_v<st_plain>);
    D_CS_CHECK(!is_cow_container_v<st_locked_rw>);
    D_CS_CHECK(!is_cow_container_v<tag_rcu>);
    return true;
}

// tests_predicates_rcu
//   Verifies is_rcu_container over its tag and both structural arms.
//   Tests the following:
//   - the rcu tag classifies as rcu
//   - an rcu_protected_type alias, or an epoch_counter_type alias, classifies
//     as rcu
//   - unrelated structure and the empty type do NOT
bool
tests_predicates_rcu()
{
    D_CS_CHECK(is_rcu_container_v<tag_rcu>);
    D_CS_CHECK(is_rcu_container_v<st_rcu_protected>);
    D_CS_CHECK(is_rcu_container_v<st_rcu_epoch>);

    D_CS_CHECK(!is_rcu_container_v<st_plain>);
    D_CS_CHECK(!is_rcu_container_v<st_cow_state>);
    D_CS_CHECK(!is_rcu_container_v<tag_atomic>);
    return true;
}

// tests_predicates_atomic
//   Verifies is_atomic_container over its tag and both structural arms.
//   Tests the following:
//   - the atomic tag classifies as atomic
//   - an atomic value_type, or a load(size_t) member, classifies as atomic
//   - a plain value_type and the empty type do NOT
bool
tests_predicates_atomic()
{
    D_CS_CHECK(is_atomic_container_v<tag_atomic>);
    D_CS_CHECK(is_atomic_container_v<st_atomic_value>);
    D_CS_CHECK(is_atomic_container_v<st_atomic_loadat>);

    D_CS_CHECK(!is_atomic_container_v<st_value_plain>);
    D_CS_CHECK(!is_atomic_container_v<st_plain>);
    D_CS_CHECK(!is_atomic_container_v<tag_hazard>);
    return true;
}

// tests_predicates_hazard
//   Verifies is_hazard_container over its tag and structural arm.
//   Tests the following:
//   - the hazard tag classifies as hazard
//   - a hazard_domain_type alias classifies as hazard
//   - unrelated structure and the empty type do NOT
bool
tests_predicates_hazard()
{
    D_CS_CHECK(is_hazard_container_v<tag_hazard>);
    D_CS_CHECK(is_hazard_container_v<st_hazard>);

    D_CS_CHECK(!is_hazard_container_v<st_plain>);
    D_CS_CHECK(!is_hazard_container_v<st_cow_state>);
    D_CS_CHECK(!is_hazard_container_v<tag_locked>);
    return true;
}

// tests_predicates_tag_precedence
//   Verifies that a declared tag is authoritative over incidental structure,
//   in both directions, and that an explicit none tag suppresses the
//   structural read entirely.
//   Tests the following:
//   - cow tag + full locked structure -> cow, and NOT locked
//   - locked tag + cow structure -> locked, and NOT cow
//   - none tag + cow structure -> neither cow nor locked (tag suppresses)
bool
tests_predicates_tag_precedence()
{
    // cow tag wins over locked structure
    D_CS_CHECK(is_cow_container_v<pr_cowtag_lockstruct>);
    D_CS_CHECK(!is_locked_container_v<pr_cowtag_lockstruct>);

    // locked tag wins over cow structure
    D_CS_CHECK(is_locked_container_v<pr_locktag_cowstruct>);
    D_CS_CHECK(!is_cow_container_v<pr_locktag_cowstruct>);

    // an explicit none tag suppresses the cow structure
    D_CS_CHECK(!is_cow_container_v<pr_nonetag_cowstruct>);
    D_CS_CHECK(!is_locked_container_v<pr_nonetag_cowstruct>);
    return true;
}

NS_END  // testing
NS_END  // djinterp
