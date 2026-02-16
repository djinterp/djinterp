/******************************************************************************
* djinterp [test]                                    min_enum_map_tests_sa.c
*
*   Master test runner for min_enum_map module.
*   Coordinates execution of all test categories.
*
*
* path:      \test\container\map\min_enum_map_tests_sa.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.24
******************************************************************************/
#include ".\min_enum_map_tests_sa.h"


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
struct d_test_object*
d_tests_min_enum_map_run_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    // create master test group
    group = d_test_object_new_interior("min_enum_map Module Tests", 4);

    if (!group)
    {
        return NULL;
    }

    // add all test categories
    idx = 0;
    group->elements[idx++] = d_tests_min_enum_map_core_all();
    group->elements[idx++] = d_tests_min_enum_map_memory_all();
    group->elements[idx++] = d_tests_min_enum_map_advanced_all();
    group->elements[idx++] = d_tests_min_enum_map_edge_stress_all();

    return group;
}
