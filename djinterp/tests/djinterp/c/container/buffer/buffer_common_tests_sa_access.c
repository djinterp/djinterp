/*******************************************************************************
* djinterp [container]                           buffer_common_tests_sa_access.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_access.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_get_element
  Tests the d_buffer_common_get_element function.
  Tests the following:
  - NULL elements returns NULL
  - zero element_size returns NULL
  - out-of-bounds index returns NULL
  - successful retrieval of first, middle, and last elements
*/
bool
d_tests_sa_buffer_common_get_element
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[5] = {10, 20, 30, 40, 50};
    void*  got;

    result = true;

    // test 1: NULL elements should return NULL
    result = d_assert_standalone(
        d_buffer_common_get_element(NULL, 5, sizeof(int), 0) == NULL,
        "get_element_null",
        "NULL elements should return NULL",
        _counter) && result;

    // test 2: zero element_size should return NULL
    result = d_assert_standalone(
        d_buffer_common_get_element(source, 5, 0, 0) == NULL,
        "get_element_zero_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_from_data(&elements, &count, &capacity,
                                   sizeof(int), source, 5, 0);

    if (elements)
    {
        // test 3: out-of-bounds index returns NULL
        result = d_assert_standalone(
            d_buffer_common_get_element(elements, count,
                                        sizeof(int), 5) == NULL,
            "get_element_oob",
            "Out-of-bounds index should return NULL",
            _counter) && result;

        // test 4: get first element
        got = d_buffer_common_get_element(elements, count,
                                           sizeof(int), 0);
        result = d_assert_standalone(
            got != NULL && *(int*)got == 10,
            "get_element_first",
            "First element should be 10",
            _counter) && result;

        // test 5: get middle element
        got = d_buffer_common_get_element(elements, count,
                                           sizeof(int), 2);
        result = d_assert_standalone(
            got != NULL && *(int*)got == 30,
            "get_element_middle",
            "Middle element should be 30",
            _counter) && result;

        // test 6: get last element
        got = d_buffer_common_get_element(elements, count,
                                           sizeof(int), 4);
        result = d_assert_standalone(
            got != NULL && *(int*)got == 50,
            "get_element_last",
            "Last element should be 50",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_set_element
  Tests the d_buffer_common_set_element function.
  Tests the following:
  - NULL elements rejection
  - NULL value rejection
  - zero element_size rejection
  - out-of-bounds index rejection
  - successful set overwrites the correct element
*/
bool
d_tests_sa_buffer_common_set_element
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {10, 20, 30};
    int    new_val;

    result = true;

    // test 1: NULL elements should fail
    new_val = 99;
    result  = d_assert_standalone(
        d_buffer_common_set_element(NULL, 3, sizeof(int),
                                    0, &new_val) == false,
        "set_element_null_elements",
        "NULL elements should return false",
        _counter) && result;

    // test 2: NULL value should fail
    result = d_assert_standalone(
        d_buffer_common_set_element(source, 3, sizeof(int),
                                    0, NULL) == false,
        "set_element_null_value",
        "NULL value should return false",
        _counter) && result;

    // test 3: zero element_size should fail
    result = d_assert_standalone(
        d_buffer_common_set_element(source, 3, 0,
                                    0, &new_val) == false,
        "set_element_zero_size",
        "Zero element_size should return false",
        _counter) && result;

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_from_data(&elements, &count, &capacity,
                                   sizeof(int), source, 3, 0);

    if (elements)
    {
        // test 4: out-of-bounds index should fail
        result = d_assert_standalone(
            d_buffer_common_set_element(elements, count, sizeof(int),
                                        3, &new_val) == false,
            "set_element_oob",
            "Out-of-bounds index should return false",
            _counter) && result;

        // test 5: successful set
        new_val = 999;
        result  = d_assert_standalone(
            d_buffer_common_set_element(elements, count, sizeof(int),
                                        1, &new_val) == true,
            "set_element_success",
            "Set element should succeed",
            _counter) && result;

        result = d_assert_standalone(
            ((int*)elements)[1] == 999,
            "set_element_value",
            "Element at index 1 should be 999",
            _counter) && result;

        // verify neighbouring elements unchanged
        result = d_assert_standalone(
            ((int*)elements)[0] == 10 && ((int*)elements)[2] == 30,
            "set_element_neighbours",
            "Neighbouring elements should be unchanged",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_peek_front
  Tests the d_buffer_common_peek_front function.
  Tests the following:
  - NULL elements returns NULL
  - zero count returns NULL
  - zero element_size returns NULL
  - successful peek returns pointer to first element
*/
bool
d_tests_sa_buffer_common_peek_front
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {11, 22, 33};
    void*  front;

    result = true;

    // test 1: NULL elements
    result = d_assert_standalone(
        d_buffer_common_peek_front(NULL, 3, sizeof(int)) == NULL,
        "peek_front_null",
        "NULL elements should return NULL",
        _counter) && result;

    // test 2: zero count
    result = d_assert_standalone(
        d_buffer_common_peek_front(source, 0, sizeof(int)) == NULL,
        "peek_front_zero_count",
        "Zero count should return NULL",
        _counter) && result;

    // test 3: zero element_size
    result = d_assert_standalone(
        d_buffer_common_peek_front(source, 3, 0) == NULL,
        "peek_front_zero_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_from_data(&elements, &count, &capacity,
                                   sizeof(int), source, 3, 0);

    if (elements)
    {
        // test 4: successful peek
        front = d_buffer_common_peek_front(elements, count,
                                            sizeof(int));
        result = d_assert_standalone(
            front != NULL && *(int*)front == 11,
            "peek_front_success",
            "Front element should be 11",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_peek_back
  Tests the d_buffer_common_peek_back function.
  Tests the following:
  - NULL elements returns NULL
  - zero count returns NULL
  - zero element_size returns NULL
  - successful peek returns pointer to last element
*/
bool
d_tests_sa_buffer_common_peek_back
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {11, 22, 33};
    void*  back;

    result = true;

    // test 1: NULL elements
    result = d_assert_standalone(
        d_buffer_common_peek_back(NULL, 3, sizeof(int)) == NULL,
        "peek_back_null",
        "NULL elements should return NULL",
        _counter) && result;

    // test 2: zero count
    result = d_assert_standalone(
        d_buffer_common_peek_back(source, 0, sizeof(int)) == NULL,
        "peek_back_zero_count",
        "Zero count should return NULL",
        _counter) && result;

    // test 3: zero element_size
    result = d_assert_standalone(
        d_buffer_common_peek_back(source, 3, 0) == NULL,
        "peek_back_zero_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // initialize test buffer
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_buffer_common_init_from_data(&elements, &count, &capacity,
                                   sizeof(int), source, 3, 0);

    if (elements)
    {
        // test 4: successful peek
        back = d_buffer_common_peek_back(elements, count,
                                          sizeof(int));
        result = d_assert_standalone(
            back != NULL && *(int*)back == 33,
            "peek_back_success",
            "Back element should be 33",
            _counter) && result;

        d_buffer_common_free_data(elements);
    }

    return result;
}


/*
d_tests_sa_buffer_common_access_all
  Aggregation function that runs all element access tests.
*/
bool
d_tests_sa_buffer_common_access_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Element Access Functions\n");
    printf("  -----------------------------------\n");

    result = d_tests_sa_buffer_common_get_element(_counter) && result;
    result = d_tests_sa_buffer_common_set_element(_counter) && result;
    result = d_tests_sa_buffer_common_peek_front(_counter) && result;
    result = d_tests_sa_buffer_common_peek_back(_counter) && result;

    return result;
}
