/*******************************************************************************
* djinterp [testing]             hierarchical_container_traits_tests_verdict.cpp
*
*   Section III - the nesting verdict.  is_hierarchical_container combines the
* signals by precedence: a container-shaped node_type OR a hierarchical tag yields
* hierarchy outright (and short-circuits the depth walk); otherwise, absent a flat
* tag, the depth heuristic (>= 2) decides.  is_depth_bounded_container is hierarchy
* AND a value_type-chain depth within a budget N.
*
*   Every precedence tie is checked: node_box is hierarchical at value_type-depth 1
* (strong node signal); node_box_flat_tagged stays hierarchical (a flat tag cannot
* override the node signal); hier_tagged is hierarchical at depth 1 (hierarchical
* tag); tagged_flat_str is NOT hierarchical despite depth 2 (flat tag suppresses the
* heuristic); and node_handle_box is NOT hierarchical (its node-handle is not
* container-shaped, so the depth-1 heuristic wins).  For the budget: a FLAT container
* is never depth-bounded (the predicate requires hierarchy first), even at depth 1.
*
*
* path:      /tests/djinterp/core/container/traits/hierarchical_container_traits_tests_verdict.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "hierarchical_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_hierarchical_container
//   node_type / hierarchical-tag decide outright, else (no flat tag) depth >= 2.
bool
tests_is_hierarchical_container()
{
    bool _hc_ok = true;

    // depth heuristic (>= 2)
    D_HC_CHECK( (is_hierarchical_container<vec2>::value) );
    D_HC_CHECK( (is_hierarchical_container<vec3>::value) );
    D_HC_CHECK( (is_hierarchical_container<std::vector<std::string>>::value) );  // string element nests
    D_HC_CHECK( (is_hierarchical_container<nested_box>::value) );

    // strong node_type signal - hierarchical even at depth 1, flat tag notwithstanding
    D_HC_CHECK( (is_hierarchical_container<node_box>::value) );
    D_HC_CHECK( (is_hierarchical_container<node_box_flat_tagged>::value) );
    // hierarchical tag at depth 1
    D_HC_CHECK( (is_hierarchical_container<hier_tagged>::value) );

    // flat cases
    D_HC_CHECK( (!is_hierarchical_container<vec1>::value) );
    D_HC_CHECK( (!is_hierarchical_container<std::string>::value) );
    D_HC_CHECK( (!is_hierarchical_container<std::set<int>>::value) );
    D_HC_CHECK( (!is_hierarchical_container<std::map<int, int>>::value) );
    D_HC_CHECK( (!is_hierarchical_container<flat_leaf>::value) );
    // flat tag suppresses the depth-2 heuristic
    D_HC_CHECK( (!is_hierarchical_container<tagged_flat_str>::value) );
    // node-handle node_type is not container-shaped -> falls to the depth-1 heuristic
    D_HC_CHECK( (!is_hierarchical_container<node_handle_box>::value) );
    // non-containers
    D_HC_CHECK( (!is_hierarchical_container<std::forward_list<int>>::value) );
    D_HC_CHECK( (!is_hierarchical_container<int>::value) );

    return _hc_ok;
}


// tests_is_depth_bounded_container
//   hierarchical AND value_type-chain depth <= N.
bool
tests_is_depth_bounded_container()
{
    bool _hc_ok = true;

    // vec2 is hierarchical at depth 2
    D_HC_CHECK( (is_depth_bounded_container<vec2, 2>::value) );
    D_HC_CHECK( (!is_depth_bounded_container<vec2, 1>::value) );   // 2 > 1
    D_HC_CHECK( (is_depth_bounded_container<vec2, 3>::value) );

    // vec3 is hierarchical at depth 3
    D_HC_CHECK( (!is_depth_bounded_container<vec3, 2>::value) );   // 3 > 2
    D_HC_CHECK( (is_depth_bounded_container<vec3, 3>::value) );

    // node_box is hierarchical at depth 1
    D_HC_CHECK( (is_depth_bounded_container<node_box, 1>::value) );
    D_HC_CHECK( (!is_depth_bounded_container<node_box, 0>::value) );  // 1 > 0

    // a FLAT container is never depth-bounded (hierarchy is required first)
    D_HC_CHECK( (!is_depth_bounded_container<vec1, 5>::value) );          // flat, depth 1 <= 5, yet false
    D_HC_CHECK( (!is_depth_bounded_container<tagged_flat_str, 5>::value) ); // flat by tag, depth 2 <= 5, yet false
    // a non-container is never depth-bounded
    D_HC_CHECK( (!is_depth_bounded_container<int, 5>::value) );

    return _hc_ok;
}


NS_END  // testing
NS_END  // djinterp
