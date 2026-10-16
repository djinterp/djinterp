/*******************************************************************************
* djinterp [container]                           buffer_common_tests_sa_search.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_search.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


// int_comparator
//   internal: comparison function for int elements used in search tests.
static int
int_comparator(const void* _a, const void* _b)
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
d_tests_sa_buffer_common_contains
  Tests the d_buffer_common_contains function.
  Tests the following:
  - NULL elements returns false
  - NULL comparator returns false
  - value present returns true
  - value absent returns false
*/
bool
d_tests_sa_buffer_common_contains
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    search_val;

    result = true;

    // test 1: NULL elements
    search_val = 10;
    result     = d_assert_standalone(
        d_buffer_common_contains(NULL, 5, sizeof(int),
                                 &search_val, int_comparator) == false,
        "contains_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // test 2: NULL comparator
    {
        int source[3] = {1, 2, 3};

        result = d_assert_standalone(
            d_buffer_common_contains(source, 3, sizeof(int),
                                     &search_val, NULL) == false,
            "contains_null_comparator",
            "NULL comparator should return false",
            _counter) && result;
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
        result     = d_assert_standalone(
            d_buffer_common_contains(elements, count, sizeof(int),
                                     &search_val,
                                     int_comparator) == true,
            "contains_present",
            "Present value should return true",
            _counter) && result;

        // test 4: value absent
        search_val = 99;
        result     = d_assert_standalone(
            d_buffer_common_contains(elements, count, sizeof(int),
                                     &search_val,
                                     int_comparator) == false,
            "contains_absent",
            "Absent value should return false",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_find
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
bool
d_tests_sa_buffer_common_find
(
    struct d_test_counter* _counter
)
{
    bool    result;
    void*   elements;
    size_t  count;
    size_t  capacity;
    int     search_val;
    ssize_t found;

    result = true;

    // test 1: NULL elements
    search_val = 10;
    result     = d_assert_standalone(
        d_buffer_common_find(NULL, 5, sizeof(int),
                             &search_val, int_comparator) == -1,
        "find_null_elements",
        "NULL elements should return -1",
        _counter) && result;

    // test 2: NULL value
    {
        int buf[3] = {1, 2, 3};

        result = d_assert_standalone(
            d_buffer_common_find(buf, 3, sizeof(int),
                                 NULL, int_comparator) == -1,
            "find_null_value",
            "NULL value should return -1",
            _counter) && result;
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
        result     = d_assert_standalone(
            found == 2,
            "find_present",
            "Value 30 should be at index 2",
            _counter) && result;

        // test 4: find first occurrence of duplicate
        search_val = 20;
        found      = d_buffer_common_find(elements, count, sizeof(int),
                                           &search_val, int_comparator);
        result     = d_assert_standalone(
            found == 1,
            "find_first_duplicate",
            "First occurrence of 20 should be at index 1",
            _counter) && result;

        // test 5: find absent value
        search_val = 99;
        found      = d_buffer_common_find(elements, count, sizeof(int),
                                           &search_val, int_comparator);
        result     = d_assert_standalone(
            found == -1,
            "find_absent",
            "Absent value should return -1",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_find_last
  Tests the d_buffer_common_find_last function.
  Tests the following:
  - NULL elements returns -1
  - empty buffer returns -1
  - value present returns last occurrence index
  - value absent returns -1
*/
bool
d_tests_sa_buffer_common_find_last
(
    struct d_test_counter* _counter
)
{
    bool    result;
    void*   elements;
    size_t  count;
    size_t  capacity;
    int     search_val;
    ssize_t found;

    result = true;

    // test 1: NULL elements
    search_val = 10;
    result     = d_assert_standalone(
        d_buffer_common_find_last(NULL, 5, sizeof(int),
                                  &search_val, int_comparator) == -1,
        "find_last_null_elements",
        "NULL elements should return -1",
        _counter) && result;

    // test 2: empty buffer
    result = d_assert_standalone(
        d_buffer_common_find_last(&search_val, 0, sizeof(int),
                                  &search_val, int_comparator) == -1,
        "find_last_empty",
        "Empty buffer should return -1",
        _counter) && result;

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
        result     = d_assert_standalone(
            found == 5,
            "find_last_duplicate",
            "Last occurrence of 20 should be at index 5",
            _counter) && result;

        // test 4: find last absent value
        search_val = 99;
        found      = d_buffer_common_find_last(elements, count,
                                                sizeof(int),
                                                &search_val,
                                                int_comparator);
        result     = d_assert_standalone(
            found == -1,
            "find_last_absent",
            "Absent value should return -1",
            _counter) && result;

        // test 5: unique value returns its index
        search_val = 10;
        found      = d_buffer_common_find_last(elements, count,
                                                sizeof(int),
                                                &search_val,
                                                int_comparator);
        result     = d_assert_standalone(
            found == 0,
            "find_last_unique",
            "Unique value 10 should be at index 0",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_search_all
  Aggregation function that runs all search tests.
*/
bool
d_tests_sa_buffer_common_search_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Search Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_buffer_common_contains(_counter) && result;
    result = d_tests_sa_buffer_common_find(_counter) && result;
    result = d_tests_sa_buffer_common_find_last(_counter) && result;

    return result;
}
