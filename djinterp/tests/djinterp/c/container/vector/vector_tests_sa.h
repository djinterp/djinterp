/*******************************************************************************
* djinterp [test]                                              vector_tests_sa.h
*
*   Unit test declarations for `vector.h` module.
*   Provides comprehensive testing of all dj_vector functions including
* constructors, capacity management, element manipulation, append/prepend
* operations, resize operations, access functions, query functions, search
* functions, utility functions, and destructors.
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_VECTOR_SA_
#define DJINTERP_TESTS_VECTOR_SA_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/vector/vector.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_new(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_new_default(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_new_from_array(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_new_from_args(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_new_copy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_new_fill(
    struct dj_test_counter* _counter
);

// I.   aggregation function
bool d_tests_sa_vector_constructor_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * II. CAPACITY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_reserve(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_shrink_to_fit(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_ensure_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_available(
    struct dj_test_counter* _counter
);

// II.  aggregation function
bool d_tests_sa_vector_capacity_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_push_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_push_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_pop_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_pop_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_insert_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_insert_elements(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_erase(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_erase_range(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_clear(
    struct dj_test_counter* _counter
);

// III. aggregation function
bool d_tests_sa_vector_element_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. APPEND/PREPEND FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_append_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_append_elements(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_append_vector(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_prepend_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_prepend_elements(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_prepend_vector(
    struct dj_test_counter* _counter
);

// IV.  aggregation function
bool d_tests_sa_vector_append_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. RESIZE FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_resize(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_resize_fill(
    struct dj_test_counter* _counter
);

// V.   aggregation function
bool d_tests_sa_vector_resize_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VI. ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_at(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_data(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_get(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_set(
    struct dj_test_counter* _counter
);

// VI.  aggregation function
bool d_tests_sa_vector_access_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VII. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_is_empty(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_is_full(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_size(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_capacity_query(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_element_size(
    struct dj_test_counter* _counter
);

// VII. aggregation function
bool d_tests_sa_vector_query_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VIII. SEARCH FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_find(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_find_last(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_contains(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_count_value(
    struct dj_test_counter* _counter
);

// VIII. aggregation function
bool d_tests_sa_vector_search_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IX. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_swap(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_reverse(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_sort(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_copy_to(
    struct dj_test_counter* _counter
);

// IX.  aggregation function
bool d_tests_sa_vector_utility_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * X. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_free(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_free_deep(
    struct dj_test_counter* _counter
);

// X.   aggregation function
bool d_tests_sa_vector_destructor_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_vector_run_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTS_VECTOR_SA_
