/*******************************************************************************
* djinterp [container]                         buffer_common_tests_sa_ordering.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_ordering.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


// int_comparator_ordering
//   internal: comparison function for int elements used in ordering tests.
static int
int_comparator_ordering(const void* _a, const void* _b)
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
d_tests_sa_buffer_common_reverse
  Tests the d_buffer_common_reverse function.
  Tests the following:
  - NULL elements returns false
  - single element is a no-op (returns true)
  - zero count is a no-op (returns true)
  - successful reverse of even-length buffer
  - successful reverse of odd-length buffer
*/
bool
d_tests_sa_buffer_common_reverse
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    result = true;

    // test 1: NULL elements
    result = d_assert_standalone(
        d_buffer_common_reverse(NULL, 5, sizeof(int)) == false,
        "reverse_null",
        "NULL elements should return false",
        _counter) && result;

    // test 2: single element no-op
    {
        int single = 42;

        result = d_assert_standalone(
            d_buffer_common_reverse(&single, 1, sizeof(int)) == true,
            "reverse_single",
            "Single element should return true",
            _counter) && result;

        result = d_assert_standalone(
            single == 42,
            "reverse_single_unchanged",
            "Single element should be unchanged",
            _counter) && result;
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
        result = d_assert_standalone(
            d_buffer_common_reverse(elements, count,
                                    sizeof(int)) == true,
            "reverse_even_success",
            "Even-length reverse should succeed",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 40) && (arr[1] == 30) &&
                           (arr[2] == 20) && (arr[3] == 10);

            result = d_assert_standalone(
                correct,
                "reverse_even_result",
                "Elements should be [40, 30, 20, 10]",
                _counter) && result;
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
        result = d_assert_standalone(
            d_buffer_common_reverse(elements, count,
                                    sizeof(int)) == true,
            "reverse_odd_success",
            "Odd-length reverse should succeed",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 5) && (arr[1] == 4) &&
                           (arr[2] == 3) && (arr[3] == 2) &&
                           (arr[4] == 1);

            result = d_assert_standalone(
                correct,
                "reverse_odd_result",
                "Elements should be [5, 4, 3, 2, 1]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_sort
  Tests the d_buffer_common_sort function.
  Tests the following:
  - NULL elements does not crash
  - NULL comparator does not crash
  - single element is a no-op
  - successful sort of unsorted buffer
  - already-sorted buffer remains correct
*/
bool
d_tests_sa_buffer_common_sort
(
    struct d_test_counter* _counter
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
    result = d_assert_standalone(
        true,
        "sort_null_elements",
        "NULL elements should not crash",
        _counter) && result;

    // test 2: NULL comparator should not crash
    {
        int buf[3] = {3, 1, 2};

        d_buffer_common_sort(buf, 3, sizeof(int), NULL);
        result = d_assert_standalone(
            true,
            "sort_null_comparator",
            "NULL comparator should not crash",
            _counter) && result;
    }

    // test 3: single element no-op
    {
        int single = 42;

        d_buffer_common_sort(&single, 1, sizeof(int),
                             int_comparator_ordering);
        result = d_assert_standalone(
            single == 42,
            "sort_single",
            "Single element should remain unchanged",
            _counter) && result;
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

            result = d_assert_standalone(
                correct,
                "sort_unsorted_result",
                "Elements should be [10, 20, 30, 40, 50]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

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

            result = d_assert_standalone(
                correct,
                "sort_already_sorted",
                "Already sorted should remain [1, 2, 3, 4]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_ordering_all
  Aggregation function that runs all ordering tests.
*/
bool
d_tests_sa_buffer_common_ordering_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Ordering Functions\n");
    printf("  -----------------------------\n");

    result = d_tests_sa_buffer_common_reverse(_counter) && result;
    result = d_tests_sa_buffer_common_sort(_counter) && result;

    return result;
}
