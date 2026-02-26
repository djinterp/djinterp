#include "./vector_common_tests_sa.h"


/*
d_tests_sa_vector_common_at
  Tests the d_vector_common_at function for element access.
  Tests the following:
  - NULL elements handling
  - zero element_size rejection
  - zero count handling
  - valid positive index access
  - valid negative index access
  - out-of-bounds index handling
*/
bool
d_tests_sa_vector_common_at
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    void*  elem_ptr;

    result = true;

    // test 1: NULL elements should return NULL
    elem_ptr = d_vector_common_at(NULL, 5, sizeof(int), 0);
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "at_null_elements",
        "NULL elements should return NULL",
        _counter) && result;

    // test 2: zero element_size should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_at(elements, 5, 0, 0);
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "at_zero_element_size",
        "Zero element_size should return NULL",
        _counter) && result;

    free(elements);

    // test 3: zero count should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_at(elements, 0, sizeof(int), 0);
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "at_zero_count",
        "Zero count should return NULL",
        _counter) && result;

    free(elements);

    // test 4: valid positive index access
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 100;
        arr[1] = 200;
        arr[2] = 300;
        count  = 3;
    }

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), 0);
    result   = d_assert_standalone(
        elem_ptr != NULL && *(int*)elem_ptr == 100,
        "at_index_0",
        "Index 0 should return pointer to 100",
        _counter) && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), 1);
    result   = d_assert_standalone(
        elem_ptr != NULL && *(int*)elem_ptr == 200,
        "at_index_1",
        "Index 1 should return pointer to 200",
        _counter) && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), 2);
    result   = d_assert_standalone(
        elem_ptr != NULL && *(int*)elem_ptr == 300,
        "at_index_2",
        "Index 2 should return pointer to 300",
        _counter) && result;

    // test 5: valid negative index access
    elem_ptr = d_vector_common_at(elements, count, sizeof(int), -1);
    result   = d_assert_standalone(
        elem_ptr != NULL && *(int*)elem_ptr == 300,
        "at_index_neg1",
        "Index -1 should return pointer to last element (300)",
        _counter) && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), -2);
    result   = d_assert_standalone(
        elem_ptr != NULL && *(int*)elem_ptr == 200,
        "at_index_neg2",
        "Index -2 should return pointer to second-to-last (200)",
        _counter) && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), -3);
    result   = d_assert_standalone(
        elem_ptr != NULL && *(int*)elem_ptr == 100,
        "at_index_neg3",
        "Index -3 should return pointer to first (100)",
        _counter) && result;

    // test 6: out-of-bounds index handling
    elem_ptr = d_vector_common_at(elements, count, sizeof(int), 10);
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "at_out_of_bounds_positive",
        "Out-of-bounds positive index should return NULL",
        _counter) && result;

    elem_ptr = d_vector_common_at(elements, count, sizeof(int), -10);
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "at_out_of_bounds_negative",
        "Out-of-bounds negative index should return NULL",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_front
  Tests the d_vector_common_front function for accessing the first element.
  Tests the following:
  - NULL elements handling
  - zero count handling
  - valid front access
*/
bool
d_tests_sa_vector_common_front
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    void*  elem_ptr;

    result = true;

    // test 1: NULL elements should return NULL
    elem_ptr = d_vector_common_front(NULL, 5);
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "front_null_elements",
        "NULL elements should return NULL",
        _counter) && result;

    // test 2: zero count should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_front(elements, 0);
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "front_zero_count",
        "Zero count should return NULL",
        _counter) && result;

    free(elements);

    // test 3: valid front access
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 111;
        arr[1] = 222;
        arr[2] = 333;
        count  = 3;
    }

    elem_ptr = d_vector_common_front(elements, count);
    result   = d_assert_standalone(
        elem_ptr != NULL && *(int*)elem_ptr == 111,
        "front_valid",
        "Front should return pointer to first element (111)",
        _counter) && result;

    result = d_assert_standalone(
        elem_ptr == elements,
        "front_same_as_elements",
        "Front pointer should be same as elements pointer",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_back
  Tests the d_vector_common_back function for accessing the last element.
  Tests the following:
  - NULL elements handling
  - zero count handling
  - zero element_size handling
  - valid back access
*/
bool
d_tests_sa_vector_common_back
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    void*  elem_ptr;

    result = true;

    // test 1: NULL elements should return NULL
    elem_ptr = d_vector_common_back(NULL, 5, sizeof(int));
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "back_null_elements",
        "NULL elements should return NULL",
        _counter) && result;

    // test 2: zero count should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_back(elements, 0, sizeof(int));
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "back_zero_count",
        "Zero count should return NULL",
        _counter) && result;

    free(elements);

    // test 3: zero element_size should return NULL
    elements = malloc(10 * sizeof(int));
    elem_ptr = d_vector_common_back(elements, 5, 0);
    result   = d_assert_standalone(
        elem_ptr == NULL,
        "back_zero_element_size",
        "Zero element_size should return NULL",
        _counter) && result;

    free(elements);

    // test 4: valid back access
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    {
        int* arr = (int*)elements;
        arr[0] = 111;
        arr[1] = 222;
        arr[2] = 333;
        count  = 3;
    }

    elem_ptr = d_vector_common_back(elements, count, sizeof(int));
    result   = d_assert_standalone(
        elem_ptr != NULL && *(int*)elem_ptr == 333,
        "back_valid",
        "Back should return pointer to last element (333)",
        _counter) && result;

    // test 5: single element vector
    count    = 1;
    elem_ptr = d_vector_common_back(elements, count, sizeof(int));
    result   = d_assert_standalone(
        elem_ptr != NULL && *(int*)elem_ptr == 111,
        "back_single_element",
        "Back of single-element vector should return first element",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_data
  Tests the d_vector_common_data function for getting raw data pointer.
  Tests the following:
  - NULL elements handling
  - valid data access
*/
bool
d_tests_sa_vector_common_data
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    void*  data_ptr;

    result = true;

    // test 1: NULL elements should return NULL
    data_ptr = d_vector_common_data(NULL);
    result   = d_assert_standalone(
        data_ptr == NULL,
        "data_null",
        "NULL elements should return NULL",
        _counter) && result;

    // test 2: valid data access
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    data_ptr = d_vector_common_data(elements);
    result   = d_assert_standalone(
        data_ptr == elements,
        "data_same_as_elements",
        "Data should return same pointer as elements",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_access_all
  Aggregation function that runs all access tests.
*/
bool
d_tests_sa_vector_common_access_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Access Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_vector_common_at(_counter) && result;
    result = d_tests_sa_vector_common_front(_counter) && result;
    result = d_tests_sa_vector_common_back(_counter) && result;
    result = d_tests_sa_vector_common_data(_counter) && result;

    return result;
}
