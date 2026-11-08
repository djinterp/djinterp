/*******************************************************************************
* djinterp [test]                                            datomic_tests_cas.c
*
*
* path:      /tests/djinterp/c/sync/datomic_tests_cas.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "datomic_tests.h"


/*
d_tests_datomic_compare_exchange_strong_int
  Tests strong CAS for int type.
  Tests the following:
  - CAS succeeds when expected matches current value
  - CAS stores the desired value on success
  - CAS fails when expected does not match
  - expected is updated to current value on failure
*/
bool d_tests_datomic_compare_exchange_strong_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    int          expected;

    d_test_suite_test(_suite, "datomic_compare_exchange_strong_int");

    result = true;

    d_atomic_init_int(&val, 100);
    expected = 100;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_int_success",
                              d_atomic_compare_exchange_strong_int(&val, &expected, 200),
                              "CAS should succeed when expected matches") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_int_new_value",
                              d_atomic_load_int(&val) == 200,
                              "CAS should set new value to 200") && result;

    expected = 100;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_int_fail",
                              !d_atomic_compare_exchange_strong_int(&val, &expected, 300),
                              "CAS should fail when expected doesn't match") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_int_expected_updated",
                              expected == 200,
                              "Expected should be updated to current value on failure") && result;

    return result;
}

/*
d_tests_datomic_compare_exchange_weak_int
  Tests weak CAS for int type.
  Tests the following:
  - weak CAS eventually succeeds (may spuriously fail)
  - value is correct after successful weak CAS
*/
bool d_tests_datomic_compare_exchange_weak_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    int          expected;
    bool         cas_result;
    int          attempts;

    d_test_suite_test(_suite, "datomic_compare_exchange_weak_int");

    result   = true;
    attempts = 0;

    d_atomic_init_int(&val, 50);

    // weak CAS may spuriously fail, so retry
    do
    {
        expected   = 50;
        cas_result = d_atomic_compare_exchange_weak_int(&val,
                                                         &expected,
                                                         75);
        attempts++;
    }
    while (!cas_result && attempts < 100);

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_weak_int_eventually_succeeds",
                              cas_result && d_atomic_load_int(&val) == 75,
                              "CAS weak should eventually succeed") && result;

    return result;
}

/*
d_tests_datomic_compare_exchange_strong_uint
  Tests strong CAS for unsigned int type.
  Tests the following:
  - CAS succeeds for unsigned int
*/
bool d_tests_datomic_compare_exchange_strong_uint(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_uint val;
    unsigned int  expected;

    d_test_suite_test(_suite, "datomic_compare_exchange_strong_uint");

    result = true;

    d_atomic_init_uint(&val, 10);
    expected = 10;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_uint",
                              d_atomic_compare_exchange_strong_uint(&val, &expected, 20),
                              "CAS strong uint should work") && result;

    return result;
}

/*
d_tests_datomic_compare_exchange_strong_long
  Tests strong CAS for long type.
  Tests the following:
  - CAS succeeds for long
*/
bool d_tests_datomic_compare_exchange_strong_long(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_long val;
    long          expected;

    d_test_suite_test(_suite, "datomic_compare_exchange_strong_long");

    result = true;

    d_atomic_init_long(&val, 30L);
    expected = 30L;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_long",
                              d_atomic_compare_exchange_strong_long(&val, &expected, 40L),
                              "CAS strong long should work") && result;

    return result;
}

/*
d_tests_datomic_compare_exchange_strong_llong
  Tests strong CAS for long long type.
  Tests the following:
  - CAS succeeds for long long
*/
bool d_tests_datomic_compare_exchange_strong_llong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_llong val;
    d_test_suite_test(_suite, "datomic_compare_exchange_strong_llong");

    long long      expected;

    result = true;

    d_atomic_init_llong(&val, 1234LL);
    expected = 1234LL;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_llong",
                              d_atomic_compare_exchange_strong_llong(&val, &expected, 5678LL),
                              "CAS strong llong should work") && result;

    return result;
}

/*
d_tests_datomic_compare_exchange_strong_ptr
  Tests strong CAS for pointer type.
  Tests the following:
  - CAS succeeds for pointer
*/
bool d_tests_datomic_compare_exchange_strong_ptr(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_ptr val;
    int          dummy1;
    int          dummy2;
    void*        expected;

    d_test_suite_test(_suite, "datomic_compare_exchange_strong_ptr");

    result = true;
    dummy1 = 1;
    dummy2 = 2;

    d_atomic_init_ptr(&val, &dummy1);
    expected = &dummy1;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_ptr",
                              d_atomic_compare_exchange_strong_ptr(&val, &expected, &dummy2),
                              "CAS strong ptr should work") && result;

    return result;
}

/*
d_tests_datomic_compare_exchange_strong_size
  Tests strong CAS for size_t type.
  Tests the following:
  - CAS succeeds for size_t
*/
bool d_tests_datomic_compare_exchange_strong_size(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_size_t val;
    size_t          expected;

    d_test_suite_test(_suite, "datomic_compare_exchange_strong_size");

    result = true;

    d_atomic_init_size(&val, 999);
    expected = 999;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_size",
                              d_atomic_compare_exchange_strong_size(&val, &expected, 888),
                              "CAS strong size should work") && result;

    return result;
}

/*
d_tests_datomic_compare_exchange_explicit
  Tests CAS with explicit memory orders.
  Tests the following:
  - strong CAS with acq_rel/relaxed ordering succeeds
  - strong CAS with acq_rel/relaxed ordering fails and updates expected
  - weak CAS with explicit ordering eventually succeeds
  - ptr CAS with explicit ordering works
*/
bool d_tests_datomic_compare_exchange_explicit(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    dj_atomic_ptr pval;
    int          expected;
    int          dummy1;
    int          dummy2;
    void*        pexp;
    bool         cas_result;
    int          attempts;

    d_test_suite_test(_suite, "datomic_compare_exchange_explicit");

    result = true;
    dummy1 = 1;
    dummy2 = 2;

    // strong explicit — success
    d_atomic_init_int(&val, 100);
    expected = 100;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_explicit_success",
                              d_atomic_compare_exchange_strong_int_explicit( &val, &expected, 200, D_MEMORY_ORDER_ACQ_REL, D_MEMORY_ORDER_RELAXED),
                              "CAS strong explicit should succeed") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_explicit_value",
                              d_atomic_load_int(&val) == 200,
                              "CAS strong explicit should set new value") && result;

    // strong explicit — failure
    expected = 999;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_explicit_fail",
                              !d_atomic_compare_exchange_strong_int_explicit( &val, &expected, 300, D_MEMORY_ORDER_ACQ_REL, D_MEMORY_ORDER_RELAXED),
                              "CAS strong explicit should fail on mismatch") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_strong_explicit_expected_updated",
                              expected == 200,
                              "Expected should be updated on explicit CAS failure") && result;

    // weak explicit
    d_atomic_store_int(&val, 50);
    attempts = 0;

    do
    {
        expected   = 50;
        cas_result = d_atomic_compare_exchange_weak_int_explicit(
                         &val, &expected, 75,
                         D_MEMORY_ORDER_SEQ_CST,
                         D_MEMORY_ORDER_SEQ_CST);
        attempts++;
    }
    while (!cas_result && attempts < 100);

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_weak_explicit_success",
                              cas_result,
                              "CAS weak explicit should eventually succeed") && result;

    // ptr explicit
    d_atomic_init_ptr(&pval, &dummy1);
    pexp = &dummy1;

    result = D_TEST_CHECK_MSG(_suite,
                              "cas_ptr_explicit",
                              d_atomic_compare_exchange_strong_ptr_explicit( &pval, &pexp, &dummy2, D_MEMORY_ORDER_SEQ_CST, D_MEMORY_ORDER_SEQ_CST),
                              "CAS ptr explicit should work") && result;

    return result;
}

/*
d_tests_datomic_compare_exchange_all
  Runs all atomic compare-and-exchange tests.
  Tests the following:
  - strong CAS for int, uint, long, llong, ptr, size
  - weak CAS for int
  - explicit memory order variants
*/
bool d_tests_datomic_compare_exchange_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "datomic_compare_exchange_all");

    result = true;

    result = d_tests_datomic_compare_exchange_strong_int(_suite)
             && result;
    result = d_tests_datomic_compare_exchange_weak_int(_suite)
             && result;
    result = d_tests_datomic_compare_exchange_strong_uint(_suite)
             && result;
    result = d_tests_datomic_compare_exchange_strong_long(_suite)
             && result;
    result = d_tests_datomic_compare_exchange_strong_llong(_suite)
             && result;
    result = d_tests_datomic_compare_exchange_strong_ptr(_suite)
             && result;
    result = d_tests_datomic_compare_exchange_strong_size(_suite)
             && result;
    result = d_tests_datomic_compare_exchange_explicit(_suite)
             && result;

    return result;
}
