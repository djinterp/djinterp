/*******************************************************************************
* djinterp [container]                                          array_tests_sa.h
*
* Unit tests for `array.h` module.
* For the file itself, go to `\inc\container\array\array.h`.
* Note: this module is required to build DTest, so it does not use DTest for
* unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_CONTAINER_ARRAY_
#define DJINTERP_TESTING_CONTAINER_ARRAY_ 1

// std
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/dmemory.h"
#include "../../../../../inc/djinterp/c/container/array/array.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/******************************************************************************
 * HELPER FUNCTION DECLARATIONS
 *****************************************************************************/

// d_test_int_comparator
//   helper: comparator function for integers used in sorting/search tests.
int d_test_int_comparator(const void* _a, const void* _b);

// d_test_arrays_equal_int
//   helper: compares two integer arrays for equality.
bool d_test_arrays_equal_int(const void* _arr1, const void* _arr2, size_t _count);

// d_test_create_test_data_int
//   helper: creates test data array of integers [1, 2, 3, ..., _count].
void d_test_create_test_data_int(int** _data, size_t _count);


/******************************************************************************
 * D_ARRAY CREATION FUNCTION TESTS
 *****************************************************************************/

// `d_array` creation function tests
bool d_tests_sa_array_new(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_default_size(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_from_arr(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_from_args(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_copy(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_copy_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_copy_range(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_copy_range_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_fill(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_merge(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_slice(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_slice_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_slice_range(struct d_test_counter* _test_info);
bool d_tests_sa_array_new_slice_range_reverse(struct d_test_counter* _test_info);


/******************************************************************************
 * D_ARRAY_S CREATION FUNCTION TESTS
 *****************************************************************************/

// `d_array_s` creation function tests
bool d_tests_sa_array_s_new(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_default_size(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_from_arr(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_from_args(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_copy(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_copy_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_copy_range(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_copy_range_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_fill(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_merge(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_slice(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_slice_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_slice_range(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_new_slice_range_reverse(struct d_test_counter* _test_info);


/******************************************************************************
 * D_ARRAY MANIPULATION FUNCTION TESTS
 *****************************************************************************/

// `d_array` manipulation function tests
bool d_tests_sa_array_append_element(struct d_test_counter* _test_info);
bool d_tests_sa_array_append_elements(struct d_test_counter* _test_info);
bool d_tests_sa_array_append_array(struct d_test_counter* _test_info);
bool d_tests_sa_array_contains(struct d_test_counter* _test_info);
bool d_tests_sa_array_fill(struct d_test_counter* _test_info);
bool d_tests_sa_array_find(struct d_test_counter* _test_info);
bool d_tests_sa_array_insert_element(struct d_test_counter* _test_info);
bool d_tests_sa_array_insert_elements(struct d_test_counter* _test_info);
bool d_tests_sa_array_insert_array(struct d_test_counter* _test_info);
bool d_tests_sa_array_is_empty(struct d_test_counter* _test_info);
bool d_tests_sa_array_prepend_element(struct d_test_counter* _test_info);
bool d_tests_sa_array_prepend_elements(struct d_test_counter* _test_info);
bool d_tests_sa_array_prepend_array(struct d_test_counter* _test_info);
bool d_tests_sa_array_resize_amount(struct d_test_counter* _test_info);
bool d_tests_sa_array_resize_factor(struct d_test_counter* _test_info);
bool d_tests_sa_array_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_shift_left(struct d_test_counter* _test_info);
bool d_tests_sa_array_shift_left_circular(struct d_test_counter* _test_info);
bool d_tests_sa_array_shift_right(struct d_test_counter* _test_info);
bool d_tests_sa_array_shift_right_circular(struct d_test_counter* _test_info);
bool d_tests_sa_array_slice(struct d_test_counter* _test_info);
bool d_tests_sa_array_slice_range(struct d_test_counter* _test_info);
bool d_tests_sa_array_sort(struct d_test_counter* _test_info);


/******************************************************************************
 * MEMORY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/

// memory management function tests
bool d_tests_sa_array_free(struct d_test_counter* _test_info);
bool d_tests_sa_array_s_free(struct d_test_counter* _test_info);


/******************************************************************************
 * INTEGRATION AND EDGE CASE TESTS
 *****************************************************************************/

// integration and edge case tests
bool d_tests_sa_array_null_parameter_safety(struct d_test_counter* _test_info);
bool d_tests_sa_array_zero_element_size_safety(struct d_test_counter* _test_info);
bool d_tests_sa_array_negative_indexing(struct d_test_counter* _test_info);
bool d_tests_sa_array_large_data_sets(struct d_test_counter* _test_info);


/******************************************************************************
 * TEST GROUP RUNNERS
 *****************************************************************************/

// group runners for each test category
bool d_tests_sa_array_run_creation_tests(struct d_test_counter* _test_info);
bool d_tests_sa_array_run_creation_s_tests(struct d_test_counter* _test_info);
bool d_tests_sa_array_run_manipulation_tests(struct d_test_counter* _test_info);
bool d_tests_sa_array_run_memory_tests(struct d_test_counter* _test_info);
bool d_tests_sa_array_run_integration_tests(struct d_test_counter* _test_info);


/******************************************************************************
 * COMPREHENSIVE TEST SUITE RUNNERS
 *****************************************************************************/

// comprehensive test suite runners
bool d_tests_sa_array_all(struct d_test_counter* _test_info);
bool d_tests_sa_array_all_extended(struct d_test_counter* _test_info);
bool d_tests_sa_array_run_all(struct d_test_counter* _test_info);


#endif  // DJINTERP_TESTING_CONTAINER_ARRAY_
