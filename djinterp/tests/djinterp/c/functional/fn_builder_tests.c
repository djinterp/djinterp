/*******************************************************************************
* djinterp [functional]                                       fn_builder_tests.c
*
*
* path:      /tests/djinterp/c/functional/fn_builder_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./fn_builder_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_fn_builder_all
  Runs all DFnBuilder unit tests across all sections:
  i.   builder creation (new)
  ii.  fluent operations (map, filter, and_then, where, chaining, grow)
  iii. builder execution (validation, passthrough, transforms, predicates,
       combined, failure, ping-pong)
  iv.  builder cleanup (free)
*/
bool d_tests_fn_builder_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    all_passed = true;

    // i.   builder creation
    all_passed &= d_tests_fn_builder_creation_all(_suite);

    // ii.  fluent operations (chainable)
    all_passed &= d_tests_fn_builder_operations_all(_suite);

    // iii. builder execution
    all_passed &= d_tests_fn_builder_execution_all(_suite);

    // iv.  builder cleanup
    all_passed &= d_tests_fn_builder_cleanup_all(_suite);

    return all_passed;
}
