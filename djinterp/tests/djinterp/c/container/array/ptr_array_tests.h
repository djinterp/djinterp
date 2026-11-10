/*******************************************************************************
* djinterp [test]                                              ptr_array_tests.h
*
*   Unit test declarations for `ptr_array.h` module.
*   Provides comprehensive testing of all dj_ptr_array functions including
* constructors, element manipulation, append/prepend operations, resize
* operations, search functions, utility functions, and destructors.
*
*
* path:      /tests/djinterp/c/container/array/ptr_array_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.31
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_PTR_ARRAY_
#define DJINTERP_TESTS_PTR_ARRAY_ 1

// std
#include <stddef.h>
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/array/ptr_array.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_ptr_array_new(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_new_default_size(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_new_from_arr(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_new_from_args(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_new_copy(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_new_merge(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_new_slice(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_new_slice_range(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_ptr_array_constructor_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_ptr_array_append_element(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_append_elements(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_append_array(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_contains(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_fill(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_find(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_insert_element(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_insert_elements(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_insert_array(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_is_empty(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_prepend_element(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_prepend_elements(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_prepend_array(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_resize_amount(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_resize_factor(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_sort(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_ptr_array_element_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_ptr_array_free(
    struct dj_test_suite* _suite
);
bool d_tests_ptr_array_deep_free(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_ptr_array_destructor_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_ptr_array_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_PTR_ARRAY_
