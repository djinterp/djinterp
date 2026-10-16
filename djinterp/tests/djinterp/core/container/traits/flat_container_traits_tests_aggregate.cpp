/*******************************************************************************
* djinterp [testing]                   flat_container_traits_tests_aggregate.cpp
*
*   Section II - flat_container_class, the assembled snapshot: is_container, depth,
* is_flat, is_strictly_flat, and the shared structure_kind summary (kind + name),
* checked member-for-member across a representative of every case - an ordinary flat
* container, a depth-heuristic hierarchy, a node_type hierarchy, a hierarchical-tag
* hierarchy, the tag-flat divergence (is_flat true, is_strictly_flat false, depth 2,
* kind flat), and non-containers.  The `_v` companions are verified too.
*
*
* path:      /tests/djinterp/core/container/traits/flat_container_traits_tests_aggregate.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "flat_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_flat_container_class
//   the assembled snapshot, one representative per case.
bool
tests_flat_container_class()
{
    bool _fc_ok = true;

    // -- std::vector<int>: ordinary flat container --
    {
        using C = flat_container_class<std::vector<int>>;
        D_FC_CHECK( (C::is_container) );
        D_FC_CHECK( (C::depth == 1) );
        D_FC_CHECK( (C::is_flat && C::is_strictly_flat) );
        D_FC_CHECK( (C::kind == structure_kind::flat) );
        D_FC_CHECK( (fc_cstr_eq(C::kind_name, "flat")) );
    }

    // -- std::set<int>: flat despite the node-handle node_type --
    {
        using C = flat_container_class<std::set<int>>;
        D_FC_CHECK( (C::is_container && C::depth == 1) );
        D_FC_CHECK( (C::is_flat && C::is_strictly_flat) );
        D_FC_CHECK( (C::kind == structure_kind::flat) );
    }

    // -- std::vector<std::vector<int>>: depth-heuristic hierarchy --
    {
        using C = flat_container_class<std::vector<std::vector<int>>>;
        D_FC_CHECK( (C::is_container) );
        D_FC_CHECK( (C::depth == 2) );
        D_FC_CHECK( (!C::is_flat && !C::is_strictly_flat) );
        D_FC_CHECK( (C::kind == structure_kind::hierarchical) );
        D_FC_CHECK( (fc_cstr_eq(C::kind_name, "hierarchical")) );
    }

    // -- node_box: node_type hierarchy at value_type-depth 1 --
    {
        using C = flat_container_class<node_box>;
        D_FC_CHECK( (C::is_container && C::depth == 1) );   // depth is 1 ...
        D_FC_CHECK( (!C::is_flat) );                         // ... yet hierarchical via node_type
        D_FC_CHECK( (C::kind == structure_kind::hierarchical) );
    }

    // -- hier_tagged: hierarchical tag at depth 1 --
    {
        using C = flat_container_class<hier_tagged>;
        D_FC_CHECK( (C::is_container && C::depth == 1) );
        D_FC_CHECK( (!C::is_flat && !C::is_strictly_flat) );
        D_FC_CHECK( (C::kind == structure_kind::hierarchical) );
    }

    // -- tagged_flat_str: the divergence - flat by tag, depth 2, kind flat --
    {
        using C = flat_container_class<tagged_flat_str>;
        D_FC_CHECK( (C::is_container) );
        D_FC_CHECK( (C::depth == 2) );              // the depth measure is unaffected by the tag
        D_FC_CHECK( (C::is_flat) );                 // flat by the tag ...
        D_FC_CHECK( (!C::is_strictly_flat) );       // ... but not by depth
        D_FC_CHECK( (C::kind == structure_kind::flat) );
        D_FC_CHECK( (fc_cstr_eq(C::kind_name, "flat")) );
    }

    // -- int / forward_list: non-containers --
    {
        using C = flat_container_class<int>;
        D_FC_CHECK( (!C::is_container) );
        D_FC_CHECK( (C::depth == 0) );
        D_FC_CHECK( (!C::is_flat && !C::is_strictly_flat) );
        D_FC_CHECK( (C::kind == structure_kind::non_container) );
        D_FC_CHECK( (fc_cstr_eq(C::kind_name, "non_container")) );
    }
    {
        using C = flat_container_class<std::forward_list<int>>;
        D_FC_CHECK( (!C::is_container) );           // no size() -> not container-shaped
        D_FC_CHECK( (C::kind == structure_kind::non_container) );
    }

    return _fc_ok;
}


// tests_variable_templates
//   the `_v` companions agree with their `::value` forms.  Gated on variable-
//   template support exactly as the header emits them.
bool
tests_variable_templates()
{
    bool _fc_ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_FC_CHECK( (is_flat_container_v<std::vector<int>>
                     == is_flat_container<std::vector<int>>::value) );
    D_FC_CHECK( (is_flat_container_v<std::vector<int>>) );
    D_FC_CHECK( (is_flat_container_v<tagged_flat_str>) );        // flat by tag
    D_FC_CHECK( (!is_flat_container_v<std::vector<std::vector<int>>>) );
    D_FC_CHECK( (!is_flat_container_v<node_box>) );
    D_FC_CHECK( (!is_flat_container_v<int>) );

    D_FC_CHECK( (is_strictly_flat_container_v<std::vector<int>>
                     == is_strictly_flat_container<std::vector<int>>::value) );
    D_FC_CHECK( (is_strictly_flat_container_v<flat_leaf>) );
    D_FC_CHECK( (!is_strictly_flat_container_v<tagged_flat_str>) );  // flat but depth 2
    D_FC_CHECK( (!is_strictly_flat_container_v<int>) );

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _fc_ok;
}


NS_END  // testing
NS_END  // djinterp
