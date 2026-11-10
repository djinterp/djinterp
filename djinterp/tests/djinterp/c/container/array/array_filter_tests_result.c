/*******************************************************************************
* djinterp [container]                               array_filter_tests_result.c
*
*
* path:      /tests/djinterp/c/container/array/array_filter_tests_result.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.20
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "array_filter_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. ARRAY FILTER RESULT STRUCTURE TESTS
 *****************************************************************************/

/*
d_tests_array_filter_result_struct
  Tests the dj_contiguous_filter_result structure.
  Tests the following:
  - data member is accessible and assignable
  - count member is accessible and assignable
  - element_size member is accessible and assignable
  - source_indices member is accessible and assignable
  - status member is accessible and assignable
  - zero-initialized struct has expected default values
*/
bool d_tests_array_filter_result_struct(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    struct dj_contiguous_filter_result res;

    d_test_suite_test(_suite, "array_filter_result_struct");

    result = true;

    // test 1: data member is accessible
    res.data = NULL;

    result = D_TEST_CHECK_MSG(_suite,
                              "result_data_accessible",
                              res.data == NULL,
                              "data member should be accessible and nullable") && result;

    // test 2: count member is accessible
    res.count = 42;

    result = D_TEST_CHECK_MSG(_suite,
                              "result_count_accessible",
                              res.count == 42,
                              "count member should be accessible") && result;

    // test 3: element_size member is accessible
    res.element_size = sizeof(int);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_element_size_accessible",
                              res.element_size == sizeof(int),
                              "element_size member should store sizeof(int)") && result;

    // test 4: source_indices member is accessible
    res.source_indices = NULL;

    result = D_TEST_CHECK_MSG(_suite,
                              "result_source_indices_accessible",
                              res.source_indices == NULL,
                              "source_indices member should be accessible and nullable") && result;

    // test 5: status member is accessible
    res.status = D_FILTER_RESULT_SUCCESS;

    result = D_TEST_CHECK_MSG(_suite,
                              "result_status_accessible",
                              res.status == D_FILTER_RESULT_SUCCESS,
                              "status member should store D_FILTER_RESULT_SUCCESS") && result;

    // test 6: zero-initialized struct
    d_memset(&res, 0, sizeof(res));

    result = D_TEST_CHECK_MSG(_suite,
                              "result_zero_init",
                              ( (res.data == NULL) && (res.count == 0) && (res.element_size == 0) ),
                              "Zero-initialized result should have NULL data and zero counts") && result;

    return result;
}


/*
d_tests_array_filter_result_status_enum
  Tests the dj_filter_result_type enum values.
  Tests the following:
  - D_FILTER_RESULT_SUCCESS is 0
  - D_FILTER_RESULT_EMPTY is positive
  - D_FILTER_RESULT_ERROR is negative
  - D_FILTER_RESULT_INVALID is negative and distinct from ERROR
  - D_FILTER_RESULT_NO_MEMORY is negative and distinct from ERROR/INVALID
  - All enum values are unique
*/
bool d_tests_array_filter_result_status_enum(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "array_filter_result_status_enum");

    result = true;

    // test 1: SUCCESS is 0
    result = D_TEST_CHECK_MSG(_suite,
                              "enum_success_is_zero",
                              D_FILTER_RESULT_SUCCESS == 0,
                              "D_FILTER_RESULT_SUCCESS should be 0") && result;

    // test 2: EMPTY is positive (non-error, but no results)
    result = D_TEST_CHECK_MSG(_suite,
                              "enum_empty_is_positive",
                              D_FILTER_RESULT_EMPTY > 0,
                              "D_FILTER_RESULT_EMPTY should be positive") && result;

    // test 3: ERROR is negative
    result = D_TEST_CHECK_MSG(_suite,
                              "enum_error_is_negative",
                              D_FILTER_RESULT_ERROR < 0,
                              "D_FILTER_RESULT_ERROR should be negative") && result;

    // test 4: INVALID is negative and distinct from ERROR
    result = D_TEST_CHECK_MSG(_suite,
                              "enum_invalid_distinct",
                              ( (D_FILTER_RESULT_INVALID < 0) && (D_FILTER_RESULT_INVALID != D_FILTER_RESULT_ERROR) ),
                              "D_FILTER_RESULT_INVALID should be negative and distinct from ERROR") && result;

    // test 5: NO_MEMORY is negative and distinct from ERROR and INVALID
    result = D_TEST_CHECK_MSG(_suite,
                              "enum_no_memory_distinct",
                              ( (D_FILTER_RESULT_NO_MEMORY < 0) && (D_FILTER_RESULT_NO_MEMORY != D_FILTER_RESULT_ERROR) && (D_FILTER_RESULT_NO_MEMORY != D_FILTER_RESULT_INVALID) ),
                              "D_FILTER_RESULT_NO_MEMORY should be negative and unique") && result;

    // test 6: all five values are unique
    result = D_TEST_CHECK_MSG(_suite,
                              "enum_all_unique",
                              ( (D_FILTER_RESULT_SUCCESS != D_FILTER_RESULT_EMPTY) && (D_FILTER_RESULT_SUCCESS != D_FILTER_RESULT_ERROR) && (D_FILTER_RESULT_SUCCESS != D_FILTER_RESULT_INVALID) && (D_FILTER_RESULT_SUCCESS != D_FILTER_RESULT_NO_MEMORY) && (D_FILTER_RESULT_EMPTY != D_FILTER_RESULT_ERROR) && (D_FILTER_RESULT_EMPTY != D_FILTER_RESULT_INVALID) && (D_FILTER_RESULT_EMPTY != D_FILTER_RESULT_NO_MEMORY) && (D_FILTER_RESULT_ERROR != D_FILTER_RESULT_INVALID) && (D_FILTER_RESULT_ERROR != D_FILTER_RESULT_NO_MEMORY) && (D_FILTER_RESULT_INVALID != D_FILTER_RESULT_NO_MEMORY) ),
                              "All dj_filter_result_type values should be unique") && result;

    return result;
}


/*
d_tests_array_filter_result_all
  Aggregation function that runs all result structure tests.
*/
bool d_tests_array_filter_result_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "array_filter_result_all");

    result = true;

    result = d_tests_array_filter_result_struct(_suite) && result;
    result = d_tests_array_filter_result_status_enum(_suite) && result;

    return result;
}
