/*******************************************************************************
* djinterp [container]                          buffer_common_tests_sa_removal.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_removal.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_remove_element
  Tests the d_buffer_common_remove_element function.
  Tests the following:
  - NULL elements rejection
  - NULL count rejection
  - zero element_size rejection
  - out-of-bounds index rejection
  - successful removal at middle shifts elements
  - successful removal at last element
  - count decremented after removal
*/
bool
d_tests_sa_buffer_common_remove_element
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
    count  = 5;
    result = d_assert_standalone(
        d_buffer_common_remove_element(NULL, &count,
                                       sizeof(int), 0) == false,
        "remove_elem_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // test 2: NULL count should fail
    {
        int buf[5] = {0};

        result = d_assert_standalone(
            d_buffer_common_remove_element(buf, NULL,
                                           sizeof(int), 0) == false,
            "remove_elem_null_count",
            "NULL count should return false",
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
        // test 3: out-of-bounds index should fail
        result = d_assert_standalone(
            d_buffer_common_remove_element(elements, &count,
                                           sizeof(int), 5) == false,
            "remove_elem_oob",
            "Out-of-bounds index should return false",
            _counter) && result;

        // test 4: remove middle element
        result = d_assert_standalone(
            d_buffer_common_remove_element(elements, &count,
                                           sizeof(int), 2) == true,
            "remove_elem_middle_success",
            "Remove at middle should succeed",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (count == 4) &&
                           (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 40) && (arr[3] == 50);

            result = d_assert_standalone(
                correct,
                "remove_elem_middle_result",
                "Elements should be [10, 20, 40, 50]",
                _counter) && result;
        }

        // test 5: remove last element
        result = d_assert_standalone(
            d_buffer_common_remove_element(elements, &count,
                                           sizeof(int),
                                           (d_index)(count - 1)) == true,
            "remove_elem_last_success",
            "Remove last should succeed",
            _counter) && result;

        result = d_assert_standalone(
            count == 3,
            "remove_elem_last_count",
            "Count should be 3",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_remove_range
  Tests the d_buffer_common_remove_range function.
  Tests the following:
  - NULL elements rejection
  - invalid range (start > end) rejection
  - range exceeding count rejection
  - zero-length range is a no-op
  - successful range removal
  - remaining elements shifted correctly
*/
bool
d_tests_sa_buffer_common_remove_range
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
    count  = 5;
    result = d_assert_standalone(
        d_buffer_common_remove_range(NULL, &count,
                                     sizeof(int), 0, 2) == false,
        "remove_range_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // initialize buffer
    {
        int source[6] = {10, 20, 30, 40, 50, 60};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 6, 0);
    }

    if (elements)
    {
        // test 2: invalid range (start > end)
        result = d_assert_standalone(
            d_buffer_common_remove_range(elements, &count,
                                         sizeof(int), 3, 1) == false,
            "remove_range_invalid",
            "start > end should return false",
            _counter) && result;

        // test 3: range exceeding count
        result = d_assert_standalone(
            d_buffer_common_remove_range(elements, &count,
                                         sizeof(int), 0, 100) == false,
            "remove_range_exceeds",
            "Range exceeding count should return false",
            _counter) && result;

        // test 4: zero-length range is a no-op
        result = d_assert_standalone(
            d_buffer_common_remove_range(elements, &count,
                                         sizeof(int), 2, 2) == true,
            "remove_range_zero_len",
            "Zero-length range should return true",
            _counter) && result;

        result = d_assert_standalone(
            count == 6,
            "remove_range_zero_len_count",
            "Count should remain 6",
            _counter) && result;

        // test 5: successful range removal [1, 4)
        result = d_assert_standalone(
            d_buffer_common_remove_range(elements, &count,
                                         sizeof(int), 1, 4) == true,
            "remove_range_success",
            "Remove range [1,4) should succeed",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (count == 3) &&
                           (arr[0] == 10) && (arr[1] == 50) &&
                           (arr[2] == 60);

            result = d_assert_standalone(
                correct,
                "remove_range_result",
                "Elements should be [10, 50, 60]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_consume_front
  Tests the d_buffer_common_consume_front function.
  Tests the following:
  - NULL elements rejection
  - amount exceeding count rejection
  - zero amount is a no-op
  - successful consume shifts remaining elements
*/
bool
d_tests_sa_buffer_common_consume_front
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
    count  = 5;
    result = d_assert_standalone(
        d_buffer_common_consume_front(NULL, &count,
                                      sizeof(int), 2) == false,
        "consume_front_null",
        "NULL elements should return false",
        _counter) && result;

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
        // test 2: amount exceeding count should fail
        result = d_assert_standalone(
            d_buffer_common_consume_front(elements, &count,
                                          sizeof(int), 100) == false,
            "consume_front_exceeds",
            "Amount exceeding count should return false",
            _counter) && result;

        // test 3: zero amount is a no-op
        result = d_assert_standalone(
            d_buffer_common_consume_front(elements, &count,
                                          sizeof(int), 0) == true,
            "consume_front_zero",
            "Zero amount should return true",
            _counter) && result;

        // test 4: successful consume of 2 from front
        result = d_assert_standalone(
            d_buffer_common_consume_front(elements, &count,
                                          sizeof(int), 2) == true,
            "consume_front_success",
            "Consume front(2) should succeed",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (count == 3) &&
                           (arr[0] == 30) && (arr[1] == 40) &&
                           (arr[2] == 50);

            result = d_assert_standalone(
                correct,
                "consume_front_result",
                "Elements should be [30, 40, 50]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_consume_back
  Tests the d_buffer_common_consume_back function.
  Tests the following:
  - NULL elements rejection
  - amount exceeding count rejection
  - successful consume from back
  - front elements preserved
*/
bool
d_tests_sa_buffer_common_consume_back
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
    count  = 5;
    result = d_assert_standalone(
        d_buffer_common_consume_back(NULL, &count,
                                     sizeof(int), 2) == false,
        "consume_back_null",
        "NULL elements should return false",
        _counter) && result;

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
        // test 2: amount exceeding count should fail
        result = d_assert_standalone(
            d_buffer_common_consume_back(elements, &count,
                                         sizeof(int), 100) == false,
            "consume_back_exceeds",
            "Amount exceeding count should return false",
            _counter) && result;

        // test 3: successful consume of 2 from back
        result = d_assert_standalone(
            d_buffer_common_consume_back(elements, &count,
                                         sizeof(int), 2) == true,
            "consume_back_success",
            "Consume back(2) should succeed",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (count == 3) &&
                           (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30);

            result = d_assert_standalone(
                correct,
                "consume_back_result",
                "Elements should be [10, 20, 30]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_removal_all
  Aggregation function that runs all removal tests.
*/
bool
d_tests_sa_buffer_common_removal_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Removal Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_buffer_common_remove_element(_counter) && result;
    result = d_tests_sa_buffer_common_remove_range(_counter) && result;
    result = d_tests_sa_buffer_common_consume_front(_counter) && result;
    result = d_tests_sa_buffer_common_consume_back(_counter) && result;

    return result;
}
