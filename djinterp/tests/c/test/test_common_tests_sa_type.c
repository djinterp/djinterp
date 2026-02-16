#include ".\test_common_tests_sa.h"


/******************************************************************************
 * HELPER TEST FUNCTIONS FOR fn_test AND fn_stage
 *****************************************************************************/

/*
helper_test_fn_always_pass
  Helper test function that always returns D_TEST_PASS.
  Used to verify fn_test function pointer type works correctly.

Return:
  D_TEST_PASS
*/
static bool
helper_test_fn_always_pass
(
    void
)
{
    return D_TEST_PASS;
}


/*
helper_test_fn_always_fail
  Helper test function that always returns D_TEST_FAIL.
  Used to verify fn_test function pointer type works correctly.

Return:
  D_TEST_FAIL
*/
static bool
helper_test_fn_always_fail
(
    void
)
{
    return D_TEST_FAIL;
}


/*
helper_stage_fn_success
  Helper stage function that always returns success.
  Used to verify fn_stage function pointer type works correctly.

Parameter(s):
  _test: pointer to test structure (unused in this helper)
Return:
  true (success)
*/
static bool
helper_stage_fn_success
(
    struct d_test* _test
)
{
    // suppress unused parameter warning
    (void)_test;

    return true;
}


/*
helper_stage_fn_failure
  Helper stage function that always returns failure.
  Used to verify fn_stage function pointer type works correctly.

Parameter(s):
  _test: pointer to test structure (unused in this helper)
Return:
  false (failure)
*/
static bool
helper_stage_fn_failure
(
    struct d_test* _test
)
{
    // suppress unused parameter warning
    (void)_test;

    return false;
}


/******************************************************************************
 * II. TYPE DEFINITION TESTS
 *****************************************************************************/

/*
d_tests_sa_test_common_test_id_type
  Tests the d_test_id typedef.
  Tests the following:
  - d_test_id is typedef'd to unsigned long long
  - d_test_id can hold large values
  - d_test_id has correct size (at least 8 bytes)
  - d_test_id is unsigned (no negative values)
  - d_test_id supports basic arithmetic operations
*/
bool
d_tests_sa_test_common_test_id_type
(
    struct d_test_counter* _counter
)
{
    bool      result;
    d_test_id id1;
    d_test_id id2;
    d_test_id max_val;

    result = true;

    // test 1: d_test_id has correct minimum size (at least 8 bytes for ULL)
    result = d_assert_standalone(
        sizeof(d_test_id) >= 8,
        "test_id_size",
        "d_test_id should be at least 8 bytes (unsigned long long)",
        _counter) && result;

    // test 2: d_test_id equals sizeof(unsigned long long)
    result = d_assert_standalone(
        sizeof(d_test_id) == sizeof(unsigned long long),
        "test_id_size_exact",
        "d_test_id should be exactly sizeof(unsigned long long)",
        _counter) && result;

    // test 3: d_test_id can hold zero
    id1    = 0;
    result = d_assert_standalone(
        id1 == 0,
        "test_id_zero",
        "d_test_id should be able to hold zero",
        _counter) && result;

    // test 4: d_test_id can hold large values
    id1    = 0xFFFFFFFFFFFFFFFFULL;
    result = d_assert_standalone(
        id1 == 0xFFFFFFFFFFFFFFFFULL,
        "test_id_max_value",
        "d_test_id should hold maximum unsigned long long value",
        _counter) && result;

    // test 5: d_test_id is unsigned (wraps on underflow)
    id1    = 0;
    id1    = id1 - 1;
    max_val = 0xFFFFFFFFFFFFFFFFULL;
    result = d_assert_standalone(
        id1 == max_val,
        "test_id_unsigned",
        "d_test_id should be unsigned (0-1 wraps to max)",
        _counter) && result;

    // test 6: d_test_id supports basic arithmetic
    id1    = 100;
    id2    = 200;
    result = d_assert_standalone(
        (id1 + id2) == 300,
        "test_id_addition",
        "d_test_id should support addition",
        _counter) && result;

    result = d_assert_standalone(
        (id2 - id1) == 100,
        "test_id_subtraction",
        "d_test_id should support subtraction",
        _counter) && result;

    result = d_assert_standalone(
        (id1 * 2) == 200,
        "test_id_multiplication",
        "d_test_id should support multiplication",
        _counter) && result;

    // test 7: d_test_id supports comparison
    id1    = 100;
    id2    = 200;
    result = d_assert_standalone(
        (id1 < id2) && (id2 > id1) && (id1 != id2),
        "test_id_comparison",
        "d_test_id should support comparison operators",
        _counter) && result;

    // test 8: d_test_id can be used as array index (implicitly converts)
    id1 = 5;
    {
        int  arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        bool idx_ok  = (arr[id1] == 5);

        result = d_assert_standalone(
            idx_ok,
            "test_id_as_index",
            "d_test_id should be usable as array index",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_test_common_fn_test_type
  Tests the fn_test function pointer typedef.
  Tests the following:
  - fn_test is a valid function pointer type
  - fn_test can store function addresses
  - fn_test functions can be called and return bool
  - fn_test can be NULL
  - fn_test function calls return D_TEST_PASS or D_TEST_FAIL
*/
bool
d_tests_sa_test_common_fn_test_type
(
    struct d_test_counter* _counter
)
{
    bool    result;
    fn_test test_func;
    bool    call_result;

    result = true;

    // test 1: fn_test has correct size (pointer size)
    result = d_assert_standalone(
        sizeof(fn_test) == sizeof(void*),
        "fn_test_size",
        "fn_test should be pointer-sized",
        _counter) && result;

    // test 2: fn_test can be NULL
    test_func = NULL;
    result    = d_assert_standalone(
        test_func == NULL,
        "fn_test_null",
        "fn_test should be assignable to NULL",
        _counter) && result;

    // test 3: fn_test can store function that returns PASS
    test_func   = helper_test_fn_always_pass;
    result      = d_assert_standalone(
        test_func != NULL,
        "fn_test_assign_pass",
        "fn_test should store pass function address",
        _counter) && result;

    // test 4: fn_test function can be called
    call_result = test_func();
    result      = d_assert_standalone(
        call_result == D_TEST_PASS,
        "fn_test_call_pass",
        "fn_test pass function should return D_TEST_PASS",
        _counter) && result;

    // test 5: fn_test can store function that returns FAIL
    test_func   = helper_test_fn_always_fail;
    call_result = test_func();
    result      = d_assert_standalone(
        call_result == D_TEST_FAIL,
        "fn_test_call_fail",
        "fn_test fail function should return D_TEST_FAIL",
        _counter) && result;

    // test 6: fn_test functions can be swapped
    {
        fn_test func_a = helper_test_fn_always_pass;
        fn_test func_b = helper_test_fn_always_fail;
        fn_test temp;

        temp   = func_a;
        func_a = func_b;
        func_b = temp;

        result = d_assert_standalone(
            func_a() == D_TEST_FAIL && func_b() == D_TEST_PASS,
            "fn_test_swap",
            "fn_test pointers should be swappable",
            _counter) && result;
    }

    // test 7: fn_test can be compared
    {
        fn_test func_a = helper_test_fn_always_pass;
        fn_test func_b = helper_test_fn_always_pass;
        fn_test func_c = helper_test_fn_always_fail;

        result = d_assert_standalone(
            (func_a == func_b) && (func_a != func_c),
            "fn_test_comparison",
            "fn_test pointers should be comparable",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_test_common_fn_stage_type
  Tests the fn_stage function pointer typedef.
  Tests the following:
  - fn_stage is a valid function pointer type
  - fn_stage can store function addresses
  - fn_stage functions can be called with d_test* parameter
  - fn_stage can be NULL
  - fn_stage functions return bool
*/
bool
d_tests_sa_test_common_fn_stage_type
(
    struct d_test_counter* _counter
)
{
    bool     result;
    fn_stage stage_func;
    bool     call_result;

    result = true;

    // test 1: fn_stage has correct size (pointer size)
    result = d_assert_standalone(
        sizeof(fn_stage) == sizeof(void*),
        "fn_stage_size",
        "fn_stage should be pointer-sized",
        _counter) && result;

    // test 2: fn_stage can be NULL
    stage_func = NULL;
    result     = d_assert_standalone(
        stage_func == NULL,
        "fn_stage_null",
        "fn_stage should be assignable to NULL",
        _counter) && result;

    // test 3: fn_stage can store function that returns success
    stage_func = helper_stage_fn_success;
    result     = d_assert_standalone(
        stage_func != NULL,
        "fn_stage_assign_success",
        "fn_stage should store success function address",
        _counter) && result;

    // test 4: fn_stage function can be called with NULL test pointer
    call_result = stage_func(NULL);
    result      = d_assert_standalone(
        call_result == true,
        "fn_stage_call_success",
        "fn_stage success function should return true",
        _counter) && result;

    // test 5: fn_stage can store function that returns failure
    stage_func  = helper_stage_fn_failure;
    call_result = stage_func(NULL);
    result      = d_assert_standalone(
        call_result == false,
        "fn_stage_call_failure",
        "fn_stage failure function should return false",
        _counter) && result;

    // test 6: fn_stage functions can be swapped
    {
        fn_stage func_a = helper_stage_fn_success;
        fn_stage func_b = helper_stage_fn_failure;
        fn_stage temp;

        temp   = func_a;
        func_a = func_b;
        func_b = temp;

        result = d_assert_standalone(
            func_a(NULL) == false && func_b(NULL) == true,
            "fn_stage_swap",
            "fn_stage pointers should be swappable",
            _counter) && result;
    }

    // test 7: fn_stage can be compared
    {
        fn_stage func_a = helper_stage_fn_success;
        fn_stage func_b = helper_stage_fn_success;
        fn_stage func_c = helper_stage_fn_failure;

        result = d_assert_standalone(
            (func_a == func_b) && (func_a != func_c),
            "fn_stage_comparison",
            "fn_stage pointers should be comparable",
            _counter) && result;
    }

    // test 8: fn_stage differs from fn_test in signature
    // fn_stage takes a struct d_test* parameter while fn_test takes nothing
    result = d_assert_standalone(
        sizeof(fn_stage) == sizeof(fn_test),
        "fn_stage_fn_test_same_size",
        "fn_stage and fn_test should have same size (both are pointers)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_common_type_all
  Aggregation function that runs all type definition tests.
*/
bool
d_tests_sa_test_common_type_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Type Definitions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_test_common_test_id_type(_counter) && result;
    result = d_tests_sa_test_common_fn_test_type(_counter) && result;
    result = d_tests_sa_test_common_fn_stage_type(_counter) && result;

    return result;
}
