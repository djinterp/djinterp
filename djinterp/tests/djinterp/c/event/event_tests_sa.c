/*******************************************************************************
* djinterp [event]                                              event_tests_sa.c
*
*
* path:      /tests/djinterp/c/event/event_tests_sa.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./event_tests_sa.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_event_run_all
  Module-level aggregation function that runs all event tests.
  Executes tests for all categories:
  - Constants and types
  - d_event constructors
  - d_event_listener constructors
  - Comparison functions
  - Search functions
  - Destructors
*/
bool
d_tests_sa_event_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_event_constant_all(_counter)             && result;
    result = d_tests_sa_event_constructor_all(_counter)          && result;
    result = d_tests_sa_event_listener_constructor_all(_counter) && result;
    result = d_tests_sa_event_compare_all(_counter)              && result;
    result = d_tests_sa_event_find_all(_counter)                 && result;
    result = d_tests_sa_event_free_all(_counter)                 && result;

    return result;
}
