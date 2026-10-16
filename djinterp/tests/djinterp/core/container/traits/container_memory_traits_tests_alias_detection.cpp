/*******************************************************************************
* djinterp [testing]           container_memory_traits_tests_alias_detection.cpp
*
*   Section I - has_memory_strategy_alias: detection of a container's explicit
* `using memory_strategy = S;` member, the highest-priority declaration of how
* it acquires storage.  Detected by presence of the nested alias, independent of
* what S resolves to (a non-strategy alias is still an alias); crucially,
* allocator_type is a DIFFERENT member and does not satisfy this trait.
*
*
* path:      /tests/djinterp/core/container/traits/container_memory_traits_tests_alias_detection.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_memory_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_has_memory_strategy_alias
//   presence of the memory_strategy alias, regardless of what it names.
bool
tests_has_memory_strategy_alias()
{
    bool _cm_ok = true;

    // types that declare a memory_strategy alias (whatever it resolves to)
    D_CM_CHECK( (has_memory_strategy_alias<pooled_via_strategy>::value) );
    D_CM_CHECK( (has_memory_strategy_alias<arena_via_strategy>::value) );
    D_CM_CHECK( (has_memory_strategy_alias<descriptive_via_strategy>::value) );
    D_CM_CHECK( (has_memory_strategy_alias<alias_unknown_container>::value) );      // alias names a non-strategy, still present
    D_CM_CHECK( (has_memory_strategy_alias<inconsistent_individual_static>::value) );

    // allocator_type is NOT a memory_strategy alias
    D_CM_CHECK( (!has_memory_strategy_alias<pooled_via_allocator>::value) );
    D_CM_CHECK( (!has_memory_strategy_alias<alloc_beats_static>::value) );

    // standard containers declare no memory_strategy alias
    D_CM_CHECK( (!has_memory_strategy_alias<std::vector<int>>::value) );
    D_CM_CHECK( (!has_memory_strategy_alias<std::array<int, 4>>::value) );
    D_CM_CHECK( (!has_memory_strategy_alias<std::set<int>>::value) );

    // other shapes, too
    D_CM_CHECK( (!has_memory_strategy_alias<reserve_only>::value) );
    D_CM_CHECK( (!has_memory_strategy_alias<extent_aggregate>::value) );
    D_CM_CHECK( (!has_memory_strategy_alias<int>::value) );

    return _cm_ok;
}


NS_END  // testing
NS_END  // djinterp
