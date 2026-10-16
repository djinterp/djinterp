/*******************************************************************************
* djinterp [testing]         container_transform_traits_tests_classification.cpp
*
*   Section VI - container_transform_class, the assembled Transformability
* snapshot: capability (transformable / source / input-only / native), realization
* (strategy, stage), and the preservation verdicts, checked member-for-member
* across a representative of every shape.  The `_v` companions are verified too,
* gated as the header emits them.
*
*   The spread covers each capability/strategy/stage combination: an indexed
* runtime transformable (std::vector), a bidirectional runtime one with no
* preserved order (std::set), a forward transformable (std::unordered_set), an
* input-only yet COMPILE-TIME external source (std::array), an input-only runtime
* one (std::forward_list), a native compile-time one (native_box), and a
* non-container whose strategy/stage are unsupported/none yet whose size and
* capacity preservation still read true (they are unconditional).
*
*
* path:      /tests/djinterp/core/container/traits/container_transform_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_transform_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_container_transform_class
//   the assembled snapshot, one representative per shape.
bool
tests_container_transform_class()
{
    bool _tx_ok = true;

    // -- std::vector<int>: indexed, runtime, transformable, order preserved --
    {
        using C = container_transform_class<std::vector<int>>;
        D_TX_CHECK( (C::is_transformable && C::is_source && !C::is_input_only && !C::has_native) );
        D_TX_CHECK( (C::strategy == transform_strategy::indexed) );
        D_TX_CHECK( (C::stage == transform_stage::runtime) );
        D_TX_CHECK( (C::preserves_order) );
        D_TX_CHECK( (C::preserves_size && C::preserves_capacity_ceiling && C::preserves_capacity_floor) );
        D_TX_CHECK( (!C::preserves_sortedness && !C::preserves_uniqueness) );
    }

    // -- std::set<int>: bidirectional, runtime, transformable, NO order --
    {
        using C = container_transform_class<std::set<int>>;
        D_TX_CHECK( (C::is_transformable && C::is_source && !C::is_input_only) );
        D_TX_CHECK( (C::strategy == transform_strategy::bidirectional) );
        D_TX_CHECK( (C::stage == transform_stage::runtime) );
        D_TX_CHECK( (!C::preserves_order) );
        D_TX_CHECK( (!C::preserves_sortedness && !C::preserves_uniqueness) );  // even a sorted, unique set
    }

    // -- std::unordered_set<int>: forward, transformable --
    {
        using C = container_transform_class<std::unordered_set<int>>;
        D_TX_CHECK( (C::is_transformable) );
        D_TX_CHECK( (C::strategy == transform_strategy::forward) );
        D_TX_CHECK( (!C::preserves_order) );
    }

    // -- std::array<int,4>: input-only, external, but COMPILE-TIME --
    {
        using C = container_transform_class<std::array<int, 4>>;
        D_TX_CHECK( (!C::is_transformable && C::is_source && C::is_input_only) );
        D_TX_CHECK( (C::strategy == transform_strategy::external) );
        D_TX_CHECK( (C::stage == transform_stage::compile_time) );
        D_TX_CHECK( (C::preserves_order) );
    }

    // -- std::forward_list<int>: input-only, external, runtime --
    {
        using C = container_transform_class<std::forward_list<int>>;
        D_TX_CHECK( (!C::is_transformable && C::is_source && C::is_input_only) );
        D_TX_CHECK( (C::strategy == transform_strategy::external) );
        D_TX_CHECK( (C::stage == transform_stage::runtime) );
        D_TX_CHECK( (C::preserves_order) );
    }

    // -- native_box: native, compile-time, transformable --
    {
        using C = container_transform_class<native_box>;
        D_TX_CHECK( (C::is_transformable && C::has_native) );
        D_TX_CHECK( (C::strategy == transform_strategy::native) );
        D_TX_CHECK( (C::stage == transform_stage::compile_time) );
        D_TX_CHECK( (C::preserves_order) );
    }

    // -- int: unsupported / none, yet size + capacity preservation read true --
    {
        using C = container_transform_class<int>;
        D_TX_CHECK( (!C::is_transformable && !C::is_source && !C::is_input_only && !C::has_native) );
        D_TX_CHECK( (C::strategy == transform_strategy::unsupported) );
        D_TX_CHECK( (C::stage == transform_stage::none) );
        D_TX_CHECK( (!C::preserves_order) );
        D_TX_CHECK( (C::preserves_size && C::preserves_capacity_ceiling && C::preserves_capacity_floor) );  // unconditional
        D_TX_CHECK( (!C::preserves_sortedness && !C::preserves_uniqueness) );
    }

    return _tx_ok;
}


// tests_transform_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated on inline-
//   variable support exactly as the header emits them.
bool
tests_transform_variable_templates()
{
    bool _tx_ok = true;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

    // capability companions
    D_TX_CHECK( (is_transform_source_v<std::vector<int>> == is_transform_source<std::vector<int>>::value) );
    D_TX_CHECK( (is_container_transformable_v<std::vector<int>>) );
    D_TX_CHECK( (is_transform_input_only_v<std::array<int, 4>>) );
    D_TX_CHECK( (has_native_transform_v<native_box>) );
    D_TX_CHECK( (!is_transform_source_v<int>) );

    // realization companions (enums)
    D_TX_CHECK( (container_transform_strategy_v<std::vector<int>> == transform_strategy::indexed) );
    D_TX_CHECK( (container_transform_strategy_v<std::unordered_set<int>> == transform_strategy::forward) );
    D_TX_CHECK( (transform_stage_of_v<std::array<int, 4>> == transform_stage::compile_time) );
    D_TX_CHECK( (transform_stage_of_v<std::vector<int>> == transform_stage::runtime) );

    // preservation companions
    D_TX_CHECK( (transform_preserves_order_v<std::vector<int>> == transform_preserves_order<std::vector<int>>::value) );
    D_TX_CHECK( (!transform_preserves_order_v<std::set<int>>) );
    D_TX_CHECK( (transform_preserves_size_v<std::set<int>>) );
    D_TX_CHECK( (transform_preserves_capacity_ceiling_v<int>) );
    D_TX_CHECK( (transform_preserves_capacity_floor_v<std::vector<int>>) );
    D_TX_CHECK( (!transform_preserves_sortedness_v<std::set<int>>) );
    D_TX_CHECK( (!transform_preserves_uniqueness_v<std::set<int>>) );

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

    return _tx_ok;
}


NS_END  // testing
NS_END  // djinterp
