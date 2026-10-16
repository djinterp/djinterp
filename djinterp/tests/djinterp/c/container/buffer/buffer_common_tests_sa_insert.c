/*******************************************************************************
* djinterp [container]                           buffer_common_tests_sa_insert.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_insert.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_append_element
  Tests the d_buffer_common_append_element function.
  Tests the following:
  - NULL elements rejection
  - NULL count rejection
  - NULL value rejection
  - zero element_size rejection
  - full buffer rejection
  - successful append to empty buffer
  - successful append to non-empty buffer
*/
bool
d_tests_sa_buffer_common_append_element
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    result = true;

    // test 1: NULL elements should fail
    count = 0;
    value = 42;
    result = d_assert_standalone(
        d_buffer_common_append_element(NULL, &count, 10,
                                       sizeof(int), &value) == false,
        "append_elem_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // test 2: NULL count should fail
    {
        int buf[10] = {0};

        result = d_assert_standalone(
            d_buffer_common_append_element(buf, NULL, 10,
                                           sizeof(int), &value) == false,
            "append_elem_null_count",
            "NULL count should return false",
            _counter) && result;
    }

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_sized(&elements, &count, &capacity,
                               sizeof(int), 4);

    if (elements)
    {
        // test 3: successful append to empty buffer
        value  = 100;
        result = d_assert_standalone(
            d_buffer_common_append_element(elements, &count, capacity,
                                           sizeof(int), &value) == true,
            "append_elem_success",
            "Append to empty should succeed",
            _counter) && result;

        result = d_assert_standalone(
            count == 1 && ((int*)elements)[0] == 100,
            "append_elem_result",
            "Count=1, element[0]=100",
            _counter) && result;

        // test 4: append to non-empty buffer
        value  = 200;
        result = d_assert_standalone(
            d_buffer_common_append_element(elements, &count, capacity,
                                           sizeof(int), &value) == true,
            "append_elem_nonempty",
            "Append to non-empty should succeed",
            _counter) && result;

        result = d_assert_standalone(
            count == 2 && ((int*)elements)[1] == 200,
            "append_elem_nonempty_result",
            "Count=2, element[1]=200",
            _counter) && result;

        // fill to capacity
        value = 300;
        d_buffer_common_append_element(elements, &count, capacity,
                                       sizeof(int), &value);
        value = 400;
        d_buffer_common_append_element(elements, &count, capacity,
                                       sizeof(int), &value);

        // test 5: full buffer rejection
        value  = 500;
        result = d_assert_standalone(
            d_buffer_common_append_element(elements, &count, capacity,
                                           sizeof(int), &value) == false,
            "append_elem_full",
            "Full buffer should return false",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_append_data
  Tests the d_buffer_common_append_data function.
  Tests the following:
  - NULL elements rejection
  - NULL data rejection
  - exceeding capacity rejection
  - successful append of multiple elements
  - data integrity after append
*/
bool
d_tests_sa_buffer_common_append_data
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    data[3] = {40, 50, 60};

    result = true;

    // test 1: NULL elements should fail
    count  = 0;
    result = d_assert_standalone(
        d_buffer_common_append_data(NULL, &count, 10,
                                    sizeof(int), data, 3) == false,
        "append_data_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // test 2: NULL data should fail
    {
        int buf[10] = {0};
        count = 0;

        result = d_assert_standalone(
            d_buffer_common_append_data(buf, &count, 10,
                                        sizeof(int), NULL, 3) == false,
            "append_data_null_data",
            "NULL data should return false",
            _counter) && result;
    }

    // initialize buffer with some data
    {
        int    source[2] = {10, 20};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 2, 10);
    }

    if (elements)
    {
        // test 3: successful append
        result = d_assert_standalone(
            d_buffer_common_append_data(elements, &count, capacity,
                                        sizeof(int), data, 3) == true,
            "append_data_success",
            "Append data should succeed",
            _counter) && result;

        result = d_assert_standalone(
            count == 5,
            "append_data_count",
            "Count should be 5",
            _counter) && result;

        // test 4: data integrity
        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 40) && (arr[3] == 50) &&
                           (arr[4] == 60);

            result = d_assert_standalone(
                correct,
                "append_data_values",
                "Elements should be [10, 20, 40, 50, 60]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_prepend_element
  Tests the d_buffer_common_prepend_element function.
  Tests the following:
  - NULL elements rejection
  - full buffer rejection
  - successful prepend shifts existing elements
*/
bool
d_tests_sa_buffer_common_prepend_element
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    result = true;

    // test 1: NULL elements should fail
    count = 0;
    value = 42;
    result = d_assert_standalone(
        d_buffer_common_prepend_element(NULL, &count, 10,
                                        sizeof(int), &value) == false,
        "prepend_elem_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // initialize buffer with data
    {
        int source[2] = {20, 30};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 2, 10);
    }

    if (elements)
    {
        // test 2: successful prepend
        value  = 10;
        result = d_assert_standalone(
            d_buffer_common_prepend_element(elements, &count, capacity,
                                            sizeof(int), &value) == true,
            "prepend_elem_success",
            "Prepend should succeed",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30);

            result = d_assert_standalone(
                correct && count == 3,
                "prepend_elem_shifted",
                "Elements should be [10, 20, 30]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_prepend_data
  Tests the d_buffer_common_prepend_data function.
  Tests the following:
  - NULL elements rejection
  - successful prepend of multiple elements
  - existing elements shifted correctly
*/
bool
d_tests_sa_buffer_common_prepend_data
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    prefix[2] = {1, 2};

    result = true;

    // test 1: NULL elements should fail
    count  = 0;
    result = d_assert_standalone(
        d_buffer_common_prepend_data(NULL, &count, 10,
                                     sizeof(int), prefix, 2) == false,
        "prepend_data_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // initialize buffer
    {
        int source[3] = {10, 20, 30};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 3, 10);
    }

    if (elements)
    {
        // test 2: successful prepend
        result = d_assert_standalone(
            d_buffer_common_prepend_data(elements, &count, capacity,
                                         sizeof(int), prefix, 2) == true,
            "prepend_data_success",
            "Prepend data should succeed",
            _counter) && result;

        result = d_assert_standalone(
            count == 5,
            "prepend_data_count",
            "Count should be 5",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 1) && (arr[1] == 2) &&
                           (arr[2] == 10) && (arr[3] == 20) &&
                           (arr[4] == 30);

            result = d_assert_standalone(
                correct,
                "prepend_data_values",
                "Elements should be [1, 2, 10, 20, 30]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_insert_element
  Tests the d_buffer_common_insert_element function.
  Tests the following:
  - NULL elements rejection
  - full buffer rejection
  - out-of-bounds index rejection
  - successful insert at beginning
  - successful insert at middle
  - successful insert at end
*/
bool
d_tests_sa_buffer_common_insert_element
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    result = true;

    // test 1: NULL elements should fail
    count = 0;
    value = 42;
    result = d_assert_standalone(
        d_buffer_common_insert_element(NULL, &count, 10,
                                       sizeof(int), &value, 0) == false,
        "insert_elem_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // initialize buffer
    {
        int source[2] = {10, 30};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 2, 10);
    }

    if (elements)
    {
        // test 2: out-of-bounds index rejection
        value  = 99;
        result = d_assert_standalone(
            d_buffer_common_insert_element(elements, &count, capacity,
                                           sizeof(int), &value,
                                           (d_index)100) == false,
            "insert_elem_oob",
            "Out-of-bounds index should return false",
            _counter) && result;

        // test 3: insert at middle
        value  = 20;
        result = d_assert_standalone(
            d_buffer_common_insert_element(elements, &count, capacity,
                                           sizeof(int), &value, 1) == true,
            "insert_elem_middle_success",
            "Insert at middle should succeed",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30);

            result = d_assert_standalone(
                correct && count == 3,
                "insert_elem_middle_result",
                "Elements should be [10, 20, 30]",
                _counter) && result;
        }

        // test 4: insert at end (append position)
        value  = 40;
        result = d_assert_standalone(
            d_buffer_common_insert_element(elements, &count, capacity,
                                           sizeof(int), &value,
                                           (d_index)count) == true,
            "insert_elem_end_success",
            "Insert at end should succeed",
            _counter) && result;

        result = d_assert_standalone(
            ((int*)elements)[3] == 40 && count == 4,
            "insert_elem_end_result",
            "Last element should be 40, count=4",
            _counter) && result;

        // test 5: insert at beginning
        value  = 5;
        result = d_assert_standalone(
            d_buffer_common_insert_element(elements, &count, capacity,
                                           sizeof(int), &value, 0) == true,
            "insert_elem_begin_success",
            "Insert at beginning should succeed",
            _counter) && result;

        result = d_assert_standalone(
            ((int*)elements)[0] == 5 && count == 5,
            "insert_elem_begin_result",
            "First element should be 5, count=5",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_insert_data
  Tests the d_buffer_common_insert_data function.
  Tests the following:
  - NULL elements rejection
  - out-of-bounds index rejection
  - successful range insert at middle
  - correct element ordering after insert
*/
bool
d_tests_sa_buffer_common_insert_data
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    insert_data[2] = {20, 30};

    result = true;

    // test 1: NULL elements should fail
    count  = 0;
    result = d_assert_standalone(
        d_buffer_common_insert_data(NULL, &count, 10,
                                    sizeof(int), insert_data,
                                    2, 0) == false,
        "insert_data_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // initialize buffer
    {
        int source[2] = {10, 40};

        elements = NULL;
        count    = 0;
        capacity = 0;
        d_buffer_common_init_from_data(&elements, &count, &capacity,
                                       sizeof(int), source, 2, 10);
    }

    if (elements)
    {
        // test 2: out-of-bounds index rejection
        result = d_assert_standalone(
            d_buffer_common_insert_data(elements, &count, capacity,
                                        sizeof(int), insert_data, 2,
                                        (d_index)100) == false,
            "insert_data_oob",
            "Out-of-bounds index should return false",
            _counter) && result;

        // test 3: successful insert at middle
        result = d_assert_standalone(
            d_buffer_common_insert_data(elements, &count, capacity,
                                        sizeof(int), insert_data,
                                        2, 1) == true,
            "insert_data_success",
            "Insert data at middle should succeed",
            _counter) && result;

        result = d_assert_standalone(
            count == 4,
            "insert_data_count",
            "Count should be 4",
            _counter) && result;

        {
            int* arr = (int*)elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30) && (arr[3] == 40);

            result = d_assert_standalone(
                correct,
                "insert_data_values",
                "Elements should be [10, 20, 30, 40]",
                _counter) && result;
        }

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_insert_all
  Aggregation function that runs all insertion (resize mode) tests.
*/
bool
d_tests_sa_buffer_common_insert_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Insertion (Resize Mode) Functions\n");
    printf("  ---------------------------------------------\n");

    result = d_tests_sa_buffer_common_append_element(_counter) && result;
    result = d_tests_sa_buffer_common_append_data(_counter) && result;
    result = d_tests_sa_buffer_common_prepend_element(_counter) && result;
    result = d_tests_sa_buffer_common_prepend_data(_counter) && result;
    result = d_tests_sa_buffer_common_insert_element(_counter) && result;
    result = d_tests_sa_buffer_common_insert_data(_counter) && result;

    return result;
}
