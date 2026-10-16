/*******************************************************************************
* djinterp [testing]             hierarchical_container_traits_tests_signals.cpp
*
*   Section I - the three detection signals.  is_container_shape is the shared "is
* this a container at all" guard (a value_type AND a const-callable size()).
* has_node_summand is the STRONG hierarchy tell - a node_type that is itself
* container-shaped; the container-shape requirement is what keeps it from firing on
* the standard library's node-handle node_type.  has_structure_category merely
* detects an opt-in tag alias.
*
*   The edges: std::forward_list is iterable but has no size(), so it is NOT
* container-shaped; a node_type that is not container-shaped (node_handle_box, and
* the real std::set / std::map node-handles) does NOT count as a node summand; and
* the tag detector fires on presence alone, whatever kind the tag carries.
*
*
* path:      /tests/djinterp/core/container/traits/hierarchical_container_traits_tests_signals.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "hierarchical_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_container_shape
//   a value_type AND a const-callable size().
bool
tests_is_container_shape()
{
    bool _hc_ok = true;

    D_HC_CHECK( (is_container_shape<std::vector<int>>::value) );
    D_HC_CHECK( (is_container_shape<std::string>::value) );
    D_HC_CHECK( (is_container_shape<std::array<int, 4>>::value) );
    D_HC_CHECK( (is_container_shape<std::set<int>>::value) );
    D_HC_CHECK( (is_container_shape<std::map<int, int>>::value) );
    D_HC_CHECK( (is_container_shape<flat_leaf>::value) );
    D_HC_CHECK( (is_container_shape<strlike>::value) );
    D_HC_CHECK( (is_container_shape<node_box>::value) );

    // forward_list has a value_type but NO size() -> not container-shaped
    D_HC_CHECK( (!is_container_shape<std::forward_list<int>>::value) );
    // nh has a value_type but no size(); int has neither
    D_HC_CHECK( (!is_container_shape<nh>::value) );
    D_HC_CHECK( (!is_container_shape<int>::value) );

    return _hc_ok;
}


// tests_has_node_summand
//   a node_type that is ITSELF container-shaped (the strong tell).
bool
tests_has_node_summand()
{
    bool _hc_ok = true;

    D_HC_CHECK( (has_node_summand<node_box>::value) );
    D_HC_CHECK( (has_node_summand<node_box_flat_tagged>::value) );

    // node_type present but NOT container-shaped -> not a node summand
    D_HC_CHECK( (!has_node_summand<node_handle_box>::value) );
    // the standard node-handle node_type is likewise excluded
    D_HC_CHECK( (!has_node_summand<std::set<int>>::value) );
    D_HC_CHECK( (!has_node_summand<std::map<int, int>>::value) );
    // no node_type at all
    D_HC_CHECK( (!has_node_summand<std::vector<int>>::value) );
    D_HC_CHECK( (!has_node_summand<flat_leaf>::value) );
    D_HC_CHECK( (!has_node_summand<int>::value) );

    return _hc_ok;
}


// tests_has_structure_category
//   detects an opt-in structure_category alias, whatever kind it carries.
bool
tests_has_structure_category()
{
    bool _hc_ok = true;

    D_HC_CHECK( (has_structure_category<tagged_flat_str>::value) );      // flat tag
    D_HC_CHECK( (has_structure_category<node_box_flat_tagged>::value) ); // flat tag
    D_HC_CHECK( (has_structure_category<hier_tagged>::value) );          // hierarchical tag

    // no tag
    D_HC_CHECK( (!has_structure_category<flat_leaf>::value) );
    D_HC_CHECK( (!has_structure_category<node_box>::value) );
    D_HC_CHECK( (!has_structure_category<std::vector<int>>::value) );
    D_HC_CHECK( (!has_structure_category<int>::value) );

    return _hc_ok;
}


NS_END  // testing
NS_END  // djinterp
