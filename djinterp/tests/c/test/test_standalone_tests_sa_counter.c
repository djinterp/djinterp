#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * III. TEST COUNTER TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_counter_struct
  Tests the d_test_counter structure.
  Tests the following:
  - Structure has expected members
  - Members are size_t type
  - Structure can be initialized
  - Members are accessible
*/
bool
d_tests_sa_standalone_counter_struct
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_counter test_counter;

    result = true;

    // test 1: structure size is reasonable (at least 4 size_t members)
    result = d_assert_standalone(
        sizeof(struct d_test_counter) >= (4 * sizeof(size_t)),
        "counter_struct_size",
        "d_test_counter should be at least 4 size_t members",
        _counter) && result;

    // test 2: assertions_total member is accessible
    test_counter.assertions_total = 100;

    result = d_assert_standalone(
        test_counter.assertions_total == 100,
        "counter_assertions_total_accessible",
        "assertions_total member should be accessible",
        _counter) && result;

    // test 3: assertions_passed member is accessible
    test_counter.assertions_passed = 90;

    result = d_assert_standalone(
        test_counter.assertions_passed == 90,
        "counter_assertions_passed_accessible",
        "assertions_passed member should be accessible",
        _counter) && result;

    // test 4: tests_total member is accessible
    test_counter.tests_total = 50;

    result = d_assert_standalone(
        test_counter.tests_total == 50,
        "counter_tests_total_accessible",
        "tests_total member should be accessible",
        _counter) && result;

    // test 5: tests_passed member is accessible
    test_counter.tests_passed = 45;

    result = d_assert_standalone(
        test_counter.tests_passed == 45,
        "counter_tests_passed_accessible",
        "tests_passed member should be accessible",
        _counter) && result;

    // test 6: members can hold large values (size_t)
    test_counter.assertions_total = SIZE_MAX;

    result = d_assert_standalone(
        test_counter.assertions_total == SIZE_MAX,
        "counter_holds_size_max",
        "Counter members should hold SIZE_MAX",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_counter_reset
  Tests the d_test_counter_reset function.
  Tests the following:
  - NULL counter is handled safely
  - All members reset to zero
  - Works on partially filled counter
*/
bool
d_tests_sa_standalone_counter_reset
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_counter test_counter;

    result = true;

    // test 1: NULL counter is handled safely (no crash)
    d_test_counter_reset(NULL);

    result = d_assert_standalone(
        true,
        "counter_reset_null_safe",
        "d_test_counter_reset(NULL) should not crash",
        _counter) && result;

    // test 2: all members reset to zero
    test_counter.assertions_total  = 100;
    test_counter.assertions_passed = 90;
    test_counter.tests_total       = 50;
    test_counter.tests_passed      = 45;

    d_test_counter_reset(&test_counter);

    result = d_assert_standalone(
        test_counter.assertions_total == 0,
        "counter_reset_assertions_total",
        "assertions_total should be reset to 0",
        _counter) && result;

    result = d_assert_standalone(
        test_counter.assertions_passed == 0,
        "counter_reset_assertions_passed",
        "assertions_passed should be reset to 0",
        _counter) && result;

    result = d_assert_standalone(
        test_counter.tests_total == 0,
        "counter_reset_tests_total",
        "tests_total should be reset to 0",
        _counter) && result;

    result = d_assert_standalone(
        test_counter.tests_passed == 0,
        "counter_reset_tests_passed",
        "tests_passed should be reset to 0",
        _counter) && result;

    // test 3: multiple resets work correctly
    test_counter.assertions_total = 999;
    d_test_counter_reset(&test_counter);
    d_test_counter_reset(&test_counter);

    result = d_assert_standalone(
        test_counter.assertions_total == 0,
        "counter_reset_idempotent",
        "Multiple resets should result in zero values",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_counter_add
  Tests the d_test_counter_add function.
  Tests the following:
  - NULL dest is handled safely
  - NULL src is handled safely
  - Values are added correctly
  - Source is not modified
*/
bool
d_tests_sa_standalone_counter_add
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_counter dest;
    struct d_test_counter src;

    result = true;

    // test 1: NULL dest is handled safely
    src.assertions_total  = 10;
    src.assertions_passed = 8;
    src.tests_total       = 5;
    src.tests_passed      = 4;

    d_test_counter_add(NULL, &src);

    result = d_assert_standalone(
        true,
        "counter_add_null_dest_safe",
        "d_test_counter_add with NULL dest should not crash",
        _counter) && result;

    // test 2: NULL src is handled safely
    d_test_counter_reset(&dest);

    d_test_counter_add(&dest, NULL);

    result = d_assert_standalone(
        dest.assertions_total == 0,
        "counter_add_null_src_safe",
        "d_test_counter_add with NULL src should not modify dest",
        _counter) && result;

    // test 3: values are added correctly
    dest.assertions_total  = 10;
    dest.assertions_passed = 8;
    dest.tests_total       = 5;
    dest.tests_passed      = 4;

    src.assertions_total  = 20;
    src.assertions_passed = 15;
    src.tests_total       = 10;
    src.tests_passed      = 9;

    d_test_counter_add(&dest, &src);

    result = d_assert_standalone(
        dest.assertions_total == 30,
        "counter_add_assertions_total",
        "assertions_total should be sum (10+20=30)",
        _counter) && result;

    result = d_assert_standalone(
        dest.assertions_passed == 23,
        "counter_add_assertions_passed",
        "assertions_passed should be sum (8+15=23)",
        _counter) && result;

    result = d_assert_standalone(
        dest.tests_total == 15,
        "counter_add_tests_total",
        "tests_total should be sum (5+10=15)",
        _counter) && result;

    result = d_assert_standalone(
        dest.tests_passed == 13,
        "counter_add_tests_passed",
        "tests_passed should be sum (4+9=13)",
        _counter) && result;

    // test 4: source is not modified
    result = d_assert_standalone(
        src.assertions_total == 20,
        "counter_add_src_unchanged",
        "Source counter should not be modified",
        _counter) && result;

    // test 5: adding zero-initialized counter
    d_test_counter_reset(&dest);
    d_test_counter_reset(&src);

    dest.assertions_total = 50;
    d_test_counter_add(&dest, &src);

    result = d_assert_standalone(
        dest.assertions_total == 50,
        "counter_add_zero_src",
        "Adding zero counter should not change dest",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_counter_all
  Aggregation function that runs all test counter tests.
*/
bool
d_tests_sa_standalone_counter_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Test Counter\n");
    printf("  -----------------------\n");

    result = d_tests_sa_standalone_counter_struct(_counter) && result;
    result = d_tests_sa_standalone_counter_reset(_counter) && result;
    result = d_tests_sa_standalone_counter_add(_counter) && result;

    return result;
}
