/*******************************************************************************
* djinterp [test]                                      circular_array_tests_sa.h
*
*   Unit test declarations for `circular_array.h` module.
*   Provides comprehensive testing of all dj_circular_array functions including
* constructors, element access, push/pop operations, query functions, search
* functions, utility functions, and memory management.
*
*
* path:      /tests/djinterp/c/container/array/circular_array_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.31
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CIRCULAR_ARRAY_SA_
#define DJINTERP_TESTS_CIRCULAR_ARRAY_SA_ 1

// std
#include <errno.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/dmemory.h"
#include "../../../../../inc/djinterp/c/container/array/circular_array.h"
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_new(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_new_default_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_new_from_arr(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_new_from_args(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_new_copy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_new_copy_resized(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_new_fill(
    struct dj_test_counter* _counter
);

// I.   aggregation function
bool d_tests_sa_circular_array_constructor_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * II. ELEMENT ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_get(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_set(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_peek(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_peek_back(
    struct dj_test_counter* _counter
);

// II.  aggregation function
bool d_tests_sa_circular_array_access_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III. MODIFICATION FUNCTION TESTS - PUSH/POP OPERATIONS
 *****************************************************************************/
bool d_tests_sa_circular_array_push(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_push_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_push_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_push_all(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_push_all_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_pop(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_pop_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_pop_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_pop_to(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_push_overwrite(
    struct dj_test_counter* _counter
);

// III. aggregation function
bool d_tests_sa_circular_array_modification_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. BULK OPERATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_clear(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_fill(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_rotate_left(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_rotate_right(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_reverse(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_swap(
    struct dj_test_counter* _counter
);

// IV.  aggregation function
bool d_tests_sa_circular_array_bulk_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_is_empty(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_is_full(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_count(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_available_space(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_element_size(
    struct dj_test_counter* _counter
);

// V.   aggregation function
bool d_tests_sa_circular_array_query_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VI. SEARCH FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_contains(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_find(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_find_last(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_count_value(
    struct dj_test_counter* _counter
);

// VI.  aggregation function
bool d_tests_sa_circular_array_search_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VII. CONVERSION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_to_linear_array(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_to_d_array(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_copy_to(
    struct dj_test_counter* _counter
);

// VII. aggregation function
bool d_tests_sa_circular_array_conversion_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VIII. UTILITY AND MEMORY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_sort(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_linearize(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_free(
    struct dj_test_counter* _counter
);
bool d_tests_sa_circular_array_free_deep(
    struct dj_test_counter* _counter
);

// VIII. aggregation function
bool d_tests_sa_circular_array_utility_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_circular_array_run_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTS_CIRCULAR_ARRAY_SA_
