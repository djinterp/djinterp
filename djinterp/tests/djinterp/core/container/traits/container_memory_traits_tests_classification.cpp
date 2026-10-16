/*******************************************************************************
* djinterp [testing]            container_memory_traits_tests_classification.cpp
*
*   Section V - container_memory_class, the assembled memory snapshot (resolved
* discipline and its name, the discipline predicates, the paired siting and its
* name, and the cross-axis consistency bit), checked member-for-member against
* the standalone traits across a representative of every discipline - plus a
* hybrid-siting type (to spell "hybrid_storage") and the inconsistent type (to
* see siting_consistent go false in the aggregate).  The `_v` variable-template
* companions of the section-III/IV bool traits are verified here too, gated
* exactly as the header emits them.
*
*
* path:      /tests/djinterp/core/container/traits/container_memory_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_memory_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_container_memory_class
//   the assembled snapshot, one representative per discipline (and per siting).
bool
tests_container_memory_class()
{
    bool _cm_ok = true;

    // -- vector<int>: individual discipline, dynamic siting --
    {
        using C = container_memory_class<std::vector<int>>;
        D_CM_CHECK( (C::discipline == memory_discipline::individual) );
        D_CM_CHECK( (cm_cstr_eq(C::discipline_name, "individual")) );
        D_CM_CHECK( (!C::is_none) );
        D_CM_CHECK( (C::is_individual) );
        D_CM_CHECK( (!C::is_pooled) );
        D_CM_CHECK( (!C::is_arena) );
        D_CM_CHECK( (C::allocates) );
        D_CM_CHECK( (C::resolved) );
        D_CM_CHECK( (C::siting == storage_duration::dynamic_storage) );
        D_CM_CHECK( (cm_cstr_eq(C::siting_name, "dynamic_storage")) );
        D_CM_CHECK( (C::siting_consistent) );
    }

    // -- array<int,4>: none discipline, static siting --
    {
        using C = container_memory_class<std::array<int, 4>>;
        D_CM_CHECK( (C::discipline == memory_discipline::none) );
        D_CM_CHECK( (cm_cstr_eq(C::discipline_name, "none")) );
        D_CM_CHECK( (C::is_none) );
        D_CM_CHECK( (!C::is_individual) );
        D_CM_CHECK( (!C::allocates) );
        D_CM_CHECK( (C::resolved) );
        D_CM_CHECK( (C::siting == storage_duration::static_storage) );
        D_CM_CHECK( (cm_cstr_eq(C::siting_name, "static_storage")) );
        D_CM_CHECK( (C::siting_consistent) );
    }

    // -- pooled_via_strategy: pooled discipline, undetermined siting --
    {
        using C = container_memory_class<pooled_via_strategy>;
        D_CM_CHECK( (C::discipline == memory_discipline::pooled) );
        D_CM_CHECK( (cm_cstr_eq(C::discipline_name, "pooled")) );
        D_CM_CHECK( (C::is_pooled) );
        D_CM_CHECK( (!C::is_individual) );
        D_CM_CHECK( (C::allocates) );
        D_CM_CHECK( (C::resolved) );
        D_CM_CHECK( (C::siting == storage_duration::unknown) );
        D_CM_CHECK( (cm_cstr_eq(C::siting_name, "unknown")) );
        D_CM_CHECK( (C::siting_consistent) );          // holds via the unknown-siting clause
    }

    // -- arena_via_strategy: arena discipline --
    {
        using C = container_memory_class<arena_via_strategy>;
        D_CM_CHECK( (C::discipline == memory_discipline::arena) );
        D_CM_CHECK( (cm_cstr_eq(C::discipline_name, "arena")) );
        D_CM_CHECK( (C::is_arena) );
        D_CM_CHECK( (C::allocates) );
        D_CM_CHECK( (C::resolved) );
        D_CM_CHECK( (C::siting_consistent) );          // arena forces no siting
    }

    // -- int: unknown discipline --
    {
        using C = container_memory_class<int>;
        D_CM_CHECK( (C::discipline == memory_discipline::unknown) );
        D_CM_CHECK( (cm_cstr_eq(C::discipline_name, "unknown")) );
        D_CM_CHECK( (!C::is_none) );
        D_CM_CHECK( (!C::is_individual) );
        D_CM_CHECK( (!C::is_pooled) );
        D_CM_CHECK( (!C::is_arena) );
        D_CM_CHECK( (!C::allocates) );
        D_CM_CHECK( (!C::resolved) );
        D_CM_CHECK( (C::siting == storage_duration::unknown) );
        D_CM_CHECK( (C::siting_consistent) );
    }

    // -- alloc_beats_static: individual discipline, HYBRID siting (both signals) --
    {
        using C = container_memory_class<alloc_beats_static>;
        D_CM_CHECK( (C::discipline == memory_discipline::individual) );
        D_CM_CHECK( (C::is_individual) );
        D_CM_CHECK( (C::siting == storage_duration::hybrid_storage) );
        D_CM_CHECK( (cm_cstr_eq(C::siting_name, "hybrid_storage")) );
        D_CM_CHECK( (C::siting_consistent) );          // hybrid carries a dynamic component
    }

    // -- inconsistent_individual_static: the aggregate reports the contradiction --
    {
        using C = container_memory_class<inconsistent_individual_static>;
        D_CM_CHECK( (C::discipline == memory_discipline::individual) );
        D_CM_CHECK( (C::is_individual) );
        D_CM_CHECK( (C::siting == storage_duration::static_storage) );
        D_CM_CHECK( (cm_cstr_eq(C::siting_name, "static_storage")) );
        D_CM_CHECK( (!C::siting_consistent) );         // individual discipline over static-only storage
    }

    return _cm_ok;
}


// tests_memory_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated on variable-
//   template support exactly as the header emits them.
bool
tests_memory_variable_templates()
{
    bool _cm_ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // strategy-alias detection
    D_CM_CHECK( (has_memory_strategy_alias_v<pooled_via_strategy>
                     == has_memory_strategy_alias<pooled_via_strategy>::value) );
    D_CM_CHECK( (has_memory_strategy_alias_v<pooled_via_strategy>) );
    D_CM_CHECK( (!has_memory_strategy_alias_v<std::vector<int>>) );

    // the discipline predicates
    D_CM_CHECK( (container_has_inline_storage_v<std::array<int, 4>>
                     == container_has_inline_storage<std::array<int, 4>>::value) );
    D_CM_CHECK( (container_uses_individual_heap_v<std::vector<int>>
                     == container_uses_individual_heap<std::vector<int>>::value) );
    D_CM_CHECK( (container_uses_pool_v<pooled_via_strategy>
                     == container_uses_pool<pooled_via_strategy>::value) );
    D_CM_CHECK( (container_uses_arena_v<arena_via_strategy>
                     == container_uses_arena<arena_via_strategy>::value) );
    D_CM_CHECK( (container_allocates_v<std::vector<int>>
                     == container_allocates<std::vector<int>>::value) );
    D_CM_CHECK( (container_memory_resolved_v<int>
                     == container_memory_resolved<int>::value) );

    // and read the expected booleans directly
    D_CM_CHECK( (container_has_inline_storage_v<std::array<int, 4>>) );
    D_CM_CHECK( (container_uses_individual_heap_v<std::vector<int>>) );
    D_CM_CHECK( (container_uses_pool_v<pooled_via_allocator>) );
    D_CM_CHECK( (container_uses_arena_v<arena_via_strategy>) );
    D_CM_CHECK( (!container_allocates_v<std::array<int, 4>>) );
    D_CM_CHECK( (!container_memory_resolved_v<int>) );

    // consistency companion
    D_CM_CHECK( (container_memory_consistent_v<std::vector<int>>
                     == container_memory_consistent<std::vector<int>>::value) );
    D_CM_CHECK( (container_memory_consistent_v<std::vector<int>>) );
    D_CM_CHECK( (!container_memory_consistent_v<inconsistent_individual_static>) );

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _cm_ok;
}


NS_END  // testing
NS_END  // djinterp
