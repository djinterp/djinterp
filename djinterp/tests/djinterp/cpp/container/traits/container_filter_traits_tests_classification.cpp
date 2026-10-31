/*******************************************************************************
* djinterp [testing]            container_filter_traits_tests_classification.cpp
*
*   Section VI - DContainerFilterClass, the assembled Filterability of a
* container (capability, realization, stage, and preserved invariants), checked
* as a coherent whole across representative types, plus the `_v` variable-
* template companions of the individual traits.
*
*   The aggregate is verified to agree, member for member, with the standalone
* traits of the earlier sections - across a filterable contiguous type
* (vector), a fixed input-only source (array, compile-staged), a unique
* associative (set), a natively-filterable type, a sorted sequence, and a
* non-source (int) - so every capability flag, both enum verdicts, and all five
* preservation flags are exercised together.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_filter_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_filter_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_container_filter_class
//   the assembled snapshot, over one representative type per shape.
bool tests_container_filter_class()
{
    bool _cf_ok = true;

    // -- vector<int>: filterable, contiguous, runtime, ordered sequence --
    {
        using C = DContainerFilterClass<std::vector<int>>;
        D_CF_CHECK( (C::is_filterable) );
        D_CF_CHECK( (C::is_source) );
        D_CF_CHECK( (!C::is_input_only) );
        D_CF_CHECK( (!C::has_native) );
        D_CF_CHECK( (C::strategy == DFilterStrategy::indexed) );
        D_CF_CHECK( (C::stage    == DFilterStage::runtime) );
        D_CF_CHECK( (C::preserves_order) );
        D_CF_CHECK( (!C::preserves_sortedness) );
        D_CF_CHECK( (!C::preserves_uniqueness) );
        D_CF_CHECK( (C::preserves_capacity_ceiling) );
        D_CF_CHECK( (!C::preserves_capacity_floor) );
    }

    // -- array<int,4>: input-only source, compile-staged, external --
    {
        using C = DContainerFilterClass<std::array<int, 4>>;
        D_CF_CHECK( (!C::is_filterable) );
        D_CF_CHECK( (C::is_source) );
        D_CF_CHECK( (C::is_input_only) );
        D_CF_CHECK( (!C::has_native) );
        D_CF_CHECK( (C::strategy == DFilterStrategy::external) );
        D_CF_CHECK( (C::stage    == DFilterStage::compile_time) );
        D_CF_CHECK( (C::preserves_order) );
        D_CF_CHECK( (!C::preserves_uniqueness) );
    }

    // -- set<int>: unique associative, bidirectional, not order-preserving --
    {
        using C = DContainerFilterClass<std::set<int>>;
        D_CF_CHECK( (C::is_filterable) );
        D_CF_CHECK( (C::is_source) );
        D_CF_CHECK( (!C::is_input_only) );
        D_CF_CHECK( (C::strategy == DFilterStrategy::bidirectional) );
        D_CF_CHECK( (C::stage    == DFilterStage::runtime) );
        D_CF_CHECK( (!C::preserves_order) );
        D_CF_CHECK( (!C::preserves_sortedness) );
        D_CF_CHECK( (C::preserves_uniqueness) );
    }

    // -- DNativeFilterable: own primitive -> native strategy --
    {
        using C = DContainerFilterClass<DNativeFilterable>;
        D_CF_CHECK( (C::is_filterable) );
        D_CF_CHECK( (C::has_native) );
        D_CF_CHECK( (C::strategy == DFilterStrategy::native) );
        D_CF_CHECK( (C::stage    == DFilterStage::runtime) );
        D_CF_CHECK( (C::preserves_order) );
        D_CF_CHECK( (!C::preserves_uniqueness) );
    }

    // -- DSortedSeq: the sortedness-preserving case --
    {
        using C = DContainerFilterClass<DSortedSeq>;
        D_CF_CHECK( (C::is_filterable) );
        D_CF_CHECK( (!C::has_native) );
        D_CF_CHECK( (C::strategy == DFilterStrategy::indexed) );
        D_CF_CHECK( (C::preserves_order) );
        D_CF_CHECK( (C::preserves_sortedness) );
        D_CF_CHECK( (!C::preserves_uniqueness) );
    }

    // -- int: not a source at all --
    {
        using C = DContainerFilterClass<int>;
        D_CF_CHECK( (!C::is_filterable) );
        D_CF_CHECK( (!C::is_source) );
        D_CF_CHECK( (!C::is_input_only) );
        D_CF_CHECK( (!C::has_native) );
        D_CF_CHECK( (C::strategy == DFilterStrategy::unsupported) );
        D_CF_CHECK( (C::stage    == DFilterStage::none) );
        D_CF_CHECK( (!C::preserves_order) );
        D_CF_CHECK( (!C::preserves_sortedness) );
        D_CF_CHECK( (!C::preserves_uniqueness) );
        D_CF_CHECK( (C::preserves_capacity_ceiling) );
        D_CF_CHECK( (!C::preserves_capacity_floor) );
    }

    return _cf_ok;
}


// tests_filter_variable_templates
//   the `_v` companions agree with their `::value` / member counterparts.  Gated
//   on variable-template support exactly as the header emits them, so the suite
//   still builds on a C++11 baseline (where the companions are absent).
bool tests_filter_variable_templates()
{
    bool _cf_ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // capability bools mirror their ::value forms
    D_CF_CHECK( (is_filter_source_v<std::vector<int>>
                     == is_filter_source<std::vector<int>>::value) );
    D_CF_CHECK( (is_container_filterable_v<std::vector<int>>
                     == is_container_filterable<std::vector<int>>::value) );
    D_CF_CHECK( (is_filter_input_only_v<std::array<int, 4>>
                     == is_filter_input_only<std::array<int, 4>>::value) );
    D_CF_CHECK( (has_native_filter_v<DNativeFilterable>
                     == has_native_filter<DNativeFilterable>::value) );

    // and read the expected booleans directly
    D_CF_CHECK( (is_filter_source_v<std::array<int, 4>>) );
    D_CF_CHECK( (!is_container_filterable_v<std::array<int, 4>>) );
    D_CF_CHECK( (is_filter_input_only_v<std::forward_list<int>>) );
    D_CF_CHECK( (has_native_filter_v<DNativeFilterable>) );
    D_CF_CHECK( (!has_native_filter_v<DNativeWrongSig>) );

    // preservation bools
    D_CF_CHECK( (filter_preserves_order_v<std::vector<int>>
                     == filter_preserves_order<std::vector<int>>::value) );
    D_CF_CHECK( (filter_preserves_sortedness_v<DSortedSeq>
                     == filter_preserves_sortedness<DSortedSeq>::value) );
    D_CF_CHECK( (filter_preserves_uniqueness_v<std::set<int>>
                     == filter_preserves_uniqueness<std::set<int>>::value) );
    D_CF_CHECK( (filter_preserves_sortedness_v<DSortedSeq>) );
    D_CF_CHECK( (!filter_preserves_order_v<std::set<int>>) );
    D_CF_CHECK( (filter_preserves_uniqueness_v<std::set<int>>) );

    // the unconditional capacity companions
    D_CF_CHECK( (filter_preserves_capacity_ceiling_v<std::vector<int>>) );
    D_CF_CHECK( (!filter_preserves_capacity_floor_v<std::vector<int>>) );
    D_CF_CHECK( (filter_preserves_capacity_ceiling_v<int>) );
    D_CF_CHECK( (!filter_preserves_capacity_floor_v<int>) );

    // the enum companions mirror their ::value forms and read the expected verdict
    D_CF_CHECK( (container_filter_strategy_v<std::vector<int>>
                     == DContainerFilterStrategy<std::vector<int>>::value) );
    D_CF_CHECK( (container_filter_strategy_v<DNativeFilterable>
                     == DFilterStrategy::native) );
    D_CF_CHECK( (container_filter_strategy_v<std::unordered_set<int>>
                     == DFilterStrategy::forward) );

    D_CF_CHECK( (filter_stage_of_v<std::array<int, 4>>
                     == filter_stage_of<std::array<int, 4>>::value) );
    D_CF_CHECK( (filter_stage_of_v<std::array<int, 4>>
                     == DFilterStage::compile_time) );
    D_CF_CHECK( (filter_stage_of_v<std::vector<int>>
                     == DFilterStage::runtime) );
    D_CF_CHECK( (filter_stage_of_v<int>
                     == DFilterStage::none) );

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _cf_ok;
}


NS_END  // testing
NS_END  // djinterp
