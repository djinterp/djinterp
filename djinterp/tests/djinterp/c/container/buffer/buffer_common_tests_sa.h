/*******************************************************************************
* djinterp [test]                                       buffer_common_tests_sa.h
*
*   Unit test declarations for `buffer_common.h` module.
*   Provides comprehensive testing of all d_buffer_common functions including
* initialization, capacity management, element access, insertion (resize and
* append modes), removal, state queries, search, copy, ordering, validation,
* and destruction.
*
*   NOTE: Section IX (filter) is excluded because it depends on the external
* d_filter_expr / d_filter_evaluate API from filter.h, which is tested
* separately.
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_BUFFER_COMMON_SA_
#define DJINTERP_TESTS_BUFFER_COMMON_SA_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/buffer/buffer_common.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


/******************************************************************************
 * I. INITIALIZATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_init_sized(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_init_from_data(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_init_copy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_init_fill(
    struct dj_test_counter* _counter
);

// I.   aggregation function
bool d_tests_sa_buffer_common_init_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * II. CAPACITY MANAGEMENT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_alloc(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_calc_growth(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_ensure_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_resize_to_fit(
    struct dj_test_counter* _counter
);

// II.  aggregation function
bool d_tests_sa_buffer_common_capacity_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * III. ELEMENT ACCESS FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_get_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_set_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_peek_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_peek_back(
    struct dj_test_counter* _counter
);

// III. aggregation function
bool d_tests_sa_buffer_common_access_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * IV. INSERTION (RESIZE MODE) FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_append_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_append_data(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_prepend_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_prepend_data(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_insert_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_insert_data(
    struct dj_test_counter* _counter
);

// IV.  aggregation function
bool d_tests_sa_buffer_common_insert_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * V. INSERTION (APPEND MODE / CHUNKED) FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_chunk_new(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_chunk_free(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_chunk_list_init(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_chunk_list_free(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_append_element_chunked(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_append_data_chunked(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_consolidate(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_total_count(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_get_element_chunked(
    struct dj_test_counter* _counter
);

// V.   aggregation function
bool d_tests_sa_buffer_common_chunked_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VI. REMOVAL FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_remove_element(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_remove_range(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_consume_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_consume_back(
    struct dj_test_counter* _counter
);

// VI.  aggregation function
bool d_tests_sa_buffer_common_removal_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VII. STATE QUERY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_clear(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_is_empty(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_is_full(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_utilization(
    struct dj_test_counter* _counter
);

// VII. aggregation function
bool d_tests_sa_buffer_common_state_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * VIII. SEARCH FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_contains(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_find(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_find_last(
    struct dj_test_counter* _counter
);

// VIII. aggregation function
bool d_tests_sa_buffer_common_search_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * X. COPY FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_copy_to(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_copy_range_to(
    struct dj_test_counter* _counter
);

// X.   aggregation function
bool d_tests_sa_buffer_common_copy_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * XI. ORDERING FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_reverse(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_sort(
    struct dj_test_counter* _counter
);

// XI.  aggregation function
bool d_tests_sa_buffer_common_ordering_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * XII. VALIDATION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_validate_params(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_validate_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_validate_index(
    struct dj_test_counter* _counter
);

// XII. aggregation function
bool d_tests_sa_buffer_common_validation_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * XIII. DESTRUCTION FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_buffer_common_free_data(
    struct dj_test_counter* _counter
);
bool d_tests_sa_buffer_common_free_data_deep(
    struct dj_test_counter* _counter
);

// XIII. aggregation function
bool d_tests_sa_buffer_common_destruction_all(
    struct dj_test_counter* _counter
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_buffer_common_run_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTS_BUFFER_COMMON_SA_
