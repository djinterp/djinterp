#include "./vector_common_tests_sa.h"


/*
d_tests_sa_vector_common_resize
  Tests the d_vector_common_resize function for resizing vectors.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - shrinking (just updates count)
  - growing (zero-initializes new elements)
  - no change when new_count equals current count
*/
bool
d_tests_sa_vector_common_resize
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 5;
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_resize(NULL,
                               &count,
                               &capacity,
                               sizeof(int),
                               3) == D_FAILURE,
        "resize_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_resize(&elements,
                               NULL,
                               &capacity,
                               sizeof(int),
                               3) == D_FAILURE,
        "resize_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    count    = 5;
    result   = d_assert_standalone(
        d_vector_common_resize(&elements,
                               &count,
                               NULL,
                               sizeof(int),
                               3) == D_FAILURE,
        "resize_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    count    = 5;
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_resize(&elements,
                               &count,
                               &capacity,
                               0,
                               3) == D_FAILURE,
        "resize_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

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

    result = d_assert_standalone(
        d_vector_common_resize(&elements,
                               &count,
                               &capacity,
                               sizeof(int),
                               3) == D_SUCCESS,
        "resize_shrink_success",
        "Shrinking should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 3,
        "resize_shrink_count",
        "Count should be 3 after shrinking",
        _counter) && result;

    // verify original elements preserved
    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

        result = d_assert_standalone(
            correct,
            "resize_shrink_values",
            "Original elements should be preserved",
            _counter) && result;
    }

    // test 6: growing (zero-initializes new elements)
    result = d_assert_standalone(
        d_vector_common_resize(&elements,
                               &count,
                               &capacity,
                               sizeof(int),
                               7) == D_SUCCESS,
        "resize_grow_success",
        "Growing should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 7,
        "resize_grow_count",
        "Count should be 7 after growing",
        _counter) && result;

    // verify new elements are zero-initialized
    {
        int* arr = (int*)elements;
        bool original_preserved = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);
        bool new_zeroed = (arr[3] == 0) && (arr[4] == 0) &&
                          (arr[5] == 0) && (arr[6] == 0);

        result = d_assert_standalone(
            original_preserved,
            "resize_grow_original_preserved",
            "Original elements should be preserved",
            _counter) && result;

        result = d_assert_standalone(
            new_zeroed,
            "resize_grow_new_zeroed",
            "New elements should be zero-initialized",
            _counter) && result;
    }

    // test 7: no change when new_count equals current count
    result = d_assert_standalone(
        d_vector_common_resize(&elements,
                               &count,
                               &capacity,
                               sizeof(int),
                               7) == D_SUCCESS,
        "resize_same_success",
        "Same size resize should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 7,
        "resize_same_count",
        "Count should remain 7",
        _counter) && result;

    // test 8: resize to zero
    result = d_assert_standalone(
        d_vector_common_resize(&elements,
                               &count,
                               &capacity,
                               sizeof(int),
                               0) == D_SUCCESS,
        "resize_zero_success",
        "Resize to zero should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 0,
        "resize_zero_count",
        "Count should be 0",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_resize_fill
  Tests the d_vector_common_resize_fill function for resizing with fill value.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - shrinking (just updates count, ignores fill)
  - growing with fill value
  - NULL fill value when growing fails
*/
bool
d_tests_sa_vector_common_resize_fill
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    fill_value;

    result = true;

    // test 1: NULL elements pointer should fail
    count      = 5;
    capacity   = 10;
    fill_value = 99;
    result     = d_assert_standalone(
        d_vector_common_resize_fill(NULL,
                                    &count,
                                    &capacity,
                                    sizeof(int),
                                    3,
                                    &fill_value) == D_FAILURE,
        "resize_fill_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_resize_fill(&elements,
                                    NULL,
                                    &capacity,
                                    sizeof(int),
                                    3,
                                    &fill_value) == D_FAILURE,
        "resize_fill_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    count    = 5;
    result   = d_assert_standalone(
        d_vector_common_resize_fill(&elements,
                                    &count,
                                    NULL,
                                    sizeof(int),
                                    3,
                                    &fill_value) == D_FAILURE,
        "resize_fill_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    count    = 5;
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_resize_fill(&elements,
                                    &count,
                                    &capacity,
                                    0,
                                    3,
                                    &fill_value) == D_FAILURE,
        "resize_fill_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

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
    result     = d_assert_standalone(
        d_vector_common_resize_fill(&elements,
                                    &count,
                                    &capacity,
                                    sizeof(int),
                                    3,
                                    &fill_value) == D_SUCCESS,
        "resize_fill_shrink_success",
        "Shrinking should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 3,
        "resize_fill_shrink_count",
        "Count should be 3 after shrinking",
        _counter) && result;

    // test 6: growing with fill value
    fill_value = 777;
    result     = d_assert_standalone(
        d_vector_common_resize_fill(&elements,
                                    &count,
                                    &capacity,
                                    sizeof(int),
                                    6,
                                    &fill_value) == D_SUCCESS,
        "resize_fill_grow_success",
        "Growing with fill should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 6,
        "resize_fill_grow_count",
        "Count should be 6 after growing",
        _counter) && result;

    // verify new elements have fill value
    {
        int* arr = (int*)elements;
        bool original_preserved = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);
        bool new_filled         = (arr[3] == 777) && (arr[4] == 777) && (arr[5] == 777);

        result = d_assert_standalone(
            original_preserved,
            "resize_fill_grow_original",
            "Original elements should be preserved",
            _counter) && result;

        result = d_assert_standalone(
            new_filled,
            "resize_fill_grow_filled",
            "New elements should have fill value (777)",
            _counter) && result;
    }

    // test 7: NULL fill value when growing should fail
    result = d_assert_standalone(
        d_vector_common_resize_fill(&elements,
                                    &count,
                                    &capacity,
                                    sizeof(int),
                                    10,
                                    NULL) == D_FAILURE,
        "resize_fill_null_grow_fail",
        "NULL fill value when growing should fail",
        _counter) && result;

    // test 8: NULL fill value when shrinking should succeed (fill ignored)
    result = d_assert_standalone(
        d_vector_common_resize_fill(&elements,
                                    &count,
                                    &capacity,
                                    sizeof(int),
                                    3,
                                    NULL) == D_SUCCESS,
        "resize_fill_null_shrink_success",
        "NULL fill value when shrinking should succeed",
        _counter) && result;

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

        result = d_assert_standalone(
            d_vector_common_resize_fill(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(struct test_struct),
                                        3,
                                        &fill) == D_SUCCESS,
            "resize_fill_struct_success",
            "Resize with struct fill should succeed",
            _counter) && result;

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

            result = d_assert_standalone(
                all_correct,
                "resize_fill_struct_values",
                "All struct elements should have fill values",
                _counter) && result;
        }

        if (elements)
        {
            free(elements);
        }
    }

    return result;
}


/*
d_tests_sa_vector_common_resize_all
  Aggregation function that runs all resize tests.
*/
bool
d_tests_sa_vector_common_resize_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Resize Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_vector_common_resize(_counter) && result;
    result = d_tests_sa_vector_common_resize_fill(_counter) && result;

    return result;
}
