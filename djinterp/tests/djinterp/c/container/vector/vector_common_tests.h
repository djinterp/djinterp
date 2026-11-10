/*******************************************************************************
* djinterp [test]                                          vector_common_tests.h
*
*   Unit test declarations for `vector_common.h` module.
*   Provides comprehensive testing of all vector_common utility functions
* including initialization, capacity management, element manipulation,
* append/prepend operations, resize operations, access functions, query
* functions, utility functions, and cleanup.
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTS_VECTOR_COMMON_
#define DJINTERP_TESTS_VECTOR_COMMON_ 1

// std
#include <stddef.h>
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/vector/vector_common.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. INITIALIZATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_common_init(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_init_from_array(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_init_from_args(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_init_copy(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_init_fill(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_vector_common_init_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. CAPACITY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_common_reserve(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_shrink_to_fit(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_ensure_capacity(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_grow(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_maybe_shrink(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_available(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_vector_common_capacity_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_common_push_back(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_push_front(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_pop_back(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_pop_front(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_insert_element(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_insert_elements(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_erase(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_erase_range(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_vector_common_element_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV. APPEND/EXTEND FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_common_append_element(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_append_elements(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_prepend_element(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_prepend_elements(
    struct dj_test_suite* _suite
);

// IV.  aggregation function
bool d_tests_vector_common_append_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * V. RESIZE FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_common_resize(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_resize_fill(
    struct dj_test_suite* _suite
);

// V.   aggregation function
bool d_tests_vector_common_resize_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VI. ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_common_at(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_front(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_back(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_data(
    struct dj_test_suite* _suite
);

// VI.  aggregation function
bool d_tests_vector_common_access_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VII. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_common_is_empty(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_is_full(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_size(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_capacity_query(
    struct dj_test_suite* _suite
);

// VII. aggregation function
bool d_tests_vector_common_query_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VIII. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_common_swap(
    struct dj_test_suite* _suite
);
bool d_tests_vector_common_copy_to(
    struct dj_test_suite* _suite
);

// VIII. aggregation function
bool d_tests_vector_common_utility_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IX. CLEANUP FUNCTION TESTS
 *****************************************************************************/
bool d_tests_vector_common_free_elements(
    struct dj_test_suite* _suite
);

// IX.  aggregation function
bool d_tests_vector_common_cleanup_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_vector_common_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_VECTOR_COMMON_
