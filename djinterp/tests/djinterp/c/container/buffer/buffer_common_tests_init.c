/*******************************************************************************
* djinterp [container]                                buffer_common_tests_init.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_init.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_init_sized
  Tests the d_buffer_common_init_sized function.
  Tests the following:
  - NULL destination pointer rejection
  - NULL count pointer rejection
  - NULL capacity pointer rejection
  - zero element_size rejection
  - zero initial_capacity uses default capacity
  - successful initialization with explicit capacity
  - count is set to zero after init
  - capacity matches requested value
*/
bool d_tests_buffer_common_init_sized(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "buffer_common_init_sized");

    result = true;

    // test 1: NULL destination should fail
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_sized_null_dest",
                                d_buffer_common_init_sized(NULL, &count, &capacity, sizeof(int), 10) == false,
                                "NULL destination should return false") && result;

    // test 2: NULL count should fail
    elements = NULL;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_sized_null_count",
                                d_buffer_common_init_sized(&elements, NULL, &capacity, sizeof(int), 10) == false,
                                "NULL count should return false") && result;

    // test 3: NULL capacity should fail
    elements = NULL;
    count    = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_sized_null_capacity",
                                d_buffer_common_init_sized(&elements, &count, NULL, sizeof(int), 10) == false,
                                "NULL capacity should return false") && result;

    // test 4: zero element_size should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_sized_zero_elem_size",
                                d_buffer_common_init_sized(&elements, &count, &capacity, 0, 10) == false,
                                "Zero element_size should return false") && result;

    // test 5: zero initial_capacity should use default
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_sized_zero_cap_success",
                                d_buffer_common_init_sized(&elements, &count, &capacity, sizeof(int), 0) == true,
                                "Zero capacity should succeed (uses default)") && result;

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "init_sized_zero_cap_default",
                                  capacity == D_BUFFER_DEFAULT_CAPACITY,
                                  "Capacity should equal D_BUFFER_DEFAULT_CAPACITY") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "init_sized_zero_cap_count",
                                  count == 0,
                                  "Count should be 0") && result;

        d_buffer_common_free_data(elements);
    }

    // test 6: successful init with explicit capacity
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_sized_explicit_success",
                                d_buffer_common_init_sized(&elements, &count, &capacity, sizeof(int), 64) == true,
                                "Explicit capacity init should succeed") && result;

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "init_sized_explicit_cap",
                                  capacity == 64,
                                  "Capacity should be 64") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "init_sized_explicit_count",
                                  count == 0,
                                  "Count should be 0") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_init_from_data
  Tests the d_buffer_common_init_from_data function.
  Tests the following:
  - NULL destination rejection
  - NULL source with non-zero count rejection
  - NULL source with zero count succeeds
  - successful copy of source data
  - extra capacity is honoured
  - count and capacity set correctly
*/
bool d_tests_buffer_common_init_from_data(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[5] = {10, 20, 30, 40, 50};

    d_test_suite_test(_suite, "buffer_common_init_from_data");

    result = true;

    // test 1: NULL destination should fail
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_from_data_null_dest",
                                d_buffer_common_init_from_data(NULL, &count, &capacity, sizeof(int), source, 5, 0) == false,
                                "NULL destination should return false") && result;

    // test 2: NULL source with non-zero count should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_from_data_null_src_nonzero",
                                d_buffer_common_init_from_data(&elements, &count, &capacity, sizeof(int), NULL, 5, 0) == false,
                                "NULL source with count>0 should return false") && result;

    // test 3: NULL source with zero count should succeed
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_from_data_null_src_zero",
                                d_buffer_common_init_from_data(&elements, &count, &capacity, sizeof(int), NULL, 0, 0) == true,
                                "NULL source with count=0 should succeed") && result;

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "init_from_data_null_src_zero_count",
                                  count == 0,
                                  "Count should be 0") && result;

        d_buffer_common_free_data(elements);
    }

    // test 4: successful copy of source data
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_from_data_success",
                                d_buffer_common_init_from_data(&elements, &count, &capacity, sizeof(int), source, 5, 0) == true,
                                "Init from data should succeed") && result;

    if (elements)
    {
        int* arr = (int*)elements;

        result = D_TEST_CHECK_MSG(_suite,
                                  "init_from_data_count",
                                  count == 5,
                                  "Count should be 5") && result;

        {
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30) && (arr[3] == 40) &&
                           (arr[4] == 50);

            result = D_TEST_CHECK_MSG(_suite,
                                      "init_from_data_values",
                                      correct,
                                      "Elements should match source [10..50]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    // test 5: extra capacity honoured
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_from_data_extra_cap_success",
                                d_buffer_common_init_from_data(&elements, &count, &capacity, sizeof(int), source, 5, 500) == true,
                                "Extra capacity init should succeed") && result;

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "init_from_data_extra_cap_value",
                                  capacity >= 505,
                                  "Capacity should be >= source_count + extra") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_init_copy
  Tests the d_buffer_common_init_copy function.
  Tests the following:
  - NULL parameters rejection
  - successful copy initialization
  - copied data matches source
*/
bool d_tests_buffer_common_init_copy(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {100, 200, 300};

    d_test_suite_test(_suite, "buffer_common_init_copy");

    result = true;

    // test 1: NULL destination should fail
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_copy_null_dest",
                                d_buffer_common_init_copy(NULL, &count, &capacity, sizeof(int), source, 3) == false,
                                "NULL destination should return false") && result;

    // test 2: successful copy
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_copy_success",
                                d_buffer_common_init_copy(&elements, &count, &capacity, sizeof(int), source, 3) == true,
                                "Init copy should succeed") && result;

    if (elements)
    {
        int* arr = (int*)elements;

        result = D_TEST_CHECK_MSG(_suite,
                                  "init_copy_count",
                                  count == 3,
                                  "Count should be 3") && result;

        {
            bool correct = (arr[0] == 100) && (arr[1] == 200) &&
                           (arr[2] == 300);

            result = D_TEST_CHECK_MSG(_suite,
                                      "init_copy_values",
                                      correct,
                                      "Elements should match source [100, 200, 300]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_init_fill
  Tests the d_buffer_common_init_fill function.
  Tests the following:
  - NULL destination rejection
  - NULL value rejection
  - zero element_size rejection
  - successful fill with single value
  - all elements match fill value
*/
bool d_tests_buffer_common_init_fill(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    fill_val;

    d_test_suite_test(_suite, "buffer_common_init_fill");

    result = true;

    // test 1: NULL destination should fail
    fill_val = 42;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_fill_null_dest",
                                d_buffer_common_init_fill(NULL, &count, &capacity, sizeof(int), 5, &fill_val) == false,
                                "NULL destination should return false") && result;

    // test 2: NULL value should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_fill_null_value",
                                d_buffer_common_init_fill(&elements, &count, &capacity, sizeof(int), 5, NULL) == false,
                                "NULL value should return false") && result;

    // test 3: zero element_size should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_fill_zero_elem_size",
                                d_buffer_common_init_fill(&elements, &count, &capacity, 0, 5, &fill_val) == false,
                                "Zero element_size should return false") && result;

    // test 4: successful fill
    elements = NULL;
    count    = 0;
    capacity = 0;
    fill_val = 77;
    result   = D_TEST_CHECK_MSG(_suite,
                                "init_fill_success",
                                d_buffer_common_init_fill(&elements, &count, &capacity, sizeof(int), 5, &fill_val) == true,
                                "Init fill should succeed") && result;

    if (elements)
    {
        int* arr = (int*)elements;

        result = D_TEST_CHECK_MSG(_suite,
                                  "init_fill_count",
                                  count == 5,
                                  "Count should be 5") && result;

        // test 5: all elements match fill value
        {
            bool all_match = true;
            size_t i;

            for (i = 0; i < 5; ++i)
            {
                if (arr[i] != 77)
                {
                    all_match = false;
                }
            }

            result = D_TEST_CHECK_MSG(_suite,
                                      "init_fill_values",
                                      all_match,
                                      "All elements should be 77") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_init_all
  Aggregation function that runs all initialization tests.
*/
bool d_tests_buffer_common_init_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_init_all");

    result = true;

    result = d_tests_buffer_common_init_sized(_suite) && result;
    result = d_tests_buffer_common_init_from_data(_suite) && result;
    result = d_tests_buffer_common_init_copy(_suite) && result;
    result = d_tests_buffer_common_init_fill(_suite) && result;

    return result;
}
