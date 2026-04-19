#include "./test_tests_sa.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

// dummy test function that always passes
static bool
d_internal_child_mgmt_pass_fn(void)
{
    return true;
}

/*
d_tests_sa_test_add_child_null_test
  Tests d_test_add_child with NULL test parameter.
  Tests the following:
  - d_test_add_child(NULL, child) returns false
*/
bool
d_tests_sa_test_add_child_null_test
(
    struct d_test_counter* _counter
)
{
    struct d_test_fn*   fn;
    struct d_test_type* child;
    bool                result;

    result = true;

    fn = d_test_fn_new((fn_test)d_internal_child_mgmt_pass_fn);

    if (!fn)
    {
        return d_assert_standalone(false,
                                   "d_test_add_child_null_test",
                                   "setup: d_test_fn_new failed",
                                   _counter);
    }

    child = d_test_type_new(D_TEST_TYPE_TEST_FN, fn);

    if (!child)
    {
        d_test_fn_free(fn);

        return d_assert_standalone(false,
                                   "d_test_add_child_null_test",
                                   "setup: d_test_type_new failed",
                                   _counter);
    }

    // attempt add to NULL test
    result = d_assert_standalone(d_test_add_child(NULL, child) == false,
                                 "d_test_add_child_null_test",
                                 "d_test_add_child(NULL, child) returns false",
                                 _counter) && result;

    // cleanup
    d_test_fn_free(fn);
    free(child);

    return result;
}

/*
d_tests_sa_test_add_child_null_child
  Tests d_test_add_child with NULL child parameter.
  Tests the following:
  - d_test_add_child(test, NULL) returns false
*/
bool
d_tests_sa_test_add_child_null_child
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_add_child_null_child",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // attempt add NULL child
    result = d_assert_standalone(d_test_add_child(test, NULL) == false,
                                 "d_test_add_child_null_child",
                                 "d_test_add_child(test, NULL) returns false",
                                 _counter) && result;

    d_test_free(test);

    return result;
}

/*
d_tests_sa_test_add_child_assert
  Tests d_test_add_child with a valid assertion-type child.
  Tests the following:
  - adding an assertion-type child succeeds
  - child count increments after adding
*/
bool
d_tests_sa_test_add_child_assert
(
    struct d_test_counter* _counter
)
{
    struct d_test*   test;
    struct d_assert* assertion;
    bool             result;
    bool             add_result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_add_child_assert",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // verify initial count
    result = d_assert_standalone(d_test_child_count(test) == 0,
                                 "d_test_add_child_assert",
                                 "initial child count is 0",
                                 _counter) && result;

    // create and add assertion
    assertion = d_assert_true(true, "pass", "fail");

    if (assertion)
    {
        add_result = d_test_add_assertion(test, assertion);

        result = d_assert_standalone(add_result == true,
                                     "d_test_add_child_assert",
                                     "d_test_add_assertion returns true",
                                     _counter) && result;

        // verify count incremented
        result = d_assert_standalone(d_test_child_count(test) == 1,
                                     "d_test_add_child_assert",
                                     "child count is 1 after adding assertion",
                                     _counter) && result;
    }
    else
    {
        result = d_assert_standalone(false,
                                     "d_test_add_child_assert",
                                     "setup: d_assert_true failed",
                                     _counter);
    }

    d_test_free(test);

    return result;
}

/*
d_tests_sa_test_add_child_invalid_type
  Tests d_test_add_child rejects children with invalid types.
  Tests the following:
  - adding a child with D_TEST_TYPE_TEST_BLOCK type is rejected
  - adding a child with D_TEST_TYPE_MODULE type is rejected
  - child count remains unchanged after rejected adds
*/
bool
d_tests_sa_test_add_child_invalid_type
(
    struct d_test_counter* _counter
)
{
    struct d_test*      test;
    struct d_test_type  fake_child;
    bool                result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_add_child_invalid_type",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // create a fake child with invalid type (TEST_BLOCK)
    d_memset(&fake_child, 0, sizeof(struct d_test_type));
    fake_child.type = D_TEST_TYPE_TEST_BLOCK;

    result = d_assert_standalone(d_test_add_child(test, &fake_child) == false,
                                 "d_test_add_child_invalid_type",
                                 "adding TEST_BLOCK type is rejected",
                                 _counter) && result;

    // create a fake child with MODULE type
    fake_child.type = D_TEST_TYPE_MODULE;

    result = d_assert_standalone(d_test_add_child(test, &fake_child) == false,
                                 "d_test_add_child_invalid_type",
                                 "adding MODULE type is rejected",
                                 _counter) && result;

    // verify count unchanged
    result = d_assert_standalone(d_test_child_count(test) == 0,
                                 "d_test_add_child_invalid_type",
                                 "child count remains 0 after rejected adds",
                                 _counter) && result;

    d_test_free(test);

    return result;
}

/*
d_tests_sa_test_add_function_valid
  Tests d_test_add_function with a valid function pointer.
  Tests the following:
  - d_test_add_function returns true
  - child count increments
*/
bool
d_tests_sa_test_add_function_valid
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_add_function_valid",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // add a function
    result = d_assert_standalone(
                 d_test_add_function(test,
                                     (fn_test)d_internal_child_mgmt_pass_fn) == true,
                 "d_test_add_function_valid",
                 "d_test_add_function returns true for valid fn",
                 _counter) && result;

    // verify child count
    result = d_assert_standalone(d_test_child_count(test) == 1,
                                 "d_test_add_function_valid",
                                 "child count is 1 after adding function",
                                 _counter) && result;

    d_test_free(test);

    return result;
}

/*
d_tests_sa_test_add_function_null
  Tests d_test_add_function with NULL parameters.
  Tests the following:
  - d_test_add_function(NULL, fn) returns false
  - d_test_add_function(test, NULL) returns false
*/
bool
d_tests_sa_test_add_function_null
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_add_function_null",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // NULL test
    result = d_assert_standalone(
                 d_test_add_function(NULL,
                                     (fn_test)d_internal_child_mgmt_pass_fn) == false,
                 "d_test_add_function_null",
                 "d_test_add_function(NULL, fn) returns false",
                 _counter) && result;

    // NULL function
    result = d_assert_standalone(d_test_add_function(test, NULL) == false,
                                 "d_test_add_function_null",
                                 "d_test_add_function(test, NULL) returns false",
                                 _counter) && result;

    d_test_free(test);

    return result;
}

/*
d_tests_sa_test_child_count_null
  Tests d_test_child_count with NULL test.
  Tests the following:
  - d_test_child_count(NULL) returns 0
*/
bool
d_tests_sa_test_child_count_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(d_test_child_count(NULL) == 0,
                                 "d_test_child_count_null",
                                 "d_test_child_count(NULL) returns 0",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_test_child_count_after_add
  Tests d_test_child_count increments correctly after multiple adds.
  Tests the following:
  - count starts at 0
  - count is 1 after first add
  - count is 2 after second add
  - count is 3 after third add
*/
bool
d_tests_sa_test_child_count_after_add
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_child_count_after_add",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // verify starting count
    result = d_assert_standalone(d_test_child_count(test) == 0,
                                 "d_test_child_count_after_add",
                                 "initial count is 0",
                                 _counter) && result;

    // add three functions
    d_test_add_function(test, (fn_test)d_internal_child_mgmt_pass_fn);

    result = d_assert_standalone(d_test_child_count(test) == 1,
                                 "d_test_child_count_after_add",
                                 "count is 1 after first add",
                                 _counter) && result;

    d_test_add_function(test, (fn_test)d_internal_child_mgmt_pass_fn);

    result = d_assert_standalone(d_test_child_count(test) == 2,
                                 "d_test_child_count_after_add",
                                 "count is 2 after second add",
                                 _counter) && result;

    d_test_add_function(test, (fn_test)d_internal_child_mgmt_pass_fn);

    result = d_assert_standalone(d_test_child_count(test) == 3,
                                 "d_test_child_count_after_add",
                                 "count is 3 after third add",
                                 _counter) && result;

    d_test_free(test);

    return result;
}

/*
d_tests_sa_test_get_child_at_valid
  Tests d_test_get_child_at with valid indices.
  Tests the following:
  - get_child_at(test, 0) returns non-NULL after adding child
  - returned child has correct type
*/
bool
d_tests_sa_test_get_child_at_valid
(
    struct d_test_counter* _counter
)
{
    struct d_test*      test;
    struct d_test_type* child;
    bool                result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_get_child_at_valid",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // add a function to create a child
    d_test_add_function(test, (fn_test)d_internal_child_mgmt_pass_fn);

    // retrieve child at index 0
    child = d_test_get_child_at(test, 0);

    result = d_assert_standalone(child != NULL,
                                 "d_test_get_child_at_valid",
                                 "d_test_get_child_at(test, 0) returns non-NULL",
                                 _counter) && result;

    if (child)
    {
        // verify child type
        result = d_assert_standalone(child->type == D_TEST_TYPE_TEST_FN,
                                     "d_test_get_child_at_valid",
                                     "child at index 0 has TEST_FN type",
                                     _counter) && result;
    }

    d_test_free(test);

    return result;
}

/*
d_tests_sa_test_get_child_at_null
  Tests d_test_get_child_at with NULL test.
  Tests the following:
  - d_test_get_child_at(NULL, 0) returns NULL
*/
bool
d_tests_sa_test_get_child_at_null
(
    struct d_test_counter* _counter
)
{
    struct d_test_type* child;
    bool                result;

    result = true;

    child = d_test_get_child_at(NULL, 0);

    result = d_assert_standalone(child == NULL,
                                 "d_test_get_child_at_null",
                                 "d_test_get_child_at(NULL, 0) returns NULL",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_test_child_mgmt_all
  Aggregation function for all child management tests.
*/
bool
d_tests_sa_test_child_mgmt_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_add_child_null_test(_counter) && result;
    result = d_tests_sa_test_add_child_null_child(_counter) && result;
    result = d_tests_sa_test_add_child_assert(_counter) && result;
    result = d_tests_sa_test_add_child_invalid_type(_counter) && result;
    result = d_tests_sa_test_add_function_valid(_counter) && result;
    result = d_tests_sa_test_add_function_null(_counter) && result;
    result = d_tests_sa_test_child_count_null(_counter) && result;
    result = d_tests_sa_test_child_count_after_add(_counter) && result;
    result = d_tests_sa_test_get_child_at_valid(_counter) && result;
    result = d_tests_sa_test_get_child_at_null(_counter) && result;

    return result;
}
