/*******************************************************************************
* djinterp [testing]           hierarchical_container_traits_tests_aggregate.cpp
*
*   Section V - hierarchical_container_class, the assembled snapshot: the three
* detection signals (node_summand, has_structure_tag, is_container), the depth and
* verdict, and the structure_kind summary (kind + name), checked member-for-member
* across a representative of every case.  The `_v` companions are verified too.
*
*   The spread ties the signals to the verdict: a node summand makes node_box
* hierarchical at depth 1; a flat tag leaves tagged_flat_str flat despite depth 2
* (has_structure_tag true, is_hierarchical false); an excluded node-handle leaves
* node_handle_box flat; and a non-container zeroes the depth and reads non_container.
*
*
* path:      /tests/djinterp/core/container/traits/hierarchical_container_traits_tests_aggregate.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "hierarchical_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_hierarchical_container_class
//   the assembled snapshot, one representative per case.
bool
tests_hierarchical_container_class()
{
    bool _hc_ok = true;

    // -- vector<int>: ordinary flat container --
    {
        using C = hierarchical_container_class<vec1>;
        D_HC_CHECK( (!C::node_summand && !C::has_structure_tag && C::is_container) );
        D_HC_CHECK( (C::depth == 1) );
        D_HC_CHECK( (!C::is_hierarchical) );
        D_HC_CHECK( (C::kind == structure_kind::flat) );
        D_HC_CHECK( (hc_cstr_eq(C::kind_name, "flat")) );
    }

    // -- vector<vector<int>>: depth-heuristic hierarchy --
    {
        using C = hierarchical_container_class<vec2>;
        D_HC_CHECK( (!C::node_summand && !C::has_structure_tag && C::is_container) );
        D_HC_CHECK( (C::depth == 2) );
        D_HC_CHECK( (C::is_hierarchical) );
        D_HC_CHECK( (C::kind == structure_kind::hierarchical) );
        D_HC_CHECK( (hc_cstr_eq(C::kind_name, "hierarchical")) );
    }

    // -- node_box: node summand -> hierarchical at depth 1 --
    {
        using C = hierarchical_container_class<node_box>;
        D_HC_CHECK( (C::node_summand && !C::has_structure_tag && C::is_container) );
        D_HC_CHECK( (C::depth == 1) );          // depth 1 ...
        D_HC_CHECK( (C::is_hierarchical) );      // ... yet hierarchical via the node summand
        D_HC_CHECK( (C::kind == structure_kind::hierarchical) );
    }

    // -- tagged_flat_str: flat tag -> flat despite depth 2 --
    {
        using C = hierarchical_container_class<tagged_flat_str>;
        D_HC_CHECK( (!C::node_summand && C::has_structure_tag && C::is_container) );
        D_HC_CHECK( (C::depth == 2) );          // depth 2 ...
        D_HC_CHECK( (!C::is_hierarchical) );     // ... yet flat (the tag suppresses the heuristic)
        D_HC_CHECK( (C::kind == structure_kind::flat) );
    }

    // -- hier_tagged: hierarchical tag at depth 1 --
    {
        using C = hierarchical_container_class<hier_tagged>;
        D_HC_CHECK( (C::has_structure_tag && !C::node_summand) );
        D_HC_CHECK( (C::depth == 1 && C::is_hierarchical) );
        D_HC_CHECK( (C::kind == structure_kind::hierarchical) );
    }

    // -- node_handle_box: excluded node-handle -> flat --
    {
        using C = hierarchical_container_class<node_handle_box>;
        D_HC_CHECK( (!C::node_summand && C::is_container) );  // node_type present, but not a node summand
        D_HC_CHECK( (!C::is_hierarchical) );
        D_HC_CHECK( (C::kind == structure_kind::flat) );
    }

    // -- int: non-container --
    {
        using C = hierarchical_container_class<int>;
        D_HC_CHECK( (!C::is_container && !C::node_summand && !C::has_structure_tag) );
        D_HC_CHECK( (C::depth == 0) );
        D_HC_CHECK( (!C::is_hierarchical) );
        D_HC_CHECK( (C::kind == structure_kind::non_container) );
        D_HC_CHECK( (hc_cstr_eq(C::kind_name, "non_container")) );
    }

    return _hc_ok;
}


// tests_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated on variable-
//   template support exactly as the header emits them.
bool
tests_variable_templates()
{
    bool _hc_ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // detection-signal companions
    D_HC_CHECK( (is_container_shape_v<std::vector<int>> == is_container_shape<std::vector<int>>::value) );
    D_HC_CHECK( (is_container_shape_v<std::vector<int>>) );
    D_HC_CHECK( (!is_container_shape_v<int>) );
    D_HC_CHECK( (has_node_summand_v<node_box>) );
    D_HC_CHECK( (!has_node_summand_v<std::set<int>>) );
    D_HC_CHECK( (has_structure_category_v<hier_tagged>) );
    D_HC_CHECK( (!has_structure_category_v<flat_leaf>) );

    // depth companions (incl. the cap)
    D_HC_CHECK( (container_depth_v<vec2> == container_depth<vec2>::value) );
    D_HC_CHECK( (container_depth_v<vec3, 2> == 2) );
    D_HC_CHECK( (max_depth_of_v<vec2> == 2) );

    // verdict companion
    D_HC_CHECK( (is_hierarchical_container_v<vec2> == is_hierarchical_container<vec2>::value) );
    D_HC_CHECK( (is_hierarchical_container_v<node_box>) );
    D_HC_CHECK( (!is_hierarchical_container_v<tagged_flat_str>) );

    // kind companion
    D_HC_CHECK( (structure_kind_of_v<vec2> == structure_kind::hierarchical) );
    D_HC_CHECK( (structure_kind_of_v<int> == structure_kind::non_container) );

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _hc_ok;
}


NS_END  // testing
NS_END  // djinterp
