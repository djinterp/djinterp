/*******************************************************************************
* djinterp [test]                                          ptr_vector_tests_sa.h
*
*   Unit test declarations for `ptr_vector.h` module.
*   Provides comprehensive testing of all dj_ptr_vector struct wrapper functions
* including constructors, capacity management, element manipulation,
* append/prepend operations, resize operations, access functions, query
* functions, search functions, utility functions, iteration helpers, and
* destructor operations.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_PTR_VECTOR_SA_
#define DJINTERP_TESTS_PTR_VECTOR_SA_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/vector/ptr_vector.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_new(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_new_default(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_new_from_array(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_new_from_args(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_new_copy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_new_fill(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_new_merge(
    struct dj_test_counter* _counter
);

/* I.   aggregation function */
bool d_tests_sa_ptr_vector_constructor_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * II. CAPACITY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_reserve(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_shrink_to_fit(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_ensure_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_available(
    struct dj_test_counter* _counter
);

/* II.  aggregation function */
bool d_tests_sa_ptr_vector_capacity_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_push_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_push_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_pop_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_pop_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_insert(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_insert_range(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_erase(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_erase_range(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_remove(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_clear(
    struct dj_test_counter* _counter
);

/* III. aggregation function */
bool d_tests_sa_ptr_vector_element_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. APPEND/EXTEND FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_append(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_append_vector(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_prepend(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_prepend_vector(
    struct dj_test_counter* _counter
);

/* IV.  aggregation function */
bool d_tests_sa_ptr_vector_append_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. RESIZE FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_resize(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_resize_fill(
    struct dj_test_counter* _counter
);

/* V.   aggregation function */
bool d_tests_sa_ptr_vector_resize_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VI. ACCESS FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_at(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_data(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_get(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_set(
    struct dj_test_counter* _counter
);

/* VI.  aggregation function */
bool d_tests_sa_ptr_vector_access_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VII. QUERY FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_is_empty(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_is_full(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_size(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_capacity_query(
    struct dj_test_counter* _counter
);

/* VII. aggregation function */
bool d_tests_sa_ptr_vector_query_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VIII. SEARCH FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_find(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_find_last(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_find_ptr(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_contains(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_contains_ptr(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_count_value(
    struct dj_test_counter* _counter
);

/* VIII. aggregation function */
bool d_tests_sa_ptr_vector_search_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IX. UTILITY FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_swap(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_reverse(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_sort(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_copy_to(
    struct dj_test_counter* _counter
);

/* IX.  aggregation function */
bool d_tests_sa_ptr_vector_utility_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * X. ITERATION HELPER FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_foreach(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_foreach_with_context(
    struct dj_test_counter* _counter
);

/* X.   aggregation function */
bool d_tests_sa_ptr_vector_iteration_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * XI. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/

bool d_tests_sa_ptr_vector_free(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_free_deep(
    struct dj_test_counter* _counter
);
bool d_tests_sa_ptr_vector_clear_deep(
    struct dj_test_counter* _counter
);

/* XI.  aggregation function */
bool d_tests_sa_ptr_vector_destructor_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/

bool d_tests_sa_ptr_vector_run_all(
    struct dj_test_counter* _counter
);


#endif  /* DJINTERP_TESTS_PTR_VECTOR_SA_ */
