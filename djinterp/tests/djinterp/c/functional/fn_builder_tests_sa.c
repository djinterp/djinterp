/*******************************************************************************
* djinterp [functional]                                    fn_builder_tests_sa.c
*
*
* path:      /tests/djinterp/c/functional/fn_builder_tests_sa.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./fn_builder_tests_sa.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_fn_builder_all
  Runs all fn_builder unit tests across all sections:
  i.   builder creation (new)
  ii.  fluent operations (map, filter, and_then, where, chaining, grow)
  iii. builder execution (validation, passthrough, transforms, predicates,
       combined, failure, ping-pong)
  iv.  builder cleanup (free)
*/
bool
d_tests_sa_fn_builder_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    // i.   builder creation
    all_passed &= d_tests_sa_fn_builder_creation_all(_test_info);

    // ii.  fluent operations (chainable)
    all_passed &= d_tests_sa_fn_builder_operations_all(_test_info);

    // iii. builder execution
    all_passed &= d_tests_sa_fn_builder_execution_all(_test_info);

    // iv.  builder cleanup
    all_passed &= d_tests_sa_fn_builder_cleanup_all(_test_info);

    return all_passed;
}
