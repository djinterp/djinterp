/*******************************************************************************
* djinterp [test]                                                  array_tests.h
*
* Unit tests for `array.h` module.
* For the file itself, go to `\inc\container\array\array.h`.
* Note: this module is required to build DTest, so it does not use DTest for
* unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
*
*
* path:      /tests/djinterp/c/container/array/array_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTING_CONTAINER_ARRAY_
#define DJINTERP_TESTING_CONTAINER_ARRAY_ 1

// std
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../../../inc/djinterp/c/container/array/array.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER FUNCTION DECLARATIONS
 *****************************************************************************/

// d_test_int_comparator
//   helper: comparator function for integers used in sorting/search tests.
int d_test_int_comparator(
    const void* _a,
    const void* _b
);

// d_test_arrays_equal_int
//   helper: compares two integer arrays for equality.
bool d_test_arrays_equal_int(
    const void* _arr1,
    const void* _arr2,
    size_t      _count
);

// d_test_create_test_data_int
//   helper: creates test data array of integers [1, 2, 3, ..., _count].
void d_test_create_test_data_int(
    int**  _data,
    size_t _count
);


/******************************************************************************
 * D_ARRAY CREATION FUNCTION TESTS
 *****************************************************************************/

// `dj_array` creation function tests
bool d_tests_array_new(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_default_size(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_from_arr(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_from_args(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_copy(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_copy_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_copy_range(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_copy_range_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_fill(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_merge(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_slice(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_slice_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_slice_range(
    struct dj_test_suite* _suite
);
bool d_tests_array_new_slice_range_reverse(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * D_ARRAY_S CREATION FUNCTION TESTS
 *****************************************************************************/

// `dj_array_s` creation function tests
bool d_tests_array_s_new(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_default_size(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_from_arr(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_from_args(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_copy(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_copy_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_copy_range(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_copy_range_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_fill(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_merge(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_slice(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_slice_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_slice_range(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_new_slice_range_reverse(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * D_ARRAY MANIPULATION FUNCTION TESTS
 *****************************************************************************/

// `dj_array` manipulation function tests
bool d_tests_array_append_element(
    struct dj_test_suite* _suite
);
bool d_tests_array_append_elements(
    struct dj_test_suite* _suite
);
bool d_tests_array_append_array(
    struct dj_test_suite* _suite
);
bool d_tests_array_contains(
    struct dj_test_suite* _suite
);
bool d_tests_array_fill(
    struct dj_test_suite* _suite
);
bool d_tests_array_find(
    struct dj_test_suite* _suite
);
bool d_tests_array_insert_element(
    struct dj_test_suite* _suite
);
bool d_tests_array_insert_elements(
    struct dj_test_suite* _suite
);
bool d_tests_array_insert_array(
    struct dj_test_suite* _suite
);
bool d_tests_array_is_empty(
    struct dj_test_suite* _suite
);
bool d_tests_array_prepend_element(
    struct dj_test_suite* _suite
);
bool d_tests_array_prepend_elements(
    struct dj_test_suite* _suite
);
bool d_tests_array_prepend_array(
    struct dj_test_suite* _suite
);
bool d_tests_array_resize_amount(
    struct dj_test_suite* _suite
);
bool d_tests_array_resize_factor(
    struct dj_test_suite* _suite
);
bool d_tests_array_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_shift_left(
    struct dj_test_suite* _suite
);
bool d_tests_array_shift_left_circular(
    struct dj_test_suite* _suite
);
bool d_tests_array_shift_right(
    struct dj_test_suite* _suite
);
bool d_tests_array_shift_right_circular(
    struct dj_test_suite* _suite
);
bool d_tests_array_slice(
    struct dj_test_suite* _suite
);
bool d_tests_array_slice_range(
    struct dj_test_suite* _suite
);
bool d_tests_array_sort(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MEMORY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/

// memory management function tests
bool d_tests_array_free(
    struct dj_test_suite* _suite
);
bool d_tests_array_s_free(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * INTEGRATION AND EDGE CASE TESTS
 *****************************************************************************/

// integration and edge case tests
bool d_tests_array_null_parameter_safety(
    struct dj_test_suite* _suite
);
bool d_tests_array_zero_element_size_safety(
    struct dj_test_suite* _suite
);
bool d_tests_array_negative_indexing(
    struct dj_test_suite* _suite
);
bool d_tests_array_large_data_sets(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * TEST GROUP RUNNERS
 *****************************************************************************/

// group runners for each test category
bool d_tests_array_run_creation_tests(
    struct dj_test_suite* _suite
);
bool d_tests_array_run_creation_s_tests(
    struct dj_test_suite* _suite
);
bool d_tests_array_run_manipulation_tests(
    struct dj_test_suite* _suite
);
bool d_tests_array_run_memory_tests(
    struct dj_test_suite* _suite
);
bool d_tests_array_run_integration_tests(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * COMPREHENSIVE TEST SUITE RUNNERS
 *****************************************************************************/

// comprehensive test suite runners
bool d_tests_array_all(
    struct dj_test_suite* _suite
);
bool d_tests_array_all_extended(
    struct dj_test_suite* _suite
);
bool d_tests_array_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTING_CONTAINER_ARRAY_
