/*******************************************************************************
* djinterp [container]                       buffer_common_tests_sa_validation.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_validation.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_validate_params
  Tests the d_buffer_common_validate_params function.
  Tests the following:
  - zero element_size returns false
  - positive element_size returns true
  - various valid sizes return true
*/
bool
d_tests_sa_buffer_common_validate_params
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: zero element_size
    result = d_assert_standalone(
        d_buffer_common_validate_params(0) == false,
        "validate_params_zero",
        "Zero element_size should return false",
        _counter) && result;

    // test 2: sizeof(int) is valid
    result = d_assert_standalone(
        d_buffer_common_validate_params(sizeof(int)) == true,
        "validate_params_int",
        "sizeof(int) should return true",
        _counter) && result;

    // test 3: sizeof(char) is valid
    result = d_assert_standalone(
        d_buffer_common_validate_params(sizeof(char)) == true,
        "validate_params_char",
        "sizeof(char) should return true",
        _counter) && result;

    // test 4: large size is valid
    result = d_assert_standalone(
        d_buffer_common_validate_params(1024) == true,
        "validate_params_large",
        "Large element_size should return true",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_validate_capacity
  Tests the d_buffer_common_validate_capacity function.
  Tests the following:
  - zero capacity returns false
  - positive capacity returns true
*/
bool
d_tests_sa_buffer_common_validate_capacity
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: zero capacity
    result = d_assert_standalone(
        d_buffer_common_validate_capacity(0) == false,
        "validate_capacity_zero",
        "Zero capacity should return false",
        _counter) && result;

    // test 2: positive capacity
    result = d_assert_standalone(
        d_buffer_common_validate_capacity(1) == true,
        "validate_capacity_one",
        "Capacity=1 should return true",
        _counter) && result;

    // test 3: large capacity
    result = d_assert_standalone(
        d_buffer_common_validate_capacity(100000) == true,
        "validate_capacity_large",
        "Large capacity should return true",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_validate_index
  Tests the d_buffer_common_validate_index function.
  Tests the following:
  - index within bounds returns true
  - index at boundary returns false
  - index beyond bounds returns false
  - zero count always returns false
  - first and last valid indices
*/
bool
d_tests_sa_buffer_common_validate_index
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: index 0 with count 5 is valid
    result = d_assert_standalone(
        d_buffer_common_validate_index(0, 5) == true,
        "validate_index_first",
        "Index 0, count 5 should return true",
        _counter) && result;

    // test 2: last valid index
    result = d_assert_standalone(
        d_buffer_common_validate_index(4, 5) == true,
        "validate_index_last",
        "Index 4, count 5 should return true",
        _counter) && result;

    // test 3: index at boundary (equal to count)
    result = d_assert_standalone(
        d_buffer_common_validate_index(5, 5) == false,
        "validate_index_boundary",
        "Index 5, count 5 should return false",
        _counter) && result;

    // test 4: index beyond bounds
    result = d_assert_standalone(
        d_buffer_common_validate_index(10, 5) == false,
        "validate_index_beyond",
        "Index 10, count 5 should return false",
        _counter) && result;

    // test 5: zero count
    result = d_assert_standalone(
        d_buffer_common_validate_index(0, 0) == false,
        "validate_index_zero_count",
        "Any index with count 0 should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_buffer_common_validation_all
  Aggregation function that runs all validation tests.
*/
bool
d_tests_sa_buffer_common_validation_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Validation Functions\n");
    printf("  -------------------------------\n");

    result = d_tests_sa_buffer_common_validate_params(_counter) && result;
    result = d_tests_sa_buffer_common_validate_capacity(_counter) && result;
    result = d_tests_sa_buffer_common_validate_index(_counter) && result;

    return result;
}
