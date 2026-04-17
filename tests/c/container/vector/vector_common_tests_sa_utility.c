#include "./vector_common_tests_sa.h"


/*
d_tests_sa_vector_common_swap
  Tests the d_vector_common_swap function for swapping elements.
  Tests the following:
  - NULL elements handling
  - zero element_size rejection
  - zero count handling
  - invalid index handling
  - same index handling (no-op)
  - successful swap with positive indices
  - successful swap with negative indices
*/
bool
d_tests_sa_vector_common_swap
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    result = true;

    // test 1: NULL elements should fail
    result = d_assert_standalone(
        d_vector_common_swap(NULL, 5, sizeof(int), 0, 1) == D_FAILURE,
        "swap_null_elements",
        "NULL elements should return D_FAILURE",
        _counter) && result;

    // test 2: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    result   = d_assert_standalone(
        d_vector_common_swap(elements, 5, 0, 0, 1) == D_FAILURE,
        "swap_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: zero count should fail
    elements = malloc(10 * sizeof(int));
    result   = d_assert_standalone(
        d_vector_common_swap(elements, 0, sizeof(int), 0, 1) == D_FAILURE,
        "swap_zero_count",
        "Zero count should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: invalid index_a should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        count  = 3;
    }

    result = d_assert_standalone(
        d_vector_common_swap(elements, count, sizeof(int), 10, 0) == D_FAILURE,
        "swap_invalid_index_a",
        "Invalid index_a should return D_FAILURE",
        _counter) && result;

    // test 5: invalid index_b should fail
    result = d_assert_standalone(
        d_vector_common_swap(elements, count, sizeof(int), 0, 10) == D_FAILURE,
        "swap_invalid_index_b",
        "Invalid index_b should return D_FAILURE",
        _counter) && result;

    // test 6: same index should succeed (no-op)
    result = d_assert_standalone(
        d_vector_common_swap(elements, count, sizeof(int), 1, 1) == D_SUCCESS,
        "swap_same_index",
        "Same index should return D_SUCCESS (no-op)",
        _counter) && result;

    result = d_assert_standalone(
        ((int*)elements)[1] == 20,
        "swap_same_index_unchanged",
        "Element should be unchanged after self-swap",
        _counter) && result;

    // test 7: successful swap with positive indices
    result = d_assert_standalone(
        d_vector_common_swap(elements, count, sizeof(int), 0, 2) == D_SUCCESS,
        "swap_positive_success",
        "Swap with positive indices should succeed",
        _counter) && result;

    {
        int* arr     = (int*)elements;
        bool swapped = (arr[0] == 30) && (arr[2] == 10);

        result = d_assert_standalone(
            swapped,
            "swap_positive_result",
            "Elements 0 and 2 should be swapped",
            _counter) && result;
    }

    // test 8: swap back using negative indices
    result = d_assert_standalone(
        d_vector_common_swap(elements, count, sizeof(int), 0, -1) == D_SUCCESS,
        "swap_negative_success",
        "Swap with negative index should succeed",
        _counter) && result;

    {
        int* arr      = (int*)elements;
        bool restored = (arr[0] == 10) && (arr[2] == 30);

        result = d_assert_standalone(
            restored,
            "swap_negative_result",
            "Elements should be swapped back to original",
            _counter) && result;
    }

    // test 9: swap with struct type
    if (elements)
    {
        free(elements);
    }

    {
        struct test_struct
        {
            int  a;
            char b[10];
        };

        struct test_struct* struct_elements;

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_vector_common_init(&elements, &count, &capacity, sizeof(struct test_struct), 10);

        struct_elements    = (struct test_struct*)elements;
        struct_elements[0].a = 100;
        d_strcpy_s(struct_elements[0].b, sizeof(struct_elements[0].b), "first");
        struct_elements[1].a = 200;
        d_strcpy_s(struct_elements[1].b, sizeof(struct_elements[1].b), "second");
        count = 2;

        result = d_assert_standalone(
            d_vector_common_swap(elements,
                                 count,
                                 sizeof(struct test_struct),
                                 0,
                                 1) == D_SUCCESS,
            "swap_struct_success",
            "Swap with struct type should succeed",
            _counter) && result;

        {
            bool correct = (struct_elements[0].a == 200) &&
                           (strcmp(struct_elements[0].b, "second") == 0) &&
                           (struct_elements[1].a == 100) &&
                           (strcmp(struct_elements[1].b, "first") == 0);

            result = d_assert_standalone(
                correct,
                "swap_struct_result",
                "Struct elements should be correctly swapped",
                _counter) && result;
        }
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}

/*
d_tests_sa_vector_common_copy_to
  Tests the d_vector_common_copy_to function for copying to a buffer.
  Tests the following:
  - NULL destination handling
  - zero element_size rejection
  - zero count (success, nothing to copy)
  - NULL source with non-zero count (success, nothing to copy)
  - destination too small handling
  - successful copy
*/
bool
d_tests_sa_vector_common_copy_to
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    dest[10];

    result = true;

    // test 1: NULL destination should fail
    elements = malloc(10 * sizeof(int));
    result   = d_assert_standalone(
        d_vector_common_copy_to(elements,
                                5,
                                sizeof(int),
                                NULL,
                                10) == D_FAILURE,
        "copy_to_null_dest",
        "NULL destination should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 2: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    result   = d_assert_standalone(
        d_vector_common_copy_to(elements,
                                5,
                                0,
                                dest,
                                10) == D_FAILURE,
        "copy_to_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: zero count should succeed (nothing to copy)
    result = d_assert_standalone(
        d_vector_common_copy_to(NULL,
                                0,
                                sizeof(int),
                                dest,
                                10) == D_SUCCESS,
        "copy_to_zero_count",
        "Zero count should return D_SUCCESS (nothing to copy)",
        _counter) && result;

    // test 4: NULL source with non-zero count should succeed (nothing to copy)
    result = d_assert_standalone(
        d_vector_common_copy_to(NULL,
                                0,
                                sizeof(int),
                                dest,
                                10) == D_SUCCESS,
        "copy_to_null_source",
        "NULL source should return D_SUCCESS",
        _counter) && result;

    // test 5: destination too small should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        size_t i;
        for (i = 0; i < 10; i++)
        {
            arr[i] = (int)((i + 1) * 10);
        }
        count = 10;
    }

    result = d_assert_standalone(
        d_vector_common_copy_to(elements,
                                count,
                                sizeof(int),
                                dest,
                                5) == D_FAILURE,
        "copy_to_dest_too_small",
        "Destination too small should return D_FAILURE",
        _counter) && result;

    // test 6: successful copy
    memset(dest, 0, sizeof(dest));
    count = 5;
    result = d_assert_standalone(
        d_vector_common_copy_to(elements,
                                count,
                                sizeof(int),
                                dest,
                                10) == D_SUCCESS,
        "copy_to_success",
        "Valid copy should return D_SUCCESS",
        _counter) && result;

    {
        int* src     = (int*)elements;
        bool correct = true;
        size_t i;

        for (i = 0; i < 5; i++)
        {
            if (dest[i] != src[i])
            {
                correct = false;

                break;
            }
        }

        result = d_assert_standalone(
            correct,
            "copy_to_values",
            "Copied values should match source",
            _counter) && result;
    }

    // test 7: exact capacity copy
    memset(dest, 0, sizeof(dest));
    count = 10;
    result = d_assert_standalone(
        d_vector_common_copy_to(elements,
                                count,
                                sizeof(int),
                                dest,
                                10) == D_SUCCESS,
        "copy_to_exact_capacity",
        "Copy with exact capacity should succeed",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_utility_all
  Aggregation function that runs all utility tests.
*/
bool
d_tests_sa_vector_common_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Utility Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_vector_common_swap(_counter) && result;
    result = d_tests_sa_vector_common_copy_to(_counter) && result;

    return result;
}
