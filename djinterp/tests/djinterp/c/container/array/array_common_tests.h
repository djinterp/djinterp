/*******************************************************************************
* djinterp [test]                                           array_common_tests.h
*
*   Comprehensive unit test suite header for `array_common.h`, on DTest's
*   C face (struct dj_test_suite). It tests every array_common function.
*   (It used the standalone harness, as one of DTest's dependencies; lane
*   B's ruling B13 retired that tier.)
*
* Test organization:
*   - array_common_tests_init.c     : Initialization function tests (12 funcs)
*   - array_common_tests_util.c     : Utility function tests (9 funcs)
*   - array_common_tests_manip.c    : Manipulation function tests (14 funcs)
*   - array_common_tests_free.c     : Cleanup/free function tests (2 funcs)
*   - array_common_tests.c          : Main test runner and helpers
*
* Test coverage summary:
*   - 37 total functions tested
*   - Each function has multiple test cases covering:
*     - Normal operation with valid inputs
*     - Edge cases (zero size, empty arrays, boundary indices)
*     - Invalid input handling (NULL pointers, invalid sizes)
*     - Negative index support (dj_index validation)
*     - Memory allocation failure handling where applicable
*
*
* path:      /tests/djinterp/c/container/array/array_common_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.25
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTING_CONTAINER_ARRAY_COMMON_
#define DJINTERP_TESTING_CONTAINER_ARRAY_COMMON_ 1

// std
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../../../inc/djinterp/c/container/array/array_common.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


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
bool d_test_init_from_args_wrapper(
    void**  _elements,
    size_t* _count,
    size_t  _element_size,
    size_t  _arg_count,
    ...
);

// d_test_int_comparator
//   helper: comparator function for integer values.
//   Returns: <0 if *_a < *_b, 0 if equal, >0 if *_a > *_b
int d_test_int_comparator(
    const void* _a,
    const void* _b
);

// d_test_free_int_ptr
//   helper: free function for integer pointers.
//   Safely frees a dynamically allocated integer pointer.
void d_test_free_int_ptr(
    void* _ptr
);

// d_test_verify_array_contents
//   helper: verifies that an integer array contains expected values.
//   Returns true if all elements match, false otherwise.
bool d_test_verify_array_contents(
    const int* _array,
    size_t     _count,
    const int* _expected
);

// d_test_verify_array_reversed
//   helper: verifies that an array is the reverse of expected values.
//   Returns true if array is correctly reversed, false otherwise.
bool d_test_verify_array_reversed(
    const int* _array,
    size_t     _count,
    const int* _original
);


/******************************************************************************
 * INITIALIZATION FUNCTION TESTS
 * (defined in array_common_tests_init.c)
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
bool d_tests_array_common_init_sized(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_init_from_array(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_init_from_args(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_init_copy(
    struct dj_test_suite* _suite
);

// reverse and range copy functions
bool d_tests_array_common_init_copy_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_init_copy_range(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_init_copy_range_reverse(
    struct dj_test_suite* _suite
);

// fill initialization
bool d_tests_array_common_init_fill(
    struct dj_test_suite* _suite
);

// slice functions (operate on pointer arrays)
bool d_tests_array_common_init_slice(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_init_slice_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_init_slice_range(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_init_slice_range_reverse(
    struct dj_test_suite* _suite
);

// initialization test group runner
bool d_tests_array_common_run_init_tests(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * UTILITY FUNCTION TESTS
 * (defined in array_common_tests_util.c)
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
bool d_tests_array_common_alloc(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_append_element(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_append_elements(
    struct dj_test_suite* _suite
);

// capacity calculation
bool d_tests_array_common_calc_capacity(
    struct dj_test_suite* _suite
);

// search and query functions
bool d_tests_array_common_contains(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_fill(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_find(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_find_closest(
    struct dj_test_suite* _suite
);

// utility test group runner
bool d_tests_array_common_run_util_tests(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MANIPULATION FUNCTION TESTS
 * (defined in array_common_tests_manip.c)
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
bool d_tests_array_common_insert_element(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_insert_elements(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_prepend_element(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_prepend_elements(
    struct dj_test_suite* _suite
);

// resize validation functions
bool d_tests_array_common_is_valid_resize_amount(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_is_valid_resize_factor(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_resize_amount(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_resize_factor(
    struct dj_test_suite* _suite
);

// array transformation functions
bool d_tests_array_common_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_shift_left(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_shift_left_circular(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_shift_right(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_shift_right_circular(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_sort(
    struct dj_test_suite* _suite
);

// manipulation test group runner
bool d_tests_array_common_run_manip_tests(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * CLEANUP/FREE FUNCTION TESTS
 * (defined in array_common_tests_free.c)
 *
 * Tests for the following functions from array_common.h:
 *   - d_array_common_free_elements_arr
 *   - d_array_common_free_elements_deep
 *****************************************************************************/

// cleanup functions
bool d_tests_array_common_free_elements_arr(
    struct dj_test_suite* _suite
);
bool d_tests_array_common_free_elements_deep(
    struct dj_test_suite* _suite
);

// cleanup test group runner
bool d_tests_array_common_run_free_tests(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * COMPREHENSIVE TEST SUITE RUNNERS
 * (defined in array_common_tests.c)
 *****************************************************************************/

// d_tests_array_common_all
//   function: runs all basic array_common test functions.
//   Executes the complete test suite including initialization, utility,
//   manipulation, and cleanup tests.
bool d_tests_array_common_all(
    struct dj_test_suite* _suite
);

// d_tests_array_common_all_extended
//   function: runs ALL array_common test functions including all available
//   functionality and provides detailed reporting.
bool d_tests_array_common_all_extended(
    struct dj_test_suite* _suite
);

// d_tests_array_common_run_all
//   function: main entry point for the test module runner. Returns pass/fail
//   status for use with the unified test runner.
bool d_tests_array_common_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTING_CONTAINER_ARRAY_COMMON_
