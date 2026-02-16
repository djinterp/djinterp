#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * IX. ASSERTION FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_assert_fn
  Tests the d_assert_standalone function.
  Tests the following:
  - Returns condition value
  - NULL counter is handled safely
  - Counter is updated on pass
  - Counter is updated on fail
  - Prints appropriate output
*/
bool
d_tests_sa_standalone_assert_fn
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_counter local_counter;
    bool                  assert_result;

    result = true;

    // test 1: returns true when condition is true
    d_test_counter_reset(&local_counter);
    assert_result = d_assert_standalone(true, "test_pass", "message", &local_counter);

    result = d_assert_standalone(
        assert_result == true,
        "assert_fn_returns_true",
        "d_assert_standalone should return true for true condition",
        _counter) && result;

    // test 2: returns false when condition is false
    d_test_counter_reset(&local_counter);
    assert_result = d_assert_standalone(false, "test_fail", "message", &local_counter);

    result = d_assert_standalone(
        assert_result == false,
        "assert_fn_returns_false",
        "d_assert_standalone should return false for false condition",
        _counter) && result;

    // test 3: NULL counter is handled safely
    assert_result = d_assert_standalone(true, "test_null", "message", NULL);

    result = d_assert_standalone(
        assert_result == true,
        "assert_fn_null_counter_safe",
        "d_assert_standalone with NULL counter should not crash",
        _counter) && result;

    // test 4: counter assertions_total is incremented on pass
    d_test_counter_reset(&local_counter);
    d_assert_standalone(true, "test_pass", "message", &local_counter);

    result = d_assert_standalone(
        local_counter.assertions_total == 1,
        "assert_fn_total_incremented_pass",
        "assertions_total should be incremented on pass",
        _counter) && result;

    // test 5: counter assertions_passed is incremented on pass
    result = d_assert_standalone(
        local_counter.assertions_passed == 1,
        "assert_fn_passed_incremented_pass",
        "assertions_passed should be incremented on pass",
        _counter) && result;

    // test 6: counter assertions_total is incremented on fail
    d_test_counter_reset(&local_counter);
    d_assert_standalone(false, "test_fail", "message", &local_counter);

    result = d_assert_standalone(
        local_counter.assertions_total == 1,
        "assert_fn_total_incremented_fail",
        "assertions_total should be incremented on fail",
        _counter) && result;

    // test 7: counter assertions_passed is NOT incremented on fail
    result = d_assert_standalone(
        local_counter.assertions_passed == 0,
        "assert_fn_passed_not_incremented_fail",
        "assertions_passed should NOT be incremented on fail",
        _counter) && result;

    // test 8: multiple assertions accumulate
    d_test_counter_reset(&local_counter);
    d_assert_standalone(true, "test1", "msg", &local_counter);
    d_assert_standalone(true, "test2", "msg", &local_counter);
    d_assert_standalone(false, "test3", "msg", &local_counter);

    result = d_assert_standalone(
        local_counter.assertions_total == 3,
        "assert_fn_accumulates_total",
        "Multiple assertions should accumulate in total",
        _counter) && result;

    result = d_assert_standalone(
        local_counter.assertions_passed == 2,
        "assert_fn_accumulates_passed",
        "Multiple passing assertions should accumulate in passed",
        _counter) && result;

    // test 9: tests_total and tests_passed are not affected
    result = d_assert_standalone(
        local_counter.tests_total == 0 && local_counter.tests_passed == 0,
        "assert_fn_tests_unchanged",
        "tests_total and tests_passed should not be affected",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_assert_fn_all
  Aggregation function that runs all assertion function tests.
*/
bool
d_tests_sa_standalone_assert_fn_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Assertion Function\n");
    printf("  -----------------------------\n");

    result = d_tests_sa_standalone_assert_fn(_counter) && result;

    return result;
}
