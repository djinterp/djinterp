/*******************************************************************************
* djinterp [container]                              vector_common_tests_resize.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests_resize.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_common_resize
  Tests the d_vector_common_resize function for resizing vectors.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - shrinking (just updates count)
  - growing (zero-initializes new elements)
  - no change when new_count equals current count
*/
bool d_tests_vector_common_resize(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_resize");

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 5;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_null_elements",
                                d_vector_common_resize(NULL, &count, &capacity, sizeof(int), 3) == D_FAILURE,
                                "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_null_count",
                                d_vector_common_resize(&elements, NULL, &capacity, sizeof(int), 3) == D_FAILURE,
                                "NULL count pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    count    = 5;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_null_capacity",
                                d_vector_common_resize(&elements, &count, NULL, sizeof(int), 3) == D_FAILURE,
                                "NULL capacity pointer should return D_FAILURE") && result;

    free(elements);

    // test 4: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    count    = 5;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_zero_element_size",
                                d_vector_common_resize(&elements, &count, &capacity, 0, 3) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 5: shrinking (just updates count)
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        arr[3] = 40;
        arr[4] = 50;
        count  = 5;
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "resize_shrink_success",
                              d_vector_common_resize(&elements, &count, &capacity, sizeof(int), 3) == D_SUCCESS,
                              "Shrinking should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "resize_shrink_count",
                              count == 3,
                              "Count should be 3 after shrinking") && result;

    // verify original elements preserved
    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_shrink_values",
                                  correct,
                                  "Original elements should be preserved") && result;
    }

    // test 6: growing (zero-initializes new elements)
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_grow_success",
                              d_vector_common_resize(&elements, &count, &capacity, sizeof(int), 7) == D_SUCCESS,
                              "Growing should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "resize_grow_count",
                              count == 7,
                              "Count should be 7 after growing") && result;

    // verify new elements are zero-initialized
    {
        int* arr = (int*)elements;
        bool original_preserved = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);
        bool new_zeroed = (arr[3] == 0) && (arr[4] == 0) &&
                          (arr[5] == 0) && (arr[6] == 0);

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_grow_original_preserved",
                                  original_preserved,
                                  "Original elements should be preserved") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_grow_new_zeroed",
                                  new_zeroed,
                                  "New elements should be zero-initialized") && result;
    }

    // test 7: no change when new_count equals current count
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_same_success",
                              d_vector_common_resize(&elements, &count, &capacity, sizeof(int), 7) == D_SUCCESS,
                              "Same size resize should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "resize_same_count",
                              count == 7,
                              "Count should remain 7") && result;

    // test 8: resize to zero
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_zero_success",
                              d_vector_common_resize(&elements, &count, &capacity, sizeof(int), 0) == D_SUCCESS,
                              "Resize to zero should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "resize_zero_count",
                              count == 0,
                              "Count should be 0") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_resize_fill
  Tests the d_vector_common_resize_fill function for resizing with fill value.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - shrinking (just updates count, ignores fill)
  - growing with fill value
  - NULL fill value when growing fails
*/
bool d_tests_vector_common_resize_fill(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    fill_value;

    d_test_suite_test(_suite, "vector_common_resize_fill");

    result = true;

    // test 1: NULL elements pointer should fail
    count      = 5;
    capacity   = 10;
    fill_value = 99;
    result     = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_null_elements",
                                  d_vector_common_resize_fill(NULL, &count, &capacity, sizeof(int), 3, &fill_value) == D_FAILURE,
                                  "NULL elements pointer should return D_FAILURE") && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_fill_null_count",
                                d_vector_common_resize_fill(&elements, NULL, &capacity, sizeof(int), 3, &fill_value) == D_FAILURE,
                                "NULL count pointer should return D_FAILURE") && result;

    free(elements);

    // test 3: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    count    = 5;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_fill_null_capacity",
                                d_vector_common_resize_fill(&elements, &count, NULL, sizeof(int), 3, &fill_value) == D_FAILURE,
                                "NULL capacity pointer should return D_FAILURE") && result;

    free(elements);

    // test 4: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    count    = 5;
    capacity = 10;
    result   = D_TEST_CHECK_MSG(_suite,
                                "resize_fill_zero_element_size",
                                d_vector_common_resize_fill(&elements, &count, &capacity, 0, 3, &fill_value) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 5: shrinking (just updates count, fill value ignored)
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        arr[3] = 40;
        arr[4] = 50;
        count  = 5;
    }

    fill_value = 99;
    result     = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_shrink_success",
                                  d_vector_common_resize_fill(&elements, &count, &capacity, sizeof(int), 3, &fill_value) == D_SUCCESS,
                                  "Shrinking should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "resize_fill_shrink_count",
                              count == 3,
                              "Count should be 3 after shrinking") && result;

    // test 6: growing with fill value
    fill_value = 777;
    result     = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_grow_success",
                                  d_vector_common_resize_fill(&elements, &count, &capacity, sizeof(int), 6, &fill_value) == D_SUCCESS,
                                  "Growing with fill should return D_SUCCESS") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "resize_fill_grow_count",
                              count == 6,
                              "Count should be 6 after growing") && result;

    // verify new elements have fill value
    {
        int* arr = (int*)elements;
        bool original_preserved = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);
        bool new_filled         = (arr[3] == 777) && (arr[4] == 777) && (arr[5] == 777);

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_grow_original",
                                  original_preserved,
                                  "Original elements should be preserved") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_grow_filled",
                                  new_filled,
                                  "New elements should have fill value (777)") && result;
    }

    // test 7: NULL fill value when growing should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_fill_null_grow_fail",
                              d_vector_common_resize_fill(&elements, &count, &capacity, sizeof(int), 10, NULL) == D_FAILURE,
                              "NULL fill value when growing should fail") && result;

    // test 8: NULL fill value when shrinking should succeed (fill ignored)
    result = D_TEST_CHECK_MSG(_suite,
                              "resize_fill_null_shrink_success",
                              d_vector_common_resize_fill(&elements, &count, &capacity, sizeof(int), 3, NULL) == D_SUCCESS,
                              "NULL fill value when shrinking should succeed") && result;

    if (elements)
    {
        free(elements);
    }

    // test 9: test with struct type
    {
        struct test_struct
        {
            int   x;
            float y;
        };

        struct test_struct fill = {42, 3.14f};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_vector_common_init(&elements, &count, &capacity, sizeof(struct test_struct), 10);

        result = D_TEST_CHECK_MSG(_suite,
                                  "resize_fill_struct_success",
                                  d_vector_common_resize_fill(&elements, &count, &capacity, sizeof(struct test_struct), 3, &fill) == D_SUCCESS,
                                  "Resize with struct fill should succeed") && result;

        {
            struct test_struct* arr = (struct test_struct*)elements;
            bool all_correct        = true;
            size_t i;

            for (i = 0; i < 3; i++)
            {
                if ( (arr[i].x != 42) ||
                     (arr[i].y != 3.14f) )
                {
                    all_correct = false;

                    break;
                }
            }

            result = D_TEST_CHECK_MSG(_suite,
                                      "resize_fill_struct_values",
                                      all_correct,
                                      "All struct elements should have fill values") && result;
        }

        if (elements)
        {
            free(elements);
        }
    }

    return result;
}


/*
d_tests_vector_common_resize_all
  Aggregation function that runs all resize tests.
*/
bool d_tests_vector_common_resize_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_common_resize_all");

    result = true;

    result = d_tests_vector_common_resize(_suite) && result;
    result = d_tests_vector_common_resize_fill(_suite) && result;

    return result;
}
