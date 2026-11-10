/*******************************************************************************
* djinterp [container]                            buffer_common_tests_ordering.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_ordering.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// int_comparator_ordering
//   internal: comparison function for int elements used in ordering tests.
static
int int_comparator_ordering(
    const void* _a,
    const void* _b
)
{
    int a_val = *(const int*)_a;
    int b_val = *(const int*)_b;

    if (a_val < b_val)
    {
        return -1;
    }

    if (a_val > b_val)
    {
        return 1;
    }

    return 0;
}


/*
d_tests_buffer_common_reverse
  Tests the d_buffer_common_reverse function.
  Tests the following:
  - NULL elements returns false
  - single element is a no-op (returns true)
  - zero count is a no-op (returns true)
  - successful reverse of even-length buffer
  - successful reverse of odd-length buffer
*/
bool d_tests_buffer_common_reverse(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "buffer_common_reverse");

    result = true;

    // test 1: NULL elements
    result = D_TEST_CHECK_MSG(_suite,
                              "reverse_null",
                              d_buffer_common_reverse(NULL, 5, sizeof(int)) == false,
                              "NULL elements should return false") && result;

    // test 2: single element no-op
    {
        int single = 42;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_single",
                                  d_buffer_common_reverse(&single, 1, sizeof(int)) == true,
                                  "Single element should return true") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_single_unchanged",
                                  single == 42,
                                  "Single element should be unchanged") && result;
    }

    // test 3: even-length reverse
    {
        int source[4] = {10, 20, 30, 40};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 4, 0);
    }

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_even_success",
                                  d_buffer_common_reverse(elements, count, sizeof(int)) == true,
                                  "Even-length reverse should succeed") && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 40) && (arr[1] == 30) &&
                           (arr[2] == 20) && (arr[3] == 10);

            result = D_TEST_CHECK_MSG(_suite,
                                      "reverse_even_result",
                                      correct,
                                      "Elements should be [40, 30, 20, 10]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    // test 4: odd-length reverse
    {
        int source[5] = {1, 2, 3, 4, 5};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 5, 0);
    }

    if (elements)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "reverse_odd_success",
                                  d_buffer_common_reverse(elements, count, sizeof(int)) == true,
                                  "Odd-length reverse should succeed") && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 5) && (arr[1] == 4) &&
                           (arr[2] == 3) && (arr[3] == 2) &&
                           (arr[4] == 1);

            result = D_TEST_CHECK_MSG(_suite,
                                      "reverse_odd_result",
                                      correct,
                                      "Elements should be [5, 4, 3, 2, 1]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/**
 * @brief Part 1 of d_tests_buffer_common_sort.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: NULL elements should not crash
 *   - test 2: NULL comparator should not crash
 *   - test 3: single element no-op
 *   - test 4: sort unsorted buffer
 */
static bool
d_internal_tests_buffer_common_sort_part1(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    result = true;

    // test 1: NULL elements should not crash
    d_buffer_common_sort(NULL, 5, sizeof(int),
                         int_comparator_ordering);
    result = D_TEST_CHECK_MSG(_suite,
                              "sort_null_elements",
                              true,
                              "NULL elements should not crash") && result;

    // test 2: NULL comparator should not crash
    {
        int buf[3] = {3, 1, 2};

        d_buffer_common_sort(buf, 3, sizeof(int), NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_null_comparator",
                                  true,
                                  "NULL comparator should not crash") && result;
    }

    // test 3: single element no-op
    {
        int single = 42;

        d_buffer_common_sort(&single, 1, sizeof(int),
                             int_comparator_ordering);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_single",
                                  single == 42,
                                  "Single element should remain unchanged") && result;
    }

    // test 4: sort unsorted buffer
    {
        int source[5] = {50, 30, 10, 40, 20};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 5, 0);
    }

    if (elements)
    {
        d_buffer_common_sort(elements, count, sizeof(int),
                             int_comparator_ordering);

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30) && (arr[3] == 40) &&
                           (arr[4] == 50);

            result = D_TEST_CHECK_MSG(_suite,
                                      "sort_unsorted_result",
                                      correct,
                                      "Elements should be [10, 20, 30, 40, 50]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_buffer_common_sort.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 5: already sorted buffer
 */
static bool
d_internal_tests_buffer_common_sort_part2(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    result = true;

    // test 5: already sorted buffer
    {
        int source[4] = {1, 2, 3, 4};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 4, 0);
    }

    if (elements)
    {
        d_buffer_common_sort(elements, count, sizeof(int),
                             int_comparator_ordering);

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 1) && (arr[1] == 2) &&
                           (arr[2] == 3) && (arr[3] == 4);

            result = D_TEST_CHECK_MSG(_suite,
                                      "sort_already_sorted",
                                      correct,
                                      "Already sorted should remain [1, 2, 3, 4]") && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_sort
  Tests the d_buffer_common_sort function.
  Tests the following:
  - NULL elements does not crash
  - NULL comparator does not crash
  - single element is a no-op
  - successful sort of unsorted buffer
  - already-sorted buffer remains correct
*/
bool d_tests_buffer_common_sort(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "buffer_common_sort");

    result = true;
    result &= d_internal_tests_buffer_common_sort_part1(_suite);
    result &= d_internal_tests_buffer_common_sort_part2(_suite);

    return result;
}


/*
d_tests_buffer_common_ordering_all
  Aggregation function that runs all ordering tests.
*/
bool d_tests_buffer_common_ordering_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_ordering_all");

    result = true;

    result = d_tests_buffer_common_reverse(_suite) && result;
    result = d_tests_buffer_common_sort(_suite) && result;

    return result;
}
