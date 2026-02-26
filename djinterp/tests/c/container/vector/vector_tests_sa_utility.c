#include "./vector_tests_sa.h"


// helper comparator for int values
static int
int_comparator_util
(
    const void* _a,
    const void* _b
)
{
    int a = *(const int*)_a;
    int b = *(const int*)_b;

    return (a > b) - (a < b);
}


/*
d_tests_sa_vector_swap
  Tests the d_vector_swap function.
  Tests the following:
  - NULL vector rejection
  - empty vector rejection
  - successful swap
  - negative index support
*/
bool
d_tests_sa_vector_swap
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_swap(NULL, 0, 1) == D_FAILURE,
        "swap_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: empty vector should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_swap(vec, 0, 1) == D_FAILURE,
            "swap_empty_vector",
            "Empty vector should return D_FAILURE",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: successful swap
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_swap(vec, 0, 2) == D_SUCCESS,
            "swap_success",
            "Swap should succeed",
            _counter) && result;

        {
            int* arr     = (int*)vec->elements;
            bool swapped = (arr[0] == 30) && (arr[2] == 10);

            result = d_assert_standalone(
                swapped,
                "swap_result",
                "Elements 0 and 2 should be swapped",
                _counter) && result;
        }

        // test 4: negative index support
        result = d_assert_standalone(
            d_vector_swap(vec, 0, -1) == D_SUCCESS,
            "swap_negative",
            "Swap with negative index should succeed",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_reverse
  Tests the d_vector_reverse function.
  Tests the following:
  - NULL vector rejection
  - successful reverse
*/
bool
d_tests_sa_vector_reverse
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_reverse(NULL) == D_FAILURE,
        "reverse_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: successful reverse
    vec = d_vector_new_from_args(sizeof(int), 5, 1, 2, 3, 4, 5);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_reverse(vec) == D_SUCCESS,
            "reverse_success",
            "Reverse should succeed",
            _counter) && result;

        {
            int* arr = (int*)vec->elements;
            bool reversed = (arr[0] == 5) && (arr[1] == 4) && (arr[2] == 3) &&
                            (arr[3] == 2) && (arr[4] == 1);

            result = d_assert_standalone(
                reversed,
                "reverse_result",
                "Elements should be [5, 4, 3, 2, 1]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_sort
  Tests the d_vector_sort function.
  Tests the following:
  - NULL vector handling
  - NULL comparator handling
  - successful sort
*/
bool
d_tests_sa_vector_sort
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector should not crash
    d_vector_sort(NULL, int_comparator_util);
    result = d_assert_standalone(
        true,
        "sort_null_vector",
        "NULL vector should not crash",
        _counter) && result;

    // test 2: NULL comparator should not crash
    vec = d_vector_new_from_args(sizeof(int), 3, 30, 10, 20);

    if (vec)
    {
        d_vector_sort(vec, NULL);
        result = d_assert_standalone(
            true,
            "sort_null_comparator",
            "NULL comparator should not crash",
            _counter) && result;

        // test 3: successful sort
        d_vector_sort(vec, int_comparator_util);

        {
            int* arr    = (int*)vec->elements;
            bool sorted = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = d_assert_standalone(
                sorted,
                "sort_result",
                "Elements should be sorted [10, 20, 30]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_copy_to
  Tests the d_vector_copy_to function.
  Tests the following:
  - NULL vector rejection
  - NULL destination rejection
  - destination too small rejection
  - successful copy
*/
bool
d_tests_sa_vector_copy_to
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              dest[10];

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_copy_to(NULL, dest, 10) == D_FAILURE,
        "copy_to_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: NULL destination should fail
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_copy_to(vec, NULL, 10) == D_FAILURE,
            "copy_to_null_dest",
            "NULL destination should return D_FAILURE",
            _counter) && result;

        // test 3: destination too small should fail
        result = d_assert_standalone(
            d_vector_copy_to(vec, dest, 2) == D_FAILURE,
            "copy_to_small_dest",
            "Small destination should return D_FAILURE",
            _counter) && result;

        // test 4: successful copy
        memset(dest, 0, sizeof(dest));
        result = d_assert_standalone(
            d_vector_copy_to(vec, dest, 10) == D_SUCCESS,
            "copy_to_success",
            "Copy should succeed",
            _counter) && result;

        {
            bool copied = (dest[0] == 10) && (dest[1] == 20) && (dest[2] == 30);

            result = d_assert_standalone(
                copied,
                "copy_to_values",
                "Destination should contain [10, 20, 30]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_utility_all
  Aggregation function that runs all utility tests.
*/
bool
d_tests_sa_vector_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Utility Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_vector_swap(_counter) && result;
    result = d_tests_sa_vector_reverse(_counter) && result;
    result = d_tests_sa_vector_sort(_counter) && result;
    result = d_tests_sa_vector_copy_to(_counter) && result;

    return result;
}
