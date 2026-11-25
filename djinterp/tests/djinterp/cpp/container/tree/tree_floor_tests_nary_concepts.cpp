/*******************************************************************************
* djinterp [cpp][container]                   tree_floor_tests_nary_concepts.cpp
*
*   nary/nary_tree_concepts.hpp (C++20): the concepts, on the suite's node
* fixtures, against the traits they are built from.
*
*
* path:      /tests/djinterp/cpp/container/tree/tree_floor_tests_nary_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "tree_floor_tests.hpp"  // D_TF_CHECK, the fixtures, the declarations
#include <djinterp/cpp/container/tree/nary/nary_tree_concepts.hpp>  // under test


NS_DJINTERP
NS_TESTING

/*
tests_tree_floor_nary_concepts
  the concepts agree with the traits
  Tests the following:
  - each child-model concept holds on its fixture
  - the classified concepts hold only where the model is the one
  - a hybrid satisfies both models and neither classified concept
  - nary_tree_node_like and nary_tree_container_type
*/
bool tests_tree_floor_nary_concepts()
{
#if defined(__cpp_concepts)
    D_TF_CHECK(lcrs_child_access_type<DTfLcrsFields>);
    D_TF_CHECK(lcrs_child_access_type<DTfLcrsMethods>);
    D_TF_CHECK(container_child_access_type<DTfContainerNode>);
    D_TF_CHECK(edge_child_access_type<DTfEdgeNode>);
    D_TF_CHECK(!edge_child_access_type<DTfPlain>);

    D_TF_CHECK(classified_lcrs_child_access_type<DTfLcrsFields>);
    D_TF_CHECK(classified_container_child_access_type<DTfContainerNode>);
    D_TF_CHECK(classified_edge_child_access_type<DTfEdgeNode>);

    D_TF_CHECK(hybrid_child_access_type<DTfHybridNode>);
    D_TF_CHECK(lcrs_child_access_type<DTfHybridNode>);
    D_TF_CHECK(container_child_access_type<DTfHybridNode>);
    D_TF_CHECK(!classified_lcrs_child_access_type<DTfHybridNode>);
    D_TF_CHECK(!classified_container_child_access_type<DTfHybridNode>);

    D_TF_CHECK(nary_tree_node_like<DTfEdgeNode>);
    D_TF_CHECK(!nary_tree_node_like<DTfPlain>);
    D_TF_CHECK(nary_tree_container_type<DTfRootedTree>);
    D_TF_CHECK(!nary_tree_container_type<DTfRootlessTree>);
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
