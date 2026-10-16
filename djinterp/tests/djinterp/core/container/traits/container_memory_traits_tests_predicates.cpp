/*******************************************************************************
* djinterp [testing]                container_memory_traits_tests_predicates.cpp
*
*   Section III - the boolean projections of container_memory_discipline:
* container_has_inline_storage (none), container_uses_individual_heap
* (individual), container_uses_pool (pooled), container_uses_arena (arena),
* container_allocates (individual OR pooled OR arena), and
* container_memory_resolved (anything but unknown).  Each is exercised on a
* representative of every discipline so it is seen both true and false.
*
*
* path:      /tests/djinterp/core/container/traits/container_memory_traits_tests_predicates.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_memory_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_container_has_inline_storage
//   true only for the NONE discipline (the container allocates nothing).
bool
tests_container_has_inline_storage()
{
    bool _cm_ok = true;

    // none: inline
    D_CM_CHECK( (container_has_inline_storage<std::array<int, 4>>::value) );
    D_CM_CHECK( (container_has_inline_storage<extent_aggregate>::value) );
    D_CM_CHECK( (container_has_inline_storage<descriptive_via_strategy>::value) );

    // every other discipline is NOT inline
    D_CM_CHECK( (!container_has_inline_storage<std::vector<int>>::value) );        // individual
    D_CM_CHECK( (!container_has_inline_storage<pooled_via_strategy>::value) );     // pooled
    D_CM_CHECK( (!container_has_inline_storage<arena_via_strategy>::value) );      // arena
    D_CM_CHECK( (!container_has_inline_storage<int>::value) );                     // unknown

    return _cm_ok;
}


// tests_container_uses_individual_heap
//   true only for the INDIVIDUAL discipline.
bool
tests_container_uses_individual_heap()
{
    bool _cm_ok = true;

    // individual, reached by several distinct branches
    D_CM_CHECK( (container_uses_individual_heap<std::vector<int>>::value) );       // step 2 default
    D_CM_CHECK( (container_uses_individual_heap<std::map<int, int>>::value) );     // step 2 default
    D_CM_CHECK( (container_uses_individual_heap<reserve_only>::value) );           // step 4
    D_CM_CHECK( (container_uses_individual_heap<alloc_beats_static>::value) );     // step 2 precedence
    D_CM_CHECK( (container_uses_individual_heap<inconsistent_individual_static>::value) );  // step 1

    // not individual
    D_CM_CHECK( (!container_uses_individual_heap<std::array<int, 4>>::value) );    // none
    D_CM_CHECK( (!container_uses_individual_heap<pooled_via_strategy>::value) );   // pooled
    D_CM_CHECK( (!container_uses_individual_heap<arena_via_strategy>::value) );    // arena
    D_CM_CHECK( (!container_uses_individual_heap<int>::value) );                   // unknown

    return _cm_ok;
}


// tests_container_uses_pool
//   true only for the POOLED discipline.
bool
tests_container_uses_pool()
{
    bool _cm_ok = true;

    D_CM_CHECK( (container_uses_pool<pooled_via_strategy>::value) );    // step 1
    D_CM_CHECK( (container_uses_pool<pooled_via_allocator>::value) );   // step 2 declared

    D_CM_CHECK( (!container_uses_pool<std::vector<int>>::value) );      // individual
    D_CM_CHECK( (!container_uses_pool<std::array<int, 4>>::value) );    // none
    D_CM_CHECK( (!container_uses_pool<arena_via_strategy>::value) );    // arena
    D_CM_CHECK( (!container_uses_pool<int>::value) );                   // unknown

    return _cm_ok;
}


// tests_container_uses_arena
//   true only for the ARENA discipline.
bool
tests_container_uses_arena()
{
    bool _cm_ok = true;

    D_CM_CHECK( (container_uses_arena<arena_via_strategy>::value) );

    D_CM_CHECK( (!container_uses_arena<std::vector<int>>::value) );     // individual
    D_CM_CHECK( (!container_uses_arena<std::array<int, 4>>::value) );   // none
    D_CM_CHECK( (!container_uses_arena<pooled_via_strategy>::value) );  // pooled
    D_CM_CHECK( (!container_uses_arena<int>::value) );                  // unknown

    return _cm_ok;
}


// tests_container_allocates
//   true iff the discipline acquires storage at runtime (individual, pooled, arena).
bool
tests_container_allocates()
{
    bool _cm_ok = true;

    // acquires storage
    D_CM_CHECK( (container_allocates<std::vector<int>>::value) );       // individual
    D_CM_CHECK( (container_allocates<pooled_via_strategy>::value) );    // pooled
    D_CM_CHECK( (container_allocates<arena_via_strategy>::value) );     // arena
    D_CM_CHECK( (container_allocates<reserve_only>::value) );           // individual

    // allocates nothing
    D_CM_CHECK( (!container_allocates<std::array<int, 4>>::value) );    // none
    D_CM_CHECK( (!container_allocates<descriptive_via_strategy>::value) );  // none
    D_CM_CHECK( (!container_allocates<int>::value) );                   // unknown

    return _cm_ok;
}


// tests_container_memory_resolved
//   true iff the discipline was determined (not unknown).
bool
tests_container_memory_resolved()
{
    bool _cm_ok = true;

    // resolved
    D_CM_CHECK( (container_memory_resolved<std::vector<int>>::value) );      // individual
    D_CM_CHECK( (container_memory_resolved<std::array<int, 4>>::value) );    // none
    D_CM_CHECK( (container_memory_resolved<pooled_via_strategy>::value) );   // pooled
    D_CM_CHECK( (container_memory_resolved<arena_via_strategy>::value) );    // arena

    // unresolved
    D_CM_CHECK( (!container_memory_resolved<int>::value) );                  // unknown
    D_CM_CHECK( (!container_memory_resolved<alias_unknown_container>::value) );  // alias -> unknown

    return _cm_ok;
}


NS_END  // testing
NS_END  // djinterp
