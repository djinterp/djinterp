#include "./test_block_tests_sa.h"


/******************************************************************************
 * EXECUTION TEST HELPERS
 *****************************************************************************/

// file-scope state for test_fn verification
static bool g_exec_test_fn_called = false;


/*
d_internal_exec_test_fn_pass
  Test function that always passes.

Return:
  true always.
*/
static bool
d_internal_exec_test_fn_pass
(
    void
)
{
    g_exec_test_fn_called = true;

    return true;
}


/*
d_internal_exec_test_fn_fail
  Test function that always fails.

Return:
  false always.
*/
static bool
d_internal_exec_test_fn_fail
(
    void
)
{
    g_exec_test_fn_called = true;

    return false;
}


/*
d_internal_exec_reset_globals
  Resets all file-scope test state flags to false.

Parameter(s):
  none.
Return:
  none.
*/
static void
d_internal_exec_reset_globals
(
    void
)
{
    g_exec_test_fn_called = false;

    return;
}


/******************************************************************************
 * EXECUTION TESTS
 *****************************************************************************/

/*
d_tests_sa_test_block_run_empty
  Tests d_test_block_run on an empty block (no children).
  Tests the following:
  - returns true for empty block (vacuous truth)
*/
bool
d_tests_sa_test_block_run_empty
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    result = d_test_block_run(block, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "run_empty: return value",
        "running an empty block should return true",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}


/*
d_tests_sa_test_block_run_null
  Tests d_test_block_run with NULL block.
  Tests the following:
  - returns false for NULL block
*/
bool
d_tests_sa_test_block_run_null
(
    struct d_test_counter* _test_info
)
{
    bool result;
    bool all_passed;

    all_passed = true;

    result = d_test_block_run(NULL, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "run_null: return value",
        "running a NULL block should return false",
        _test_info);

    return all_passed;
}


/*
d_tests_sa_test_block_run_with_passing_assertions
  Tests d_test_block_run with multiple passing assertions.
  Tests the following:
  - returns true when all assertions pass
*/
bool
d_tests_sa_test_block_run_with_passing_assertions
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_assert*     a1;
    struct d_assert*     a2;
    struct d_test_type*  t1;
    struct d_test_type*  t2;
    struct d_test_type*  children[2];
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    a1 = d_assert_true(true, "pass1", "fail1");
    a2 = d_assert_true(true, "pass2", "fail2");

    if ( (!a1) || (!a2) )
    {
        d_assert_free(a1);
        d_assert_free(a2);

        return false;
    }

    t1 = d_test_type_new(D_TEST_TYPE_ASSERT, a1);
    t2 = d_test_type_new(D_TEST_TYPE_ASSERT, a2);

    if ( (!t1) || (!t2) )
    {
        if (t1) { d_test_type_free(t1); } else { d_assert_free(a1); }
        if (t2) { d_test_type_free(t2); } else { d_assert_free(a2); }

        return false;
    }

    children[0] = t1;
    children[1] = t2;

    block = d_test_block_new(children, 2);

    if (!block)
    {
        d_test_type_free(t1);
        d_test_type_free(t2);

        return false;
    }

    result = d_test_block_run(block, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "run_passing_asserts: return value",
        "block with all passing assertions should return true",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}


/*
d_tests_sa_test_block_run_with_failing_assertion
  Tests d_test_block_run with a mix of passing and failing assertions.
  Tests the following:
  - returns false when any assertion fails
  - all children are still evaluated (no short-circuit)
*/
bool
d_tests_sa_test_block_run_with_failing_assertion
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_assert*     a1;
    struct d_assert*     a2;
    struct d_test_type*  t1;
    struct d_test_type*  t2;
    struct d_test_type*  children[2];
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    // first passes, second fails
    a1 = d_assert_true(true,  "pass1", "fail1");
    a2 = d_assert_true(false, "pass2", "fail2");

    if ( (!a1) || (!a2) )
    {
        d_assert_free(a1);
        d_assert_free(a2);

        return false;
    }

    t1 = d_test_type_new(D_TEST_TYPE_ASSERT, a1);
    t2 = d_test_type_new(D_TEST_TYPE_ASSERT, a2);

    if ( (!t1) || (!t2) )
    {
        if (t1) { d_test_type_free(t1); } else { d_assert_free(a1); }
        if (t2) { d_test_type_free(t2); } else { d_assert_free(a2); }

        return false;
    }

    children[0] = t1;
    children[1] = t2;

    block = d_test_block_new(children, 2);

    if (!block)
    {
        d_test_type_free(t1);
        d_test_type_free(t2);

        return false;
    }

    result = d_test_block_run(block, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "run_failing_assert: return value",
        "block with a failing assertion should return false",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}


/*
d_tests_sa_test_block_run_with_test_fn
  Tests d_test_block_run with a test function child.
  Tests the following:
  - test function is invoked during run
  - return value reflects the test function result
*/
bool
d_tests_sa_test_block_run_with_test_fn
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_test_fn*    test_fn;
    struct d_test_type*  fn_type;
    struct d_test_type*  children[1];
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    d_internal_exec_reset_globals();

    test_fn = d_test_fn_new((fn_test)d_internal_exec_test_fn_pass);

    if (!test_fn)
    {
        return false;
    }

    fn_type = d_test_type_new(D_TEST_TYPE_TEST_FN, test_fn);

    if (!fn_type)
    {
        d_test_fn_free(test_fn);

        return false;
    }

    children[0] = fn_type;

    block = d_test_block_new(children, 1);

    if (!block)
    {
        d_test_type_free(fn_type);

        return false;
    }

    result = d_test_block_run(block, NULL);

    all_passed &= d_assert_standalone(
        g_exec_test_fn_called == true,
        "run_test_fn: invoked",
        "test function should have been called during run",
        _test_info);

    all_passed &= d_assert_standalone(
        result == true,
        "run_test_fn: return value",
        "block should return true when test_fn passes",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}


/*
d_tests_sa_test_block_run_with_nested_block
  Tests d_test_block_run with a nested child block.
  Tests the following:
  - child block's run is invoked recursively
  - result propagates from child to parent
*/
bool
d_tests_sa_test_block_run_with_nested_block
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* parent;
    struct d_test_block* child;
    struct d_assert*     assertion;
    struct d_test_type*  assert_type;
    struct d_test_type*  children[1];
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    // create child block with a passing assertion
    assertion = d_assert_true(true, "nested_pass", "nested_fail");

    if (!assertion)
    {
        return false;
    }

    assert_type = d_test_type_new(D_TEST_TYPE_ASSERT, assertion);

    if (!assert_type)
    {
        d_assert_free(assertion);

        return false;
    }

    children[0] = assert_type;

    child = d_test_block_new(children, 1);

    if (!child)
    {
        d_test_type_free(assert_type);

        return false;
    }

    // create parent and add child block
    parent = d_test_block_new(NULL, 0);

    if (!parent)
    {
        d_test_block_free(child);

        return false;
    }

    d_test_block_add_block(parent, child);

    result = d_test_block_run(parent, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "run_nested: return value",
        "nested block with passing assertion should return true",
        _test_info);

    d_test_block_free(parent);

    return all_passed;
}


/*
d_tests_sa_test_block_run_mixed_children
  Tests d_test_block_run with a mix of assertions, test functions,
  and nested blocks.
  Tests the following:
  - all child types are handled correctly
  - one failing child causes overall false result
  - all children are still evaluated (no short-circuit)
*/
bool
d_tests_sa_test_block_run_mixed_children
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_test_block* child_block;
    struct d_assert*     assertion;
    struct d_test_type*  assert_type;
    struct d_test_fn*    test_fn;
    struct d_test_type*  fn_type;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    d_internal_exec_reset_globals();

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    // add a passing assertion
    assertion = d_assert_true(true, "pass", "fail");

    if (assertion)
    {
        assert_type = d_test_type_new(D_TEST_TYPE_ASSERT, assertion);

        if (assert_type)
        {
            d_test_block_add_child(block, assert_type);
        }
        else
        {
            d_assert_free(assertion);
        }
    }

    // add a test function that fails
    test_fn = d_test_fn_new((fn_test)d_internal_exec_test_fn_fail);

    if (test_fn)
    {
        fn_type = d_test_type_new(D_TEST_TYPE_TEST_FN, test_fn);

        if (fn_type)
        {
            d_test_block_add_child(block, fn_type);
        }
        else
        {
            d_test_fn_free(test_fn);
        }
    }

    // add a nested empty block (should pass)
    child_block = d_test_block_new(NULL, 0);

    if (child_block)
    {
        d_test_block_add_block(block, child_block);
    }

    result = d_test_block_run(block, NULL);

    // the failing test_fn should cause overall failure
    all_passed &= d_assert_standalone(
        result == false,
        "run_mixed: return value",
        "block with a failing child should return false",
        _test_info);

    all_passed &= d_assert_standalone(
        g_exec_test_fn_called == true,
        "run_mixed: test_fn called",
        "test function should have been invoked",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}


/*
d_tests_sa_test_block_execution_all
  Aggregation function for all execution tests.
  Tests the following:
  - d_test_block_run (empty, NULL, passing, failing, test_fn, nested)
  - mixed child type handling
*/
bool
d_tests_sa_test_block_execution_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_test_block_run_empty(_test_info);
    all_passed &= d_tests_sa_test_block_run_null(_test_info);
    all_passed &= d_tests_sa_test_block_run_with_passing_assertions(_test_info);
    all_passed &= d_tests_sa_test_block_run_with_failing_assertion(_test_info);
    all_passed &= d_tests_sa_test_block_run_with_test_fn(_test_info);
    all_passed &= d_tests_sa_test_block_run_with_nested_block(_test_info);
    all_passed &= d_tests_sa_test_block_run_mixed_children(_test_info);

    return all_passed;
}
