#include ".\test_common_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

/*
test_helper_simple_test
  A simple test function for d_test_fn testing.
*/
static bool
test_helper_simple_test
(
    void
)
{
    return D_TEST_PASS;
}


/******************************************************************************
 * d_test_fn INITIALIZATION TESTS
 *****************************************************************************/

/*
d_tests_sa_tc_test_fn_init
  Tests d_test_fn structure initialization.
  Tests the following:
  - can use designated initializer
  - can use compound literal
  - all fields can be initialized
  - default values work correctly
*/
struct d_test_object*
d_tests_sa_tc_test_fn_init
(
    void
)
{
    struct d_test_object* group;
    struct d_test_fn      tf_designated;
    struct d_test_fn      tf_default;
    bool                  test_designated;
    bool                  test_fields_set;
    bool                  test_null_args;
    bool                  test_zero_count;
    size_t                idx;

    // test designated initializer
    tf_designated = (struct d_test_fn){
        .test_fn = test_helper_simple_test,
        .count   = 0,
        .args    = NULL
    };

    test_designated = (tf_designated.test_fn == test_helper_simple_test);
    test_fields_set = (tf_designated.test_fn != NULL);
    test_null_args  = (tf_designated.args == NULL);
    test_zero_count = (tf_designated.count == 0);

    // build result tree
    group = d_test_object_new_interior("d_test_fn Initialization", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("designated",
                                           test_designated,
                                           "can use designated initializer");
    group->elements[idx++] = D_ASSERT_TRUE("fields_set",
                                           test_fields_set,
                                           "all fields can be initialized");
    group->elements[idx++] = D_ASSERT_TRUE("null_args",
                                           test_null_args,
                                           "args can be NULL");
    group->elements[idx++] = D_ASSERT_TRUE("zero_count",
                                           test_zero_count,
                                           "count can be zero");

    return group;
}


/******************************************************************************
 * d_test_fn FIELD TESTS
 *****************************************************************************/

/*
d_tests_sa_tc_test_fn_fields
  Tests d_test_fn structure field assignment.
  Tests the following:
  - test_fn field is assignable
  - count field is assignable
  - args field is assignable
  - args array can hold values
*/
struct d_test_object*
d_tests_sa_tc_test_fn_fields
(
    void
)
{
    struct d_test_object* group;
    struct d_test_fn      tf;
    void*                 args[3];
    bool                  test_fn_assign;
    bool                  test_count_assign;
    bool                  test_args_assign;
    bool                  test_args_values;
    size_t                idx;

    // setup args array
    args[0] = (void*)1;
    args[1] = (void*)2;
    args[2] = (void*)3;

    // test field assignment
    tf.test_fn = test_helper_simple_test;
    tf.count   = 3;
    tf.args    = args;

    test_fn_assign    = (tf.test_fn == test_helper_simple_test);
    test_count_assign = (tf.count == 3);
    test_args_assign  = (tf.args == args);
    test_args_values  = (tf.args[0] == (void*)1) &&
                        (tf.args[1] == (void*)2) &&
                        (tf.args[2] == (void*)3);

    // build result tree
    group = d_test_object_new_interior("d_test_fn Fields", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("fn_assign",
                                           test_fn_assign,
                                           "test_fn field is assignable");
    group->elements[idx++] = D_ASSERT_TRUE("count_assign",
                                           test_count_assign,
                                           "count field is assignable");
    group->elements[idx++] = D_ASSERT_TRUE("args_assign",
                                           test_args_assign,
                                           "args field is assignable");
    group->elements[idx++] = D_ASSERT_TRUE("args_values",
                                           test_args_values,
                                           "args array holds values correctly");

    return group;
}


/******************************************************************************
 * d_test_fn INVOCATION TESTS
 *****************************************************************************/

/*
d_tests_sa_tc_test_fn_invocation
  Tests d_test_fn function invocation.
  Tests the following:
  - can invoke test_fn through structure
  - invocation returns correct result
*/
struct d_test_object*
d_tests_sa_tc_test_fn_invocation
(
    void
)
{
    struct d_test_object* group;
    struct d_test_fn      tf;
    bool                  result;
    bool                  test_invoke;
    bool                  test_result;
    size_t                idx;

    // setup test_fn
    tf.test_fn = test_helper_simple_test;
    tf.count   = 0;
    tf.args    = NULL;

    // test invocation
    result = tf.test_fn();

    test_invoke = true;  // if we get here, invocation worked
    test_result = (result == D_TEST_PASS);

    // build result tree
    group = d_test_object_new_interior("d_test_fn Invocation", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("invoke",
                                           test_invoke,
                                           "can invoke test_fn through structure");
    group->elements[idx++] = D_ASSERT_TRUE("result",
                                           test_result,
                                           "invocation returns correct result");

    return group;
}


/******************************************************************************
 * d_test_fn MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_tc_test_fn_all
  Runs all d_test_fn structure tests.
  Tests the following:
  - initialization
  - field assignment
  - invocation
*/
struct d_test_object*
d_tests_sa_tc_test_fn_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("d_test_fn Structure", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_tc_test_fn_init();
    group->elements[idx++] = d_tests_sa_tc_test_fn_fields();
    group->elements[idx++] = d_tests_sa_tc_test_fn_invocation();

    return group;
}
