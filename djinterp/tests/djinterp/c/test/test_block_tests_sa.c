#include "./test_block_tests_sa.h"


/******************************************************************************
 * ROOT AGGREGATION
 *****************************************************************************/

/*
d_tests_sa_test_block_all
  Root aggregation function that runs ALL test_block unit tests.
  Delegates to per-section aggregation functions covering constructors,
child management, run-config stage hooks, execution, and utilities.

Parameter(s):
  _test_info: test counter for tracking assertions and results.
Return:
  true if all tests passed, false otherwise.
*/
bool
d_tests_sa_test_block_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    // I.   constructor/destructor tests
    all_passed &= d_tests_sa_test_block_constructor_all(_test_info);

    // II.  child management tests
    all_passed &= d_tests_sa_test_block_child_mgmt_all(_test_info);

    // III. run-config stage hook tests
    all_passed &= d_tests_sa_test_block_stage_hooks_all(_test_info);

    // IV.  execution tests
    all_passed &= d_tests_sa_test_block_execution_all(_test_info);

    // V.   utility tests
    all_passed &= d_tests_sa_test_block_utility_all(_test_info);

    return all_passed;
}
