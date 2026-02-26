/******************************************************************************
* djinterp [test]                                          ptr_vector_tests_sa.h
*
*   Unit test declarations for `ptr_vector.h` module.
*   Provides comprehensive testing of all d_ptr_vector struct wrapper functions
* including constructors, capacity management, element manipulation, 
* append/prepend operations, resize operations, access functions, query 
* functions, search functions, utility functions, iteration helpers, and 
* destructor operations.
*
*
* path:      \tests\container\vector\ptr_vector_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#ifndef DJINTERP_TESTS_PTR_VECTOR_SA_
#define DJINTERP_TESTS_PTR_VECTOR_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../../inc/c/test/test_standalone.h"
#include "../../../../inc/c/container/vector/ptr_vector.h"
#include "../../../../inc/c/string_fn.h"


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_new(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_new_default(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_new_from_array(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_new_from_args(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_new_copy(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_new_fill(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_new_merge(struct d_test_counter* _counter);

/* I.   aggregation function */
bool d_tests_sa_ptr_vector_constructor_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. CAPACITY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_reserve(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_shrink_to_fit(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_ensure_capacity(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_available(struct d_test_counter* _counter);

/* II.  aggregation function */
bool d_tests_sa_ptr_vector_capacity_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_push_back(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_push_front(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_pop_back(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_pop_front(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_insert(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_insert_range(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_erase(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_erase_range(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_remove(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_clear(struct d_test_counter* _counter);

/* III. aggregation function */
bool d_tests_sa_ptr_vector_element_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. APPEND/EXTEND FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_append(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_append_vector(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_prepend(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_prepend_vector(struct d_test_counter* _counter);

/* IV.  aggregation function */
bool d_tests_sa_ptr_vector_append_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. RESIZE FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_resize(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_resize_fill(struct d_test_counter* _counter);

/* V.   aggregation function */
bool d_tests_sa_ptr_vector_resize_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. ACCESS FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_at(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_front(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_back(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_data(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_get(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_set(struct d_test_counter* _counter);

/* VI.  aggregation function */
bool d_tests_sa_ptr_vector_access_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. QUERY FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_is_empty(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_is_full(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_size(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_capacity_query(struct d_test_counter* _counter);

/* VII. aggregation function */
bool d_tests_sa_ptr_vector_query_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. SEARCH FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_find(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_find_last(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_find_ptr(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_contains(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_contains_ptr(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_count_value(struct d_test_counter* _counter);

/* VIII. aggregation function */
bool d_tests_sa_ptr_vector_search_all(struct d_test_counter* _counter);


/******************************************************************************
 * IX. UTILITY FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_swap(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_reverse(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_sort(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_copy_to(struct d_test_counter* _counter);

/* IX.  aggregation function */
bool d_tests_sa_ptr_vector_utility_all(struct d_test_counter* _counter);


/******************************************************************************
 * X. ITERATION HELPER FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_foreach(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_foreach_with_context(struct d_test_counter* _counter);

/* X.   aggregation function */
bool d_tests_sa_ptr_vector_iteration_all(struct d_test_counter* _counter);


/******************************************************************************
 * XI. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_free(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_free_deep(struct d_test_counter* _counter);
bool d_tests_sa_ptr_vector_clear_deep(struct d_test_counter* _counter);

/* XI.  aggregation function */
bool d_tests_sa_ptr_vector_destructor_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/

bool d_tests_sa_ptr_vector_run_all(struct d_test_counter* _counter);


#endif  /* DJINTERP_TESTS_PTR_VECTOR_SA_ */
