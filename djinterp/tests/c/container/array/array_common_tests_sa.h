/******************************************************************************
* djinterp [container]                                 array_common_tests_sa.h
*
*   Comprehensive unit test suite header for `array_common.h`.
*   This module provides thorough, rigorous testing of all array_common 
*   functions using the standalone test framework (test_standalone.h).
*
*   Note: This module is a dependency of DTest, so it uses `test_standalone.h`
*   rather than DTest for unit testing. Any modules that are not dependencies
*   of DTest should use DTest for unit tests.
*
* Test organization:
*   - array_common_tests_sa_init.c  : Initialization function tests (12 funcs)
*   - array_common_tests_sa_util.c  : Utility function tests (9 funcs)
*   - array_common_tests_sa_manip.c : Manipulation function tests (14 funcs)
*   - array_common_tests_sa_free.c  : Cleanup/free function tests (2 funcs)
*   - array_common_tests_sa.c       : Main test runner and helpers
*
* Test coverage summary:
*   - 37 total functions tested
*   - Each function has multiple test cases covering:
*     - Normal operation with valid inputs
*     - Edge cases (zero size, empty arrays, boundary indices)
*     - Invalid input handling (NULL pointers, invalid sizes)
*     - Negative index support (d_index validation)
*     - Memory allocation failure handling where applicable
*
*
* path:      \test\container\array\array_common_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.25
******************************************************************************/

#ifndef DJINTERP_TESTING_CONTAINER_ARRAY_COMMON_
#define DJINTERP_TESTING_CONTAINER_ARRAY_COMMON_ 1

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "..\..\..\inc\djinterp.h"
#include "..\..\..\inc\dmemory.h"
#include "..\..\..\inc\test\test_standalone.h"
#include "..\..\..\inc\container\array\array_common.h"


/******************************************************************************
 * TEST CONFIGURATION CONSTANTS
 *****************************************************************************/

// D_TEST_AC_MAX_ELEMENTS
//   constant: maximum number of elements used in test arrays.
#define D_TEST_AC_MAX_ELEMENTS 256

// D_TEST_AC_LARGE_ALLOCATION
//   constant: size for testing large allocations.
#define D_TEST_AC_LARGE_ALLOCATION 1000

// D_TEST_AC_STRUCT_SIZE
//   constant: size of a compound test struct for testing different element
//   sizes beyond simple integers.
#define D_TEST_AC_STRUCT_SIZE 64


/******************************************************************************
 * HELPER FUNCTION DECLARATIONS
 *****************************************************************************/

// d_test_init_from_args_wrapper
//   helper: wrapper for d_array_common_init_from_args that handles va_list.
//   This function converts variadic arguments to a va_list for testing the
//   underlying init_from_args function.
bool d_test_init_from_args_wrapper(void**  _elements,
                                   size_t* _count,
                                   size_t  _element_size,
                                   size_t  _arg_count,
                                   ...);

// d_test_int_comparator
//   helper: comparator function for integer values.
//   Returns: <0 if *_a < *_b, 0 if equal, >0 if *_a > *_b
int d_test_int_comparator(const void* _a,
                          const void* _b);

// d_test_free_int_ptr
//   helper: free function for integer pointers.
//   Safely frees a dynamically allocated integer pointer.
void d_test_free_int_ptr(void* _ptr);

// d_test_verify_array_contents
//   helper: verifies that an integer array contains expected values.
//   Returns true if all elements match, false otherwise.
bool d_test_verify_array_contents(const int* _array,
                                  size_t     _count,
                                  const int* _expected);

// d_test_verify_array_reversed
//   helper: verifies that an array is the reverse of expected values.
//   Returns true if array is correctly reversed, false otherwise.
bool d_test_verify_array_reversed(const int* _array,
                                  size_t     _count,
                                  const int* _original);


/******************************************************************************
 * INITIALIZATION FUNCTION TESTS
 * (defined in array_common_tests_sa_init.c)
 *
 * Tests for the following functions from array_common.h:
 *   - d_array_common_init_sized
 *   - d_array_common_init_from_array
 *   - d_array_common_init_from_args
 *   - d_array_common_init_copy
 *   - d_array_common_init_copy_reverse
 *   - d_array_common_init_copy_range
 *   - d_array_common_init_copy_range_reverse
 *   - d_array_common_init_fill
 *   - d_array_common_init_slice
 *   - d_array_common_init_slice_reverse
 *   - d_array_common_init_slice_range
 *   - d_array_common_init_slice_range_reverse
 *****************************************************************************/

// basic initialization functions
bool d_tests_sa_array_common_init_sized(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_init_from_array(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_init_from_args(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_init_copy(struct d_test_counter* _test_info);

// reverse and range copy functions
bool d_tests_sa_array_common_init_copy_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_init_copy_range(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_init_copy_range_reverse(struct d_test_counter* _test_info);

// fill initialization
bool d_tests_sa_array_common_init_fill(struct d_test_counter* _test_info);

// slice functions (operate on pointer arrays)
bool d_tests_sa_array_common_init_slice(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_init_slice_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_init_slice_range(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_init_slice_range_reverse(struct d_test_counter* _test_info);

// initialization test group runner
bool d_tests_sa_array_common_run_init_tests(struct d_test_counter* _test_info);


/******************************************************************************
 * UTILITY FUNCTION TESTS
 * (defined in array_common_tests_sa_util.c)
 *
 * Tests for the following functions from array_common.h:
 *   - d_array_common_alloc
 *   - d_array_common_append_element
 *   - d_array_common_append_elements
 *   - d_array_common_calc_capacity
 *   - d_array_common_contains
 *   - d_array_common_fill
 *   - d_array_common_find
 *   - d_array_common_find_closest
 *****************************************************************************/

// allocation and append functions
bool d_tests_sa_array_common_alloc(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_append_element(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_append_elements(struct d_test_counter* _test_info);

// capacity calculation
bool d_tests_sa_array_common_calc_capacity(struct d_test_counter* _test_info);

// search and query functions
bool d_tests_sa_array_common_contains(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_fill(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_find(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_find_closest(struct d_test_counter* _test_info);

// utility test group runner
bool d_tests_sa_array_common_run_util_tests(struct d_test_counter* _test_info);


/******************************************************************************
 * MANIPULATION FUNCTION TESTS
 * (defined in array_common_tests_sa_manip.c)
 *
 * Tests for the following functions from array_common.h:
 *   - d_array_common_insert_element
 *   - d_array_common_insert_elements
 *   - d_array_common_is_valid_resize_amount
 *   - d_array_common_is_valid_resize_factor
 *   - d_array_common_prepend_element
 *   - d_array_common_prepend_elements
 *   - d_array_common_resize_amount
 *   - d_array_common_resize_factor
 *   - d_array_common_reverse
 *   - d_array_common_shift_left
 *   - d_array_common_shift_left_circular
 *   - d_array_common_shift_right
 *   - d_array_common_shift_right_circular
 *   - d_array_common_sort
 *****************************************************************************/

// insert and prepend functions
bool d_tests_sa_array_common_insert_element(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_insert_elements(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_prepend_element(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_prepend_elements(struct d_test_counter* _test_info);

// resize validation functions
bool d_tests_sa_array_common_is_valid_resize_amount(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_is_valid_resize_factor(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_resize_amount(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_resize_factor(struct d_test_counter* _test_info);

// array transformation functions
bool d_tests_sa_array_common_reverse(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_shift_left(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_shift_left_circular(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_shift_right(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_shift_right_circular(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_sort(struct d_test_counter* _test_info);

// manipulation test group runner
bool d_tests_sa_array_common_run_manip_tests(struct d_test_counter* _test_info);


/******************************************************************************
 * CLEANUP/FREE FUNCTION TESTS
 * (defined in array_common_tests_sa_free.c)
 *
 * Tests for the following functions from array_common.h:
 *   - d_array_common_free_elements_arr
 *   - d_array_common_free_elements_deep
 *****************************************************************************/

// cleanup functions
bool d_tests_sa_array_common_free_elements_arr(struct d_test_counter* _test_info);
bool d_tests_sa_array_common_free_elements_deep(struct d_test_counter* _test_info);

// cleanup test group runner
bool d_tests_sa_array_common_run_free_tests(struct d_test_counter* _test_info);


/******************************************************************************
 * COMPREHENSIVE TEST SUITE RUNNERS
 * (defined in array_common_tests_sa.c)
 *****************************************************************************/

// d_tests_sa_array_common_all
//   function: runs all basic array_common test functions.
//   Executes the complete test suite including initialization, utility,
//   manipulation, and cleanup tests.
bool d_tests_sa_array_common_all(struct d_test_counter* _test_info);

// d_tests_sa_array_common_all_extended
//   function: runs ALL array_common test functions including all available
//   functionality and provides detailed reporting.
bool d_tests_sa_array_common_all_extended(struct d_test_counter* _test_info);

// d_tests_sa_array_common_run_all
//   function: main entry point for the test module runner. Returns pass/fail
//   status for use with the unified test runner.
bool d_tests_sa_array_common_run_all(struct d_test_counter* _test_info);


#endif  // DJINTERP_TESTING_CONTAINER_ARRAY_COMMON_
