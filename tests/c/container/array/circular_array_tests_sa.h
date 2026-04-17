/******************************************************************************
* djinterp [test]                                   circular_array_tests_sa.h
*
*   Unit test declarations for `circular_array.h` module.
*   Provides comprehensive testing of all d_circular_array functions including
* constructors, element access, push/pop operations, query functions, search
* functions, utility functions, and memory management.
*
*
* path:      /tests/container/array/circular_array_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.31
******************************************************************************/

#ifndef DJINTERP_TESTS_CIRCULAR_ARRAY_SA_
#define DJINTERP_TESTS_CIRCULAR_ARRAY_SA_ 1

#include <stdlib.h>
#include <errno.h>
#include "../../../../inc/c/djinterp.h"
#include "../../../../inc/c/dmemory.h"
#include "../../../../inc/c/test/test_standalone.h"
#include "../../../../inc/c/container/array/circular_array.h"


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_new(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_new_default_capacity(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_new_from_arr(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_new_from_args(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_new_copy(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_new_copy_resized(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_new_fill(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_circular_array_constructor_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. ELEMENT ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_get(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_set(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_front(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_back(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_peek(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_peek_back(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_circular_array_access_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. MODIFICATION FUNCTION TESTS - PUSH/POP OPERATIONS
 *****************************************************************************/
bool d_tests_sa_circular_array_push(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_push_front(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_push_back(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_push_all(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_push_all_front(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_pop(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_pop_front(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_pop_back(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_pop_to(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_push_overwrite(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_circular_array_modification_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. BULK OPERATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_clear(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_fill(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_rotate_left(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_rotate_right(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_reverse(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_swap(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_circular_array_bulk_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_is_empty(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_is_full(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_count(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_capacity(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_available_space(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_element_size(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_circular_array_query_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. SEARCH FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_contains(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_find(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_find_last(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_count_value(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_circular_array_search_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. CONVERSION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_to_linear_array(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_to_d_array(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_copy_to(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_circular_array_conversion_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. UTILITY AND MEMORY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_circular_array_sort(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_linearize(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_free(struct d_test_counter* _counter);
bool d_tests_sa_circular_array_free_deep(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_circular_array_utility_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_circular_array_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_CIRCULAR_ARRAY_SA_
