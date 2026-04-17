/******************************************************************************
* djinterp [test]                                       vector_common_tests_sa.h
*
*   Unit test declarations for `vector_common.h` module.
*   Provides comprehensive testing of all vector_common utility functions
* including initialization, capacity management, element manipulation, 
* append/prepend operations, resize operations, access functions, query 
* functions, utility functions, and cleanup.
*
*
* path:      \tests\container\vector\vector_common_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#ifndef DJINTERP_TESTS_VECTOR_COMMON_SA_
#define DJINTERP_TESTS_VECTOR_COMMON_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../inc/c/test/test_standalone.h"
#include "../../../../inc/c/container/vector/vector_common.h"
#include "../../../../inc/c/string_fn.h"


/******************************************************************************
 * I. INITIALIZATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_init(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_init_from_array(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_init_from_args(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_init_copy(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_init_fill(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_vector_common_init_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. CAPACITY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_reserve(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_shrink_to_fit(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_ensure_capacity(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_grow(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_maybe_shrink(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_available(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_vector_common_capacity_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_push_back(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_push_front(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_pop_back(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_pop_front(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_insert_element(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_insert_elements(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_erase(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_erase_range(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_vector_common_element_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. APPEND/EXTEND FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_append_element(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_append_elements(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_prepend_element(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_prepend_elements(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_vector_common_append_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. RESIZE FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_resize(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_resize_fill(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_vector_common_resize_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_at(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_front(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_back(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_data(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_vector_common_access_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_is_empty(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_is_full(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_size(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_capacity_query(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_vector_common_query_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_swap(struct d_test_counter* _counter);
bool d_tests_sa_vector_common_copy_to(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_vector_common_utility_all(struct d_test_counter* _counter);


/******************************************************************************
 * IX. CLEANUP FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_free_elements(struct d_test_counter* _counter);

// IX.  aggregation function
bool d_tests_sa_vector_common_cleanup_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_vector_common_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_VECTOR_COMMON_SA_
