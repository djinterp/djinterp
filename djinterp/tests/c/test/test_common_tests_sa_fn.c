#include ".\test_common_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

/*
test_helper_passing_test
  A test function that always passes.
*/
static bool
test_helper_passing_test
(
    void
)
{
    return D_TEST_PASS;
}

/*
test_helper_failing_test
  A test function that always fails.
*/
static bool
test_helper_failing_test
(
    void
)
{
    return D_TEST_FAIL;
}

/*
test_helper_stage_hook
  A stage hook function that always succeeds.
*/
static bool
test_helper_stage_hook
(
    struct d_test* _test
)
{
    (void)_test;

    return D_SUCCESS;
}


/******************************************************************************
 * fn_test TYPE TESTS
 *****************************************************************************/

/*
d_tests_sa_tc_fn_test_basic
  Tests fn_test function pointer type basic usage.
  Tests the following:
  - can assign passing test function
  - can assign failing test function
  - can call assigned function
  - returns correct result
*/
struct d_test_object*
d_tests_sa_tc_fn_test_basic
(
    void
)
{
    struct d_test_object* group;
    fn_test               pass_fn;
    fn_test               fail_fn;
    bool                  test_assign_pass;
    bool                  test_assign_fail;
    bool                  test_call_pass;
    bool                  test_call_fail;
    size_t                idx;

    // test assignment
    pass_fn = test_helper_passing_test;
    fail_fn = test_helper_failing_test;

    test_assign_pass = (pass_fn == test_helper_passing_test);
    test_assign_fail = (fail_fn == test_helper_failing_test);

    // test calling
    test_call_pass = (pass_fn() == D_TEST_PASS);
    test_call_fail = (fail_fn() == D_TEST_FAIL);

    // build result tree
    group = d_test_object_new_interior("fn_test Basic", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("assign_pass",
                                           test_assign_pass,
                                           "can assign passing test function");
    group->elements[idx++] = D_ASSERT_TRUE("assign_fail",
                                           test_assign_fail,
                                           "can assign failing test function");
    group->elements[idx++] = D_ASSERT_TRUE("call_pass",
                                           test_call_pass,
                                           "returns D_TEST_PASS correctly");
    group->elements[idx++] = D_ASSERT_TRUE("call_fail",
                                           test_call_fail,
                                           "returns D_TEST_FAIL correctly");

    return group;
}


/*
d_tests_sa_tc_fn_test_null
  Tests fn_test function pointer NULL handling.
  Tests the following:
  - can be assigned NULL
  - NULL check works correctly
*/
struct d_test_object*
d_tests_sa_tc_fn_test_null
(
    void
)
{
    struct d_test_object* group;
    fn_test               null_fn;
    bool                  test_assign_null;
    bool                  test_check_null;
    size_t                idx;

    // test NULL assignment
    null_fn = NULL;

    test_assign_null = (null_fn == NULL);
    test_check_null  = (null_fn == NULL);

    // build result tree
    group = d_test_object_new_interior("fn_test NULL", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("assign_null",
                                           test_assign_null,
                                           "can be assigned NULL");
    group->elements[idx++] = D_ASSERT_TRUE("check_null",
                                           test_check_null,
                                           "NULL check works correctly");

    return group;
}


/******************************************************************************
 * fn_stage TYPE TESTS
 *****************************************************************************/

/*
d_tests_sa_tc_fn_stage_basic
  Tests fn_stage function pointer type basic usage.
  Tests the following:
  - can assign stage hook function
  - can call with NULL parameter
  - returns correct result
*/
struct d_test_object*
d_tests_sa_tc_fn_stage_basic
(
    void
)
{
    struct d_test_object* group;
    fn_stage              stage_fn;
    bool                  test_assign;
    bool                  test_call;
    bool                  test_result;
    size_t                idx;

    // test assignment
    stage_fn = test_helper_stage_hook;
    test_assign = (stage_fn == test_helper_stage_hook);

    // test calling with NULL
    test_call = true;  // if we get here without crash, call works
    test_result = (stage_fn(NULL) == D_SUCCESS);

    // build result tree
    group = d_test_object_new_interior("fn_stage Basic", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("assign",
                                           test_assign,
                                           "can assign stage hook function");
    group->elements[idx++] = D_ASSERT_TRUE("call",
                                           test_call,
                                           "can call with NULL parameter");
    group->elements[idx++] = D_ASSERT_TRUE("result",
                                           test_result,
                                           "returns D_SUCCESS correctly");

    return group;
}


/*
d_tests_sa_tc_fn_stage_null
  Tests fn_stage function pointer NULL handling.
  Tests the following:
  - can be assigned NULL
  - NULL check works correctly
*/
struct d_test_object*
d_tests_sa_tc_fn_stage_null
(
    void
)
{
    struct d_test_object* group;
    fn_stage              null_fn;
    bool                  test_assign_null;
    bool                  test_check_null;
    size_t                idx;

    // test NULL assignment
    null_fn = NULL;

    test_assign_null = (null_fn == NULL);
    test_check_null  = (null_fn == NULL);

    // build result tree
    group = d_test_object_new_interior("fn_stage NULL", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("assign_null",
                                           test_assign_null,
                                           "can be assigned NULL");
    group->elements[idx++] = D_ASSERT_TRUE("check_null",
                                           test_check_null,
                                           "NULL check works correctly");

    return group;
}


/******************************************************************************
 * FUNCTION POINTER MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_tc_fn_all
  Runs all function pointer tests.
  Tests the following:
  - fn_test basic usage
  - fn_test NULL handling
  - fn_stage basic usage
  - fn_stage NULL handling
*/
struct d_test_object*
d_tests_sa_tc_fn_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Function Pointer Types", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_tc_fn_test_basic();
    group->elements[idx++] = d_tests_sa_tc_fn_test_null();
    group->elements[idx++] = d_tests_sa_tc_fn_stage_basic();
    group->elements[idx++] = d_tests_sa_tc_fn_stage_null();

    return group;
}
