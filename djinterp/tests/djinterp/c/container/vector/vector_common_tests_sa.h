/*******************************************************************************
* djinterp [test]                                       vector_common_tests_sa.h
*
*   Unit test declarations for `vector_common.h` module.
*   Provides comprehensive testing of all vector_common utility functions
* including initialization, capacity management, element manipulation,
* append/prepend operations, resize operations, access functions, query
* functions, utility functions, and cleanup.
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_VECTOR_COMMON_SA_
#define DJINTERP_TESTS_VECTOR_COMMON_SA_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/vector/vector_common.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * I. INITIALIZATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_init(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_init_from_array(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_init_from_args(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_init_copy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_init_fill(
    struct dj_test_counter* _counter
);

// I.   aggregation function
bool d_tests_sa_vector_common_init_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * II. CAPACITY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_reserve(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_shrink_to_fit(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_ensure_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_grow(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_maybe_shrink(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_available(
    struct dj_test_counter* _counter
);

// II.  aggregation function
bool d_tests_sa_vector_common_capacity_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III. ELEMENT MANIPULATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_push_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_push_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_pop_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_pop_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_insert_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_insert_elements(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_erase(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_erase_range(
    struct dj_test_counter* _counter
);

// III. aggregation function
bool d_tests_sa_vector_common_element_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. APPEND/EXTEND FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_append_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_append_elements(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_prepend_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_prepend_elements(
    struct dj_test_counter* _counter
);

// IV.  aggregation function
bool d_tests_sa_vector_common_append_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. RESIZE FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_resize(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_resize_fill(
    struct dj_test_counter* _counter
);

// V.   aggregation function
bool d_tests_sa_vector_common_resize_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VI. ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_at(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_data(
    struct dj_test_counter* _counter
);

// VI.  aggregation function
bool d_tests_sa_vector_common_access_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VII. QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_is_empty(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_is_full(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_size(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_capacity_query(
    struct dj_test_counter* _counter
);

// VII. aggregation function
bool d_tests_sa_vector_common_query_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VIII. UTILITY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_swap(
    struct dj_test_counter* _counter
);
bool d_tests_sa_vector_common_copy_to(
    struct dj_test_counter* _counter
);

// VIII. aggregation function
bool d_tests_sa_vector_common_utility_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IX. CLEANUP FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_vector_common_free_elements(
    struct dj_test_counter* _counter
);

// IX.  aggregation function
bool d_tests_sa_vector_common_cleanup_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_vector_common_run_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTS_VECTOR_COMMON_SA_
