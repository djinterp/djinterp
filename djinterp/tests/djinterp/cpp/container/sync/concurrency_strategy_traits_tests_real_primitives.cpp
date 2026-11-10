/*******************************************************************************
* djinterp [test]          concurrency_strategy_traits_tests_real_primitives.cpp
*
*   Integration - the classification run against the actual updated
* /core/sync/ primitives (not synthetic fixtures).  Every primitive self-tags
* via concurrency_strategy_tag, so the tag fast-path is what these tests
* exercise; the last test pins the property that a primitive's incidental
* structure (DCowState's lock_policy_type, DRcuProtected's read_lock()) never
* overrides its tag.  Defines the "real_primitives" block.
*
*
* path:      /tests/djinterp/cpp/container/sync/concurrency_strategy_traits_tests_real_primitives.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/

// FLOOR, FOR NOW: below C++17, concurrency_strategy_traits.hpp's floor,
// this unit is empty, as that header is.
#include "../../../../../inc/djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#include "concurrency_strategy_traits_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_real_cow_state
//   Verifies DCowState<int> classifies as cow and carries the cow signature.
//   Tests the following:
//   - is_cow_container_v true; strategy == cow; not locked; not hybrid
//   - wait-free reads, lock-free writes, reference-count reclamation
bool tests_real_cow_state()
{
    D_CS_CHECK(is_cow_container_v<real_cow_t>);
    D_CS_CHECK(concurrency_strategy_of_v<real_cow_t> ==
               DConcurrencyStrategy::cow);
    D_CS_CHECK(!is_locked_container_v<real_cow_t>);
    D_CS_CHECK(concurrency_strategy_of_v<real_cow_t> !=
               DConcurrencyStrategy::hybrid);

    D_CS_CHECK(read_progress_of_v<real_cow_t> ==
               concurrency_progress::wait_free);
    D_CS_CHECK(write_progress_of_v<real_cow_t> ==
               concurrency_progress::lock_free);
    D_CS_CHECK(reclamation_obligation_of_v<real_cow_t> ==
               reclamation_obligation::reference_count);
    return true;
}

// tests_real_rcu_protected
//   Verifies DRcuProtected<int> classifies as rcu and carries the rcu
//   signature.
//   Tests the following:
//   - is_rcu_container_v true; strategy == rcu; not locked
//   - wait-free reads, grace-period reclamation
bool tests_real_rcu_protected()
{
    D_CS_CHECK(is_rcu_container_v<real_rcu_t>);
    D_CS_CHECK(concurrency_strategy_of_v<real_rcu_t> ==
               DConcurrencyStrategy::rcu);
    D_CS_CHECK(!is_locked_container_v<real_rcu_t>);

    D_CS_CHECK(read_progress_of_v<real_rcu_t> ==
               concurrency_progress::wait_free);
    D_CS_CHECK(reclamation_obligation_of_v<real_rcu_t> ==
               reclamation_obligation::grace_period);
    return true;
}

// tests_real_atomic_types
//   Verifies atomic_size and atomic_version classify as atomic and carry the
//   atomic signature.  (Their value_type is a plain integer, so they rely on
//   the tag, not the value-is-atomic structural probe.)
//   Tests the following:
//   - both are atomic; strategy == atomic; not hybrid
//   - lock-free both sides, mwmr, no reclamation obligation
bool tests_real_atomic_types()
{
    D_CS_CHECK(is_atomic_container_v<real_asize_t>);
    D_CS_CHECK(is_atomic_container_v<real_aver_t>);
    D_CS_CHECK(concurrency_strategy_of_v<real_asize_t> ==
               DConcurrencyStrategy::atomic);
    D_CS_CHECK(concurrency_strategy_of_v<real_aver_t> ==
               DConcurrencyStrategy::atomic);

    D_CS_CHECK(is_lock_free_container_v<real_asize_t>);
    D_CS_CHECK(concurrency_arity_of_v<real_asize_t> ==
               concurrency_arity::mwmr);
    D_CS_CHECK(reclamation_obligation_of_v<real_asize_t> ==
               reclamation_obligation::none);
    return true;
}

// tests_real_hazard_domains
//   Verifies hazard_domain and DMultiHazardDomain classify as hazard and
//   carry the hazard signature.
//   Tests the following:
//   - both are hazard; strategy == hazard
//   - lock-free, hazard reclamation obligation
bool tests_real_hazard_domains()
{
    D_CS_CHECK(is_hazard_container_v<real_haz_t>);
    D_CS_CHECK(is_hazard_container_v<real_mhaz_t>);
    D_CS_CHECK(concurrency_strategy_of_v<real_haz_t> ==
               DConcurrencyStrategy::hazard);
    D_CS_CHECK(concurrency_strategy_of_v<real_mhaz_t> ==
               DConcurrencyStrategy::hazard);

    D_CS_CHECK(is_lock_free_container_v<real_haz_t>);
    D_CS_CHECK(reclamation_obligation_of_v<real_haz_t> ==
               reclamation_obligation::hazard);
    return true;
}

// tests_real_tag_over_structure
//   Verifies the tag fast-path is not undermined by the primitives' incidental
//   structure - the exact false-positive the structural fallback is designed
//   to avoid.
//   Tests the following:
//   - DCowState exposes lock_policy_type yet is NOT locked and NOT hybrid
//   - DRcuProtected exposes a read_lock() yet is NOT locked
//   - each real primitive resolves to exactly one strategy, never hybrid
bool tests_real_tag_over_structure()
{
    // DCowState has a lock_policy_type but no read_lock()/write_lock(): the
    // locked AND-condition fails structurally AND the cow tag governs anyway.
    D_CS_CHECK(!is_locked_container_v<real_cow_t>);
    D_CS_CHECK(concurrency_strategy_of_v<real_cow_t> !=
               DConcurrencyStrategy::hybrid);

    // DRcuProtected has a read_lock() but no lock_policy_type: not locked.
    D_CS_CHECK(!is_locked_container_v<real_rcu_t>);

    // none of the real primitives is hybrid
    D_CS_CHECK(concurrency_strategy_of_v<real_asize_t> !=
               DConcurrencyStrategy::hybrid);
    D_CS_CHECK(concurrency_strategy_of_v<real_haz_t> !=
               DConcurrencyStrategy::hybrid);
    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
