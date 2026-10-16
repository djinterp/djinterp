/*******************************************************************************
* djinterp [testing]               container_memory_traits_tests_consistency.cpp
*
*   Section IV - container_memory_consistent, the cross-check between a
* container's resolved DISCIPLINE and its independently classified SITING (from
* container_storage_traits.hpp) under discipline_siting_consistent.  The bridge
* holds when any of three clauses is met:
*     (a) the discipline forces no siting  (arena, unknown);
*     (b) the siting is undetermined       (unknown storage);
*     (c) the siting carries the component the discipline forces (none -> a static
*         part; individual / pooled -> a dynamic part).
*   All three are exercised, and so is the ONE failure the trait exists to catch:
* an individual-heap discipline (which forces DYNAMIC siting) declared on a type
* whose storage is purely static_storage - a contradiction that satisfies no
* clause, so the trait reports false.
*
*
* path:      /tests/djinterp/core/container/traits/container_memory_traits_tests_consistency.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_memory_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_container_memory_consistent
//   the discipline<->siting bridge across all three holding clauses and the one
//   contradiction.
bool
tests_container_memory_consistent()
{
    bool _cm_ok = true;

    // -- clause (a): the discipline forces no siting --
    D_CM_CHECK( (container_memory_consistent<arena_via_strategy>::value) );   // arena implies nothing
    D_CM_CHECK( (container_memory_consistent<int>::value) );                  // unknown discipline

    // -- clause (b): the siting is undetermined (no storage signal) --
    D_CM_CHECK( (container_memory_consistent<pooled_via_strategy>::value) );      // pooled, siting unknown
    D_CM_CHECK( (container_memory_consistent<descriptive_via_strategy>::value) ); // none, siting unknown

    // -- clause (c): the siting carries the forced component --
    D_CM_CHECK( (container_memory_consistent<std::vector<int>>::value) );    // individual + dynamic
    D_CM_CHECK( (container_memory_consistent<std::map<int, int>>::value) );  // individual + dynamic
    D_CM_CHECK( (container_memory_consistent<reserve_only>::value) );        // individual + dynamic
    D_CM_CHECK( (container_memory_consistent<std::array<int, 4>>::value) );  // none + static
    D_CM_CHECK( (container_memory_consistent<extent_aggregate>::value) );    // none + static
    D_CM_CHECK( (container_memory_consistent<pooled_via_allocator>::value) );// pooled + dynamic
    // hybrid siting carries a dynamic component, so an individual discipline is consistent
    D_CM_CHECK( (container_memory_consistent<alloc_beats_static>::value) );  // individual + hybrid

    // -- the contradiction: individual discipline (forces dynamic) over static-only storage --
    D_CM_CHECK( (!container_memory_consistent<inconsistent_individual_static>::value) );

    return _cm_ok;
}


NS_END  // testing
NS_END  // djinterp
