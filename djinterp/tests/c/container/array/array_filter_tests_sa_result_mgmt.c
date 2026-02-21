#include ".\array_filter_tests_sa.h"


/******************************************************************************
 * VI. RESULT MANAGEMENT
 *****************************************************************************/

/*
d_tests_sa_array_filter_result_data
  Tests the d_array_filter_result_data function.
  Tests the following:
  - Returns pointer to internal data on valid result
  - Returns NULL for result with no data
  - Returns NULL for NULL result pointer
*/
bool
d_tests_sa_array_filter_result_data
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE];
    struct d_array_filter_result res;
    void*                        ptr;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: valid result returns data pointer
    res = d_array_filter_take_first(data,
                                    D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    5);
    ptr = d_array_filter_result_data(&res);

    result = d_assert_standalone(
        ( (ptr != NULL) &&
          (ptr == res.data) ),
        "result_data_valid",
        "result_data should return the internal data pointer",
        _counter) && result;

    d_array_filter_result_free(&res);

    // test 2: empty result returns NULL
    res = d_array_filter_take_first(data,
                                    D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    0);
    ptr = d_array_filter_result_data(&res);

    result = d_assert_standalone(
        ptr == NULL,
        "result_data_empty",
        "result_data on empty result should return NULL",
        _counter) && result;

    d_array_filter_result_free(&res);

    // test 3: NULL result pointer
    ptr = d_array_filter_result_data(NULL);

    result = d_assert_standalone(
        ptr == NULL,
        "result_data_null",
        "result_data(NULL) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_result_count_fn
  Tests the d_array_filter_result_count function.
  Tests the following:
  - Returns correct count on valid result
  - Returns 0 for empty result
  - Returns 0 for NULL result pointer
*/
bool
d_tests_sa_array_filter_result_count_fn
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE];
    struct d_array_filter_result res;
    size_t                       cnt;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: valid result
    res = d_array_filter_take_first(data,
                                    D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    7);
    cnt = d_array_filter_result_count(&res);

    result = d_assert_standalone(
        cnt == 7,
        "result_count_valid",
        "result_count should return 7",
        _counter) && result;

    d_array_filter_result_free(&res);

    // test 2: empty result
    res = d_array_filter_take_first(data,
                                    D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    0);
    cnt = d_array_filter_result_count(&res);

    result = d_assert_standalone(
        cnt == 0,
        "result_count_empty",
        "result_count on empty result should return 0",
        _counter) && result;

    d_array_filter_result_free(&res);

    // test 3: NULL pointer
    cnt = d_array_filter_result_count(NULL);

    result = d_assert_standalone(
        cnt == 0,
        "result_count_null",
        "result_count(NULL) should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_result_ok
  Tests the d_array_filter_result_ok function.
  Tests the following:
  - Returns true for D_FILTER_RESULT_SUCCESS
  - Returns true for D_FILTER_RESULT_EMPTY
  - Returns false for error statuses
  - Returns false for NULL result pointer
*/
bool
d_tests_sa_array_filter_result_ok
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE];
    struct d_array_filter_result res;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: successful operation -> true
    res = d_array_filter_take_first(data,
                                    D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    3);

    result = d_assert_standalone(
        d_array_filter_result_ok(&res) == true,
        "result_ok_success",
        "result_ok should be true for successful operation",
        _counter) && result;

    d_array_filter_result_free(&res);

    // test 2: empty result (non-error) -> true
    res = d_array_filter_take_first(data,
                                    D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    0);

    result = d_assert_standalone(
        d_array_filter_result_ok(&res) == true,
        "result_ok_empty",
        "result_ok should be true for empty (non-error) result",
        _counter) && result;

    d_array_filter_result_free(&res);

    // test 3: error status -> false
    res = d_array_filter_where(NULL,
                               D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                               sizeof(int),
                               d_tests_array_filter_is_even,
                               NULL);

    result = d_assert_standalone(
        d_array_filter_result_ok(&res) == false,
        "result_ok_error",
        "result_ok should be false for error result",
        _counter) && result;

    d_array_filter_result_free(&res);

    // test 4: NULL pointer -> false
    result = d_assert_standalone(
        d_array_filter_result_ok(NULL) == false,
        "result_ok_null",
        "result_ok(NULL) should be false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_result_release
  Tests the d_array_filter_result_release function.
  Tests the following:
  - Returns data pointer and transfers ownership
  - Sets result data to NULL after release
  - Writes count to out_count
  - NULL result returns NULL
  - NULL out_count is tolerated
*/
bool
d_tests_sa_array_filter_result_release
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE];
    struct d_array_filter_result res;
    void*                        released;
    size_t                       out_count;
    int*                         out;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: release transfers ownership
    res = d_array_filter_take_first(data,
                                    D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    4);

    out_count = 0;
    released = d_array_filter_result_release(&res, &out_count);

    result = d_assert_standalone(
        released != NULL,
        "release_returns_data",
        "release should return non-NULL data pointer",
        _counter) && result;

    result = d_assert_standalone(
        out_count == 4,
        "release_out_count",
        "release should write 4 to out_count",
        _counter) && result;

    // test 2: result is nulled after release
    result = d_assert_standalone(
        res.data == NULL,
        "release_nulls_result",
        "result.data should be NULL after release",
        _counter) && result;

    // test 3: verify released data integrity
    out = (int*)released;

    if (out)
    {
        result = d_assert_standalone(
            ( (out[0] == 0) &&
              (out[3] == 3) ),
            "release_data_intact",
            "Released data should contain original values",
            _counter) && result;

        // caller must free the released data
        free(released);
    }

    // test 4: NULL result
    released = d_array_filter_result_release(NULL, &out_count);

    result = d_assert_standalone(
        released == NULL,
        "release_null",
        "release(NULL) should return NULL",
        _counter) && result;

    // test 5: NULL out_count is tolerated
    res = d_array_filter_take_first(data,
                                    D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    2);

    released = d_array_filter_result_release(&res, NULL);

    result = d_assert_standalone(
        released != NULL,
        "release_null_out_count",
        "release with NULL out_count should still return data",
        _counter) && result;

    if (released)
    {
        free(released);
    }

    return result;
}


/*
d_tests_sa_array_filter_result_free
  Tests the d_array_filter_result_free function.
  Tests the following:
  - Frees a valid result without crash
  - NULL pointer is handled safely
  - Double-free is safe (data set to NULL after free)
*/
bool
d_tests_sa_array_filter_result_free
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    int                          data[D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE];
    struct d_array_filter_result res;

    result = true;
    d_tests_array_filter_fill_sequential(data, D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE);

    // test 1: free valid result
    res = d_array_filter_take_first(data,
                                    D_INTERNAL_TEST_ARRAY_FILTER_DATA_SIZE,
                                    sizeof(int),
                                    5);
    d_array_filter_result_free(&res);

    result = d_assert_standalone(
        true,
        "free_valid",
        "Freeing valid result should not crash",
        _counter) && result;

    // test 2: result data is NULL after free
    result = d_assert_standalone(
        res.data == NULL,
        "free_nulls_data",
        "data should be NULL after free",
        _counter) && result;

    // test 3: NULL pointer
    d_array_filter_result_free(NULL);

    result = d_assert_standalone(
        true,
        "free_null_safe",
        "result_free(NULL) should not crash",
        _counter) && result;

    // test 4: double-free safety
    d_array_filter_result_free(&res);

    result = d_assert_standalone(
        true,
        "free_double_safe",
        "Double-free should be safe (data already NULL)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_array_filter_result_mgmt_all
  Aggregation function that runs all result management tests.
*/
bool
d_tests_sa_array_filter_result_mgmt_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Result Management\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_array_filter_result_data(_counter) && result;
    result = d_tests_sa_array_filter_result_count_fn(_counter) && result;
    result = d_tests_sa_array_filter_result_ok(_counter) && result;
    result = d_tests_sa_array_filter_result_release(_counter) && result;
    result = d_tests_sa_array_filter_result_free(_counter) && result;

    return result;
}
