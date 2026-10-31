/*******************************************************************************
* djinterp [test]                                         text_buffer_tests_sa.h
*
*   Unit test declarations for `text_buffer.h` module.
*   Provides comprehensive testing of all dj_text_buffer functions including
* creation, capacity management, string operations (resize and append modes),
* modification, access, search, comparison, text processing, utility,
* conversion, and memory management.
*
*   NOTE: Section X (filter) is excluded because it depends on the external
* d_filter_expr / d_filter_evaluate API from filter.h, which is tested
* separately.
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.15
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEXT_BUFFER_SA_
#define DJINTERP_TESTS_TEXT_BUFFER_SA_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/container/buffer/text_buffer.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


// I. creation function tests
bool d_tests_sa_text_buffer_new(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_new_default_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_new_from_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_new_from_string_n(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_new_from_strings(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_new_from_buffer(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_new_copy(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_new_copy_range(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_new_fill(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_new_formatted(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_creation_all(
    struct dj_test_counter* _counter
);

// capacity management function tests
bool d_tests_sa_text_buffer_ensure_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_resize_to_fit(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_reserve(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_capacity_all(
    struct dj_test_counter* _counter
);

// string operations (resize mode) function tests
bool d_tests_sa_text_buffer_append_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_string_n(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_buffer(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_chars(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_formatted(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_buffer_obj(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_prepend_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_prepend_buffer(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_prepend_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_insert_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_insert_buffer(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_insert_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_string_ops_all(
    struct dj_test_counter* _counter
);

// string operations (append mode) function tests
bool d_tests_sa_text_buffer_append_string_chunked(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_buffer_chunked(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_char_chunked(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_formatted_chunked(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_consolidate(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_total_length(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_has_chunks(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_append_mode_all(
    struct dj_test_counter* _counter
);

// modification operations function tests
bool d_tests_sa_text_buffer_set_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_set_buffer(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_set_formatted(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_replace_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_replace_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_replace_range(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_remove_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_remove_range(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_consume_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_consume_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_modification_all(
    struct dj_test_counter* _counter
);

// access operations function test
bool d_tests_sa_text_buffer_get_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_set_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_get_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_get_range_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_access_all(
    struct dj_test_counter* _counter
);

// search operations function tests
bool d_tests_sa_text_buffer_find_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_find_char_from(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_find_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_find_string_from(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_find_last_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_find_last_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_contains_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_contains_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_starts_with(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_ends_with(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_count_char(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_count_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_search_all(
    struct dj_test_counter* _counter
);

// comparison operations function tests
bool d_tests_sa_text_buffer_compare(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_compare_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_compare_n(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_equals(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_equals_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_comparison_all(
    struct dj_test_counter* _counter
);

// text processing function tests
bool d_tests_sa_text_buffer_trim_whitespace(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_trim_front(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_trim_back(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_trim_chars(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_to_upper(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_to_lower(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_reverse(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_pad_left(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_pad_right(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_text_processing_all(
    struct dj_test_counter* _counter
);

// filter operations function tests
bool d_tests_sa_text_buffer_filter(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_filter_in_place(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_filter_indices(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_count_matching(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_filter_chunked(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_filter_all(
    struct dj_test_counter* _counter
);

// utility function tests
bool d_tests_sa_text_buffer_clear(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_is_empty(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_length(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_capacity(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_utilization(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_hash(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_utility_all(
    struct dj_test_counter* _counter
);

// conversion function tests
bool d_tests_sa_text_buffer_to_cstring(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_copy_to_buffer(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_copy_to_buffer_n(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_to_d_string(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_conversion_all(
    struct dj_test_counter* _counter
);

// destruction tests
bool d_tests_sa_text_buffer_free(
    struct dj_test_counter* _counter
);
bool d_tests_sa_text_buffer_memory_all(
    struct dj_test_counter* _counter
);


// module-level aggregation
bool d_tests_sa_text_buffer_run_all(
    struct dj_test_counter* _counter
);


#endif  // DJINTERP_TESTS_TEXT_BUFFER_SA_
