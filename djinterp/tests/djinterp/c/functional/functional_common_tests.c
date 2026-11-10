/*******************************************************************************
* djinterp [functional]                                functional_common_tests.c
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./functional_common_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_functional_common_all
  Runs all unit tests for the functional_common module.
  Aggregates results from every test section: identity, constant, comparison,
predicate, map/filter, fold, and iteration/query functions.

Parameter(s):
  _suite: pointer to the test counter that tracks pass/fail totals.
Return:
  A boolean value corresponding to either:
  - true, if all tests in all sections passed, or
  - false, if any test in any section failed, or _suite was NULL.
*/
bool d_tests_functional_common_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_block(_suite, "functional_common_all");

    all_passed = true;

    // i.    identity function tests
    printf("\n  [identity functions]\n");
    all_passed &= d_tests_functional_identity_transformer(_suite);
    all_passed &= d_tests_functional_identity_predicate(_suite);

    // ii.   constant function tests
    printf("\n  [constant functions]\n");
    all_passed &= d_tests_functional_constant_true(_suite);
    all_passed &= d_tests_functional_constant_false(_suite);

    // iii.  comparison utility tests
    printf("\n  [comparison utilities]\n");
    all_passed &= d_tests_functional_compare_int(_suite);
    all_passed &= d_tests_functional_compare_size_t(_suite);
    all_passed &= d_tests_functional_compare_double(_suite);
    all_passed &= d_tests_functional_equal_int(_suite);
    all_passed &= d_tests_functional_equal_size_t(_suite);

    // iv.   predicate utility tests
    printf("\n  [predicate utilities]\n");
    all_passed &= d_tests_functional_is_null(_suite);
    all_passed &= d_tests_functional_is_not_null(_suite);

    // v.    fold tests
    printf("\n  [fold operations]\n");
    all_passed &= d_tests_functional_fold_left(_suite);
    all_passed &= d_tests_functional_fold_right(_suite);

    // vi.   iteration and query tests
    printf("\n  [iteration and query]\n");
    all_passed &= d_tests_functional_for_each(_suite);
    all_passed &= d_tests_functional_for_each_const(_suite);
    all_passed &= d_tests_functional_any(_suite);
    all_passed &= d_tests_functional_all(_suite);
    all_passed &= d_tests_functional_none(_suite);
    all_passed &= d_tests_functional_count_if(_suite);
    all_passed &= d_tests_functional_find_if(_suite);

    // vii.  the fc groups
    printf("\n  [fc: higher-order, quantifiers, utilities]\n");
    all_passed &= d_tests_fc_higher_order_all(_suite);
    all_passed &= d_tests_fc_quantifier_all(_suite);
    all_passed &= d_tests_fc_utility_all(_suite);

    return all_passed;
}
