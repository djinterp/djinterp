/*******************************************************************************
* djinterp [cpp][container]                     tree_floor_tests_nary_traits.cpp
*
*   nary/nary_tree_traits.hpp: the access probes and the child-model
* classification, on the suite's node fixtures.
*
*
* path:      /tests/djinterp/cpp/container/tree/tree_floor_tests_nary_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "tree_floor_tests.hpp"  // D_TF_CHECK, the fixtures, the declarations
#include <djinterp/cpp/container/tree/nary/nary_tree_traits.hpp>  // under test


NS_DJINTERP
NS_TESTING

/*
tests_tree_floor_nary_traits_access
  the access probes
  Tests the following:
  - first_child and next_sibling as fields, and as const methods
  - prev_sibling and last_child as methods; a parent field
  - children() iterable; edges() and edge_count()
  - nothing on a type without any of them
*/
bool tests_tree_floor_nary_traits_access()
{
    D_TF_CHECK(has_first_child_field<DTfLcrsFields>::value);
    D_TF_CHECK(has_first_child_access<DTfLcrsFields>::value);
    D_TF_CHECK(has_next_sibling_access<DTfLcrsFields>::value);
    D_TF_CHECK(has_parent_field<DTfLcrsFields>::value);

    D_TF_CHECK(!has_first_child_field<DTfLcrsMethods>::value);
    D_TF_CHECK(has_first_child_access<DTfLcrsMethods>::value);
    D_TF_CHECK(has_next_sibling_method<DTfLcrsMethods>::value);
    D_TF_CHECK(has_prev_sibling_access<DTfLcrsMethods>::value);
    D_TF_CHECK(has_last_child_access<DTfLcrsMethods>::value);

    D_TF_CHECK(has_iterable_children<DTfContainerNode>::value);
    D_TF_CHECK(has_edges_method<DTfEdgeNode>::value);
    D_TF_CHECK(has_edge_count_method<DTfEdgeNode>::value);

    D_TF_CHECK(!has_first_child_access<DTfPlain>::value);
    D_TF_CHECK(!has_next_sibling_access<DTfPlain>::value);
    D_TF_CHECK(!has_iterable_children<DTfPlain>::value);
    D_TF_CHECK(!has_edges_method<DTfPlain>::value);
    D_TF_CHECK(!has_parent_field<DTfPlain>::value);

    return true;
}

/*
tests_tree_floor_nary_traits_classification
  the child model of each fixture
  Tests the following:
  - LCRS by fields or by methods; container; edges
  - two models at once is hybrid; none is none
  - is_nary_tree_node holds for every model, not for none
  - is_nary_tree asks for root() as well
*/
bool tests_tree_floor_nary_traits_classification()
{
    D_TF_CHECK(child_access_strategy<DTfLcrsFields>::value ==
               DNaryChildAccess::lcrs);
    D_TF_CHECK(child_access_strategy<DTfLcrsMethods>::value ==
               DNaryChildAccess::lcrs);
    D_TF_CHECK(child_access_strategy<DTfContainerNode>::value ==
               DNaryChildAccess::container);
    D_TF_CHECK(child_access_strategy<DTfEdgeNode>::value ==
               DNaryChildAccess::edges);
    D_TF_CHECK(child_access_strategy<DTfHybridNode>::value ==
               DNaryChildAccess::hybrid);
    D_TF_CHECK(child_access_strategy<DTfPlain>::value ==
               DNaryChildAccess::none);

    D_TF_CHECK(is_nary_tree_node<DTfLcrsFields>::value);
    D_TF_CHECK(is_nary_tree_node<DTfContainerNode>::value);
    D_TF_CHECK(is_nary_tree_node<DTfEdgeNode>::value);
    D_TF_CHECK(is_nary_tree_node<DTfHybridNode>::value);
    D_TF_CHECK(!is_nary_tree_node<DTfPlain>::value);

    D_TF_CHECK(is_nary_tree<DTfRootedTree>::value);
    D_TF_CHECK(!is_nary_tree<DTfRootlessTree>::value);
    D_TF_CHECK(!is_nary_tree<DTfPlain>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
