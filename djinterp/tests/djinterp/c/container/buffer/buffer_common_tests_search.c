/*******************************************************************************
* djinterp [container]                              buffer_common_tests_search.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_search.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// int_comparator
//   internal: comparison function for int elements used in search tests.
static
int int_comparator(
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
d_tests_buffer_common_contains
  Tests the d_buffer_common_contains function.
  Tests the following:
  - NULL elements returns false
  - NULL comparator returns false
  - value present returns true
  - value absent returns false
*/
bool d_tests_buffer_common_contains(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    search_val;

    d_test_suite_test(_suite, "buffer_common_contains");

    result = true;

    // test 1: NULL elements
    search_val = 10;
    result     = D_TEST_CHECK_MSG(_suite,
                                  "contains_null_elements",
                                  d_buffer_common_contains(NULL, 5, sizeof(int), &search_val, int_comparator) == false,
                                  "NULL elements should return false") && result;

    // test 2: NULL comparator
    {
        int source[3] = {1, 2, 3};

        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_null_comparator",
                                  d_buffer_common_contains(source, 3, sizeof(int), &search_val, NULL) == false,
                                  "NULL comparator should return false") && result;
    }

    // initialize buffer
    {
        int source[5] = {10, 20, 30, 40, 50};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 5, 0);
    }

    if (elements)
    {
        // test 3: value present
        search_val = 30;
        result     = D_TEST_CHECK_MSG(_suite,
                                      "contains_present",
                                      d_buffer_common_contains(elements, count, sizeof(int), &search_val, int_comparator) == true,
                                      "Present value should return true") && result;

        // test 4: value absent
        search_val = 99;
        result     = D_TEST_CHECK_MSG(_suite,
                                      "contains_absent",
                                      d_buffer_common_contains(elements, count, sizeof(int), &search_val, int_comparator) == false,
                                      "Absent value should return false") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_find
  Tests the d_buffer_common_find function.
  Tests the following:
  - NULL elements returns -1
  - NULL value returns -1
  - NULL comparator returns -1
  - zero element_size returns -1
  - value present returns correct index
  - value absent returns -1
  - finds first occurrence when duplicates exist
*/
bool d_tests_buffer_common_find(
    struct dj_test_suite* _suite
)
{
    bool    result;
    void*   elements;
    size_t  count;
    size_t  capacity;
    int     search_val;
    ssize_t found;

    d_test_suite_test(_suite, "buffer_common_find");

    result = true;

    // test 1: NULL elements
    search_val = 10;
    result     = D_TEST_CHECK_MSG(_suite,
                                  "find_null_elements",
                                  d_buffer_common_find(NULL, 5, sizeof(int), &search_val, int_comparator) == -1,
                                  "NULL elements should return -1") && result;

    // test 2: NULL value
    {
        int buf[3] = {1, 2, 3};

        result = D_TEST_CHECK_MSG(_suite,
                                  "find_null_value",
                                  d_buffer_common_find(buf, 3, sizeof(int), NULL, int_comparator) == -1,
                                  "NULL value should return -1") && result;
    }

    // initialize buffer with duplicates
    {
        int source[6] = {10, 20, 30, 20, 40, 50};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 6, 0);
    }

    if (elements)
    {
        // test 3: find present value
        search_val = 30;
        found      = d_buffer_common_find(elements, count, sizeof(int),
                                           &search_val, int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_present",
                                      found == 2,
                                      "Value 30 should be at index 2") && result;

        // test 4: find first occurrence of duplicate
        search_val = 20;
        found      = d_buffer_common_find(elements, count, sizeof(int),
                                           &search_val, int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_first_duplicate",
                                      found == 1,
                                      "First occurrence of 20 should be at index 1") && result;

        // test 5: find absent value
        search_val = 99;
        found      = d_buffer_common_find(elements, count, sizeof(int),
                                           &search_val, int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_absent",
                                      found == -1,
                                      "Absent value should return -1") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_find_last
  Tests the d_buffer_common_find_last function.
  Tests the following:
  - NULL elements returns -1
  - empty buffer returns -1
  - value present returns last occurrence index
  - value absent returns -1
*/
bool d_tests_buffer_common_find_last(
    struct dj_test_suite* _suite
)
{
    bool    result;
    void*   elements;
    size_t  count;
    size_t  capacity;
    int     search_val;
    ssize_t found;

    d_test_suite_test(_suite, "buffer_common_find_last");

    result = true;

    // test 1: NULL elements
    search_val = 10;
    result     = D_TEST_CHECK_MSG(_suite,
                                  "find_last_null_elements",
                                  d_buffer_common_find_last(NULL, 5, sizeof(int), &search_val, int_comparator) == -1,
                                  "NULL elements should return -1") && result;

    // test 2: empty buffer
    result = D_TEST_CHECK_MSG(_suite,
                              "find_last_empty",
                              d_buffer_common_find_last(&search_val, 0, sizeof(int), &search_val, int_comparator) == -1,
                              "Empty buffer should return -1") && result;

    // initialize buffer with duplicates
    {
        int source[6] = {10, 20, 30, 20, 40, 20};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 6, 0);
    }

    if (elements)
    {
        // test 3: find last occurrence of duplicate
        search_val = 20;
        found      = d_buffer_common_find_last(elements, count,
                                                sizeof(int),
                                                &search_val,
                                                int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_last_duplicate",
                                      found == 5,
                                      "Last occurrence of 20 should be at index 5") && result;

        // test 4: find last absent value
        search_val = 99;
        found      = d_buffer_common_find_last(elements, count,
                                                sizeof(int),
                                                &search_val,
                                                int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_last_absent",
                                      found == -1,
                                      "Absent value should return -1") && result;

        // test 5: unique value returns its index
        search_val = 10;
        found      = d_buffer_common_find_last(elements, count,
                                                sizeof(int),
                                                &search_val,
                                                int_comparator);
        result     = D_TEST_CHECK_MSG(_suite,
                                      "find_last_unique",
                                      found == 0,
                                      "Unique value 10 should be at index 0") && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_buffer_common_search_all
  Aggregation function that runs all search tests.
*/
bool d_tests_buffer_common_search_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_search_all");

    result = true;

    result = d_tests_buffer_common_contains(_suite) && result;
    result = d_tests_buffer_common_find(_suite) && result;
    result = d_tests_buffer_common_find_last(_suite) && result;

    return result;
}
