/*******************************************************************************
* djinterp [cpp][container]                             tree_floor_tests_avl.cpp
*
*   avl/AVLtree.hpp. The header declares nothing yet: its namespace is empty.
* Its floor is that it compiles alone, at the suite's level, and that this
* unit, which includes it and nothing else of the trees, keeps it so.
*
*
* path:      /tests/djinterp/cpp/container/tree/tree_floor_tests_avl.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include "tree_floor_tests.hpp"  // D_TF_CHECK, the declarations
#include <djinterp/cpp/container/tree/avl/AVLtree.hpp>  // under test


NS_DJINTERP
NS_TESTING

/*
tests_tree_floor_avl_alone
  the header, compiled alone
  Tests the following:
  - this unit compiles with the header as its only tree include
  - its include guard is defined: the header was read, not skipped
*/
bool tests_tree_floor_avl_alone()
{
#if defined(DJINTERP_CONTAINER_TREE_AVL_AVLTREE_HPP)
    D_TF_CHECK(true);

    return true;
#else
    D_TF_CHECK(!"AVLtree.hpp's include guard is not defined");

    return false;
#endif
}

NS_END  // testing
NS_END  // djinterp
