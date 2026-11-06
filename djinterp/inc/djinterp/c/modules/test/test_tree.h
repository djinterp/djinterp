/*******************************************************************************
* djinterp [c/test]                                                  test_tree.h
*
*   Core test framework providing individual test structures that can contain
* a mix of test functions and assertions. Tests can be created via the D_TEST
* macro which supports optional configuration arguments and variadic test items.
*
* A test can contain only:
*   - dj_assert (assertions)
*   - dj_test_fn (test functions)
*
* Configuration is resolved at runtime by combining the test's own config with
* the run config passed to d_test_run(). Parent types pass their resolved
* config down when executing children.
*
*
* path:      /inc/djinterp/c/modules/test/test_tree.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.05
*                                                            revised: 2026.11.05
*******************************************************************************/

#ifndef DJINTERP_C_TEST_TEST_TREE_H
#define DJINTERP_C_TEST_TEST_TREE_H 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../../memory/dmemory.h"


/*******************************************************************************
 * TEST TYPE (DISCRIMINATED                                               UNION)
 *****************************************************************************/


struct dj_test_tree_node
{
    int32_t type;                   // your "type" / token / tag id
    void*   context;                // payload / backpointer / per-type data

    struct dj_test_tree_node* parent;
    struct dj_test_tree_node* first_child;
    struct dj_test_tree_node* last_child;
    struct dj_test_tree_node* next_sibling;
};

struct dj_test_tree
{
    size_t                   count;
    struct dj_test_tree_node* root;
};


struct dj_test_tree_node* d_test_tree_node_new(
    int32_t                   _type,
    void*                     _context,
    struct dj_test_tree_node* _parent_node,
    struct dj_test_tree_node* _first_child_node,
    struct dj_test_tree_node* _last_child_node,
    struct dj_test_tree_node* _next_sibling_node
);

struct dj_test_tree* d_test_tree_new(void);
// deep-copies _count node templates, remapping their links; NULL when
// _count is 0, a link points outside the array, or memory or the size of the
// pointer table runs out
struct dj_test_tree* d_test_tree_new_arr(
    struct dj_test_tree_node* _test_tree_nodes,
    size_t                    _count
);



void d_test_tree_node_free(
    struct dj_test_tree_node* _test_tree_node
);
void d_test_tree_free(
    struct dj_test_tree* _test_tree
);



#endif  // DJINTERP_C_TEST_TEST_TREE_H
