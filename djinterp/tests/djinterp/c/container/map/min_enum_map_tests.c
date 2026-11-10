/*******************************************************************************
* djinterp [test]                                           min_enum_map_tests.c
*
*   Master test runner for min_enum_map module.
*   Coordinates execution of all test categories.
*
*
* path:      /tests/djinterp/c/container/map/min_enum_map_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.01.24
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./min_enum_map_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_min_enum_map_run_all
  Master test runner for all min_enum_map tests.
  Tests the following:
  - Core map operations (new, put, get, contains, count, remove)
  - Memory management (clear, free)
  - Advanced operations (merge, macros, ordering)
  - Edge cases and stress testing

  This function aggregates all test categories and returns a complete
  test tree for execution by the test framework.
*/
bool d_tests_min_enum_map_run_all(struct dj_test_suite* _suite)
{
    // create master test group
    d_test_suite_block(_suite, "min_enum_map Module Tests");

    // add all test categories
    d_tests_min_enum_map_core_all(_suite);
    d_tests_min_enum_map_memory_all(_suite);
    d_tests_min_enum_map_advanced_all(_suite);
    d_tests_min_enum_map_edge_stress_all(_suite);

    return true;
}
