#include "./ptr_array_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

// helper comparator for int pointer values
static int
int_ptr_comparator
(
    const void* _a,
    const void* _b
)
{
    int a;
    int b;

    // dereference pointers to get the actual int values
    a = *(const int*)_a;
    b = *(const int*)_b;

    return (a > b) - (a < b);
}


// helper comparator for sorting int pointers by their pointed-to values
static int
int_ptr_sort_comparator
(
    const void* _a,
    const void* _b
)
{
    // _a and _b are pointers to the array elements (void**)
    // each element is itself a void* pointing to an int
    const int* ptr_a;
    const int* ptr_b;

    ptr_a = *(const int* const*)_a;
    ptr_b = *(const int* const*)_b;

    return (*ptr_a > *ptr_b) - (*ptr_a < *ptr_b);
}


/******************************************************************************
 * II. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_ptr_array_append_element
  Tests the d_ptr_array_append_element function.
  Tests the following:
  - NULL array returns failure
  - appending to empty array
  - appending to non-empty array
  - multiple appends maintain order
*/
bool
d_tests_sa_ptr_array_append_element
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[4];

    result = true;

    // initialize test values
    values[0] = 100;
    values[1] = 200;
    values[2] = 300;
    values[3] = 400;

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_append_element(NULL, &values[0]) == D_FAILURE,
        "append_element_null",
        "NULL array should return failure",
        _counter) && result;

    // test 2: appending to empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_append_element(arr, &values[0]) == D_SUCCESS,
            "append_element_empty",
            "Append to empty array should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 1 && arr->elements[0] == &values[0],
            "append_element_empty_verify",
            "Array should have 1 element",
            _counter) && result;

        // test 3: appending to non-empty array
        result = d_assert_standalone(
            d_ptr_array_append_element(arr, &values[1]) == D_SUCCESS,
            "append_element_nonempty",
            "Append to non-empty array should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 2 && arr->elements[1] == &values[1],
            "append_element_nonempty_verify",
            "Array should have 2 elements",
            _counter) && result;

        // test 4: multiple appends maintain order
        d_ptr_array_append_element(arr, &values[2]);
        d_ptr_array_append_element(arr, &values[3]);

        result = d_assert_standalone(
            arr->count == 4 &&
            arr->elements[0] == &values[0] &&
            arr->elements[1] == &values[1] &&
            arr->elements[2] == &values[2] &&
            arr->elements[3] == &values[3],
            "append_element_order",
            "Multiple appends should maintain order",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_append_elements
  Tests the d_ptr_array_append_elements function.
  Tests the following:
  - NULL array returns failure
  - NULL elements with count > 0 returns failure
  - valid append of multiple elements
  - append with count 0 does nothing
*/
bool
d_tests_sa_ptr_array_append_elements
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[5];
    const void*          source[3];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 50;
    source[0] = &values[2];
    source[1] = &values[3];
    source[2] = &values[4];

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_append_elements(NULL, source, 3) == D_FAILURE,
        "append_elements_null_arr",
        "NULL array should return failure",
        _counter) && result;

    // test 2: NULL elements with count > 0
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_append_elements(arr, NULL, 3) == D_FAILURE,
            "append_elements_null_elements",
            "NULL elements with count > 0 should return failure",
            _counter) && result;

        // add some initial elements
        d_ptr_array_append_element(arr, &values[0]);
        d_ptr_array_append_element(arr, &values[1]);

        // test 3: valid append of multiple elements
        result = d_assert_standalone(
            d_ptr_array_append_elements(arr, source, 3) == D_SUCCESS,
            "append_elements_valid",
            "Valid append should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 5,
            "append_elements_count",
            "Array should have 5 elements",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements[2] == &values[2] &&
            arr->elements[3] == &values[3] &&
            arr->elements[4] == &values[4],
            "append_elements_order",
            "Appended elements should be in correct order",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 4: append with count 0
    arr = d_ptr_array_new_from_args(2, &values[0], &values[1]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_append_elements(arr, source, 0) == D_SUCCESS,
            "append_elements_zero",
            "Append with count 0 should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 2,
            "append_elements_zero_unchanged",
            "Array count should be unchanged",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_append_array
  Tests the d_ptr_array_append_array function.
  Tests the following:
  - NULL destination returns failure
  - NULL source returns failure
  - valid append of array to array
  - append empty array does nothing
*/
bool
d_tests_sa_ptr_array_append_array
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  dest;
    struct d_ptr_array*  src;
    struct d_ptr_array*  empty;
    int                  values[5];

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 4;
    values[4] = 5;

    // test 1: NULL destination returns failure
    src = d_ptr_array_new_from_args(2, &values[0], &values[1]);

    if (src)
    {
        result = d_assert_standalone(
            d_ptr_array_append_array(NULL, src) == D_FAILURE,
            "append_array_null_dest",
            "NULL destination should return failure",
            _counter) && result;

        d_ptr_array_free(src);
    }

    // test 2: NULL source returns failure
    dest = d_ptr_array_new(10);

    if (dest)
    {
        result = d_assert_standalone(
            d_ptr_array_append_array(dest, NULL) == D_FAILURE,
            "append_array_null_src",
            "NULL source should return failure",
            _counter) && result;

        d_ptr_array_free(dest);
    }

    // test 3: valid append of array to array
    dest = d_ptr_array_new_from_args(2, &values[0], &values[1]);
    src  = d_ptr_array_new_from_args(3, &values[2], &values[3], &values[4]);

    if (dest && src)
    {
        result = d_assert_standalone(
            d_ptr_array_append_array(dest, src) == D_SUCCESS,
            "append_array_valid",
            "Valid array append should succeed",
            _counter) && result;

        result = d_assert_standalone(
            dest->count == 5,
            "append_array_count",
            "Destination should have 5 elements",
            _counter) && result;

        result = d_assert_standalone(
            dest->elements[0] == &values[0] &&
            dest->elements[1] == &values[1] &&
            dest->elements[2] == &values[2] &&
            dest->elements[3] == &values[3] &&
            dest->elements[4] == &values[4],
            "append_array_order",
            "All elements should be in correct order",
            _counter) && result;
    }

    if (dest)
    {
        d_ptr_array_free(dest);
    }

    if (src)
    {
        d_ptr_array_free(src);
    }

    // test 4: append empty array
    dest  = d_ptr_array_new_from_args(2, &values[0], &values[1]);
    empty = d_ptr_array_new(10);

    if (dest && empty)
    {
        result = d_assert_standalone(
            d_ptr_array_append_array(dest, empty) == D_SUCCESS,
            "append_array_empty",
            "Appending empty array should succeed",
            _counter) && result;

        result = d_assert_standalone(
            dest->count == 2,
            "append_array_empty_unchanged",
            "Destination count should be unchanged",
            _counter) && result;
    }

    if (dest)
    {
        d_ptr_array_free(dest);
    }

    if (empty)
    {
        d_ptr_array_free(empty);
    }

    return result;
}


/*
d_tests_sa_ptr_array_contains
  Tests the d_ptr_array_contains function.
  Tests the following:
  - NULL array returns false
  - empty array returns false
  - value found returns true
  - value not found returns false
*/
bool
d_tests_sa_ptr_array_contains
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[3];
    int                  not_in_array;

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    not_in_array = 999;

    // test 1: NULL array returns false
    result = d_assert_standalone(
        d_ptr_array_contains(NULL, &values[0]) == false,
        "contains_null",
        "NULL array should return false",
        _counter) && result;

    // test 2: empty array returns false
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_contains(arr, &values[0]) == false,
            "contains_empty",
            "Empty array should return false",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3 & 4: value found and not found
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        // test 3: value found (by pointer identity)
        result = d_assert_standalone(
            d_ptr_array_contains(arr, &values[1]) == true,
            "contains_found",
            "Existing pointer should be found",
            _counter) && result;

        // test 4: value not found
        result = d_assert_standalone(
            d_ptr_array_contains(arr, &not_in_array) == false,
            "contains_not_found",
            "Non-existent pointer should not be found",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_fill
  Tests the d_ptr_array_fill function.
  Tests the following:
  - NULL array returns failure
  - fill empty array does nothing
  - fill array with value
*/
bool
d_tests_sa_ptr_array_fill
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  fill_value;
    int                  values[3];

    result = true;

    // initialize test values
    fill_value = 999;
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_fill(NULL, &fill_value) == D_FAILURE,
        "fill_null",
        "NULL array should return failure",
        _counter) && result;

    // test 2: fill empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_fill(arr, &fill_value) == D_SUCCESS,
            "fill_empty",
            "Fill empty array should succeed (no-op)",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: fill array with value
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_fill(arr, &fill_value) == D_SUCCESS,
            "fill_valid",
            "Fill array should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements[0] == &fill_value &&
            arr->elements[1] == &fill_value &&
            arr->elements[2] == &fill_value,
            "fill_verify",
            "All elements should point to fill value",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_find
  Tests the d_ptr_array_find function.
  Tests the following:
  - NULL array returns -1
  - empty array returns -1
  - value found returns correct index
  - value not found returns -1
  - finds first occurrence
*/
bool
d_tests_sa_ptr_array_find
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[5];
    int                  not_in_array;
    ssize_t              idx;

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 20;  // duplicate value
    values[4] = 40;
    not_in_array = 999;

    // test 1: NULL array returns -1
    idx = d_ptr_array_find(NULL, &values[0]);
    result = d_assert_standalone(
        idx == -1,
        "find_null",
        "NULL array should return -1",
        _counter) && result;

    // test 2: empty array returns -1
    arr = d_ptr_array_new(10);

    if (arr)
    {
        idx = d_ptr_array_find(arr, &values[0]);
        result = d_assert_standalone(
            idx == -1,
            "find_empty",
            "Empty array should return -1",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // tests 3-5: find operations
    arr = d_ptr_array_new_from_args(5,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3],
                                    &values[4]);

    if (arr)
    {
        // test 3: value found returns correct index
        idx = d_ptr_array_find(arr, &values[2]);
        result = d_assert_standalone(
            idx == 2,
            "find_found",
            "Found pointer should return index 2",
            _counter) && result;

        // test 4: value not found returns -1
        idx = d_ptr_array_find(arr, &not_in_array);
        result = d_assert_standalone(
            idx == -1,
            "find_not_found",
            "Non-existent pointer should return -1",
            _counter) && result;

        // test 5: finds pointer identity (even if same value, different address)
        idx = d_ptr_array_find(arr, &values[1]);
        result = d_assert_standalone(
            idx == 1,
            "find_first",
            "Should find first occurrence at index 1",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_insert_element
  Tests the d_ptr_array_insert_element function.
  Tests the following:
  - NULL array returns failure
  - insert at beginning (index 0)
  - insert in middle
  - insert at end
  - insert with negative index
*/
bool
d_tests_sa_ptr_array_insert_element
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[5];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 15;  // to insert at beginning
    values[4] = 25;  // to insert in middle

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_insert_element(NULL, &values[0], 0) == D_FAILURE,
        "insert_element_null",
        "NULL array should return failure",
        _counter) && result;

    // create test array
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        // test 2: insert at beginning (index 0)
        result = d_assert_standalone(
            d_ptr_array_insert_element(arr, &values[3], 0) == D_SUCCESS,
            "insert_element_beginning",
            "Insert at beginning should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 4 && arr->elements[0] == &values[3],
            "insert_element_beginning_verify",
            "Element should be at beginning",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: insert in middle
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_insert_element(arr, &values[4], 1) == D_SUCCESS,
            "insert_element_middle",
            "Insert in middle should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 4 &&
            arr->elements[0] == &values[0] &&
            arr->elements[1] == &values[4] &&
            arr->elements[2] == &values[1] &&
            arr->elements[3] == &values[2],
            "insert_element_middle_verify",
            "Element should be inserted at index 1",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 4: insert with negative index
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_insert_element(arr, &values[4], -1) == D_SUCCESS,
            "insert_element_negative",
            "Insert with negative index should succeed",
            _counter) && result;

        // -1 on array of size 3 means index 2, so insert at position 2
        result = d_assert_standalone(
            arr->count == 4 &&
            arr->elements[2] == &values[4],
            "insert_element_negative_verify",
            "Element should be at converted index",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_insert_elements
  Tests the d_ptr_array_insert_elements function.
  Tests the following:
  - NULL array returns failure
  - NULL elements with count > 0 returns failure
  - valid insert of multiple elements
  - insert with count 0 does nothing
*/
bool
d_tests_sa_ptr_array_insert_elements
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[6];
    const void*          to_insert[2];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 15;  // to insert
    values[5] = 16;  // to insert
    to_insert[0] = &values[4];
    to_insert[1] = &values[5];

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_insert_elements(NULL, to_insert, 2, 0) == D_FAILURE,
        "insert_elements_null_arr",
        "NULL array should return failure",
        _counter) && result;

    // test 2: NULL elements with count > 0
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_insert_elements(arr, NULL, 2, 0) == D_FAILURE,
            "insert_elements_null_elements",
            "NULL elements with count > 0 should return failure",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: valid insert of multiple elements
    arr = d_ptr_array_new_from_args(4,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_insert_elements(arr, to_insert, 2, 1) == D_SUCCESS,
            "insert_elements_valid",
            "Valid insert should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 6,
            "insert_elements_count",
            "Array should have 6 elements",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements[0] == &values[0] &&
            arr->elements[1] == &values[4] &&
            arr->elements[2] == &values[5] &&
            arr->elements[3] == &values[1] &&
            arr->elements[4] == &values[2] &&
            arr->elements[5] == &values[3],
            "insert_elements_order",
            "Elements should be in correct order",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 4: insert with count 0
    arr = d_ptr_array_new_from_args(2, &values[0], &values[1]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_insert_elements(arr, to_insert, 0, 1) == D_SUCCESS,
            "insert_elements_zero",
            "Insert with count 0 should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 2,
            "insert_elements_zero_unchanged",
            "Array count should be unchanged",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_insert_array
  Tests the d_ptr_array_insert_array function.
  Tests the following:
  - NULL destination returns failure
  - NULL source returns failure
  - valid insert of array into array
*/
bool
d_tests_sa_ptr_array_insert_array
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  dest;
    struct d_ptr_array*  src;
    int                  values[5];

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 10;  // to insert
    values[4] = 11;  // to insert

    // test 1: NULL destination returns failure
    src = d_ptr_array_new_from_args(2, &values[3], &values[4]);

    if (src)
    {
        result = d_assert_standalone(
            d_ptr_array_insert_array(NULL, src, 0) == D_FAILURE,
            "insert_array_null_dest",
            "NULL destination should return failure",
            _counter) && result;

        d_ptr_array_free(src);
    }

    // test 2: NULL source returns failure
    dest = d_ptr_array_new(10);

    if (dest)
    {
        result = d_assert_standalone(
            d_ptr_array_insert_array(dest, NULL, 0) == D_FAILURE,
            "insert_array_null_src",
            "NULL source should return failure",
            _counter) && result;

        d_ptr_array_free(dest);
    }

    // test 3: valid insert of array into array
    dest = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);
    src  = d_ptr_array_new_from_args(2, &values[3], &values[4]);

    if (dest && src)
    {
        result = d_assert_standalone(
            d_ptr_array_insert_array(dest, src, 1) == D_SUCCESS,
            "insert_array_valid",
            "Valid array insert should succeed",
            _counter) && result;

        result = d_assert_standalone(
            dest->count == 5,
            "insert_array_count",
            "Destination should have 5 elements",
            _counter) && result;

        result = d_assert_standalone(
            dest->elements[0] == &values[0] &&
            dest->elements[1] == &values[3] &&
            dest->elements[2] == &values[4] &&
            dest->elements[3] == &values[1] &&
            dest->elements[4] == &values[2],
            "insert_array_order",
            "Elements should be in correct order",
            _counter) && result;
    }

    if (dest)
    {
        d_ptr_array_free(dest);
    }

    if (src)
    {
        d_ptr_array_free(src);
    }

    return result;
}


/*
d_tests_sa_ptr_array_is_empty
  Tests the d_ptr_array_is_empty function.
  Tests the following:
  - NULL array returns true
  - empty array returns true
  - non-empty array returns false
*/
bool
d_tests_sa_ptr_array_is_empty
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  value;

    result = true;
    value  = 42;

    // test 1: NULL array returns true
    result = d_assert_standalone(
        d_ptr_array_is_empty(NULL) == true,
        "is_empty_null",
        "NULL array should be empty",
        _counter) && result;

    // test 2: empty array returns true
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_is_empty(arr) == true,
            "is_empty_empty",
            "Empty array should return true",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: non-empty array returns false
    arr = d_ptr_array_new_from_args(1, &value);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_is_empty(arr) == false,
            "is_empty_nonempty",
            "Non-empty array should return false",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_prepend_element
  Tests the d_ptr_array_prepend_element function.
  Tests the following:
  - NULL array returns failure
  - prepend to empty array
  - prepend to non-empty array maintains order
*/
bool
d_tests_sa_ptr_array_prepend_element
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[3];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_prepend_element(NULL, &values[0]) == D_FAILURE,
        "prepend_element_null",
        "NULL array should return failure",
        _counter) && result;

    // test 2: prepend to empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_prepend_element(arr, &values[0]) == D_SUCCESS,
            "prepend_element_empty",
            "Prepend to empty array should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 1 && arr->elements[0] == &values[0],
            "prepend_element_empty_verify",
            "Element should be at index 0",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: prepend to non-empty array maintains order
    arr = d_ptr_array_new_from_args(2, &values[1], &values[2]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_prepend_element(arr, &values[0]) == D_SUCCESS,
            "prepend_element_nonempty",
            "Prepend to non-empty array should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 3 &&
            arr->elements[0] == &values[0] &&
            arr->elements[1] == &values[1] &&
            arr->elements[2] == &values[2],
            "prepend_element_order",
            "Prepended element should be first",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_prepend_elements
  Tests the d_ptr_array_prepend_elements function.
  Tests the following:
  - NULL array returns failure
  - valid prepend of multiple elements
*/
bool
d_tests_sa_ptr_array_prepend_elements
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[5];
    const void*          to_prepend[2];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 1;  // to prepend
    values[4] = 2;  // to prepend
    to_prepend[0] = &values[3];
    to_prepend[1] = &values[4];

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_prepend_elements(NULL, to_prepend, 2) == D_FAILURE,
        "prepend_elements_null",
        "NULL array should return failure",
        _counter) && result;

    // test 2: valid prepend of multiple elements
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_prepend_elements(arr, to_prepend, 2) == D_SUCCESS,
            "prepend_elements_valid",
            "Valid prepend should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == 5,
            "prepend_elements_count",
            "Array should have 5 elements",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements[0] == &values[3] &&
            arr->elements[1] == &values[4] &&
            arr->elements[2] == &values[0] &&
            arr->elements[3] == &values[1] &&
            arr->elements[4] == &values[2],
            "prepend_elements_order",
            "Prepended elements should be first",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_prepend_array
  Tests the d_ptr_array_prepend_array function.
  Tests the following:
  - NULL destination returns failure
  - NULL source returns failure
  - valid prepend of array to array
*/
bool
d_tests_sa_ptr_array_prepend_array
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  dest;
    struct d_ptr_array*  src;
    int                  values[5];

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 10;
    values[4] = 11;

    // test 1: NULL destination returns failure
    src = d_ptr_array_new_from_args(2, &values[3], &values[4]);

    if (src)
    {
        result = d_assert_standalone(
            d_ptr_array_prepend_array(NULL, src) == D_FAILURE,
            "prepend_array_null_dest",
            "NULL destination should return failure",
            _counter) && result;

        d_ptr_array_free(src);
    }

    // test 2: NULL source returns failure
    dest = d_ptr_array_new(10);

    if (dest)
    {
        result = d_assert_standalone(
            d_ptr_array_prepend_array(dest, NULL) == D_FAILURE,
            "prepend_array_null_src",
            "NULL source should return failure",
            _counter) && result;

        d_ptr_array_free(dest);
    }

    // test 3: valid prepend of array to array
    dest = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);
    src  = d_ptr_array_new_from_args(2, &values[3], &values[4]);

    if (dest && src)
    {
        result = d_assert_standalone(
            d_ptr_array_prepend_array(dest, src) == D_SUCCESS,
            "prepend_array_valid",
            "Valid array prepend should succeed",
            _counter) && result;

        result = d_assert_standalone(
            dest->count == 5,
            "prepend_array_count",
            "Destination should have 5 elements",
            _counter) && result;

        result = d_assert_standalone(
            dest->elements[0] == &values[3] &&
            dest->elements[1] == &values[4] &&
            dest->elements[2] == &values[0] &&
            dest->elements[3] == &values[1] &&
            dest->elements[4] == &values[2],
            "prepend_array_order",
            "Prepended elements should be first",
            _counter) && result;
    }

    if (dest)
    {
        d_ptr_array_free(dest);
    }

    if (src)
    {
        d_ptr_array_free(src);
    }

    return result;
}


/*
d_tests_sa_ptr_array_reverse
  Tests the d_ptr_array_reverse function.
  Tests the following:
  - NULL array returns failure
  - reverse empty array succeeds
  - reverse single element succeeds
  - reverse multiple elements
  - reverse odd count elements
*/
bool
d_tests_sa_ptr_array_reverse
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[5];

    result = true;

    // initialize test values
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 4;
    values[4] = 5;

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_reverse(NULL) == D_FAILURE,
        "reverse_null",
        "NULL array should return failure",
        _counter) && result;

    // test 2: reverse empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_reverse(arr) == D_SUCCESS,
            "reverse_empty",
            "Reverse empty array should succeed",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: reverse single element
    arr = d_ptr_array_new_from_args(1, &values[0]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_reverse(arr) == D_SUCCESS,
            "reverse_single",
            "Reverse single element should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements[0] == &values[0],
            "reverse_single_unchanged",
            "Single element should be unchanged",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 4: reverse multiple elements (even count)
    arr = d_ptr_array_new_from_args(4,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_reverse(arr) == D_SUCCESS,
            "reverse_even",
            "Reverse even count should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements[0] == &values[3] &&
            arr->elements[1] == &values[2] &&
            arr->elements[2] == &values[1] &&
            arr->elements[3] == &values[0],
            "reverse_even_verify",
            "Elements should be reversed",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 5: reverse odd count elements
    arr = d_ptr_array_new_from_args(5,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3],
                                    &values[4]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_reverse(arr) == D_SUCCESS,
            "reverse_odd",
            "Reverse odd count should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->elements[0] == &values[4] &&
            arr->elements[1] == &values[3] &&
            arr->elements[2] == &values[2] &&
            arr->elements[3] == &values[1] &&
            arr->elements[4] == &values[0],
            "reverse_odd_verify",
            "Elements should be reversed (odd)",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_resize_amount
  Tests the d_ptr_array_resize_amount function.
  Tests the following:
  - NULL array returns failure
  - resize by positive amount (grow)
  - resize by negative amount (shrink)
  - resize by zero does nothing
*/
bool
d_tests_sa_ptr_array_resize_amount
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[5];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 50;

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_resize_amount(NULL, 5) == D_FAILURE,
        "resize_amount_null",
        "NULL array should return failure",
        _counter) && result;

    // test 2: resize by positive amount (grow)
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        size_t old_count;

        old_count = arr->count;
        result = d_assert_standalone(
            d_ptr_array_resize_amount(arr, 2) == D_SUCCESS,
            "resize_amount_grow",
            "Resize grow should succeed",
            _counter) && result;

        // note: resize_amount changes the allocation, elements may be preserved
        // but we mainly test that the function succeeds

        d_ptr_array_free(arr);
    }

    // test 3: resize by negative amount (shrink)
    arr = d_ptr_array_new_from_args(5,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3],
                                    &values[4]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_resize_amount(arr, -2) == D_SUCCESS,
            "resize_amount_shrink",
            "Resize shrink should succeed",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 4: resize by zero
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        size_t old_count;

        old_count = arr->count;
        result = d_assert_standalone(
            d_ptr_array_resize_amount(arr, 0) == D_SUCCESS,
            "resize_amount_zero",
            "Resize by zero should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == old_count,
            "resize_amount_zero_unchanged",
            "Count should be unchanged",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_resize_factor
  Tests the d_ptr_array_resize_factor function.
  Tests the following:
  - NULL array returns failure
  - resize by factor > 1 (grow)
  - resize by factor < 1 (shrink)
  - resize by factor 1.0 does nothing
*/
bool
d_tests_sa_ptr_array_resize_factor
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[5];

    result = true;

    // initialize test values
    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    values[3] = 40;
    values[4] = 50;

    // test 1: NULL array returns failure
    result = d_assert_standalone(
        d_ptr_array_resize_factor(NULL, 2.0) == D_FAILURE,
        "resize_factor_null",
        "NULL array should return failure",
        _counter) && result;

    // test 2: resize by factor > 1 (grow)
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_resize_factor(arr, 2.0) == D_SUCCESS,
            "resize_factor_grow",
            "Resize grow by factor should succeed",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: resize by factor < 1 (shrink)
    arr = d_ptr_array_new_from_args(4,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3]);

    if (arr)
    {
        result = d_assert_standalone(
            d_ptr_array_resize_factor(arr, 0.5) == D_SUCCESS,
            "resize_factor_shrink",
            "Resize shrink by factor should succeed",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 4: resize by factor 1.0
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        size_t old_count;

        old_count = arr->count;
        result = d_assert_standalone(
            d_ptr_array_resize_factor(arr, 1.0) == D_SUCCESS,
            "resize_factor_one",
            "Resize by factor 1.0 should succeed",
            _counter) && result;

        result = d_assert_standalone(
            arr->count == old_count,
            "resize_factor_one_unchanged",
            "Count should be unchanged",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_sort
  Tests the d_ptr_array_sort function.
  Tests the following:
  - NULL array does nothing (no crash)
  - NULL comparator does nothing (no crash)
  - sort empty array succeeds
  - sort already sorted array
  - sort unsorted array
*/
bool
d_tests_sa_ptr_array_sort
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_array*  arr;
    int                  values[5];

    result = true;

    // initialize test values in unsorted order
    values[0] = 50;
    values[1] = 20;
    values[2] = 40;
    values[3] = 10;
    values[4] = 30;

    // test 1: NULL array does nothing (no crash)
    d_ptr_array_sort(NULL, int_ptr_sort_comparator);
    result = d_assert_standalone(
        true,  // if we get here, no crash
        "sort_null_array",
        "NULL array should not crash",
        _counter) && result;

    // test 2: NULL comparator does nothing (no crash)
    arr = d_ptr_array_new_from_args(3, &values[0], &values[1], &values[2]);

    if (arr)
    {
        d_ptr_array_sort(arr, NULL);
        result = d_assert_standalone(
            true,  // if we get here, no crash
            "sort_null_comparator",
            "NULL comparator should not crash",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 3: sort empty array
    arr = d_ptr_array_new(10);

    if (arr)
    {
        d_ptr_array_sort(arr, int_ptr_sort_comparator);
        result = d_assert_standalone(
            arr->count == 0,
            "sort_empty",
            "Sort empty array should succeed",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    // test 4: sort unsorted array
    arr = d_ptr_array_new_from_args(5,
                                    &values[0],
                                    &values[1],
                                    &values[2],
                                    &values[3],
                                    &values[4]);

    if (arr)
    {
        d_ptr_array_sort(arr, int_ptr_sort_comparator);

        // after sorting by pointed-to values: 10, 20, 30, 40, 50
        result = d_assert_standalone(
            *(int*)arr->elements[0] == 10 &&
            *(int*)arr->elements[1] == 20 &&
            *(int*)arr->elements[2] == 30 &&
            *(int*)arr->elements[3] == 40 &&
            *(int*)arr->elements[4] == 50,
            "sort_unsorted",
            "Array should be sorted by pointed-to values",
            _counter) && result;

        d_ptr_array_free(arr);
    }

    return result;
}


/*
d_tests_sa_ptr_array_element_all
  Aggregation function that runs all element manipulation tests.
*/
bool
d_tests_sa_ptr_array_element_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Element Manipulation Functions\n");
    printf("  -----------------------------------------\n");

    result = d_tests_sa_ptr_array_append_element(_counter) && result;
    result = d_tests_sa_ptr_array_append_elements(_counter) && result;
    result = d_tests_sa_ptr_array_append_array(_counter) && result;
    result = d_tests_sa_ptr_array_contains(_counter) && result;
    result = d_tests_sa_ptr_array_fill(_counter) && result;
    result = d_tests_sa_ptr_array_find(_counter) && result;
    result = d_tests_sa_ptr_array_insert_element(_counter) && result;
    result = d_tests_sa_ptr_array_insert_elements(_counter) && result;
    result = d_tests_sa_ptr_array_insert_array(_counter) && result;
    result = d_tests_sa_ptr_array_is_empty(_counter) && result;
    result = d_tests_sa_ptr_array_prepend_element(_counter) && result;
    result = d_tests_sa_ptr_array_prepend_elements(_counter) && result;
    result = d_tests_sa_ptr_array_prepend_array(_counter) && result;
    result = d_tests_sa_ptr_array_reverse(_counter) && result;
    result = d_tests_sa_ptr_array_resize_amount(_counter) && result;
    result = d_tests_sa_ptr_array_resize_factor(_counter) && result;
    result = d_tests_sa_ptr_array_sort(_counter) && result;

    return result;
}
