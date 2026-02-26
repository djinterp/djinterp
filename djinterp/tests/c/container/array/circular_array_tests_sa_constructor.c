#include "./circular_array_tests_sa.h"


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_circular_array_new
  Tests the d_circular_array_new function.
  Tests the following:
  - creation with valid parameters
  - zero capacity returns NULL
  - zero element_size returns NULL
  - overflow protection
  - proper initialization of all fields
*/
bool
d_tests_sa_circular_array_new
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;

    result = true;

    // test 1: creation with valid parameters
    arr = d_circular_array_new(5, sizeof(int));
    result = d_assert_standalone(
        arr != NULL,
        "new_valid",
        "Should create array with valid parameters",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->capacity == 5,
            "new_capacity",
            "Capacity should be 5",
            _counter) && result;

        result = d_assert_standalone(
            arr->element_size == sizeof(int),
            "new_element_size",
            "Element size should match sizeof(int)",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 0,
            "new_count_zero",
            "Count should be 0",
            _counter) && result;

        result = d_assert_standalone(
            arr->head == 0,
            "new_head_zero",
            "Head should be 0",
            _counter) && result;

        result = d_assert_standalone(
            arr->tail == 0,
            "new_tail_zero",
            "Tail should be 0",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements != NULL,
            "new_elements_allocated",
            "Elements buffer should be allocated",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 2: zero capacity returns NULL
    arr = d_circular_array_new(0, sizeof(int));
    result = d_assert_standalone(
        arr == NULL,
        "new_zero_capacity",
        "Zero capacity should return NULL",
        _counter) && result;

    // test 3: zero element_size returns NULL
    arr = d_circular_array_new(5, 0);
    result = d_assert_standalone(
        arr == NULL,
        "new_zero_element_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // test 4: overflow protection
    arr = d_circular_array_new(SIZE_MAX / 2, 3);
    result = d_assert_standalone(
        arr == NULL,
        "new_overflow",
        "Overflow should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_new_default_capacity
  Tests the d_circular_array_new_default_capacity function.
  Tests the following:
  - creation with default capacity
  - zero element_size returns NULL
*/
bool
d_tests_sa_circular_array_new_default_capacity
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;

    result = true;

    // test 1: creation with default capacity
    arr = d_circular_array_new_default_capacity(sizeof(int));
    result = d_assert_standalone(
        arr != NULL,
        "new_default_valid",
        "Should create array with default capacity",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->capacity == D_CIRCULAR_ARRAY_DEFAULT_CAPACITY,
            "new_default_capacity_value",
            "Capacity should match default",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 0,
            "new_default_count",
            "Count should be 0",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 2: zero element_size returns NULL
    arr = d_circular_array_new_default_capacity(0);
    result = d_assert_standalone(
        arr == NULL,
        "new_default_zero_element_size",
        "Zero element_size should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_new_from_arr
  Tests the d_circular_array_new_from_arr function.
  Tests the following:
  - creation from valid array
  - array smaller than capacity
  - array larger than capacity (truncation)
  - NULL source returns NULL
  - zero source count returns NULL
*/
bool
d_tests_sa_circular_array_new_from_arr
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       test_data[] = {10, 20, 30, 40, 50};
    int*                      data_ptr;

    result = true;

    // test 1: array fits exactly
    arr = d_circular_array_new_from_arr(5, sizeof(int), test_data, 5);
    result = d_assert_standalone(
        arr != NULL,
        "from_arr_exact_fit",
        "Should create array from exact-fit source",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 5,
            "from_arr_exact_count",
            "Count should be 5",
            _counter) && result;

        result = d_assert_standalone(
            arr->head == 0 && arr->tail == 0,
            "from_arr_exact_wrapped",
            "Full array should have tail wrapped to 0",
            _counter) && result;

        data_ptr = (int*)arr->elements;
        result = d_assert_standalone(
            data_ptr[0] == 10 && data_ptr[4] == 50,
            "from_arr_exact_data",
            "Data should be copied correctly",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 2: array smaller than capacity
    arr = d_circular_array_new_from_arr(10, sizeof(int), test_data, 3);
    result = d_assert_standalone(
        arr != NULL,
        "from_arr_partial",
        "Should create partially filled array",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 3,
            "from_arr_partial_count",
            "Count should be 3",
            _counter) && result;

        result = d_assert_standalone(
            arr->tail == 3,
            "from_arr_partial_tail",
            "Tail should be 3 for partial fill",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: array larger than capacity (truncation)
    arr = d_circular_array_new_from_arr(3, sizeof(int), test_data, 5);
    result = d_assert_standalone(
        arr != NULL,
        "from_arr_truncate",
        "Should create truncated array",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 3,
            "from_arr_truncate_count",
            "Count should be limited to capacity",
            _counter) && result;

        data_ptr = (int*)arr->elements;
        result = d_assert_standalone(
            data_ptr[0] == 10 && data_ptr[1] == 20 && data_ptr[2] == 30,
            "from_arr_truncate_data",
            "Only first 3 elements should be copied",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 4: NULL source returns NULL
    arr = d_circular_array_new_from_arr(5, sizeof(int), NULL, 3);
    result = d_assert_standalone(
        arr == NULL,
        "from_arr_null_source",
        "NULL source should return NULL",
        _counter) && result;

    // test 5: zero source count returns NULL
    arr = d_circular_array_new_from_arr(5, sizeof(int), test_data, 0);
    result = d_assert_standalone(
        arr == NULL,
        "from_arr_zero_count",
        "Zero source count should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_new_from_args
  Tests the d_circular_array_new_from_args function.
  Tests the following:
  - creation from variadic arguments
  - arguments limited by capacity
  - zero arg count returns NULL
*/
bool
d_tests_sa_circular_array_new_from_args
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int*                      data_ptr;

    result = true;

    // test 1: creation from variadic arguments
    arr = d_circular_array_new_from_args(5, sizeof(int), 3, 100, 200, 300);
    result = d_assert_standalone(
        arr != NULL,
        "from_args_valid",
        "Should create array from args",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 3,
            "from_args_count",
            "Count should be 3",
            _counter) && result;

        data_ptr = (int*)arr->elements;
        result = d_assert_standalone(
            data_ptr[0] == 100 && data_ptr[1] == 200 && data_ptr[2] == 300,
            "from_args_data",
            "Data should match arguments",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 2: more args than capacity
    arr = d_circular_array_new_from_args(2, sizeof(int), 5, 1, 2, 3, 4, 5);
    result = d_assert_standalone(
        arr != NULL,
        "from_args_truncate",
        "Should create array with truncated args",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 2,
            "from_args_truncate_count",
            "Count should be limited to capacity",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 3: zero arg count returns NULL
    arr = d_circular_array_new_from_args(5, sizeof(int), 0);
    result = d_assert_standalone(
        arr == NULL,
        "from_args_zero_count",
        "Zero arg count should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_new_copy
  Tests the d_circular_array_new_copy function.
  Tests the following:
  - copy of empty array
  - copy of partially filled array
  - copy with wraparound preserves logical order
  - NULL source returns NULL
  - copy has independent memory
*/
bool
d_tests_sa_circular_array_new_copy
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  source;
    struct d_circular_array*  copy;
    int                       values[5] = {10, 20, 30, 40, 50};
    int*                      data_ptr;

    result = true;

    // test 1: copy of empty array
    source = d_circular_array_new(5, sizeof(int));

    if (source)
    {
        copy = d_circular_array_new_copy(source);
        result = d_assert_standalone(
            copy != NULL,
            "copy_empty",
            "Should copy empty array",
            _counter) && result;

        if (copy)
        {
            result = d_assert_standalone(
                copy->count == 0 && copy->capacity == 5,
                "copy_empty_properties",
                "Copy should have same properties",
                _counter) && result;

            result = d_assert_standalone(
                copy->elements != source->elements,
                "copy_empty_independent",
                "Copy should have independent memory",
                _counter) && result;

            d_circular_array_free(copy);
        }

        d_circular_array_free(source);
    }

    // test 2: copy of partially filled array
    source = d_circular_array_new(5, sizeof(int));

    if (source)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(source, &values[i]);
        }

        copy = d_circular_array_new_copy(source);
        result = d_assert_standalone(
            copy != NULL,
            "copy_partial",
            "Should copy partially filled array",
            _counter) && result;

        if (copy)
        {
            result = d_assert_standalone(
                copy->count == 3,
                "copy_partial_count",
                "Copy should have same count",
                _counter) && result;

            data_ptr = (int*)copy->elements;
            result = d_assert_standalone(
                data_ptr[0] == 10 && data_ptr[1] == 20 && data_ptr[2] == 30,
                "copy_partial_data",
                "Copy should have same data",
                _counter) && result;

            result = d_assert_standalone(
                copy->head == 0 && copy->tail == 3,
                "copy_partial_linearized",
                "Copy should be linearized",
                _counter) && result;

            d_circular_array_free(copy);
        }

        d_circular_array_free(source);
    }

    // test 3: copy with wraparound preserves logical order
    source = d_circular_array_new(3, sizeof(int));

    if (source)
    {
        // fill, then pop and push to create wraparound
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(source, &values[i]);
        }

        // After push 10, 20, 30: head=0, tail=0 (wrapped), count=3
        // Elements at physical [0,1,2] = [10, 20, 30]
        d_circular_array_pop(source);
        // After pop: head=1, tail=0, count=2
        // Logical order: 20, 30
        d_circular_array_push(source, &values[3]);
        // After push 40: head=1, tail=1, count=3
        // Physical [0,1,2] = [40, 20, 30], logical order: 20, 30, 40

        copy = d_circular_array_new_copy(source);
        result = d_assert_standalone(
            copy != NULL,
            "copy_wraparound",
            "Should copy array with wraparound",
            _counter) && result;

        if (copy)
        {
            int* data_ptr = (int*)copy->elements;
            // logical order should be: 20, 30, 40
            result = d_assert_standalone(
                data_ptr[0] == 20 && data_ptr[1] == 30 && data_ptr[2] == 40,
                "copy_wraparound_order",
                "Copy should preserve logical order",
                _counter) && result;

            d_circular_array_free(copy);
        }

        d_circular_array_free(source);
    }

    // test 4: NULL source returns NULL
    copy = d_circular_array_new_copy(NULL);
    result = d_assert_standalone(
        copy == NULL,
        "copy_null_source",
        "NULL source should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_new_copy_resized
  Tests the d_circular_array_new_copy_resized function.
  Tests the following:
  - copy with larger capacity
  - copy with smaller capacity (truncates)
  - NULL source returns NULL
*/
bool
d_tests_sa_circular_array_new_copy_resized
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  source;
    struct d_circular_array*  copy;
    int                       values[5] = {10, 20, 30, 40, 50};

    result = true;

    // test 1: copy with larger capacity
    source = d_circular_array_new(3, sizeof(int));

    if (source)
    {
        for (int i = 0; i < 3; i++)
        {
            d_circular_array_push(source, &values[i]);
        }

        copy = d_circular_array_new_copy_resized(source, 10);
        result = d_assert_standalone(
            copy != NULL,
            "copy_resized_larger",
            "Should create larger copy",
            _counter) && result;

        if (copy)
        {
            result = d_assert_standalone(
                copy->capacity == 10 && copy->count == 3,
                "copy_resized_larger_properties",
                "Copy should have new capacity but same count",
                _counter) && result;

            d_circular_array_free(copy);
        }

        d_circular_array_free(source);
    }

    // test 2: copy with smaller capacity returns NULL (doesn't truncate)
    source = d_circular_array_new(5, sizeof(int));

    if (source)
    {
        for (int i = 0; i < 5; i++)
        {
            d_circular_array_push(source, &values[i]);
        }

        // Implementation returns NULL if new_capacity < count
        copy = d_circular_array_new_copy_resized(source, 3);
        result = d_assert_standalone(
            copy == NULL,
            "copy_resized_smaller",
            "Smaller capacity than count should return NULL",
            _counter) && result;

        d_circular_array_free(source);
    }

    // test 3: NULL source returns NULL
    copy = d_circular_array_new_copy_resized(NULL, 5);
    result = d_assert_standalone(
        copy == NULL,
        "copy_resized_null",
        "NULL source should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_new_fill
  Tests the d_circular_array_new_fill function.
  Tests the following:
  - creation with fill value
  - NULL fill value returns NULL
  - zero capacity returns NULL
*/
bool
d_tests_sa_circular_array_new_fill
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_circular_array*  arr;
    int                       fill_value = 42;
    int*                      data_ptr;

    result = true;

    // test 1: creation with fill value
    arr = d_circular_array_new_fill(5, sizeof(int), &fill_value);
    result = d_assert_standalone(
        arr != NULL,
        "new_fill_valid",
        "Should create filled array",
        _counter) && result;

    if (arr)
    {
        result = d_assert_standalone(
            arr->count == 5,
            "new_fill_count",
            "Count should equal capacity",
            _counter) && result;

        data_ptr = (int*)arr->elements;
        result = d_assert_standalone(
            data_ptr[0] == 42 && data_ptr[2] == 42 && data_ptr[4] == 42,
            "new_fill_data",
            "All elements should be filled",
            _counter) && result;

        d_circular_array_free(arr);
    }

    // test 2: NULL fill value returns NULL
    arr = d_circular_array_new_fill(5, sizeof(int), NULL);
    result = d_assert_standalone(
        arr == NULL,
        "new_fill_null_value",
        "NULL fill value should return NULL",
        _counter) && result;

    // test 3: zero capacity returns NULL
    arr = d_circular_array_new_fill(0, sizeof(int), &fill_value);
    result = d_assert_standalone(
        arr == NULL,
        "new_fill_zero_capacity",
        "Zero capacity should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_circular_array_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool
d_tests_sa_circular_array_constructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constructor Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_circular_array_new(_counter) && result;
    result = d_tests_sa_circular_array_new_default_capacity(_counter) && result;
    result = d_tests_sa_circular_array_new_from_arr(_counter) && result;
    result = d_tests_sa_circular_array_new_from_args(_counter) && result;
    result = d_tests_sa_circular_array_new_copy(_counter) && result;
    result = d_tests_sa_circular_array_new_copy_resized(_counter) && result;
    result = d_tests_sa_circular_array_new_fill(_counter) && result;

    return result;
}
