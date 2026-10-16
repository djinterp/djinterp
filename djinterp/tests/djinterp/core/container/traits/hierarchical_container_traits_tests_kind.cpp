/*******************************************************************************
* djinterp [testing]                hierarchical_container_traits_tests_kind.cpp
*
*   Section IV - the structure-kind summary: non_container (not container-shaped),
* flat (container, depth 1), or hierarchical (nests).  structure_kind_name spells
* it; structure_kind_of classifies a type and exposes the value plus an
* integral_constant carrier as ::type.
*
*   The classification folds in every verdict edge: tagged_flat_str and
* node_handle_box classify FLAT (flat tag / excluded node-handle), while node_box
* and hier_tagged classify HIERARCHICAL at value_type-depth 1; non-container-shaped
* types (int, std::forward_list) classify non_container.
*
*
* path:      /tests/djinterp/core/container/traits/hierarchical_container_traits_tests_kind.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "hierarchical_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_structure_kind_name
//   the stable spellings.
bool
tests_structure_kind_name()
{
    bool _hc_ok = true;

    D_HC_CHECK( (hc_cstr_eq(structure_kind_name(structure_kind::non_container), "non_container")) );
    D_HC_CHECK( (hc_cstr_eq(structure_kind_name(structure_kind::flat),          "flat")) );
    D_HC_CHECK( (hc_cstr_eq(structure_kind_name(structure_kind::hierarchical),  "hierarchical")) );

    D_HC_CHECK( (!hc_cstr_eq(structure_kind_name(structure_kind::flat),
                             "hierarchical")) );

    return _hc_ok;
}


// tests_structure_kind_of
//   the classification, plus the ::type carrier and _v companion.
bool
tests_structure_kind_of()
{
    bool _hc_ok = true;

    // non_container
    D_HC_CHECK( (structure_kind_of<int>::value == structure_kind::non_container) );
    D_HC_CHECK( (structure_kind_of<std::forward_list<int>>::value == structure_kind::non_container) );

    // flat (incl. the flat-tag and node-handle edges)
    D_HC_CHECK( (structure_kind_of<vec1>::value == structure_kind::flat) );
    D_HC_CHECK( (structure_kind_of<std::string>::value == structure_kind::flat) );
    D_HC_CHECK( (structure_kind_of<std::set<int>>::value == structure_kind::flat) );
    D_HC_CHECK( (structure_kind_of<std::map<int, int>>::value == structure_kind::flat) );
    D_HC_CHECK( (structure_kind_of<flat_leaf>::value == structure_kind::flat) );
    D_HC_CHECK( (structure_kind_of<tagged_flat_str>::value == structure_kind::flat) );
    D_HC_CHECK( (structure_kind_of<node_handle_box>::value == structure_kind::flat) );

    // hierarchical (depth heuristic, node summand, and hierarchical tag)
    D_HC_CHECK( (structure_kind_of<vec2>::value == structure_kind::hierarchical) );
    D_HC_CHECK( (structure_kind_of<vec3>::value == structure_kind::hierarchical) );
    D_HC_CHECK( (structure_kind_of<nested_box>::value == structure_kind::hierarchical) );
    D_HC_CHECK( (structure_kind_of<node_box>::value == structure_kind::hierarchical) );
    D_HC_CHECK( (structure_kind_of<hier_tagged>::value == structure_kind::hierarchical) );

    // ::type carrier
    D_HC_CHECK( (structure_kind_of_t<vec1>::value == structure_kind::flat) );
    D_HC_CHECK( (structure_kind_of_t<vec2>::value == structure_kind::hierarchical) );
    D_HC_CHECK( (structure_kind_of_t<int>::value  == structure_kind::non_container) );

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES || D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // _v companion
    D_HC_CHECK( (structure_kind_of_v<vec2> == structure_kind_of<vec2>::value) );
    D_HC_CHECK( (structure_kind_of_v<tagged_flat_str> == structure_kind::flat) );
    D_HC_CHECK( (structure_kind_of_v<node_box> == structure_kind::hierarchical) );
#endif

    return _hc_ok;
}


NS_END  // testing
NS_END  // djinterp
