/*******************************************************************************
* djinterp [testing]           container_storage_traits_tests_classification.cpp
*
*   Section V - container_storage_class, the assembled storage snapshot (the two
* raw signals, the resolved siting and its name, the exclusive and component
* predicates, and the siting<->size coherence bit), checked member-for-member
* against the standalone traits across a representative of every siting.  Two
* dissociations are pinned here: optin_dynamic_override raises the STATIC signal
* yet has NO static component (the opt-in resolved it to dynamic), and
* optin_static_no_size reports size_lifetime_consistent == false in the aggregate.
* The `_v` variable-template companions are verified too, gated as the header
* emits them.
*
*
* path:      /tests/djinterp/core/container/traits/container_storage_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_storage_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_container_storage_class
//   the assembled snapshot, one representative per siting (and per dissociation).
bool
tests_container_storage_class()
{
    bool _stg_ok = true;

    // -- vector<int>: dynamic --
    {
        using C = container_storage_class<std::vector<int>>;
        D_STG_CHECK( (!C::has_static_signal) );
        D_STG_CHECK( (C::has_dynamic_signal) );
        D_STG_CHECK( (C::duration == storage_duration::dynamic_storage) );
        D_STG_CHECK( (stg_cstr_eq(C::duration_name, "dynamic_storage")) );
        D_STG_CHECK( (!C::is_static && C::is_dynamic && !C::is_hybrid && !C::is_unknown) );
        D_STG_CHECK( (!C::has_static_component && C::has_dynamic_component) );
        D_STG_CHECK( (C::size_lifetime_consistent) );
    }

    // -- array<int,4>: static --
    {
        using C = container_storage_class<std::array<int, 4>>;
        D_STG_CHECK( (C::has_static_signal) );
        D_STG_CHECK( (!C::has_dynamic_signal) );
        D_STG_CHECK( (C::duration == storage_duration::static_storage) );
        D_STG_CHECK( (stg_cstr_eq(C::duration_name, "static_storage")) );
        D_STG_CHECK( (C::is_static && !C::is_dynamic && !C::is_hybrid && !C::is_unknown) );
        D_STG_CHECK( (C::has_static_component && !C::has_dynamic_component) );
        D_STG_CHECK( (C::size_lifetime_consistent) );
    }

    // -- hybrid_shape: hybrid (both signals, both components) --
    {
        using C = container_storage_class<hybrid_shape>;
        D_STG_CHECK( (C::has_static_signal && C::has_dynamic_signal) );
        D_STG_CHECK( (C::duration == storage_duration::hybrid_storage) );
        D_STG_CHECK( (stg_cstr_eq(C::duration_name, "hybrid_storage")) );
        D_STG_CHECK( (!C::is_static && !C::is_dynamic && C::is_hybrid && !C::is_unknown) );
        D_STG_CHECK( (C::has_static_component && C::has_dynamic_component) );
        D_STG_CHECK( (C::size_lifetime_consistent) );
    }

    // -- int: unknown (neither signal, neither component) --
    {
        using C = container_storage_class<int>;
        D_STG_CHECK( (!C::has_static_signal && !C::has_dynamic_signal) );
        D_STG_CHECK( (C::duration == storage_duration::unknown) );
        D_STG_CHECK( (stg_cstr_eq(C::duration_name, "unknown")) );
        D_STG_CHECK( (!C::is_static && !C::is_dynamic && !C::is_hybrid && C::is_unknown) );
        D_STG_CHECK( (!C::has_static_component && !C::has_dynamic_component) );
        D_STG_CHECK( (C::size_lifetime_consistent) );
    }

    // -- optin_dynamic_override: the STATIC signal fires, but the opt-in resolves
    //    the siting to dynamic -> NO static component (signal != siting) --
    {
        using C = container_storage_class<optin_dynamic_override>;
        D_STG_CHECK( (C::has_static_signal) );             // the raw extent signal
        D_STG_CHECK( (!C::has_dynamic_signal) );
        D_STG_CHECK( (C::duration == storage_duration::dynamic_storage) );  // ... but the opt-in wins
        D_STG_CHECK( (C::is_dynamic) );
        D_STG_CHECK( (!C::has_static_component) );          // resolved siting has no static part
        D_STG_CHECK( (C::has_dynamic_component) );
        D_STG_CHECK( (C::size_lifetime_consistent) );
    }

    // -- optin_static_no_size: the aggregate surfaces the coherence violation --
    {
        using C = container_storage_class<optin_static_no_size>;
        D_STG_CHECK( (C::duration == storage_duration::static_storage) );
        D_STG_CHECK( (C::is_static) );
        D_STG_CHECK( (C::has_static_component) );
        D_STG_CHECK( (!C::size_lifetime_consistent) );     // static siting, runtime-only size
    }

    return _stg_ok;
}


// tests_storage_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated on variable-
//   template support exactly as the header emits them.
bool
tests_storage_variable_templates()
{
    bool _stg_ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // section-I signal companions
    D_STG_CHECK( (has_static_storage_signal_v<std::array<int, 4>>
                     == has_static_storage_signal<std::array<int, 4>>::value) );
    D_STG_CHECK( (has_dynamic_storage_signal_v<std::vector<int>>
                     == has_dynamic_storage_signal<std::vector<int>>::value) );
    D_STG_CHECK( (has_static_storage_signal_v<std::array<int, 4>>) );
    D_STG_CHECK( (!has_static_storage_signal_v<std::vector<int>>) );
    D_STG_CHECK( (has_dynamic_storage_signal_v<std::set<int>>) );

    // section-III predicate companions
    D_STG_CHECK( (is_static_storage_container_v<std::array<int, 4>>
                     == is_static_storage_container<std::array<int, 4>>::value) );
    D_STG_CHECK( (is_dynamic_storage_container_v<std::vector<int>>
                     == is_dynamic_storage_container<std::vector<int>>::value) );
    D_STG_CHECK( (is_hybrid_storage_container_v<hybrid_shape>
                     == is_hybrid_storage_container<hybrid_shape>::value) );
    D_STG_CHECK( (has_static_storage_component_container_v<hybrid_shape>) );
    D_STG_CHECK( (has_dynamic_storage_component_container_v<hybrid_shape>) );
    D_STG_CHECK( (is_static_storage_container_v<std::array<int, 4>>) );
    D_STG_CHECK( (!is_hybrid_storage_container_v<std::vector<int>>) );

    // section-IV entailment companion
    D_STG_CHECK( (static_storage_implies_constexpr_size_v<std::array<int, 4>>
                     == static_storage_implies_constexpr_size<std::array<int, 4>>::value) );
    D_STG_CHECK( (static_storage_implies_constexpr_size_v<std::vector<int>>) );
    D_STG_CHECK( (!static_storage_implies_constexpr_size_v<optin_static_no_size>) );

    // section-II enum companion
    D_STG_CHECK( (storage_duration_of_v<hybrid_shape> == storage_duration::hybrid_storage) );

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _stg_ok;
}


NS_END  // testing
NS_END  // djinterp
