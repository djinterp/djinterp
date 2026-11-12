/*******************************************************************************
* djinterp [c][sync]                                        datomic_tests_flag.c
*
*
* path:      /tests/djinterp/c/sync/datomic_tests_flag.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "datomic_tests.h"


/*
d_tests_datomic_flag_test_and_set
  Tests atomic flag test-and-set operation.
  Tests the following:
  - initial test_and_set returns false (flag was clear)
  - second test_and_set returns true (flag was set)
  - flag remains set on subsequent calls
*/
bool d_tests_datomic_flag_test_and_set(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_flag flag;

    d_test_suite_test(_suite, "datomic_flag_test_and_set");

    result = true;
    flag   = (dj_atomic_flag)D_ATOMIC_FLAG_INIT;

    // initial test_and_set should return false
    result = D_TEST_CHECK_MSG(_suite,
                              "flag_test_and_set_initial",
                              !d_atomic_flag_test_and_set(&flag),
                              "Initial flag should be clear (return false)") && result;

    // second test_and_set should return true
    result = D_TEST_CHECK_MSG(_suite,
                              "flag_test_and_set_set",
                              d_atomic_flag_test_and_set(&flag),
                              "Flag should be set (return true)") && result;

    // flag remains set
    result = D_TEST_CHECK_MSG(_suite,
                              "flag_test_and_set_remains",
                              d_atomic_flag_test_and_set(&flag),
                              "Flag should remain set") && result;

    return result;
}

/*
d_tests_datomic_flag_clear
  Tests atomic flag clear operation.
  Tests the following:
  - flag can be cleared after being set
  - test_and_set returns false after clear
*/
bool d_tests_datomic_flag_clear(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_flag flag;

    d_test_suite_test(_suite, "datomic_flag_clear");

    result = true;
    flag   = (dj_atomic_flag)D_ATOMIC_FLAG_INIT;

    // set the flag first
    d_atomic_flag_test_and_set(&flag);

    // clear the flag
    d_atomic_flag_clear(&flag);

    // test_and_set after clear should return false
    result = D_TEST_CHECK_MSG(_suite,
                              "flag_clear_works",
                              !d_atomic_flag_test_and_set(&flag),
                              "Flag should be clear after clear()") && result;

    return result;
}

/*
d_tests_datomic_flag_explicit
  Tests atomic flag with explicit memory orders.
  Tests the following:
  - test_and_set_explicit with seq_cst ordering
  - clear_explicit with seq_cst ordering
*/
bool d_tests_datomic_flag_explicit(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_flag flag;

    d_test_suite_test(_suite, "datomic_flag_explicit");

    result = true;
    flag   = (dj_atomic_flag)D_ATOMIC_FLAG_INIT;

    // test with explicit memory order
    result = D_TEST_CHECK_MSG(_suite,
                              "flag_explicit_test_and_set",
                              !d_atomic_flag_test_and_set_explicit( &flag, D_MEMORY_ORDER_SEQ_CST),
                              "Explicit test_and_set should work") && result;

    d_atomic_flag_clear_explicit(&flag, D_MEMORY_ORDER_SEQ_CST);

    result = D_TEST_CHECK_MSG(_suite,
                              "flag_explicit_clear",
                              !d_atomic_flag_test_and_set(&flag),
                              "Explicit clear should work") && result;

    return result;
}

/*
d_tests_datomic_flag_all
  Runs all atomic flag tests.
  Tests the following:
  - test_and_set, clear, explicit
*/
bool d_tests_datomic_flag_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "datomic_flag_all");

    result = true;

    result = d_tests_datomic_flag_test_and_set(_suite) && result;
    result = d_tests_datomic_flag_clear(_suite) && result;
    result = d_tests_datomic_flag_explicit(_suite) && result;

    return result;
}
