#include "./test_common_tests_sa.h"


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
  - all fields can be initialized
  - context can be NULL
  - function pointer can be set
*/
struct d_test_object*
d_tests_sa_tc_test_fn_init
(
    void
)
{
    struct d_test_object* group;
    struct d_test_fn      tf_designated;
    bool                  test_designated;
    bool                  test_fields_set;
    bool                  test_null_ctx;
    bool                  test_fn_set;
    size_t                idx;

    // test designated initializer
    tf_designated = (struct d_test_fn){
        .test_fn = test_helper_simple_test,
        .context = NULL
    };

    test_designated = (tf_designated.test_fn == test_helper_simple_test);
    test_fields_set = (tf_designated.test_fn != NULL);
    test_null_ctx   = (tf_designated.context == NULL);
    test_fn_set     = (tf_designated.test_fn == test_helper_simple_test);

    // build result tree
    group = d_test_standalone_object_new_interior(
                "d_test_fn Initialization", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("designated",
                       test_designated,
                       "can use designated initializer"),
        idx++);
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("fields_set",
                       test_fields_set,
                       "all fields can be initialized"),
        idx++);
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("null_ctx",
                       test_null_ctx,
                       "context can be NULL"),
        idx++);
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("fn_set",
                       test_fn_set,
                       "function pointer can be set"),
        idx++);

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
  - context field is assignable
  - context can hold typed pointer
  - context value is accessible
*/
struct d_test_object*
d_tests_sa_tc_test_fn_fields
(
    void
)
{
    struct d_test_object* group;
    struct d_test_fn      tf;
    int                   ctx_val;
    bool                  test_fn_assign;
    bool                  test_ctx_assign;
    bool                  test_ctx_typed;
    bool                  test_ctx_value;
    size_t                idx;

    // setup context
    ctx_val = 42;

    // test field assignment
    tf.test_fn = test_helper_simple_test;
    tf.context = &ctx_val;

    test_fn_assign  = (tf.test_fn == test_helper_simple_test);
    test_ctx_assign = (tf.context != NULL);
    test_ctx_typed  = (tf.context == &ctx_val);
    test_ctx_value  = (*(int*)tf.context == 42);

    // build result tree
    group = d_test_standalone_object_new_interior(
                "d_test_fn Fields", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("fn_assign",
                       test_fn_assign,
                       "test_fn field is assignable"),
        idx++);
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("ctx_assign",
                       test_ctx_assign,
                       "context field is assignable"),
        idx++);
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("ctx_typed",
                       test_ctx_typed,
                       "context holds typed pointer"),
        idx++);
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("ctx_value",
                       test_ctx_value,
                       "context value is accessible"),
        idx++);

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
    tf.context = NULL;

    // test invocation
    result = tf.test_fn();

    test_invoke = true;  // if we get here, invocation worked
    test_result = (result == D_TEST_PASS);

    // build result tree
    group = d_test_standalone_object_new_interior(
                "d_test_fn Invocation", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("invoke",
                       test_invoke,
                       "can invoke test_fn through structure"),
        idx++);
    d_test_standalone_object_add_child(group,
        D_ASSERT_TRUE("result",
                       test_result,
                       "invocation returns correct result"),
        idx++);

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

    group = d_test_standalone_object_new_interior(
                "d_test_fn Structure", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    d_test_standalone_object_add_child(group,
        d_tests_sa_tc_test_fn_init(), idx++);
    d_test_standalone_object_add_child(group,
        d_tests_sa_tc_test_fn_fields(), idx++);
    d_test_standalone_object_add_child(group,
        d_tests_sa_tc_test_fn_invocation(), idx++);

    return group;
}
