/******************************************************************************
* djinterp [test]      concurrency_strategy_traits_tests_classification.cpp
*
*   Section VII - container_concurrency_class, the aggregate that gathers the
* strategy predicates, the monograph signature, and the forwarded safety level
* into one record.  Each member is checked against the standalone trait it
* mirrors.  Defines the "classification" block.
******************************************************************************/

#include "concurrency_strategy_traits_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_classification_strategy_fields
//   Verifies the five strategy booleans and the strategy enum agree with the
//   standalone predicates.
//   Tests the following:
//   - is_locked / is_cow / is_rcu / is_atomic / is_hazard == the predicates
//   - strategy == concurrency_strategy_of_v, on representative inputs
bool
tests_classification_strategy_fields()
{
    using cow_c = container_concurrency_class<tag_cow>;
    D_CS_CHECK(cow_c::is_cow    == is_cow_container_v<tag_cow>);
    D_CS_CHECK(cow_c::is_locked == is_locked_container_v<tag_cow>);
    D_CS_CHECK(cow_c::strategy  == concurrency_strategy_of_v<tag_cow>);

    using lock_c = container_concurrency_class<st_locked_rw>;
    D_CS_CHECK(lock_c::is_locked == is_locked_container_v<st_locked_rw>);
    D_CS_CHECK(lock_c::is_cow    == is_cow_container_v<st_locked_rw>);
    D_CS_CHECK(lock_c::strategy  == concurrency_strategy_of_v<st_locked_rw>);

    using haz_c = container_concurrency_class<st_hazard>;
    D_CS_CHECK(haz_c::is_hazard == is_hazard_container_v<st_hazard>);
    D_CS_CHECK(haz_c::is_atomic == is_atomic_container_v<st_hazard>);
    D_CS_CHECK(haz_c::strategy  == concurrency_strategy::hazard);
    return true;
}

// tests_classification_aggregate_fields
//   Verifies the concurrency-axis booleans forward the standalone traits.
//   Tests the following:
//   - is_concurrent / is_synchronized / is_vacuously_concurrent /
//     is_sequential == the matching traits, on strategy, empty, and immutable
//     inputs
bool
tests_classification_aggregate_fields()
{
    using cow_c = container_concurrency_class<tag_cow>;
    D_CS_CHECK(cow_c::is_concurrent   == is_concurrent_container_v<tag_cow>);
    D_CS_CHECK(cow_c::is_synchronized == is_synchronized_container_v<tag_cow>);
    D_CS_CHECK(cow_c::is_sequential   == is_sequential_container_v<tag_cow>);

    using plain_c = container_concurrency_class<st_plain>;
    D_CS_CHECK(plain_c::is_synchronized ==
               is_synchronized_container_v<st_plain>);
    D_CS_CHECK(plain_c::is_sequential == is_sequential_container_v<st_plain>);

    using imm_c = container_concurrency_class<immutable_probe>;
    D_CS_CHECK(imm_c::is_vacuously_concurrent ==
               is_vacuously_concurrent_v<immutable_probe>);
    D_CS_CHECK(imm_c::is_concurrent ==
               is_concurrent_container_v<immutable_probe>);
    return true;
}

// tests_classification_signature_fields
//   Verifies the aggregate's signature and its five flattened projections
//   equal concurrency_signature_of.
//   Tests the following:
//   - read_progress / write_progress / arity / iteration / reclamation match
//     the projections, and signature.* matches concurrency_signature_of_v.*
bool
tests_classification_signature_fields()
{
    using rcu_c = container_concurrency_class<tag_rcu>;
    D_CS_CHECK(rcu_c::read_progress  == read_progress_of_v<tag_rcu>);
    D_CS_CHECK(rcu_c::write_progress == write_progress_of_v<tag_rcu>);
    D_CS_CHECK(rcu_c::arity          == concurrency_arity_of_v<tag_rcu>);
    D_CS_CHECK(rcu_c::iteration      == iteration_consistency_of_v<tag_rcu>);
    D_CS_CHECK(rcu_c::reclamation    == reclamation_obligation_of_v<tag_rcu>);

    D_CS_CHECK(rcu_c::signature.read_progress ==
               concurrency_signature_of_v<tag_rcu>.read_progress);
    D_CS_CHECK(rcu_c::signature.reclamation ==
               concurrency_signature_of_v<tag_rcu>.reclamation);

    // a concrete cross-check on the atomic row
    using at_c = container_concurrency_class<tag_atomic>;
    D_CS_CHECK(at_c::write_progress == concurrency_progress::lock_free);
    D_CS_CHECK(at_c::reclamation    == reclamation_obligation::none);
    return true;
}

// tests_classification_level_forwarding
//   Verifies the two members forwarded from the threadsafe container traits.
//   Tests the following:
//   - level == container_thread_safety_level_v
//   - concurrent_reads == supports_concurrent_reads_v
bool
tests_classification_level_forwarding()
{
    using cow_c = container_concurrency_class<tag_cow>;
    D_CS_CHECK(cow_c::level == container_thread_safety_level_v<tag_cow>);
    D_CS_CHECK(cow_c::concurrent_reads ==
               supports_concurrent_reads_v<tag_cow>);

    using plain_c = container_concurrency_class<st_plain>;
    D_CS_CHECK(plain_c::level == container_thread_safety_level_v<st_plain>);
    D_CS_CHECK(plain_c::concurrent_reads ==
               supports_concurrent_reads_v<st_plain>);
    return true;
}

NS_END  // testing
NS_END  // djinterp
