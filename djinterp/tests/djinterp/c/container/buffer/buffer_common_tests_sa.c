/*******************************************************************************
* djinterp [container]                                  buffer_common_tests_sa.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_buffer_common_run_all
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
bool
d_tests_sa_buffer_common_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_buffer_common_init_all(_counter) && result;
    result = d_tests_sa_buffer_common_capacity_all(_counter) && result;
    result = d_tests_sa_buffer_common_access_all(_counter) && result;
    result = d_tests_sa_buffer_common_insert_all(_counter) && result;
    result = d_tests_sa_buffer_common_chunked_all(_counter) && result;
    result = d_tests_sa_buffer_common_removal_all(_counter) && result;
    result = d_tests_sa_buffer_common_state_all(_counter) && result;
    result = d_tests_sa_buffer_common_search_all(_counter) && result;
    result = d_tests_sa_buffer_common_copy_all(_counter) && result;
    result = d_tests_sa_buffer_common_ordering_all(_counter) && result;
    result = d_tests_sa_buffer_common_validation_all(_counter) && result;
    result = d_tests_sa_buffer_common_destruction_all(_counter) && result;

    return result;
}
