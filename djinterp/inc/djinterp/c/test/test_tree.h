/******************************************************************************
* djinterp [test]                                                test_object.h
*
*   Core test framework providing individual test structures that can contain
* a mix of test functions and assertions. Tests can be created via the D_TEST
* macro which supports optional configuration arguments and variadic test items.
*
* A test can contain only:
*   - d_assert (assertions)
*   - d_test_fn (test functions)
*
* Configuration is resolved at runtime by combining the test's own config with
* the run config passed to d_test_run(). Parent types pass their resolved
* config down when executing children.
*
*
* path:      /inc/c/test/test_object.h       
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.05
******************************************************************************/

#ifndef DJINTERP_TEST_OBJECT_
#define DJINTERP_TEST_OBJECT_ 1

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "../djinterp.h"
#include "../dmemory.h"


/******************************************************************************
 * TEST TYPE (DISCRIMINATED UNION)
 *****************************************************************************/


struct d_test_tree_node 
{
    int32_t type;                   // your "type" / token / tag id
    void*   context;                // payload / backpointer / per-type data

    struct d_test_tree_node* parent;
    struct d_test_tree_node* first_child;
    struct d_test_tree_node* last_child;
    struct d_test_tree_node* next_sibling;
};

struct d_test_tree
{
    size_t                   count;
    struct d_test_tree_node* root;
};


struct d_test_tree_node* d_test_tree_node_new(int32_t                  _type, 
                                              void*                    _context, 
                                              struct d_test_tree_node* _parent_node, 
                                              struct d_test_tree_node* _first_child_node, 
                                              struct d_test_tree_node* _last_child_node, 
                                              struct d_test_tree_node* _next_sibling_node);

struct d_test_tree*      d_test_tree_new(void);
struct d_test_tree*      d_test_tree_new_arr(struct d_test_tree_node* _root);



void d_test_tree_node_free(struct d_test_tree_node* _test_tree_node);
void d_test_tree_free(struct d_test_tree* _test_tree);



#endif  // DJINTERP_TEST_TEST_