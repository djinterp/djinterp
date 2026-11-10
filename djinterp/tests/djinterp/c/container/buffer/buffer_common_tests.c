/*******************************************************************************
* djinterp [container]                                     buffer_common_tests.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_buffer_common_run_all
  Module-level aggregation function that runs all buffer_common tests.
  Executes tests for all categories:
  - Initialization functions
  - Capacity management functions
  - Element access functions
  - Insertion (resize mode) functions
  - Insertion (append mode / chunked) functions
  - Removal functions
  - State query functions
  - Search functions
  - Copy functions
  - Ordering functions
  - Validation functions
  - Destruction functions
*/
bool d_tests_buffer_common_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_buffer_common_init_all(_suite) && result;
    result = d_tests_buffer_common_capacity_all(_suite) && result;
    result = d_tests_buffer_common_access_all(_suite) && result;
    result = d_tests_buffer_common_insert_all(_suite) && result;
    result = d_tests_buffer_common_chunked_all(_suite) && result;
    result = d_tests_buffer_common_removal_all(_suite) && result;
    result = d_tests_buffer_common_state_all(_suite) && result;
    result = d_tests_buffer_common_search_all(_suite) && result;
    result = d_tests_buffer_common_copy_all(_suite) && result;
    result = d_tests_buffer_common_ordering_all(_suite) && result;
    result = d_tests_buffer_common_validation_all(_suite) && result;
    result = d_tests_buffer_common_destruction_all(_suite) && result;

    return result;
}
