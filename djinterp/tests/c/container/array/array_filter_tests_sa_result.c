#include ".\array_filter_tests_sa.h"


/******************************************************************************
 * I. ARRAY FILTER RESULT STRUCTURE TESTS
 *****************************************************************************/

/*
d_tests_sa_array_filter_result_struct
  Tests the d_array_filter_result structure.
  Tests the following:
  - data member is accessible and assignable
  - count member is accessible and assignable
  - element_size member is accessible and assignable
  - source_indices member is accessible and assignable
  - status member is accessible and assignable
  - zero-initialized struct has expected default values
*/
bool
d_tests_sa_array_filter_result_struct
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    struct d_array_filter_result res;

    result = true;

    // test 1: data member is accessible
    res.data = NULL;

    result = d_assert_standalone(
        res.data == NULL,
        "result_data_accessible",
        "data member should be accessible and nullable",
        _counter) && result;

    // test 2: count member is accessible
    res.count = 42;

    result = d_assert_standalone(
        res.count == 42,
        "result_count_accessible",
        "count member should be accessible",
        _counter) && result;

    // test 3: element_size member is accessible
    res.element_size = sizeof(int);

    result = d_assert_standalone(
        res.element_size == sizeof(int),
        "result_element_size_accessible",
        "element_size member should store sizeof(int)",
        _counter) && result;

    // test 4: source_indices member is accessible
    res.source_indices = NULL;

    result = d_assert_standalone(
        res.source_indices == NULL,
        "result_source_indices_accessible",
        "source_indices member should be accessible and nullable",
        _counter) && result;

    // test 5: status member is accessible
    res.status = D_FILTER_RESULT_SUCCESS;

    result = d_assert_standalone(
        res.status == D_FILTER_RESULT_SUCCESS,
        "result_status_accessible",
        "status member should store D_FILTER_RESULT_SUCCESS",
        _counter) && result;

    // test 6: zero-initialized struct
    d_memset(&res, 0, sizeof(res));

    result = d_assert_standalone(
        ( (res.data == NULL)     &&
          (res.count == 0)       &&
          (res.element_size == 0) ),
        "result_zero_init",
        "Zero-initialized result should have NULL data and zero counts",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_result_status_enum
  Tests the d_filter_result_type enum values.
  Tests the following:
  - D_FILTER_RESULT_SUCCESS is 0
  - D_FILTER_RESULT_EMPTY is positive
  - D_FILTER_RESULT_ERROR is negative
  - D_FILTER_RESULT_INVALID is negative and distinct from ERROR
  - D_FILTER_RESULT_NO_MEMORY is negative and distinct from ERROR/INVALID
  - All enum values are unique
*/
bool
d_tests_sa_array_filter_result_status_enum
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: SUCCESS is 0
    result = d_assert_standalone(
        D_FILTER_RESULT_SUCCESS == 0,
        "enum_success_is_zero",
        "D_FILTER_RESULT_SUCCESS should be 0",
        _counter) && result;

    // test 2: EMPTY is positive (non-error, but no results)
    result = d_assert_standalone(
        D_FILTER_RESULT_EMPTY > 0,
        "enum_empty_is_positive",
        "D_FILTER_RESULT_EMPTY should be positive",
        _counter) && result;

    // test 3: ERROR is negative
    result = d_assert_standalone(
        D_FILTER_RESULT_ERROR < 0,
        "enum_error_is_negative",
        "D_FILTER_RESULT_ERROR should be negative",
        _counter) && result;

    // test 4: INVALID is negative and distinct from ERROR
    result = d_assert_standalone(
        ( (D_FILTER_RESULT_INVALID < 0) &&
          (D_FILTER_RESULT_INVALID != D_FILTER_RESULT_ERROR) ),
        "enum_invalid_distinct",
        "D_FILTER_RESULT_INVALID should be negative and distinct from ERROR",
        _counter) && result;

    // test 5: NO_MEMORY is negative and distinct from ERROR and INVALID
    result = d_assert_standalone(
        ( (D_FILTER_RESULT_NO_MEMORY < 0)                       &&
          (D_FILTER_RESULT_NO_MEMORY != D_FILTER_RESULT_ERROR)  &&
          (D_FILTER_RESULT_NO_MEMORY != D_FILTER_RESULT_INVALID) ),
        "enum_no_memory_distinct",
        "D_FILTER_RESULT_NO_MEMORY should be negative and unique",
        _counter) && result;

    // test 6: all five values are unique
    result = d_assert_standalone(
        ( (D_FILTER_RESULT_SUCCESS   != D_FILTER_RESULT_EMPTY)     &&
          (D_FILTER_RESULT_SUCCESS   != D_FILTER_RESULT_ERROR)     &&
          (D_FILTER_RESULT_SUCCESS   != D_FILTER_RESULT_INVALID)   &&
          (D_FILTER_RESULT_SUCCESS   != D_FILTER_RESULT_NO_MEMORY) &&
          (D_FILTER_RESULT_EMPTY     != D_FILTER_RESULT_ERROR)     &&
          (D_FILTER_RESULT_EMPTY     != D_FILTER_RESULT_INVALID)   &&
          (D_FILTER_RESULT_EMPTY     != D_FILTER_RESULT_NO_MEMORY) &&
          (D_FILTER_RESULT_ERROR     != D_FILTER_RESULT_INVALID)   &&
          (D_FILTER_RESULT_ERROR     != D_FILTER_RESULT_NO_MEMORY) &&
          (D_FILTER_RESULT_INVALID   != D_FILTER_RESULT_NO_MEMORY) ),
        "enum_all_unique",
        "All d_filter_result_type values should be unique",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_result_all
  Aggregation function that runs all result structure tests.
*/
bool
d_tests_sa_array_filter_result_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Array Filter Result Structure\n");
    printf("  -----------------------------------------\n");

    result = d_tests_sa_array_filter_result_struct(_counter) && result;
    result = d_tests_sa_array_filter_result_status_enum(_counter) && result;

    return result;
}
