#include ".\test_common_tests_sa.h"


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
  - struct d_test_fn has expected size
  - test_fn member is fn_test function pointer
  - count member is size_t
  - args member is void** (array of void pointers)
  - struct can be initialized with no arguments
  - struct can be initialized with arguments
  - struct can represent various test configurations
*/
bool
d_tests_sa_test_common_test_fn
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_test_fn test_fn_struct;
    void*            args_array[4];
    int              int_args[4];
    bool             call_result;

    result = true;

    // test 1: struct size is at least fn_test + size_t + void**
    result = d_assert_standalone(
        sizeof(struct d_test_fn) >= (sizeof(fn_test) + sizeof(size_t) + sizeof(void**)),
        "test_fn_size",
        "d_test_fn should be at least fn_test + size_t + void** size",
        _counter) && result;

    // test 2: test_fn member is accessible
    test_fn_struct.test_fn = NULL;
    result                 = d_assert_standalone(
        test_fn_struct.test_fn == NULL,
        "test_fn_member_accessible",
        "d_test_fn.test_fn should be accessible",
        _counter) && result;

    // test 3: count member is accessible
    test_fn_struct.count = 0;
    result               = d_assert_standalone(
        test_fn_struct.count == 0,
        "test_fn_count_accessible",
        "d_test_fn.count should be accessible",
        _counter) && result;

    // test 4: args member is accessible
    test_fn_struct.args = NULL;
    result              = d_assert_standalone(
        test_fn_struct.args == NULL,
        "test_fn_args_accessible",
        "d_test_fn.args should be accessible",
        _counter) && result;

    // test 5: test_fn can store function pointer
    test_fn_struct.test_fn = helper_test_fn_for_wrapper;
    result                 = d_assert_standalone(
        test_fn_struct.test_fn != NULL,
        "test_fn_store_function",
        "d_test_fn.test_fn should store function pointer",
        _counter) && result;

    // test 6: test_fn function can be called
    call_result = test_fn_struct.test_fn();
    result      = d_assert_standalone(
        call_result == D_TEST_PASS,
        "test_fn_call_function",
        "d_test_fn.test_fn should be callable",
        _counter) && result;

    // test 7: struct with no arguments
    test_fn_struct.test_fn = helper_test_fn_for_wrapper;
    test_fn_struct.count   = 0;
    test_fn_struct.args    = NULL;

    result = d_assert_standalone(
        (test_fn_struct.test_fn != NULL) &&
        (test_fn_struct.count == 0) &&
        (test_fn_struct.args == NULL),
        "test_fn_no_args",
        "d_test_fn should represent test with no arguments",
        _counter) && result;

    // test 8: struct with arguments
    int_args[0]   = 10;
    int_args[1]   = 20;
    int_args[2]   = 30;
    int_args[3]   = 40;
    args_array[0] = &int_args[0];
    args_array[1] = &int_args[1];
    args_array[2] = &int_args[2];
    args_array[3] = &int_args[3];

    test_fn_struct.test_fn = helper_test_fn_return_fail;
    test_fn_struct.count   = 4;
    test_fn_struct.args    = args_array;

    result = d_assert_standalone(
        test_fn_struct.count == 4,
        "test_fn_with_args_count",
        "d_test_fn should store argument count",
        _counter) && result;

    result = d_assert_standalone(
        test_fn_struct.args != NULL,
        "test_fn_with_args_array",
        "d_test_fn should store args array pointer",
        _counter) && result;

    // test 9: args array elements are accessible
    result = d_assert_standalone(
        *(int*)test_fn_struct.args[0] == 10,
        "test_fn_args_first",
        "d_test_fn should access first arg",
        _counter) && result;

    result = d_assert_standalone(
        *(int*)test_fn_struct.args[3] == 40,
        "test_fn_args_last",
        "d_test_fn should access last arg",
        _counter) && result;

    // test 10: iteration through args
    {
        size_t i;
        int    sum;
        bool   iter_ok;

        sum     = 0;
        iter_ok = true;

        for (i = 0; i < test_fn_struct.count; i++)
        {
            if (test_fn_struct.args[i] != NULL)
            {
                sum += *(int*)test_fn_struct.args[i];
            }
            else
            {
                iter_ok = false;
            }
        }

        result = d_assert_standalone(
            iter_ok && (sum == 100),
            "test_fn_args_iteration",
            "d_test_fn should support args iteration (sum=100)",
            _counter) && result;
    }

    // test 11: struct can be copied (shallow)
    {
        struct d_test_fn test_fn_copy;

        test_fn_copy = test_fn_struct;

        result = d_assert_standalone(
            (test_fn_copy.test_fn == test_fn_struct.test_fn) &&
            (test_fn_copy.count == test_fn_struct.count) &&
            (test_fn_copy.args == test_fn_struct.args),
            "test_fn_copy",
            "d_test_fn should be copyable (shallow)",
            _counter) && result;
    }

    // test 12: count type is size_t
    test_fn_struct.count = SIZE_MAX;
    result               = d_assert_standalone(
        test_fn_struct.count == SIZE_MAX,
        "test_fn_count_type",
        "d_test_fn.count should be size_t (hold SIZE_MAX)",
        _counter) && result;

    // test 13: different test functions can be assigned
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

    // test 14: args can contain NULL elements
    args_array[1] = NULL;
    test_fn_struct.args = args_array;

    result = d_assert_standalone(
        test_fn_struct.args[1] == NULL,
        "test_fn_args_null_element",
        "d_test_fn.args should allow NULL elements",
        _counter) && result;

    // test 15: compound literal initialization
    {
        void*            test_args[2];
        int              test_val1;
        int              test_val2;
        struct d_test_fn literal_fn;

        test_val1    = 111;
        test_val2    = 222;
        test_args[0] = &test_val1;
        test_args[1] = &test_val2;

        literal_fn.test_fn = helper_test_fn_for_wrapper;
        literal_fn.count   = 2;
        literal_fn.args    = test_args;

        result = d_assert_standalone(
            (literal_fn.test_fn != NULL) &&
            (literal_fn.count == 2) &&
            (*(int*)literal_fn.args[0] == 111) &&
            (*(int*)literal_fn.args[1] == 222),
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
