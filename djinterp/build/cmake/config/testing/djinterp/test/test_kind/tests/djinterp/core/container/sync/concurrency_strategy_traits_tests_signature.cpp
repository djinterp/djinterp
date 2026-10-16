/******************************************************************************
* djinterp [test]           concurrency_strategy_traits_tests_signature.cpp
*
*   Section VI - the strategy -> signature table, the convenience projections,
* and the progress-grade shorthands.  The seven tag fixtures select the seven
* rows cleanly.  Defines the "signature" block.
******************************************************************************/

#include "concurrency_strategy_traits_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_signature_none_locked
//   Verifies the none and locked rows of the signature table in full.
//   Tests the following:
//   - none: sequential / sequential / swmr / inconsistent / none
//   - locked: blocking / blocking / mwmr / snapshot / individual
bool
tests_signature_none_locked()
{
    D_CS_CHECK(concurrency_signature_of_v<tag_none>.read_progress ==
               concurrency_progress::sequential);
    D_CS_CHECK(concurrency_signature_of_v<tag_none>.write_progress ==
               concurrency_progress::sequential);
    D_CS_CHECK(concurrency_signature_of_v<tag_none>.arity ==
               concurrency_arity::swmr);
    D_CS_CHECK(concurrency_signature_of_v<tag_none>.iteration ==
               iteration_consistency::inconsistent);
    D_CS_CHECK(concurrency_signature_of_v<tag_none>.reclamation ==
               reclamation_obligation::none);

    D_CS_CHECK(concurrency_signature_of_v<tag_locked>.read_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<tag_locked>.write_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<tag_locked>.arity ==
               concurrency_arity::mwmr);
    D_CS_CHECK(concurrency_signature_of_v<tag_locked>.iteration ==
               iteration_consistency::snapshot);
    D_CS_CHECK(concurrency_signature_of_v<tag_locked>.reclamation ==
               reclamation_obligation::individual);
    return true;
}

// tests_signature_cow_rcu
//   Verifies the cow and rcu rows in full - the two wait-free-reader
//   strategies, which differ on the writer side and on reclamation.
//   Tests the following:
//   - cow: wait_free / lock_free / swmr / snapshot / reference_count
//   - rcu: wait_free / blocking / swmr / weakly_consistent / grace_period
bool
tests_signature_cow_rcu()
{
    D_CS_CHECK(concurrency_signature_of_v<tag_cow>.read_progress ==
               concurrency_progress::wait_free);
    D_CS_CHECK(concurrency_signature_of_v<tag_cow>.write_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<tag_cow>.arity ==
               concurrency_arity::swmr);
    D_CS_CHECK(concurrency_signature_of_v<tag_cow>.iteration ==
               iteration_consistency::snapshot);
    D_CS_CHECK(concurrency_signature_of_v<tag_cow>.reclamation ==
               reclamation_obligation::reference_count);

    D_CS_CHECK(concurrency_signature_of_v<tag_rcu>.read_progress ==
               concurrency_progress::wait_free);
    D_CS_CHECK(concurrency_signature_of_v<tag_rcu>.write_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<tag_rcu>.arity ==
               concurrency_arity::swmr);
    D_CS_CHECK(concurrency_signature_of_v<tag_rcu>.iteration ==
               iteration_consistency::weakly_consistent);
    D_CS_CHECK(concurrency_signature_of_v<tag_rcu>.reclamation ==
               reclamation_obligation::grace_period);
    return true;
}

// tests_signature_atomic_hazard
//   Verifies the atomic and hazard rows - both lock-free MWMR, differing
//   only on the reclamation obligation.
//   Tests the following:
//   - atomic: lock_free / lock_free / mwmr / weakly_consistent / none
//   - hazard: lock_free / lock_free / mwmr / weakly_consistent / hazard
bool
tests_signature_atomic_hazard()
{
    D_CS_CHECK(concurrency_signature_of_v<tag_atomic>.read_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<tag_atomic>.write_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<tag_atomic>.arity ==
               concurrency_arity::mwmr);
    D_CS_CHECK(concurrency_signature_of_v<tag_atomic>.iteration ==
               iteration_consistency::weakly_consistent);
    D_CS_CHECK(concurrency_signature_of_v<tag_atomic>.reclamation ==
               reclamation_obligation::none);

    D_CS_CHECK(concurrency_signature_of_v<tag_hazard>.read_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<tag_hazard>.write_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<tag_hazard>.arity ==
               concurrency_arity::mwmr);
    D_CS_CHECK(concurrency_signature_of_v<tag_hazard>.iteration ==
               iteration_consistency::weakly_consistent);
    D_CS_CHECK(concurrency_signature_of_v<tag_hazard>.reclamation ==
               reclamation_obligation::hazard);
    return true;
}

// tests_signature_hybrid
//   Verifies the hybrid row - the conservative meet reported when no single
//   signature is exact.
//   Tests the following:
//   - hybrid: blocking / blocking / mwmr / weakly_consistent / individual
bool
tests_signature_hybrid()
{
    D_CS_CHECK(concurrency_signature_of_v<tag_hybrid>.read_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<tag_hybrid>.write_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<tag_hybrid>.arity ==
               concurrency_arity::mwmr);
    D_CS_CHECK(concurrency_signature_of_v<tag_hybrid>.iteration ==
               iteration_consistency::weakly_consistent);
    D_CS_CHECK(concurrency_signature_of_v<tag_hybrid>.reclamation ==
               reclamation_obligation::individual);
    return true;
}

// tests_signature_projections
//   Verifies the five convenience projections read back the corresponding
//   field of concurrency_signature_of - the projection templates themselves.
//   Tests the following:
//   - read/write_progress_of, arity_of, iteration_of, reclamation_of == the
//     matching signature field, on representative strategies
bool
tests_signature_projections()
{
    D_CS_CHECK(read_progress_of_v<tag_cow> ==
               concurrency_signature_of_v<tag_cow>.read_progress);
    D_CS_CHECK(write_progress_of_v<tag_cow> ==
               concurrency_signature_of_v<tag_cow>.write_progress);
    D_CS_CHECK(concurrency_arity_of_v<tag_atomic> ==
               concurrency_signature_of_v<tag_atomic>.arity);
    D_CS_CHECK(iteration_consistency_of_v<tag_locked> ==
               concurrency_signature_of_v<tag_locked>.iteration);
    D_CS_CHECK(reclamation_obligation_of_v<tag_hazard> ==
               concurrency_signature_of_v<tag_hazard>.reclamation);

    // and a couple of concrete projected values, for good measure
    D_CS_CHECK(read_progress_of_v<tag_rcu> ==
               concurrency_progress::wait_free);
    D_CS_CHECK(reclamation_obligation_of_v<tag_cow> ==
               reclamation_obligation::reference_count);
    return true;
}

// tests_signature_shorthands
//   Verifies the three progress-grade shorthands across every strategy.
//   Tests the following:
//   - is_lock_free_container_v: true for cow, atomic, hazard; false otherwise
//     (rcu is excluded - its writer side is blocking)
//   - is_wait_free_reader_container_v: true for cow and rcu only
//   - offers_snapshot_iteration_v: true for locked and cow only
bool
tests_signature_shorthands()
{
    // both sides at least lock-free
    D_CS_CHECK(is_lock_free_container_v<tag_cow>);
    D_CS_CHECK(is_lock_free_container_v<tag_atomic>);
    D_CS_CHECK(is_lock_free_container_v<tag_hazard>);
    D_CS_CHECK(!is_lock_free_container_v<tag_rcu>);
    D_CS_CHECK(!is_lock_free_container_v<tag_locked>);
    D_CS_CHECK(!is_lock_free_container_v<tag_none>);
    D_CS_CHECK(!is_lock_free_container_v<tag_hybrid>);

    // wait-free readers
    D_CS_CHECK(is_wait_free_reader_container_v<tag_cow>);
    D_CS_CHECK(is_wait_free_reader_container_v<tag_rcu>);
    D_CS_CHECK(!is_wait_free_reader_container_v<tag_atomic>);
    D_CS_CHECK(!is_wait_free_reader_container_v<tag_locked>);
    D_CS_CHECK(!is_wait_free_reader_container_v<tag_none>);

    // snapshot iteration
    D_CS_CHECK(offers_snapshot_iteration_v<tag_locked>);
    D_CS_CHECK(offers_snapshot_iteration_v<tag_cow>);
    D_CS_CHECK(!offers_snapshot_iteration_v<tag_rcu>);
    D_CS_CHECK(!offers_snapshot_iteration_v<tag_atomic>);
    D_CS_CHECK(!offers_snapshot_iteration_v<tag_none>);
    return true;
}

NS_END  // testing
NS_END  // djinterp
