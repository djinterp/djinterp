#include "./vector_common_tests_sa.h"


/*
d_tests_sa_vector_common_push_back
  Tests the d_vector_common_push_back function for appending elements.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - NULL value rejection
  - successful push to empty vector
  - successful push to non-empty vector
  - automatic capacity growth when full
  - count correctly incremented
*/
bool
d_tests_sa_vector_common_push_back
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

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = d_assert_standalone(
        d_vector_common_push_back(NULL,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  &value) == D_FAILURE,
        "push_back_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_push_back(&elements,
                                  NULL,
                                  &capacity,
                                  sizeof(int),
                                  &value) == D_FAILURE,
        "push_back_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: NULL value should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_push_back(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  NULL) == D_FAILURE,
        "push_back_null_value",
        "NULL value should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: successful push to empty vector
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 5);

    value  = 100;
    result = d_assert_standalone(
        d_vector_common_push_back(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  &value) == D_SUCCESS,
        "push_back_empty_success",
        "Push to empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 1,
        "push_back_empty_count",
        "Count should be 1 after first push",
        _counter) && result;

    result = d_assert_standalone(
        ((int*)elements)[0] == 100,
        "push_back_empty_value",
        "Pushed value should be at index 0",
        _counter) && result;

    // test 5: successful push to non-empty vector
    value  = 200;
    result = d_assert_standalone(
        d_vector_common_push_back(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  &value) == D_SUCCESS,
        "push_back_nonempty_success",
        "Push to non-empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 2 && ((int*)elements)[1] == 200,
        "push_back_nonempty_value",
        "Second value should be at index 1",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    // test 6: automatic capacity growth when full
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 2);

    value = 1;
    d_vector_common_push_back(&elements, &count, &capacity, sizeof(int), &value);
    value = 2;
    d_vector_common_push_back(&elements, &count, &capacity, sizeof(int), &value);

    size_t old_capacity = capacity;
    value               = 3;
    result              = d_assert_standalone(
        d_vector_common_push_back(&elements,
                                  &count,
                                  &capacity,
                                  sizeof(int),
                                  &value) == D_SUCCESS,
        "push_back_grow_success",
        "Push beyond capacity should succeed (with growth)",
        _counter) && result;

    result = d_assert_standalone(
        capacity > old_capacity,
        "push_back_grow_capacity",
        "Capacity should have grown",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_push_front
  Tests the d_vector_common_push_front function for prepending elements.
  Tests the following:
  - NULL parameter handling
  - successful push to empty vector
  - successful push to non-empty vector (shifts existing)
*/
bool
d_tests_sa_vector_common_push_front
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

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = d_assert_standalone(
        d_vector_common_push_front(NULL,
                                   &count,
                                   &capacity,
                                   sizeof(int),
                                   &value) == D_FAILURE,
        "push_front_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: successful push to empty vector
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 5);

    value  = 100;
    result = d_assert_standalone(
        d_vector_common_push_front(&elements,
                                   &count,
                                   &capacity,
                                   sizeof(int),
                                   &value) == D_SUCCESS,
        "push_front_empty_success",
        "Push front to empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 1 && ((int*)elements)[0] == 100,
        "push_front_empty_value",
        "Pushed value should be at index 0",
        _counter) && result;

    // test 3: push to non-empty vector (shifts existing)
    value  = 200;
    result = d_assert_standalone(
        d_vector_common_push_front(&elements,
                                   &count,
                                   &capacity,
                                   sizeof(int),
                                   &value) == D_SUCCESS,
        "push_front_nonempty_success",
        "Push front to non-empty vector should succeed",
        _counter) && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 200) && (arr[1] == 100);

        result = d_assert_standalone(
            correct,
            "push_front_shifted",
            "Elements should be shifted right",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_pop_back
  Tests the d_vector_common_pop_back function for removing the last element.
  Tests the following:
  - NULL parameter handling
  - empty vector rejection
  - successful pop with output parameter
  - successful pop without output parameter
*/
bool
d_tests_sa_vector_common_pop_back
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    out_value;

    result = true;

    // test 1: NULL elements should fail
    count  = 1;
    result = d_assert_standalone(
        d_vector_common_pop_back(NULL,
                                 &count,
                                 sizeof(int),
                                 &out_value) == D_FAILURE,
        "pop_back_null_elements",
        "NULL elements should return D_FAILURE",
        _counter) && result;

    // test 2: empty vector should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = d_assert_standalone(
        d_vector_common_pop_back(elements,
                                 &count,
                                 sizeof(int),
                                 &out_value) == D_FAILURE,
        "pop_back_empty",
        "Empty vector should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: successful pop with output
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

    out_value = 0;
    result    = d_assert_standalone(
        d_vector_common_pop_back(elements,
                                 &count,
                                 sizeof(int),
                                 &out_value) == D_SUCCESS,
        "pop_back_valid_success",
        "Pop back should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        out_value == 30 && count == 2,
        "pop_back_valid_result",
        "Should pop value 30 and decrement count",
        _counter) && result;

    // test 4: pop with NULL out_value
    result = d_assert_standalone(
        d_vector_common_pop_back(elements,
                                 &count,
                                 sizeof(int),
                                 NULL) == D_SUCCESS,
        "pop_back_null_out",
        "Pop with NULL out_value should succeed",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_pop_front
  Tests the d_vector_common_pop_front function for removing the first element.
  Tests the following:
  - NULL parameter handling
  - empty vector rejection
  - successful pop with output parameter
  - remaining elements correctly shifted
*/
bool
d_tests_sa_vector_common_pop_front
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    out_value;

    result = true;

    // test 1: NULL elements should fail
    count  = 1;
    result = d_assert_standalone(
        d_vector_common_pop_front(NULL,
                                  &count,
                                  sizeof(int),
                                  &out_value) == D_FAILURE,
        "pop_front_null_elements",
        "NULL elements should return D_FAILURE",
        _counter) && result;

    // test 2: empty vector should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = d_assert_standalone(
        d_vector_common_pop_front(elements,
                                  &count,
                                  sizeof(int),
                                  &out_value) == D_FAILURE,
        "pop_front_empty",
        "Empty vector should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: successful pop
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

    out_value = 0;
    result    = d_assert_standalone(
        d_vector_common_pop_front(elements,
                                  &count,
                                  sizeof(int),
                                  &out_value) == D_SUCCESS,
        "pop_front_valid_success",
        "Pop front should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        out_value == 10 && count == 2,
        "pop_front_valid_result",
        "Should pop value 10 and decrement count",
        _counter) && result;

    // test 4: verify elements shifted
    {
        int* arr     = (int*)elements;
        bool shifted = (arr[0] == 20) && (arr[1] == 30);

        result = d_assert_standalone(
            shifted,
            "pop_front_shifted",
            "Remaining elements should be shifted left",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_insert_element
  Tests the d_vector_common_insert_element function for inserting at specific index.
  Tests the following:
  - NULL parameter handling
  - successful insert at beginning, middle, and end
  - negative index support
*/
bool
d_tests_sa_vector_common_insert_element
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
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = d_assert_standalone(
        d_vector_common_insert_element(NULL,
                               &count,
                               &capacity,
                               sizeof(int),
                               0,
                               &value) == D_FAILURE,
        "insert_null_elements",
        "NULL elements should return D_FAILURE",
        _counter) && result;

    // test 2: successful insert at beginning
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 20;
        arr[1] = 30;
        count  = 2;
    }

    value  = 10;
    result = d_assert_standalone(
        d_vector_common_insert_element(&elements,
                               &count,
                               &capacity,
                               sizeof(int),
                               0,
                               &value) == D_SUCCESS,
        "insert_beginning_success",
        "Insert at beginning should succeed",
        _counter) && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

        result = d_assert_standalone(
            correct && count == 3,
            "insert_beginning_order",
            "Elements should be [10, 20, 30]",
            _counter) && result;
    }

    // test 3: insert at middle
    value  = 25;
    result = d_assert_standalone(
        d_vector_common_insert_element(&elements,
                               &count,
                               &capacity,
                               sizeof(int),
                               2,
                               &value) == D_SUCCESS,
        "insert_middle_success",
        "Insert at middle should succeed",
        _counter) && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[2] == 25) && (arr[3] == 30);

        result = d_assert_standalone(
            correct,
            "insert_middle_order",
            "25 should be at index 2, 30 shifted to 3",
            _counter) && result;
    }

    // test 4: insert at end
    value  = 40;
    result = d_assert_standalone(
        d_vector_common_insert_element(&elements,
                               &count,
                               &capacity,
                               sizeof(int),
                               (d_index)count,
                               &value) == D_SUCCESS,
        "insert_end_success",
        "Insert at end should succeed",
        _counter) && result;

    result = d_assert_standalone(
        ((int*)elements)[count - 1] == 40,
        "insert_end_value",
        "40 should be at end",
        _counter) && result;

    // test 5: negative index
    value  = 99;
    result = d_assert_standalone(
        d_vector_common_insert_element(&elements,
                               &count,
                               &capacity,
                               sizeof(int),
                               -1,
                               &value) == D_SUCCESS,
        "insert_negative_success",
        "Insert with negative index should succeed",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_insert_elements
  Tests the d_vector_common_insert_elements function for inserting multiple elements.
  Tests the following:
  - NULL parameter handling
  - zero source_count (no-op)
  - NULL source handling
  - successful range insert
*/
bool
d_tests_sa_vector_common_insert_elements
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {100, 200, 300};

    result = true;

    // test 1: NULL elements should fail
    count    = 0;
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_insert_elements(NULL,
                                     &count,
                                     &capacity,
                                     sizeof(int),
                                     0,
                                     source,
                                     3) == D_FAILURE,
        "insert_range_null_elements",
        "NULL elements should return D_FAILURE",
        _counter) && result;

    // test 2: zero source_count is no-op success
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    result = d_assert_standalone(
        d_vector_common_insert_elements(&elements,
                                     &count,
                                     &capacity,
                                     sizeof(int),
                                     0,
                                     source,
                                     0) == D_SUCCESS,
        "insert_range_zero_count",
        "Zero source_count should return D_SUCCESS",
        _counter) && result;

    // test 3: NULL source with non-zero count should fail
    result = d_assert_standalone(
        d_vector_common_insert_elements(&elements,
                                     &count,
                                     &capacity,
                                     sizeof(int),
                                     0,
                                     NULL,
                                     3) == D_FAILURE,
        "insert_range_null_source",
        "NULL source with non-zero count should fail",
        _counter) && result;

    // test 4: successful range insert
    {
        int* arr = (int*)elements;
        arr[0] = 10;
        arr[1] = 40;
        count  = 2;
    }

    result = d_assert_standalone(
        d_vector_common_insert_elements(&elements,
                                     &count,
                                     &capacity,
                                     sizeof(int),
                                     1,
                                     source,
                                     3) == D_SUCCESS,
        "insert_range_valid_success",
        "Insert range should succeed",
        _counter) && result;

    result = d_assert_standalone(
        count == 5,
        "insert_range_count",
        "Count should be 5",
        _counter) && result;

    {
        int* arr = (int*)elements;
        bool correct = (arr[0] == 10) && (arr[1] == 100) && (arr[2] == 200) &&
                       (arr[3] == 300) && (arr[4] == 40);

        result = d_assert_standalone(
            correct,
            "insert_range_order",
            "Elements should be [10, 100, 200, 300, 40]",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_erase
  Tests the d_vector_common_erase function for removing single elements.
  Tests the following:
  - NULL parameter handling
  - empty vector rejection
  - invalid index handling
  - successful erase at various positions
  - negative index support
*/
bool
d_tests_sa_vector_common_erase
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
    count  = 1;
    result = d_assert_standalone(
        d_vector_common_erase(NULL,
                              &count,
                              sizeof(int),
                              0) == D_FAILURE,
        "erase_null_elements",
        "NULL elements should return D_FAILURE",
        _counter) && result;

    // test 2: empty vector should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = d_assert_standalone(
        d_vector_common_erase(elements,
                              &count,
                              sizeof(int),
                              0) == D_FAILURE,
        "erase_empty",
        "Empty vector should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: invalid index should fail
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
        d_vector_common_erase(elements,
                              &count,
                              sizeof(int),
                              10) == D_FAILURE,
        "erase_invalid_index",
        "Invalid index should return D_FAILURE",
        _counter) && result;

    // test 4: successful erase at beginning
    result = d_assert_standalone(
        d_vector_common_erase(elements,
                              &count,
                              sizeof(int),
                              0) == D_SUCCESS,
        "erase_beginning_success",
        "Erase at beginning should succeed",
        _counter) && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 20) && (arr[1] == 30) && (count == 2);

        result = d_assert_standalone(
            correct,
            "erase_beginning_result",
            "Elements should be [20, 30]",
            _counter) && result;
    }

    // test 5: erase at end
    result = d_assert_standalone(
        d_vector_common_erase(elements,
                              &count,
                              sizeof(int),
                              -1) == D_SUCCESS,
        "erase_end_success",
        "Erase at end (negative index) should succeed",
        _counter) && result;

    result = d_assert_standalone(
        count == 1 && ((int*)elements)[0] == 20,
        "erase_end_result",
        "Only element 20 should remain",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_erase_range
  Tests the d_vector_common_erase_range function for removing element ranges.
  Tests the following:
  - NULL parameter handling
  - empty vector rejection
  - invalid range handling
  - successful range erase
*/
bool
d_tests_sa_vector_common_erase_range
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
        d_vector_common_erase_range(NULL,
                                    &count,
                                    sizeof(int),
                                    0,
                                    2) == D_FAILURE,
        "erase_range_null_elements",
        "NULL elements should return D_FAILURE",
        _counter) && result;

    // test 2: empty vector should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = d_assert_standalone(
        d_vector_common_erase_range(elements,
                                    &count,
                                    sizeof(int),
                                    0,
                                    0) == D_FAILURE,
        "erase_range_empty",
        "Empty vector should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: start > end should fail
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        size_t i;
        for (i = 0; i < 5; i++)
        {
            arr[i] = (int)((i + 1) * 10);
        }
        count = 5;  // [10, 20, 30, 40, 50]
    }

    result = d_assert_standalone(
        d_vector_common_erase_range(elements,
                                    &count,
                                    sizeof(int),
                                    3,
                                    1) == D_FAILURE,
        "erase_range_invalid",
        "start > end should return D_FAILURE",
        _counter) && result;

    // test 4: successful range erase in middle
    result = d_assert_standalone(
        d_vector_common_erase_range(elements,
                                    &count,
                                    sizeof(int),
                                    1,
                                    2) == D_SUCCESS,
        "erase_range_middle_success",
        "Erase range [1,2] should succeed",
        _counter) && result;

    {
        int* arr     = (int*)elements;
        bool correct = (count == 3) && (arr[0] == 10) &&
                       (arr[1] == 40) && (arr[2] == 50);

        result = d_assert_standalone(
            correct,
            "erase_range_middle_result",
            "Elements should be [10, 40, 50]",
            _counter) && result;
    }

    // test 5: erase entire remaining vector
    result = d_assert_standalone(
        d_vector_common_erase_range(elements,
                                    &count,
                                    sizeof(int),
                                    0,
                                    (d_index)(count - 1)) == D_SUCCESS,
        "erase_range_all_success",
        "Erase all elements should succeed",
        _counter) && result;

    result = d_assert_standalone(
        count == 0,
        "erase_range_all_count",
        "Count should be 0",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_element_all
  Aggregation function that runs all element manipulation tests.
*/
bool
d_tests_sa_vector_common_element_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Element Manipulation Functions\n");
    printf("  -----------------------------------------\n");

    result = d_tests_sa_vector_common_push_back(_counter) && result;
    result = d_tests_sa_vector_common_push_front(_counter) && result;
    result = d_tests_sa_vector_common_pop_back(_counter) && result;
    result = d_tests_sa_vector_common_pop_front(_counter) && result;
    result = d_tests_sa_vector_common_insert_element(_counter) && result;
    result = d_tests_sa_vector_common_insert_elements(_counter) && result;
    result = d_tests_sa_vector_common_erase(_counter) && result;
    result = d_tests_sa_vector_common_erase_range(_counter) && result;

    return result;
}