/*******************************************************************************
* djinterp [container]                             vector_common_tests_utility.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests_utility.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_common_swap
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
bool d_tests_vector_common_swap(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_swap");

    result = true;

    // test 1: NULL elements should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "swap_null_elements",
                              d_vector_common_swap(NULL, 5, sizeof(int), 0, 1) == D_FAILURE,
                              "NULL elements should return D_FAILURE") && result;

    // test 2: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "swap_zero_element_size",
                                d_vector_common_swap(elements, 5, 0, 0, 1) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 3: zero count should fail
    elements = malloc(10 * sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "swap_zero_count",
                                d_vector_common_swap(elements, 0, sizeof(int), 0, 1) == D_FAILURE,
                                "Zero count should return D_FAILURE") && result;

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

    result = D_TEST_CHECK_MSG(_suite,
                              "swap_invalid_index_a",
                              d_vector_common_swap(elements, count, sizeof(int), 10, 0) == D_FAILURE,
                              "Invalid index_a should return D_FAILURE") && result;

    // test 5: invalid index_b should fail
    result = D_TEST_CHECK_MSG(_suite,
                              "swap_invalid_index_b",
                              d_vector_common_swap(elements, count, sizeof(int), 0, 10) == D_FAILURE,
                              "Invalid index_b should return D_FAILURE") && result;

    // test 6: same index should succeed (no-op)
    result = D_TEST_CHECK_MSG(_suite,
                              "swap_same_index",
                              d_vector_common_swap(elements, count, sizeof(int), 1, 1) == D_SUCCESS,
                              "Same index should return D_SUCCESS (no-op)") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "swap_same_index_unchanged",
                              ((int*)elements)[1] == 20,
                              "Element should be unchanged after self-swap") && result;

    // test 7: successful swap with positive indices
    result = D_TEST_CHECK_MSG(_suite,
                              "swap_positive_success",
                              d_vector_common_swap(elements, count, sizeof(int), 0, 2) == D_SUCCESS,
                              "Swap with positive indices should succeed") && result;

    {
        int* arr     = (int*)elements;
        bool swapped = (arr[0] == 30) && (arr[2] == 10);

        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_positive_result",
                                  swapped,
                                  "Elements 0 and 2 should be swapped") && result;
    }

    // test 8: swap back using negative indices
    result = D_TEST_CHECK_MSG(_suite,
                              "swap_negative_success",
                              d_vector_common_swap(elements, count, sizeof(int), 0, -1) == D_SUCCESS,
                              "Swap with negative index should succeed") && result;

    {
        int* arr      = (int*)elements;
        bool restored = (arr[0] == 10) && (arr[2] == 30);

        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_negative_result",
                                  restored,
                                  "Elements should be swapped back to original") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "swap_struct_success",
                                  d_vector_common_swap(elements, count, sizeof(struct test_struct), 0, 1) == D_SUCCESS,
                                  "Swap with struct type should succeed") && result;

        {
            bool correct = (struct_elements[0].a == 200) &&
                           (strcmp(struct_elements[0].b, "second") == 0) &&
                           (struct_elements[1].a == 100) &&
                           (strcmp(struct_elements[1].b, "first") == 0);

            result = D_TEST_CHECK_MSG(_suite,
                                      "swap_struct_result",
                                      correct,
                                      "Struct elements should be correctly swapped") && result;
        }
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}

/*
d_tests_vector_common_copy_to
  Tests the d_vector_common_copy_to function for copying to a buffer.
  Tests the following:
  - NULL destination handling
  - zero element_size rejection
  - zero count (success, nothing to copy)
  - NULL source with non-zero count (success, nothing to copy)
  - destination too small handling
  - successful copy
*/
bool d_tests_vector_common_copy_to(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    dest[10];

    d_test_suite_test(_suite, "vector_common_copy_to");

    result = true;

    // test 1: NULL destination should fail
    elements = malloc(10 * sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "copy_to_null_dest",
                                d_vector_common_copy_to(elements, 5, sizeof(int), NULL, 10) == D_FAILURE,
                                "NULL destination should return D_FAILURE") && result;

    free(elements);

    // test 2: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    result   = D_TEST_CHECK_MSG(_suite,
                                "copy_to_zero_element_size",
                                d_vector_common_copy_to(elements, 5, 0, dest, 10) == D_FAILURE,
                                "Zero element_size should return D_FAILURE") && result;

    free(elements);

    // test 3: zero count should succeed (nothing to copy)
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_zero_count",
                              d_vector_common_copy_to(NULL, 0, sizeof(int), dest, 10) == D_SUCCESS,
                              "Zero count should return D_SUCCESS (nothing to copy)") && result;

    // test 4: NULL source with non-zero count should succeed (nothing to copy)
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_null_source",
                              d_vector_common_copy_to(NULL, 0, sizeof(int), dest, 10) == D_SUCCESS,
                              "NULL source should return D_SUCCESS") && result;

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

    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_dest_too_small",
                              d_vector_common_copy_to(elements, count, sizeof(int), dest, 5) == D_FAILURE,
                              "Destination too small should return D_FAILURE") && result;

    // test 6: successful copy
    memset(dest, 0, sizeof(dest));
    count = 5;
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_success",
                              d_vector_common_copy_to(elements, count, sizeof(int), dest, 10) == D_SUCCESS,
                              "Valid copy should return D_SUCCESS") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_values",
                                  correct,
                                  "Copied values should match source") && result;
    }

    // test 7: exact capacity copy
    memset(dest, 0, sizeof(dest));
    count = 10;
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_exact_capacity",
                              d_vector_common_copy_to(elements, count, sizeof(int), dest, 10) == D_SUCCESS,
                              "Copy with exact capacity should succeed") && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_vector_common_utility_all
  Aggregation function that runs all utility tests.
*/
bool d_tests_vector_common_utility_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_common_utility_all");

    result = true;

    result = d_tests_vector_common_swap(_suite) && result;
    result = d_tests_vector_common_copy_to(_suite) && result;

    return result;
}
