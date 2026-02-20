/*******************************************************************************
* djinterp [test]                                       assert_tests_sa_array.c
*
*   Array function tests for d_assert module.
*   Tests: d_assert_array_is_valid, d_assert_arrays_eq
*
*
* link:      TBA
* file:      \tests\assert_tests_sa_array.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.09.26
*******************************************************************************/

#include ".\assert_tests_sa.h"


/******************************************************************************
 * INDIVIDUAL TEST FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_assert_array_is_valid
  Tests d_assert_array_is_valid function for array validation.
  Tests the following:
  - correctly identifies valid array
  - correctly identifies NULL array
  - correctly identifies array with NULL first element
  - correctly identifies zero length array
*/
struct d_test_object*
d_tests_sa_assert_array_is_valid
(
    void
)
{
    struct d_test_object* group;
    int                   val1;
    int                   val2;
    int                   val3;
    const void*           valid_array[3];
    const void*           null_first_array[3];
    const void**          null_array;
    struct d_assert*   valid_true;
    struct d_assert*   null_array_false;
    struct d_assert*   null_element_false;
    struct d_assert*   zero_length_false;
    bool                  test_valid;
    bool                  test_null_array;
    bool                  test_null_element;
    bool                  test_zero_length;
    size_t                idx;

    val1 = 1;
    val2 = 2;
    val3 = 3;

    valid_array[0] = &val1;
    valid_array[1] = &val2;
    valid_array[2] = &val3;

    null_first_array[0] = NULL;
    null_first_array[1] = &val2;
    null_first_array[2] = &val3;

    null_array = NULL;

    // test 1: valid array
    valid_true = d_assert_array_is_valid(valid_array, 3,
                                         "Array is valid", "Array is invalid");
    test_valid = (valid_true != NULL) && (valid_true->result == true);

    // test 2: NULL array
    null_array_false = d_assert_array_is_valid(null_array, 3,
                                               "Array is valid", "Array is invalid");
    test_null_array = (null_array_false != NULL) && 
                      (null_array_false->result == false);

    // test 3: array with NULL first element
    null_element_false = d_assert_array_is_valid(null_first_array, 3,
                                                 "Array is valid", "Array is invalid");
    test_null_element = (null_element_false != NULL) && 
                        (null_element_false->result == false);

    // test 4: zero length array
    zero_length_false = d_assert_array_is_valid(valid_array, 0,
                                                "Array is valid", "Array is invalid");
    test_zero_length = (zero_length_false != NULL) && 
                       (zero_length_false->result == false);

    // cleanup
    d_assert_free(valid_true);
    d_assert_free(null_array_false);
    d_assert_free(null_element_false);
    d_assert_free(zero_length_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_array_is_valid", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_array",
                                           test_valid,
                                           "correctly identifies valid array");
    group->elements[idx++] = D_ASSERT_TRUE("null_array",
                                           test_null_array,
                                           "correctly identifies NULL array");
    group->elements[idx++] = D_ASSERT_TRUE("null_first_element",
                                           test_null_element,
                                           "correctly identifies NULL first element");
    group->elements[idx++] = D_ASSERT_TRUE("zero_length",
                                           test_zero_length,
                                           "correctly identifies zero length array");

    return group;
}

/*
d_tests_sa_assert_arrays_eq
  Tests d_assert_arrays_eq function for array equality comparison.
  Tests the following:
  - correctly identifies equal arrays
  - correctly identifies unequal arrays
  - correctly handles NULL array
*/
struct d_test_object*
d_tests_sa_assert_arrays_eq
(
    void
)
{
    struct d_test_object* group;
    int                   arr1[5];
    int                   arr2[5];
    int                   arr3[5];
    struct d_assert*   arrays_eq_true;
    struct d_assert*   arrays_eq_false;
    struct d_assert*   null_array_false;
    bool                  test_equal;
    bool                  test_unequal;
    bool                  test_null;
    size_t                idx;

    // initialize arrays
    arr1[0] = 1; arr1[1] = 2; arr1[2] = 3; arr1[3] = 4; arr1[4] = 5;
    arr2[0] = 1; arr2[1] = 2; arr2[2] = 3; arr2[3] = 4; arr2[4] = 5;
    arr3[0] = 1; arr3[1] = 2; arr3[2] = 3; arr3[3] = 4; arr3[4] = 6;

    // test 1: equal arrays
    arrays_eq_true = d_assert_arrays_eq(arr1, arr2, 5, sizeof(int),
                                        d_test_int_comparator,
                                        "Arrays equal", "Arrays not equal");
    test_equal = (arrays_eq_true != NULL) && (arrays_eq_true->result == true);

    // test 2: unequal arrays
    arrays_eq_false = d_assert_arrays_eq(arr1, arr3, 5, sizeof(int),
                                         d_test_int_comparator,
                                         "Arrays equal", "Arrays not equal");
    test_unequal = (arrays_eq_false != NULL) && (arrays_eq_false->result == false);

    // test 3: NULL array
    null_array_false = d_assert_arrays_eq(NULL, arr2, 5, sizeof(int),
                                          d_test_int_comparator,
                                          "Arrays equal", "Arrays not equal");
    test_null = (null_array_false != NULL) && (null_array_false->result == false);

    // cleanup
    d_assert_free(arrays_eq_true);
    d_assert_free(arrays_eq_false);
    d_assert_free(null_array_false);

    // build result tree
    group = d_test_object_new_interior("d_assert_arrays_eq", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("equal_arrays",
                                           test_equal,
                                           "correctly identifies equal arrays");
    group->elements[idx++] = D_ASSERT_TRUE("unequal_arrays",
                                           test_unequal,
                                           "correctly identifies unequal arrays");
    group->elements[idx++] = D_ASSERT_TRUE("null_array",
                                           test_null,
                                           "correctly handles NULL array");

    return group;
}


/******************************************************************************
 * CATEGORY RUNNER
 *****************************************************************************/

/*
d_tests_sa_assert_array_all
  Runs all array function tests for d_assert module.
*/
struct d_test_object*
d_tests_sa_assert_array_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Array Functions", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_assert_array_is_valid();
    group->elements[idx++] = d_tests_sa_assert_arrays_eq();

    return group;
}