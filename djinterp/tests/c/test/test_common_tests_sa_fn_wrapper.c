#include "./test_common_tests_sa.h"


/******************************************************************************
 * HELPER TEST FUNCTIONS FOR d_test_fn
 *****************************************************************************/

/*
helper_test_fn_for_wrapper
  Helper test function to use with d_test_fn wrapper tests.

Return:
  D_TEST_PASS
*/
static bool
helper_test_fn_for_wrapper
(
    void
)
{
    return D_TEST_PASS;
}


/*
helper_test_fn_return_fail
  Helper test function that returns fail for wrapper tests.

Return:
  D_TEST_FAIL
*/
static bool
helper_test_fn_return_fail
(
    void
)
{
    return D_TEST_FAIL;
}


/******************************************************************************
 * IV. TEST FUNCTION WRAPPER TESTS
 *****************************************************************************/

/*
d_tests_sa_test_common_test_fn
  Tests the d_test_fn structure.
  Tests the following:
  - struct d_test_fn has expected size (fn_test + void*)
  - test_fn member is fn_test function pointer
  - context member is void*
  - struct can be initialized with no context
  - struct can be initialized with context
  - struct supports function swapping
  - struct can be copied (shallow)
*/
bool
d_tests_sa_test_common_test_fn
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_test_fn test_fn_struct;
    int              ctx_val;
    bool             call_result;

    result = true;

    // test 1: struct size is at least fn_test + void*
    result = d_assert_standalone(
        sizeof(struct d_test_fn) >= (sizeof(fn_test) + sizeof(void*)),
        "test_fn_size",
        "d_test_fn should be at least fn_test + void* size",
        _counter) && result;

    // test 2: test_fn member is accessible
    test_fn_struct.test_fn = NULL;
    result                 = d_assert_standalone(
        test_fn_struct.test_fn == NULL,
        "test_fn_member_accessible",
        "d_test_fn.test_fn should be accessible",
        _counter) && result;

    // test 3: context member is accessible
    test_fn_struct.context = NULL;
    result                 = d_assert_standalone(
        test_fn_struct.context == NULL,
        "test_fn_context_accessible",
        "d_test_fn.context should be accessible",
        _counter) && result;

    // test 4: test_fn can store function pointer
    test_fn_struct.test_fn = helper_test_fn_for_wrapper;
    result                 = d_assert_standalone(
        test_fn_struct.test_fn != NULL,
        "test_fn_store_function",
        "d_test_fn.test_fn should store function pointer",
        _counter) && result;

    // test 5: test_fn function can be called
    call_result = test_fn_struct.test_fn();
    result      = d_assert_standalone(
        call_result == D_TEST_PASS,
        "test_fn_call_function",
        "d_test_fn.test_fn should be callable",
        _counter) && result;

    // test 6: struct with no context
    test_fn_struct.test_fn = helper_test_fn_for_wrapper;
    test_fn_struct.context = NULL;

    result = d_assert_standalone(
        (test_fn_struct.test_fn != NULL) &&
        (test_fn_struct.context == NULL),
        "test_fn_no_context",
        "d_test_fn should represent test with no context",
        _counter) && result;

    // test 7: struct with context
    ctx_val                = 42;
    test_fn_struct.test_fn = helper_test_fn_return_fail;
    test_fn_struct.context = &ctx_val;

    result = d_assert_standalone(
        (test_fn_struct.test_fn != NULL) &&
        (test_fn_struct.context != NULL) &&
        (*(int*)test_fn_struct.context == 42),
        "test_fn_with_context",
        "d_test_fn should store context pointer",
        _counter) && result;

    // test 8: context can hold various types
    {
        double             dbl_ctx;
        char               str_ctx[16];

        dbl_ctx = 3.14;
        test_fn_struct.context = &dbl_ctx;

        result = d_assert_standalone(
            *(double*)test_fn_struct.context == 3.14,
            "test_fn_context_double",
            "d_test_fn.context should hold double pointer",
            _counter) && result;

        d_strcpy_s(str_ctx, sizeof(str_ctx), "hello");
        test_fn_struct.context = str_ctx;

        result = d_assert_standalone(
            d_strcasecmp((char*)test_fn_struct.context, "hello") == 0,
            "test_fn_context_string",
            "d_test_fn.context should hold string pointer",
            _counter) && result;
    }

    // test 9: struct can be copied (shallow)
    {
        struct d_test_fn test_fn_copy;

        test_fn_struct.test_fn = helper_test_fn_for_wrapper;
        test_fn_struct.context = &ctx_val;

        test_fn_copy = test_fn_struct;

        result = d_assert_standalone(
            (test_fn_copy.test_fn == test_fn_struct.test_fn) &&
            (test_fn_copy.context == test_fn_struct.context),
            "test_fn_copy",
            "d_test_fn should be copyable (shallow)",
            _counter) && result;
    }

    // test 10: different test functions can be assigned
    test_fn_struct.test_fn = helper_test_fn_for_wrapper;
    call_result            = test_fn_struct.test_fn();
    result                 = d_assert_standalone(
        call_result == D_TEST_PASS,
        "test_fn_swap_pass",
        "d_test_fn should call pass function",
        _counter) && result;

    test_fn_struct.test_fn = helper_test_fn_return_fail;
    call_result            = test_fn_struct.test_fn();
    result                 = d_assert_standalone(
        call_result == D_TEST_FAIL,
        "test_fn_swap_fail",
        "d_test_fn should call fail function after swap",
        _counter) && result;

    // test 11: compound literal-style initialization
    {
        struct d_test_fn literal_fn;
        int              lit_ctx;

        lit_ctx = 999;

        literal_fn.test_fn = helper_test_fn_for_wrapper;
        literal_fn.context = &lit_ctx;

        result = d_assert_standalone(
            (literal_fn.test_fn != NULL) &&
            (*(int*)literal_fn.context == 999),
            "test_fn_literal_init",
            "d_test_fn should support initialization",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_test_common_fn_wrapper_all
  Aggregation function that runs all test function wrapper tests.
*/
bool
d_tests_sa_test_common_fn_wrapper_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Test Function Wrapper\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_test_common_test_fn(_counter) && result;

    return result;
}
