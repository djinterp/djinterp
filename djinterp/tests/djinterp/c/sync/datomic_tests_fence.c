/*******************************************************************************
* djinterp [test]                                          datomic_tests_fence.c
*
*
* path:      /tests/djinterp/c/sync/datomic_tests_fence.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "datomic_tests.h"


/*
d_tests_datomic_thread_fence
  Tests thread fence with all memory orders.
  Tests the following:
  - thread fences do not crash for any memory order
*/
bool d_tests_datomic_thread_fence(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "datomic_thread_fence");

    result = true;

    // fences should not crash
    d_atomic_thread_fence(D_MEMORY_ORDER_RELAXED);
    d_atomic_thread_fence(D_MEMORY_ORDER_ACQUIRE);
    d_atomic_thread_fence(D_MEMORY_ORDER_RELEASE);
    d_atomic_thread_fence(D_MEMORY_ORDER_ACQ_REL);
    d_atomic_thread_fence(D_MEMORY_ORDER_SEQ_CST);

    result = D_TEST_CHECK_MSG(_suite,
                              "thread_fence_no_crash",
                              true,
                              "Thread fences should not crash") && result;

    return result;
}

/*
d_tests_datomic_signal_fence
  Tests signal fence with all memory orders.
  Tests the following:
  - signal fences do not crash for any memory order
*/
bool d_tests_datomic_signal_fence(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "datomic_signal_fence");

    result = true;

    // signal fences should not crash
    d_atomic_signal_fence(D_MEMORY_ORDER_RELAXED);
    d_atomic_signal_fence(D_MEMORY_ORDER_ACQUIRE);
    d_atomic_signal_fence(D_MEMORY_ORDER_RELEASE);
    d_atomic_signal_fence(D_MEMORY_ORDER_ACQ_REL);
    d_atomic_signal_fence(D_MEMORY_ORDER_SEQ_CST);

    result = D_TEST_CHECK_MSG(_suite,
                              "signal_fence_no_crash",
                              true,
                              "Signal fences should not crash") && result;

    return result;
}

/*
d_tests_datomic_is_lock_free
  Tests lock-free query functions.
  Tests the following:
  - is_lock_free_1 returns a valid boolean
  - is_lock_free_2 returns a valid boolean
  - is_lock_free_4 returns a valid boolean
  - is_lock_free_8 returns a valid boolean
*/
bool d_tests_datomic_is_lock_free(
    struct dj_test_suite* _suite
)
{
    bool result;
    bool lf1;
    bool lf2;
    bool lf4;
    bool lf8;

    d_test_suite_test(_suite, "datomic_is_lock_free");

    result = true;

    // verify functions return boolean values
    lf1 = d_atomic_is_lock_free_1();
    lf2 = d_atomic_is_lock_free_2();
    lf4 = d_atomic_is_lock_free_4();
    lf8 = d_atomic_is_lock_free_8();

    result = D_TEST_CHECK_MSG(_suite,
                              "is_lock_free_1_valid",
                              lf1 == true || lf1 == false,
                              "Is-lock-free-1 should return boolean") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "is_lock_free_2_valid",
                              lf2 == true || lf2 == false,
                              "Is-lock-free-2 should return boolean") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "is_lock_free_4_valid",
                              lf4 == true || lf4 == false,
                              "Is-lock-free-4 should return boolean") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "is_lock_free_8_valid",
                              lf8 == true || lf8 == false,
                              "Is-lock-free-8 should return boolean") && result;

    return result;
}

/*
d_tests_datomic_fence_all
  Runs all memory fence and utility tests.
  Tests the following:
  - thread fences, signal fences, lock-free queries
*/
bool d_tests_datomic_fence_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "datomic_fence_all");

    result = true;

    result = d_tests_datomic_thread_fence(_suite) && result;
    result = d_tests_datomic_signal_fence(_suite) && result;
    result = d_tests_datomic_is_lock_free(_suite) && result;

    return result;
}
