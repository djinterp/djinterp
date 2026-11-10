/*******************************************************************************
* djinterp [container]                                       text_buffer_tests.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_run_all
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
bool d_tests_text_buffer_run_all(
    struct dj_test_suite* _suite
)
{
    // run all test categories
    return d_tests_text_buffer_creation_all(_suite)        &&
           d_tests_text_buffer_capacity_all(_suite)        &&
           d_tests_text_buffer_string_ops_all(_suite)      &&
           d_tests_text_buffer_append_mode_all(_suite)     &&
           d_tests_text_buffer_modification_all(_suite)    &&
           d_tests_text_buffer_access_all(_suite)          &&
           d_tests_text_buffer_search_all(_suite)          &&
           d_tests_text_buffer_comparison_all(_suite)      &&
           d_tests_text_buffer_text_processing_all(_suite) &&
           d_tests_text_buffer_filter_all(_suite)          &&
           d_tests_text_buffer_utility_all(_suite)         &&
           d_tests_text_buffer_conversion_all(_suite)      &&
           d_tests_text_buffer_memory_all(_suite);
}
