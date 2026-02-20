#include ".\functional_common_tests_sa.h"


/*
d_tests_sa_functional_common_all
  Runs all unit tests for the functional_common module.
  Aggregates results from every test section: identity, constant, comparison,
predicate, map/filter, fold, and iteration/query functions.

Parameter(s):
  _counter: pointer to the test counter that tracks pass/fail totals.
Return:
  A boolean value corresponding to either:
  - true, if all tests in all sections passed, or
  - false, if any test in any section failed, or _counter was NULL.
*/
bool
d_tests_sa_functional_common_all
(
    struct d_test_counter* _counter
)
{
    bool all_passed;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // i.    identity function tests
    printf("\n  [identity functions]\n");
    all_passed &= d_tests_sa_functional_identity_transformer(_counter);
    all_passed &= d_tests_sa_functional_identity_predicate(_counter);

    // ii.   constant function tests
    printf("\n  [constant functions]\n");
    all_passed &= d_tests_sa_functional_constant_true(_counter);
    all_passed &= d_tests_sa_functional_constant_false(_counter);

    // iii.  comparison utility tests
    printf("\n  [comparison utilities]\n");
    all_passed &= d_tests_sa_functional_compare_int(_counter);
    all_passed &= d_tests_sa_functional_compare_size_t(_counter);
    all_passed &= d_tests_sa_functional_compare_double(_counter);
    all_passed &= d_tests_sa_functional_equal_int(_counter);
    all_passed &= d_tests_sa_functional_equal_size_t(_counter);

    // iv.   predicate utility tests
    printf("\n  [predicate utilities]\n");
    all_passed &= d_tests_sa_functional_is_null(_counter);
    all_passed &= d_tests_sa_functional_is_not_null(_counter);

    // v.    fold tests
    printf("\n  [fold operations]\n");
    all_passed &= d_tests_sa_functional_fold_left(_counter);
    all_passed &= d_tests_sa_functional_fold_right(_counter);

    // vi.   iteration and query tests
    printf("\n  [iteration and query]\n");
    all_passed &= d_tests_sa_functional_for_each(_counter);
    all_passed &= d_tests_sa_functional_for_each_const(_counter);
    all_passed &= d_tests_sa_functional_any(_counter);
    all_passed &= d_tests_sa_functional_all(_counter);
    all_passed &= d_tests_sa_functional_none(_counter);
    all_passed &= d_tests_sa_functional_count_if(_counter);
    all_passed &= d_tests_sa_functional_find_if(_counter);

    return all_passed;
}
