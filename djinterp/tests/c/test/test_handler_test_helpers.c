/******************************************************************************
* djinterp [test]                                   test_handler_test_helpers.c
*
*   Implementation of shared helper functions used across the test_handler
* standalone test suites.
*
*
* path:      \test\c\test\test_handler_test_helpers.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.28
******************************************************************************/

#include "test_handler_test_helpers.h"


/******************************************************************************
 * INTERNAL TEST FUNCTIONS
 *****************************************************************************/

/*
d_internal_passing_test_fn
  A test function that unconditionally returns D_TEST_PASS.
Used by helper_make_passing_test to create tests that always succeed.

Parameter(s):
  none
Return:
  D_TEST_PASS (true)
*/
static bool
d_internal_passing_test_fn
(
    void
)
{
    return D_TEST_PASS;
}

/*
d_internal_failing_test_fn
  A test function that unconditionally returns D_TEST_FAIL.
Used by helper_make_failing_test to create tests that always fail.

Parameter(s):
  none
Return:
  D_TEST_FAIL (false)
*/
static bool
d_internal_failing_test_fn
(
    void
)
{
    return D_TEST_FAIL;
}


/******************************************************************************
 * INTERNAL HELPER
 *****************************************************************************/

/*
d_internal_make_test_with_fn
  Creates a d_test containing a single test function child.

Parameter(s):
  _fn: the test function to wrap inside the new test.
Return:
  A pointer to the newly-created d_test, or NULL on failure.
*/
static struct d_test*
d_internal_make_test_with_fn
(
    fn_test _fn
)
{
    struct d_test_fn*   test_fn;
    struct d_test_type* child;
    struct d_test_type* children[1];
    struct d_test*      test;

    // create the test function wrapper
    test_fn = d_test_fn_new(_fn);
    if (!test_fn)
    {
        return NULL;
    }

    // wrap in a test type node
    child = d_test_type_new(D_TEST_TYPE_TEST_FN, test_fn);
    if (!child)
    {
        d_test_fn_free(test_fn);

        return NULL;
    }

    // create the test with this single child
    children[0] = child;
    test = d_test_new(children, 1);

    return test;
}


/******************************************************************************
 * TEST FACTORY FUNCTIONS
 *****************************************************************************/

/*
helper_make_passing_test
  Creates a d_test containing a single test function that unconditionally
passes (returns D_TEST_PASS).

Parameter(s):
  none
Return:
  A pointer to the newly-created passing d_test, or NULL on failure.
*/
struct d_test*
helper_make_passing_test
(
    void
)
{
    return d_internal_make_test_with_fn(d_internal_passing_test_fn);
}

/*
helper_make_failing_test
  Creates a d_test containing a single test function that unconditionally
fails (returns D_TEST_FAIL).

Parameter(s):
  none
Return:
  A pointer to the newly-created failing d_test, or NULL on failure.
*/
struct d_test*
helper_make_failing_test
(
    void
)
{
    return d_internal_make_test_with_fn(d_internal_failing_test_fn);
}


/******************************************************************************
 * BLOCK POPULATION FUNCTIONS
 *****************************************************************************/

/*
helper_add_passing_test_to_block
  Creates a passing test and adds it as a child of the given block.

Parameter(s):
  _block: the test block to which a passing test will be added.
Return:
  true if the test was successfully created and added; false otherwise.
*/
bool
helper_add_passing_test_to_block
(
    struct d_test_block* _block
)
{
    struct d_test* test;

    if (!_block)
    {
        return false;
    }

    // create a passing test
    test = helper_make_passing_test();
    if (!test)
    {
        return false;
    }

    // add to block
    return d_test_block_add_test(_block, test);
}

/*
helper_add_failing_test_to_block
  Creates a failing test and adds it as a child of the given block.

Parameter(s):
  _block: the test block to which a failing test will be added.
Return:
  true if the test was successfully created and added; false otherwise.
*/
bool
helper_add_failing_test_to_block
(
    struct d_test_block* _block
)
{
    struct d_test* test;

    if (!_block)
    {
        return false;
    }

    // create a failing test
    test = helper_make_failing_test();
    if (!test)
    {
        return false;
    }

    // add to block
    return d_test_block_add_test(_block, test);
}

/*
helper_add_block_child_to_block
  Creates an empty child block and adds it as a child of the given parent
block.

Parameter(s):
  _parent: the parent block to which the new child block will be added.
Return:
  true if the child block was successfully created and added; false otherwise.
*/
bool
helper_add_block_child_to_block
(
    struct d_test_block* _parent
)
{
    struct d_test_block* child;

    if (!_parent)
    {
        return false;
    }

    // create an empty child block
    child = d_test_block_new(NULL, 0);
    if (!child)
    {
        return false;
    }

    // add child block to parent
    return d_test_block_add_block(_parent, child);
}
