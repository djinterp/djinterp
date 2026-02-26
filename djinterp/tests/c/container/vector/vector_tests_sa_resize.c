#include "./vector_tests_sa.h"


/*
d_tests_sa_vector_resize
  Tests the d_vector_resize function.
  Tests the following:
  - NULL vector rejection
  - successful shrink
  - successful grow (zero-initialized)
*/
bool
d_tests_sa_vector_resize
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_resize(NULL, 10) == D_FAILURE,
        "resize_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: successful shrink
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 30, 40, 50);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_resize(vec, 3) == D_SUCCESS,
            "resize_shrink_success",
            "Resize shrink should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 3,
            "resize_shrink_count",
            "Count should be 3",
            _counter) && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = d_assert_standalone(
                correct,
                "resize_shrink_values",
                "First 3 elements preserved",
                _counter) && result;
        }

        // test 3: successful grow (zero-initialized)
        result = d_assert_standalone(
            d_vector_resize(vec, 5) == D_SUCCESS,
            "resize_grow_success",
            "Resize grow should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 5,
            "resize_grow_count",
            "Count should be 5",
            _counter) && result;

        {
            int* arr          = (int*)vec->elements;
            bool original_ok  = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);
            bool new_zeroed   = (arr[3] == 0) && (arr[4] == 0);

            result = d_assert_standalone(
                original_ok && new_zeroed,
                "resize_grow_values",
                "Original preserved, new elements zero-initialized",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_resize_fill
  Tests the d_vector_resize_fill function.
  Tests the following:
  - NULL vector rejection
  - successful shrink (fill ignored)
  - successful grow with fill value
  - NULL fill when growing should fail
*/
bool
d_tests_sa_vector_resize_fill
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              fill_value;

    result = true;

    // test 1: NULL vector should fail
    fill_value = 99;
    result     = d_assert_standalone(
        d_vector_resize_fill(NULL, 10, &fill_value) == D_FAILURE,
        "resize_fill_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: successful shrink (fill ignored)
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 30, 40, 50);

    if (vec)
    {
        fill_value = 999;
        result     = d_assert_standalone(
            d_vector_resize_fill(vec, 2, &fill_value) == D_SUCCESS,
            "resize_fill_shrink_success",
            "Resize fill shrink should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 2,
            "resize_fill_shrink_count",
            "Count should be 2",
            _counter) && result;

        // test 3: successful grow with fill value
        fill_value = 777;
        result     = d_assert_standalone(
            d_vector_resize_fill(vec, 5, &fill_value) == D_SUCCESS,
            "resize_fill_grow_success",
            "Resize fill grow should succeed",
            _counter) && result;

        {
            int* arr          = (int*)vec->elements;
            bool original_ok  = (arr[0] == 10) && (arr[1] == 20);
            bool new_filled   = (arr[2] == 777) && (arr[3] == 777) && (arr[4] == 777);

            result = d_assert_standalone(
                original_ok && new_filled,
                "resize_fill_grow_values",
                "Original preserved, new elements filled with 777",
                _counter) && result;
        }

        // test 4: NULL fill when growing should fail
        result = d_assert_standalone(
            d_vector_resize_fill(vec, 10, NULL) == D_FAILURE,
            "resize_fill_null_grow",
            "NULL fill when growing should fail",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_resize_all
  Aggregation function that runs all resize tests.
*/
bool
d_tests_sa_vector_resize_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Resize Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_vector_resize(_counter) && result;
    result = d_tests_sa_vector_resize_fill(_counter) && result;

    return result;
}
