/*******************************************************************************
* djinterp [container]                                    text_buffer_tests_sa.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_sa.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_buffer_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_text_buffer_run_all
  Module-level aggregation function that runs all text_buffer tests.
  Executes tests for all categories:
  - Creation functions
  - Capacity management functions
  - String operations (resize mode) functions
  - String operations (append mode) functions
  - Modification operations functions
  - Access operations functions
  - Search operations functions
  - Comparison operations functions
  - Text processing functions
  - Filter functions
  - Utility functions
  - Conversion functions
  - Memory management functions
*/
bool
d_tests_sa_text_buffer_run_all
(
    struct d_test_counter* _counter
)
{
    // run all test categories
    return d_tests_sa_text_buffer_creation_all(_counter)        &&
           d_tests_sa_text_buffer_capacity_all(_counter)        &&
           d_tests_sa_text_buffer_string_ops_all(_counter)      &&
           d_tests_sa_text_buffer_append_mode_all(_counter)     &&
           d_tests_sa_text_buffer_modification_all(_counter)    &&
           d_tests_sa_text_buffer_access_all(_counter)          &&
           d_tests_sa_text_buffer_search_all(_counter)          &&
           d_tests_sa_text_buffer_comparison_all(_counter)      &&
           d_tests_sa_text_buffer_text_processing_all(_counter) &&
           d_tests_sa_text_buffer_filter_all(_counter)          &&
           d_tests_sa_text_buffer_utility_all(_counter)         &&
           d_tests_sa_text_buffer_conversion_all(_counter)      &&
           d_tests_sa_text_buffer_memory_all(_counter);
}
