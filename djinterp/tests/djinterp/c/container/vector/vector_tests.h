/*******************************************************************************
* djinterp [test]                                                 vector_tests.h
*
*   Unit test declarations for `vector.h` module.
*   Provides comprehensive testing of all dj_vector functions including
* constructors, capacity management, element manipulation, append/prepend
* operations, resize operations, access functions, query functions, search
* functions, utility functions, and destructors.
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_VECTOR_
#define DJINTERP_TESTS_VECTOR_ 1

// std
#include <stddef.h>
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/vector/vector.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. CONSTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_new(
    struct dj_test_suite* _suite
);
bool d_tests_vector_new_default(
    struct dj_test_suite* _suite
);
bool d_tests_vector_new_from_array(
    struct dj_test_suite* _suite
);
bool d_tests_vector_new_from_args(
    struct dj_test_suite* _suite
);
bool d_tests_vector_new_copy(
    struct dj_test_suite* _suite
);
bool d_tests_vector_new_fill(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_vector_constructor_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. CAPACITY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_reserve(
    struct dj_test_suite* _suite
);
bool d_tests_vector_shrink_to_fit(
    struct dj_test_suite* _suite
);
bool d_tests_vector_ensure_capacity(
    struct dj_test_suite* _suite
);
bool d_tests_vector_available(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_vector_capacity_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_push_back(
    struct dj_test_suite* _suite
);
bool d_tests_vector_push_front(
    struct dj_test_suite* _suite
);
bool d_tests_vector_pop_back(
    struct dj_test_suite* _suite
);
bool d_tests_vector_pop_front(
    struct dj_test_suite* _suite
);
bool d_tests_vector_insert_element(
    struct dj_test_suite* _suite
);
bool d_tests_vector_insert_elements(
    struct dj_test_suite* _suite
);
bool d_tests_vector_erase(
    struct dj_test_suite* _suite
);
bool d_tests_vector_erase_range(
    struct dj_test_suite* _suite
);
bool d_tests_vector_clear(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_vector_element_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV. APPEND/PREPEND FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_append_element(
    struct dj_test_suite* _suite
);
bool d_tests_vector_append_elements(
    struct dj_test_suite* _suite
);
bool d_tests_vector_append_vector(
    struct dj_test_suite* _suite
);
bool d_tests_vector_prepend_element(
    struct dj_test_suite* _suite
);
bool d_tests_vector_prepend_elements(
    struct dj_test_suite* _suite
);
bool d_tests_vector_prepend_vector(
    struct dj_test_suite* _suite
);

// IV.  aggregation function
bool d_tests_vector_append_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * V. RESIZE FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_resize(
    struct dj_test_suite* _suite
);
bool d_tests_vector_resize_fill(
    struct dj_test_suite* _suite
);

// V.   aggregation function
bool d_tests_vector_resize_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VI. ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_at(
    struct dj_test_suite* _suite
);
bool d_tests_vector_front(
    struct dj_test_suite* _suite
);
bool d_tests_vector_back(
    struct dj_test_suite* _suite
);
bool d_tests_vector_data(
    struct dj_test_suite* _suite
);
bool d_tests_vector_get(
    struct dj_test_suite* _suite
);
bool d_tests_vector_set(
    struct dj_test_suite* _suite
);

// VI.  aggregation function
bool d_tests_vector_access_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VII. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_is_empty(
    struct dj_test_suite* _suite
);
bool d_tests_vector_is_full(
    struct dj_test_suite* _suite
);
bool d_tests_vector_size(
    struct dj_test_suite* _suite
);
bool d_tests_vector_capacity_query(
    struct dj_test_suite* _suite
);
bool d_tests_vector_element_size(
    struct dj_test_suite* _suite
);

// VII. aggregation function
bool d_tests_vector_query_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VIII. SEARCH FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_find(
    struct dj_test_suite* _suite
);
bool d_tests_vector_find_last(
    struct dj_test_suite* _suite
);
bool d_tests_vector_contains(
    struct dj_test_suite* _suite
);
bool d_tests_vector_count_value(
    struct dj_test_suite* _suite
);

// VIII. aggregation function
bool d_tests_vector_search_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IX. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_swap(
    struct dj_test_suite* _suite
);
bool d_tests_vector_reverse(
    struct dj_test_suite* _suite
);
bool d_tests_vector_sort(
    struct dj_test_suite* _suite
);
bool d_tests_vector_copy_to(
    struct dj_test_suite* _suite
);

// IX.  aggregation function
bool d_tests_vector_utility_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * X. DESTRUCTOR FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_free(
    struct dj_test_suite* _suite
);
bool d_tests_vector_free_deep(
    struct dj_test_suite* _suite
);

// X.   aggregation function
bool d_tests_vector_destructor_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_vector_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_VECTOR_
