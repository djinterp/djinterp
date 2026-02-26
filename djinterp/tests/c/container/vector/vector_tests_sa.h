/******************************************************************************
* djinterp [test]                                              vector_tests_sa.h
*
*   Unit test declarations for `vector.h` module.
*   Provides comprehensive testing of all d_vector functions including
* constructors, capacity management, element manipulation, append/prepend
* operations, resize operations, access functions, query functions, search
* functions, utility functions, and destructors.
*
*
* path:      \tests\container\vector\vector_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#ifndef DJINTERP_TESTS_VECTOR_SA_
#define DJINTERP_TESTS_VECTOR_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../inc/c/test/test_standalone.h"
#include "../../../../inc/c/container/vector/vector.h"
#include "../../../../inc/c/string_fn.h"


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_new(struct d_test_counter* _counter);
bool d_tests_sa_vector_new_default(struct d_test_counter* _counter);
bool d_tests_sa_vector_new_from_array(struct d_test_counter* _counter);
bool d_tests_sa_vector_new_from_args(struct d_test_counter* _counter);
bool d_tests_sa_vector_new_copy(struct d_test_counter* _counter);
bool d_tests_sa_vector_new_fill(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_vector_constructor_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. CAPACITY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_reserve(struct d_test_counter* _counter);
bool d_tests_sa_vector_shrink_to_fit(struct d_test_counter* _counter);
bool d_tests_sa_vector_ensure_capacity(struct d_test_counter* _counter);
bool d_tests_sa_vector_available(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_vector_capacity_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_push_back(struct d_test_counter* _counter);
bool d_tests_sa_vector_push_front(struct d_test_counter* _counter);
bool d_tests_sa_vector_pop_back(struct d_test_counter* _counter);
bool d_tests_sa_vector_pop_front(struct d_test_counter* _counter);
bool d_tests_sa_vector_insert_element(struct d_test_counter* _counter);
bool d_tests_sa_vector_insert_elements(struct d_test_counter* _counter);
bool d_tests_sa_vector_erase(struct d_test_counter* _counter);
bool d_tests_sa_vector_erase_range(struct d_test_counter* _counter);
bool d_tests_sa_vector_clear(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_vector_element_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. APPEND/PREPEND FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_append_element(struct d_test_counter* _counter);
bool d_tests_sa_vector_append_elements(struct d_test_counter* _counter);
bool d_tests_sa_vector_append_vector(struct d_test_counter* _counter);
bool d_tests_sa_vector_prepend_element(struct d_test_counter* _counter);
bool d_tests_sa_vector_prepend_elements(struct d_test_counter* _counter);
bool d_tests_sa_vector_prepend_vector(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_vector_append_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. RESIZE FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_resize(struct d_test_counter* _counter);
bool d_tests_sa_vector_resize_fill(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_vector_resize_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_at(struct d_test_counter* _counter);
bool d_tests_sa_vector_front(struct d_test_counter* _counter);
bool d_tests_sa_vector_back(struct d_test_counter* _counter);
bool d_tests_sa_vector_data(struct d_test_counter* _counter);
bool d_tests_sa_vector_get(struct d_test_counter* _counter);
bool d_tests_sa_vector_set(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_vector_access_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_is_empty(struct d_test_counter* _counter);
bool d_tests_sa_vector_is_full(struct d_test_counter* _counter);
bool d_tests_sa_vector_size(struct d_test_counter* _counter);
bool d_tests_sa_vector_capacity_query(struct d_test_counter* _counter);
bool d_tests_sa_vector_element_size(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_vector_query_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. SEARCH FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_find(struct d_test_counter* _counter);
bool d_tests_sa_vector_find_last(struct d_test_counter* _counter);
bool d_tests_sa_vector_contains(struct d_test_counter* _counter);
bool d_tests_sa_vector_count_value(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_vector_search_all(struct d_test_counter* _counter);


/******************************************************************************
 * IX. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_swap(struct d_test_counter* _counter);
bool d_tests_sa_vector_reverse(struct d_test_counter* _counter);
bool d_tests_sa_vector_sort(struct d_test_counter* _counter);
bool d_tests_sa_vector_copy_to(struct d_test_counter* _counter);

// IX.  aggregation function
bool d_tests_sa_vector_utility_all(struct d_test_counter* _counter);


/******************************************************************************
 * X. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_free(struct d_test_counter* _counter);
bool d_tests_sa_vector_free_deep(struct d_test_counter* _counter);

// X.   aggregation function
bool d_tests_sa_vector_destructor_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_vector_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_VECTOR_SA_
