/*******************************************************************************
* djinterp [container]                          array_filter_tests_result_mgmt.c
*
*
* path:      /tests/djinterp/c/container/array/array_filter_tests_result_mgmt.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.20
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "array_filter_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * VI. RESULT MANAGEMENT
 *****************************************************************************/

/*
d_tests_array_filter_result_data
  Tests the d_contiguous_filter_result_data function.
  Tests the following:
  - Returns pointer to internal data on valid result
  - Returns NULL for result with no data
  - Returns NULL for NULL result pointer
*/
bool d_tests_array_filter_result_data(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    void*                        ptr;

    d_test_suite_test(_suite, "array_filter_result_data");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: valid result returns data pointer
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    5);
    ptr = d_contiguous_filter_result_data(&res);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_data_valid",
                              ( (ptr != NULL) && (ptr == res.data) ),
                              "result_data should return the internal data pointer") && result;

    d_contiguous_filter_result_free(&res);

    // test 2: empty result returns NULL
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    0);
    ptr = d_contiguous_filter_result_data(&res);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_data_empty",
                              ptr == NULL,
                              "result_data on empty result should return NULL") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: NULL result pointer
    ptr = d_contiguous_filter_result_data(NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_data_null",
                              ptr == NULL,
                              "result_data(NULL) should return NULL") && result;

    return result;
}


/*
d_tests_array_filter_result_count_fn
  Tests the d_contiguous_filter_result_count function.
  Tests the following:
  - Returns correct count on valid result
  - Returns 0 for empty result
  - Returns 0 for NULL result pointer
*/
bool d_tests_array_filter_result_count_fn(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    size_t                       cnt;

    d_test_suite_test(_suite, "array_filter_result_count_fn");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: valid result
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    7);
    cnt = d_contiguous_filter_result_count(&res);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_count_valid",
                              cnt == 7,
                              "result_count should return 7") && result;

    d_contiguous_filter_result_free(&res);

    // test 2: empty result
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    0);
    cnt = d_contiguous_filter_result_count(&res);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_count_empty",
                              cnt == 0,
                              "result_count on empty result should return 0") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: NULL pointer
    cnt = d_contiguous_filter_result_count(NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_count_null",
                              cnt == 0,
                              "result_count(NULL) should return 0") && result;

    return result;
}


/*
d_tests_array_filter_result_ok
  Tests the d_contiguous_filter_result_ok function.
  Tests the following:
  - Returns true for D_FILTER_RESULT_SUCCESS
  - Returns true for D_FILTER_RESULT_EMPTY
  - Returns false for error statuses
  - Returns false for NULL result pointer
*/
bool d_tests_array_filter_result_ok(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;

    d_test_suite_test(_suite, "array_filter_result_ok");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: successful operation -> true
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    3);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_ok_success",
                              d_contiguous_filter_result_ok(&res) == true,
                              "result_ok should be true for successful operation") && result;

    d_contiguous_filter_result_free(&res);

    // test 2: empty result (non-error) -> true
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    0);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_ok_empty",
                              d_contiguous_filter_result_ok(&res) == true,
                              "result_ok should be true for empty (non-error) result") && result;

    d_contiguous_filter_result_free(&res);

    // test 3: error status -> false
    res = d_contiguous_filter_where(NULL,
                               D_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               d_test_af_is_even,
                               NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "result_ok_error",
                              d_contiguous_filter_result_ok(&res) == false,
                              "result_ok should be false for error result") && result;

    d_contiguous_filter_result_free(&res);

    // test 4: NULL pointer -> false
    result = D_TEST_CHECK_MSG(_suite,
                              "result_ok_null",
                              d_contiguous_filter_result_ok(NULL) == false,
                              "result_ok(NULL) should be false") && result;

    return result;
}


/*
d_tests_array_filter_result_release
  Tests the d_contiguous_filter_result_release function.
  Tests the following:
  - Returns data pointer and transfers ownership
  - Sets result data to NULL after release
  - Writes count to out_count
  - NULL result returns NULL
  - NULL out_count is tolerated
*/
bool d_tests_array_filter_result_release(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;
    void*                        released;
    size_t                       out_count;
    int*                         out;

    d_test_suite_test(_suite, "array_filter_result_release");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: release transfers ownership
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    4);

    out_count = 0;
    released = d_contiguous_filter_result_release(&res, &out_count);

    result = D_TEST_CHECK_MSG(_suite,
                              "release_returns_data",
                              released != NULL,
                              "release should return non-NULL data pointer") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "release_out_count",
                              out_count == 4,
                              "release should write 4 to out_count") && result;

    // test 2: result is nulled after release
    result = D_TEST_CHECK_MSG(_suite,
                              "release_nulls_result",
                              res.data == NULL,
                              "result.data should be NULL after release") && result;

    // test 3: verify released data integrity
    out = (int*)released;

    if (out)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "release_data_intact",
                                  ( (out[0] == 0) && (out[3] == 3) ),
                                  "Released data should contain original values") && result;

        // caller must free the released data
        free(released);
    }

    // test 4: NULL result
    released = d_contiguous_filter_result_release(NULL, &out_count);

    result = D_TEST_CHECK_MSG(_suite,
                              "release_null",
                              released == NULL,
                              "release(NULL) should return NULL") && result;

    // test 5: NULL out_count is tolerated
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    2);

    released = d_contiguous_filter_result_release(&res, NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "release_null_out_count",
                              released != NULL,
                              "release with NULL out_count should still return data") && result;

    if (released)
    {
        free(released);
    }

    return result;
}


/*
d_tests_array_filter_result_free
  Tests the d_contiguous_filter_result_free function.
  Tests the following:
  - Frees a valid result without crash
  - NULL pointer is handled safely
  - Double-free is safe (data set to NULL after free)
*/
bool d_tests_array_filter_result_free(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    int                          data[D_TEST_ARRAY_FILTER_DATA_SIZE];
    struct dj_contiguous_filter_result res;

    d_test_suite_test(_suite, "array_filter_result_free");

    result = true;
    d_test_af_fill_sequential(data, D_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: free valid result
    res = d_contiguous_filter_take_first(data,
                                    D_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    5);
    d_contiguous_filter_result_free(&res);

    result = D_TEST_CHECK_MSG(_suite,
                              "free_valid",
                              true,
                              "Freeing valid result should not crash") && result;

    // test 2: result data is NULL after free
    result = D_TEST_CHECK_MSG(_suite,
                              "free_nulls_data",
                              res.data == NULL,
                              "data should be NULL after free") && result;

    // test 3: NULL pointer
    d_contiguous_filter_result_free(NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "free_null_safe",
                              true,
                              "result_free(NULL) should not crash") && result;

    // test 4: double-free safety
    d_contiguous_filter_result_free(&res);

    result = D_TEST_CHECK_MSG(_suite,
                              "free_double_safe",
                              true,
                              "Double-free should be safe (data already NULL)") && result;

    return result;
}


/*
d_tests_array_filter_result_mgmt_all
  Aggregation function that runs all result management tests.
*/
bool d_tests_array_filter_result_mgmt_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "array_filter_result_mgmt_all");

    result = true;

    result = d_tests_array_filter_result_data(_suite) && result;
    result = d_tests_array_filter_result_count_fn(_suite) && result;
    result = d_tests_array_filter_result_ok(_suite) && result;
    result = d_tests_array_filter_result_release(_suite) && result;
    result = d_tests_array_filter_result_free(_suite) && result;

    return result;
}
