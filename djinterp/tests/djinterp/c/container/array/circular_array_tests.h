/*******************************************************************************
* djinterp [test]                                         circular_array_tests.h
*
*   Unit test declarations for `circular_array.h` module.
*   Provides comprehensive testing of all dj_circular_array functions including
* constructors, element access, push/pop operations, query functions, search
* functions, utility functions, and memory management.
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.31
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_CIRCULAR_ARRAY_
#define DJINTERP_TESTS_CIRCULAR_ARRAY_ 1

// std
#include <errno.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../../../inc/djinterp/c/container/array/circular_array.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_circular_array_new(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_new_default_capacity(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_new_from_arr(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_new_from_args(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_new_copy(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_new_copy_resized(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_new_fill(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_circular_array_constructor_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. ELEMENT ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_circular_array_get(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_set(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_front(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_back(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_peek(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_peek_back(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_circular_array_access_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. MODIFICATION FUNCTION TESTS - PUSH/POP OPERATIONS
 *****************************************************************************/
bool d_tests_circular_array_push(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_push_front(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_push_back(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_push_all(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_push_all_front(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_pop(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_pop_front(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_pop_back(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_pop_to(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_push_overwrite(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_circular_array_modification_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV. BULK OPERATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_circular_array_clear(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_fill(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_rotate_left(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_rotate_right(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_swap(
    struct dj_test_suite* _suite
);

// IV.  aggregation function
bool d_tests_circular_array_bulk_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * V. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_circular_array_is_empty(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_is_full(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_count(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_capacity(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_available_space(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_element_size(
    struct dj_test_suite* _suite
);

// V.   aggregation function
bool d_tests_circular_array_query_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VI. SEARCH FUNCTION TESTS
 *****************************************************************************/
bool d_tests_circular_array_contains(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_find(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_find_last(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_count_value(
    struct dj_test_suite* _suite
);

// VI.  aggregation function
bool d_tests_circular_array_search_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VII. CONVERSION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_circular_array_to_linear_array(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_to_d_array(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_copy_to(
    struct dj_test_suite* _suite
);

// VII. aggregation function
bool d_tests_circular_array_conversion_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VIII. UTILITY AND MEMORY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_circular_array_sort(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_linearize(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_free(
    struct dj_test_suite* _suite
);
bool d_tests_circular_array_free_deep(
    struct dj_test_suite* _suite
);

// VIII. aggregation function
bool d_tests_circular_array_utility_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_circular_array_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_CIRCULAR_ARRAY_
