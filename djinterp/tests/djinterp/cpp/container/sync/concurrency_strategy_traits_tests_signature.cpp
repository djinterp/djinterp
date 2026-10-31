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
bool tests_signature_none_locked()
{
    D_CS_CHECK(concurrency_signature_of_v<DTagNone>.read_progress ==
               concurrency_progress::sequential);
    D_CS_CHECK(concurrency_signature_of_v<DTagNone>.write_progress ==
               concurrency_progress::sequential);
    D_CS_CHECK(concurrency_signature_of_v<DTagNone>.arity ==
               concurrency_arity::swmr);
    D_CS_CHECK(concurrency_signature_of_v<DTagNone>.iteration ==
               iteration_consistency::inconsistent);
    D_CS_CHECK(concurrency_signature_of_v<DTagNone>.reclamation ==
               reclamation_obligation::none);

    D_CS_CHECK(concurrency_signature_of_v<DTagLocked>.read_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<DTagLocked>.write_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<DTagLocked>.arity ==
               concurrency_arity::mwmr);
    D_CS_CHECK(concurrency_signature_of_v<DTagLocked>.iteration ==
               iteration_consistency::snapshot);
    D_CS_CHECK(concurrency_signature_of_v<DTagLocked>.reclamation ==
               reclamation_obligation::individual);
    return true;
}

// tests_signature_cow_rcu
//   Verifies the cow and rcu rows in full - the two wait-free-reader
//   strategies, which differ on the writer side and on reclamation.
//   Tests the following:
//   - cow: wait_free / lock_free / swmr / snapshot / reference_count
//   - rcu: wait_free / blocking / swmr / weakly_consistent / grace_period
bool tests_signature_cow_rcu()
{
    D_CS_CHECK(concurrency_signature_of_v<DTagCow>.read_progress ==
               concurrency_progress::wait_free);
    D_CS_CHECK(concurrency_signature_of_v<DTagCow>.write_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<DTagCow>.arity ==
               concurrency_arity::swmr);
    D_CS_CHECK(concurrency_signature_of_v<DTagCow>.iteration ==
               iteration_consistency::snapshot);
    D_CS_CHECK(concurrency_signature_of_v<DTagCow>.reclamation ==
               reclamation_obligation::reference_count);

    D_CS_CHECK(concurrency_signature_of_v<DTagRcu>.read_progress ==
               concurrency_progress::wait_free);
    D_CS_CHECK(concurrency_signature_of_v<DTagRcu>.write_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<DTagRcu>.arity ==
               concurrency_arity::swmr);
    D_CS_CHECK(concurrency_signature_of_v<DTagRcu>.iteration ==
               iteration_consistency::weakly_consistent);
    D_CS_CHECK(concurrency_signature_of_v<DTagRcu>.reclamation ==
               reclamation_obligation::grace_period);
    return true;
}

// tests_signature_atomic_hazard
//   Verifies the atomic and hazard rows - both lock-free MWMR, differing
//   only on the reclamation obligation.
//   Tests the following:
//   - atomic: lock_free / lock_free / mwmr / weakly_consistent / none
//   - hazard: lock_free / lock_free / mwmr / weakly_consistent / hazard
bool tests_signature_atomic_hazard()
{
    D_CS_CHECK(concurrency_signature_of_v<DTagAtomic>.read_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<DTagAtomic>.write_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<DTagAtomic>.arity ==
               concurrency_arity::mwmr);
    D_CS_CHECK(concurrency_signature_of_v<DTagAtomic>.iteration ==
               iteration_consistency::weakly_consistent);
    D_CS_CHECK(concurrency_signature_of_v<DTagAtomic>.reclamation ==
               reclamation_obligation::none);

    D_CS_CHECK(concurrency_signature_of_v<DTagHazard>.read_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<DTagHazard>.write_progress ==
               concurrency_progress::lock_free);
    D_CS_CHECK(concurrency_signature_of_v<DTagHazard>.arity ==
               concurrency_arity::mwmr);
    D_CS_CHECK(concurrency_signature_of_v<DTagHazard>.iteration ==
               iteration_consistency::weakly_consistent);
    D_CS_CHECK(concurrency_signature_of_v<DTagHazard>.reclamation ==
               reclamation_obligation::hazard);
    return true;
}

// tests_signature_hybrid
//   Verifies the hybrid row - the conservative meet reported when no single
//   signature is exact.
//   Tests the following:
//   - hybrid: blocking / blocking / mwmr / weakly_consistent / individual
bool tests_signature_hybrid()
{
    D_CS_CHECK(concurrency_signature_of_v<DTagHybrid>.read_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<DTagHybrid>.write_progress ==
               concurrency_progress::blocking);
    D_CS_CHECK(concurrency_signature_of_v<DTagHybrid>.arity ==
               concurrency_arity::mwmr);
    D_CS_CHECK(concurrency_signature_of_v<DTagHybrid>.iteration ==
               iteration_consistency::weakly_consistent);
    D_CS_CHECK(concurrency_signature_of_v<DTagHybrid>.reclamation ==
               reclamation_obligation::individual);
    return true;
}

// tests_signature_projections
//   Verifies the five convenience projections read back the corresponding
//   field of concurrency_signature_of - the projection templates themselves.
//   Tests the following:
//   - read/write_progress_of, arity_of, iteration_of, reclamation_of == the
//     matching signature field, on representative strategies
bool tests_signature_projections()
{
    D_CS_CHECK(read_progress_of_v<DTagCow> ==
               concurrency_signature_of_v<DTagCow>.read_progress);
    D_CS_CHECK(write_progress_of_v<DTagCow> ==
               concurrency_signature_of_v<DTagCow>.write_progress);
    D_CS_CHECK(concurrency_arity_of_v<DTagAtomic> ==
               concurrency_signature_of_v<DTagAtomic>.arity);
    D_CS_CHECK(iteration_consistency_of_v<DTagLocked> ==
               concurrency_signature_of_v<DTagLocked>.iteration);
    D_CS_CHECK(reclamation_obligation_of_v<DTagHazard> ==
               concurrency_signature_of_v<DTagHazard>.reclamation);

    // and a couple of concrete projected values, for good measure
    D_CS_CHECK(read_progress_of_v<DTagRcu> ==
               concurrency_progress::wait_free);
    D_CS_CHECK(reclamation_obligation_of_v<DTagCow> ==
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
bool tests_signature_shorthands()
{
    // both sides at least lock-free
    D_CS_CHECK(is_lock_free_container_v<DTagCow>);
    D_CS_CHECK(is_lock_free_container_v<DTagAtomic>);
    D_CS_CHECK(is_lock_free_container_v<DTagHazard>);
    D_CS_CHECK(!is_lock_free_container_v<DTagRcu>);
    D_CS_CHECK(!is_lock_free_container_v<DTagLocked>);
    D_CS_CHECK(!is_lock_free_container_v<DTagNone>);
    D_CS_CHECK(!is_lock_free_container_v<DTagHybrid>);

    // wait-free readers
    D_CS_CHECK(is_wait_free_reader_container_v<DTagCow>);
    D_CS_CHECK(is_wait_free_reader_container_v<DTagRcu>);
    D_CS_CHECK(!is_wait_free_reader_container_v<DTagAtomic>);
    D_CS_CHECK(!is_wait_free_reader_container_v<DTagLocked>);
    D_CS_CHECK(!is_wait_free_reader_container_v<DTagNone>);

    // snapshot iteration
    D_CS_CHECK(offers_snapshot_iteration_v<DTagLocked>);
    D_CS_CHECK(offers_snapshot_iteration_v<DTagCow>);
    D_CS_CHECK(!offers_snapshot_iteration_v<DTagRcu>);
    D_CS_CHECK(!offers_snapshot_iteration_v<DTagAtomic>);
    D_CS_CHECK(!offers_snapshot_iteration_v<DTagNone>);
    return true;
}

NS_END  // testing
NS_END  // djinterp
